#ifndef MEMORY_SCAN_H
#define MEMORY_SCAN_H
#include <Windows.h>
#include <vector>

struct MEMBLOCK {
    unsigned long long addr;
    size_t size;
    // unsigned char *searchmask;
    unsigned matches;
    size_t data_size;
};

class MemoryScan {
    private:
        std::vector<MEMBLOCK> mb_list;
        std::vector<unsigned long long> addr_list;
        int pid = -1;
        HANDLE hProc = NULL;
        double min = 1.0;
        double max = -1.0;
        size_t data_size = 8;

    public:
        void setPID(int);
        void setBounds(double, double);
        void createScan();
        void convert(size_t, size_t);
        std::vector<unsigned long long> updateScan(size_t, size_t);
        size_t getMatchCount();
        const std::vector<unsigned long long>& getMatches();

        class InvalidSearchBounds{};
        class InvalidIndex{};
        class InvalidPID{};

};

#endif