#include <iostream>
#include <vector>
#include <sstream>
#include <Windows.h>
#include <utility>



using namespace std;

struct MEMBLOCK {
    unsigned char* addr;
    int size;
    unsigned char *searchmask;
    int matches;
    int data_size;
};

#define IS_IN_SEARCH(mb,offset) (mb->searchmask[(offset)/8] & (1<<((offset) % 8)))
#define REMOVE_FROM_SEARCH(mb,offset) mb->searchmask[(offset)/8] &= ~(1<<((offset) % 8));

MEMBLOCK* create_memblock(HANDLE hProc, MEMORY_BASIC_INFORMATION *meminfo, int data_size) {
    MEMBLOCK* mb = new MEMBLOCK;
    if (mb) {
        mb->hProc = hProc;
        mb->addr = (unsigned char*)meminfo->BaseAddress;
        mb->size = meminfo->RegionSize;
        mb->buffer = new unsigned char[meminfo->RegionSize];
        mb->searchmask = new unsigned char[meminfo->RegionSize / 8];
        memset(mb->searchmask, 0xff, meminfo->RegionSize/8);
        mb->matches = meminfo->RegionSize;
        mb->data_size = data_size;
        mb->next = nullptr;
    }
    return mb;
}

void free_memblock(MEMBLOCK *mb) {
    if (mb) {
        if (mb->buffer)
            delete mb->buffer;
        if (mb->searchmask)
            delete mb->searchmask;
        delete mb;
    }
}

// Find a way to trim memory blocks to save memory

// Returns mb->next and whether last was deleted
pair<MEMBLOCK*, bool> update_memblock(MEMBLOCK *mb, SEARCH_CONDITION condition, const vector<double>& val) {
    static unsigned char tempbuf[128*1024];
    unsigned long long bytes_left;
    unsigned long long total_read;
    unsigned long long bytes_to_read;
    unsigned long long bytes_read;

    pair<MEMBLOCK*, bool> ret = {mb->next, false};

    if (mb->matches > 0) {
        bytes_left = mb->size;
        total_read = 0;
        mb->matches = 0;
        
        while (bytes_left) {
            bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
            ReadProcessMemory (mb->hProc, mb->addr + total_read, tempbuf, bytes_to_read, &bytes_read);

            if (bytes_read != bytes_to_read) break;

            if (condition == COND_UNCONDITIONAL) {
                memset(mb->searchmask + (total_read/8), 0xff, bytes_read/8);
                mb->matches += bytes_read;
            } else {
                unsigned long long offset;

                for (offset = 0; offset < bytes_read; offset += mb->data_size) {
                    if (IS_IN_SEARCH(mb, (total_read+offset))) {
                        bool is_match = false;
                        unsigned temp_val;
                        unsigned int prev_val = 0;
                        
                        switch (mb->data_size) {
                            case 1:
                                temp_val = tempbuf[offset];
                                prev_val = *((unsigned char*)&mb->buffer[total_read+offset]);
                                break;
                            case 2:
                                temp_val = *((unsigned short*)&tempbuf[offset]);
                                prev_val = *((unsigned short*)&mb->buffer[total_read+offset]);
                                break;
                            case 4:
                                temp_val = *((unsigned*)&tempbuf[offset]);
                                prev_val = *((unsigned*)&mb->buffer[total_read+offset]);
                                break;
                            case 8:
                                // Takes address at tempbuff[offset]
                                // Converts to a pointer to double
                                // Dereferences to grab double value of 8 bytes
                                temp_val = *((double*)&tempbuf[offset]);
                                prev_val = *((double*)&mb->buffer[total_read+offset]);
                                break;
                                
                        }

                        switch (condition) {
                            case COND_EQUALS:
                                is_match = (temp_val == val[0]);
                                break;
                            case COND_INCREASED:
                                is_match = (temp_val > prev_val);
                                break;
                            case COND_DECREASED:
                                is_match = (temp_val < prev_val);
                                break;
                            case COND_BETW:
                                is_match = (temp_val > val[0] && temp_val < val[1]);
                                break;
                        }

                        if (is_match) {
                            mb->matches++;
                        } else {
                            REMOVE_FROM_SEARCH(mb, (total_read+offset));
                        }

                        

                    }
                }
            }

            memcpy(mb->buffer + total_read, tempbuf, bytes_read);

            bytes_left -= bytes_read;
            total_read += bytes_read;
        }
        
        // Remove block if matches is 0 by the end?
        if (mb->matches > 0)
            mb->size = total_read;
        else {
            if (mb->prev)
                mb->prev->next = ret.first;
            if (ret.first) {
                ret.first->prev = mb->prev;
            }
            free_memblock(mb);
            ret.second = true;
        }
    }

    return ret;
}

