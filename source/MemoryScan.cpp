#include "MemoryScan.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

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
    unsigned long long addr = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        if (VirtualQueryEx(hProc, (void*)addr, &meminfo, sizeof(meminfo)) == 0) break;

#define WRITABLE (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

        if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE)) {

            // for (size_t offset = 0; offset < meminfo.RegionSize; offset += data_size) {
            //     // std::stringstream stream;
            //     // stream << std::hex << addr+offset;
            //     // std::cout << "Found address: " << stream.str() << std::endl;
            //     addr_list.push_back(addr+offset);
            // }

                
            mb_list.emplace_back((unsigned long long)meminfo.BaseAddress, meminfo.RegionSize, 0, 8);
        }

        addr = (unsigned long long)meminfo.BaseAddress + meminfo.RegionSize;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Duration to create: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start) << std::endl;
}

size_t MemoryScan::getMatchCount() {
    return mb_list.size();
}

// MemoryScan::convert(size_t first, size_t last) {
//     if ((first < 0) || (last >= mb_list.last)
// }
