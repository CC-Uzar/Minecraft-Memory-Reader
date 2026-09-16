#include "MemoryScan.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <algorithm>

#define IS_IN_SEARCH(mb,offset) ((mb)->searchmask[(offset)/8] & (1<<((offset) % 8)))
#define REMOVE_FROM_SEARCH(mb,offset) (mb)->searchmask[(offset)/8] &= ~(1<<((offset) % 8));

void MemoryScan::setPID(int p) {
    if (p < 0)
        throw InvalidPID();
    pid = p;
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProc == NULL) throw InvalidPID();
}

void MemoryScan::setBounds(double l, double h) {
    if (l >= h) throw InvalidSearchBounds();

    min = l;
    max = h;
}

void MemoryScan::createScan() {
    if (hProc == NULL) throw InvalidPID();

    MEMORY_BASIC_INFORMATION meminfo;
    std::uintptr_t addr = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        if (VirtualQueryEx(hProc, (void*)addr, &meminfo, sizeof(meminfo)) == 0) break;

#define WRITABLE (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

        if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE)) {

            std::unique_ptr<unsigned char[]> searchmask = std::make_unique<unsigned char[]>(meminfo.RegionSize / 8);
            memset(searchmask.get(), 0xff, meminfo.RegionSize / 8);
                
            mb_list.emplace_back((std::uintptr_t)meminfo.BaseAddress, meminfo.RegionSize, std::move(searchmask), meminfo.RegionSize, data_size);
        }

        addr = (std::uintptr_t)meminfo.BaseAddress + meminfo.RegionSize;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Duration to create: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start) << std::endl;
}

void MemoryScan::clearScan() {
    mb_list.clear();
}

size_t MemoryScan::getMatchCount() {
    size_t matches = 0;
    for (const MEMBLOCK& mb : mb_list)
        matches += mb.matches;
    return matches;
}

void MemoryScan::searchScan(size_t first, size_t last) {
    if ((first < 0) || (last > mb_list.size())) throw InvalidIndex();

    static unsigned char tempbuf[128*1024];
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = first; i < last; i++) {
        MEMBLOCK* mb = &(mb_list.at(i));

        unsigned long long bytes_left;
        unsigned long long total_read;
        unsigned long long bytes_to_read;
        unsigned long long bytes_read;

        if (mb->matches > 0) {
            bytes_left = mb->size;
            total_read = 0;
            mb->matches = 0;

            while (bytes_left) {
                bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
                ReadProcessMemory (hProc, (PBYTE*)(mb->addr + total_read), tempbuf, bytes_to_read, &bytes_read);

                if (bytes_read != bytes_to_read) break;

                unsigned long long offset;
                for (unsigned long long offset = 0; offset < bytes_read; offset += data_size) {
                    if (IS_IN_SEARCH(mb, total_read + offset)) {
                        double temp_val = *((double*)&tempbuf[offset]);

                        if (temp_val > min && temp_val < max) {
                            mb->matches++;
                        } else {
                            REMOVE_FROM_SEARCH(mb, (total_read + offset));
                        }
                    }


                }

                bytes_left -= bytes_read;
                total_read += bytes_read;
            }
        }

        if (mb->matches > 0)
            mb->size = total_read;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Duration to search: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start) << std::endl;
}

void MemoryScan::clearMisses() {
    auto start = std::chrono::high_resolution_clock::now();
    mb_list.erase(std::remove_if(mb_list.begin(), mb_list.end(), [](MEMBLOCK& mb) {
        return mb.matches == 0;
    }), mb_list.end());
    mb_list.shrink_to_fit();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Duration to delete: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start) << std::endl;
}

size_t MemoryScan::getSize() {
    return mb_list.size();
}


void MemoryScan::printMatches() {
    for (MEMBLOCK& mb : mb_list) {
        for (std::uintptr_t offset = 0; offset < mb.size; offset += data_size) {
            if (IS_IN_SEARCH(&mb, offset)) {
                double val = peek(mb.addr + offset);
                std::stringstream stream;
                stream << std::hex << mb.addr + offset;
                std::cout << stream.str() << " : " << val << std::endl;
            }
        }
    }
}

double MemoryScan::peek(std::uintptr_t addr) {
    double val = 0.0;
    if (ReadProcessMemory(hProc, (PBYTE*)addr, &val, data_size, 0) == 0)
        std::cout << "Peek failed\n";
    return val;
}

std::vector<std::uintptr_t> MemoryScan::getMatches() {
    std::vector<std::uintptr_t> matches;
    for (const MEMBLOCK& mb : mb_list) {
        for (std::uintptr_t offset = 0; offset < mb.size; offset += data_size) {
            if (IS_IN_SEARCH(&mb, offset))
                matches.push_back(mb.addr + offset);
        }
    }
    return matches;
}