MEMBLOCK* create_scan (unsigned pid, int data_size) {
    MEMBLOCK* mb_list = nullptr;
    MEMORY_BASIC_INFORMATION meminfo;
    unsigned char* addr = 0;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (hProc) {
        while (true) {
            if (VirtualQueryEx(hProc, addr, &meminfo, sizeof(meminfo)) == 0) break;

#define WRITABLE (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
            if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE)) {
                MEMBLOCK *mb = create_memblock(hProc, &meminfo, data_size);
                if (mb) {
                    mb->next = mb_list;

                    if (mb_list) {
                        mb_list->prev = mb;
                    }

                    mb_list = mb;


                }
            }
            addr = (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize;
        }
    }

    return mb_list;
}

void free_scan(MEMBLOCK *mb_list) {
    CloseHandle(mb_list->hProc);
    while (mb_list) {
        MEMBLOCK *mb = mb_list;
        mb_list = mb_list->next;
        free_memblock(mb);
    }
}

void dump_scan_info (MEMBLOCK *mb_list) {
    MEMBLOCK *mb = mb_list;
    while (mb) {
        printf("0x%08x %d\r\n", mb->addr, mb->size);

        int i;
        for (i = 0; i < mb->size; i++) {
            printf("%02x", mb->buffer[i]);
        }
        printf("\r\n");

        mb = mb->next;
    }
}



void update_scan(MEMBLOCK*& mb_list, SEARCH_CONDITION condition, const vector<double>& val) {
    MEMBLOCK* mb = mb_list;
    bool onHead = true;
    
    while (mb) {
        pair<MEMBLOCK*, bool> updOut;
        updOut = update_memblock(mb, condition, val);

        if (onHead) {
            if (updOut.second) {
                mb_list = updOut.first;
            } else {
                onHead = false;
            }
        }

        mb = updOut.first;
    }
}

// void poke(HANDLE hProc, int data_size, unsigned addr, unsigned val) {
//     if (WriteProcessMemory(hProc, (void*)addr, &val, data_size, NULL) == 0) {
//         printf("poke failed\r\n");
//     }
// }

double peek (HANDLE hProc, int data_size, unsigned long long addr) {
    double val = 0.0;
    if (ReadProcessMemory(hProc, (PBYTE*)addr, &val, data_size, 0) == 0) {
        printf("peek failed\r\n");
    }
    return val;
}

void print_matches(MEMBLOCK* mb_list) {
    unsigned long long offset;
    MEMBLOCK* mb = mb_list;
    while (mb) {
        for (offset = 0; offset < mb->size; offset += mb->data_size) {
            if (IS_IN_SEARCH(mb, offset)) {
                double val = peek(mb->hProc, mb->data_size, (unsigned long long)mb->addr + offset);
                stringstream stream;
                stream << hex << (unsigned long long)mb->addr + offset;
                cout << stream.str() << " : " << val << endl;
                // printf("08%08x: 0x%08x (%d) \r\n", (unsigned long long)mb->addr + offset, val, val);
            }
        }
        mb = mb->next;
    }
}

int get_match_count(MEMBLOCK* mb_list) {
    MEMBLOCK* mb = mb_list;
    unsigned long long count = 0;
    while (mb) {
        count += mb->matches;
        mb = mb->next;
    }
    return count;
}


int main() {
    vector<double> search = {1000};

    MEMORY_BASIC_INFORMATION meminfo;
    unsigned char* addr = 0;

    // HWND hwnd = FindWindowA(NULL, "Con");




    DWORD procID;
    // GetWindowThreadProcessId(hwnd, &procID);
    
    // cout << "Proccess ID: " << procID << endl << endl;
    // HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

    cout << "Enter procID: ";
    cin >> procID;
    
    MEMBLOCK *scan  = create_scan(procID, 8);
    if (scan) {
        for (int i = 0; i < 4; i++) {
            int input;
            search.clear();

            cout << "Lower Bound: ";
            cin >> input;
            search.push_back(input);

            cout << "Upper bound: ";
            cin >> input;
            search.push_back(input);

            update_scan(scan, COND_BETW, search);
            cout << "Matches: " << get_match_count(scan) << endl;
            getchar();

        }
        print_matches(scan);
        free_scan(scan);
    }
            
    


        // cout << "Enter max value: ";
        // cin >> max;
        // cout << "Enter min value: ";
        // cin >> min;

        // ReadProcessMemory(handle, (PBYTE*)i, &read, sizeof(read), 0);
        // 60EABF2F8
        
    return 0;
}