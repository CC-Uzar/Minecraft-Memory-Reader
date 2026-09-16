#ifndef MEMORY_SCAN_H
#define MEMORY_SCAN_H
#include <Windows.h>
#include <vector>
#include <cstdint>
#include <memory>

struct MEMBLOCK {
    std::uintptr_t addr;
    size_t size;
    std::unique_ptr<unsigned char[]> searchmask;
    size_t matches;
    size_t data_size;

    MEMBLOCK(std::uintptr_t a, size_t s, std::unique_ptr<unsigned char[]> sm, size_t m, size_t ds) :
        addr(a), size(s), searchmask(std::move(sm)), matches(m), data_size(ds) {};
};

class MemoryScan {
    private:
        std::vector<MEMBLOCK> mb_list;
        int pid = -1;
        HANDLE hProc = NULL;
        double min = 1.0;
        double max = -1.0;
        size_t data_size = 8;

    public:
        void setPID(int);
        void setBounds(double, double);
        void createScan();
        void searchScan(size_t, size_t);
        void clearMisses();

        size_t getMatchCount();
        std::vector<std::uintptr_t> getMatches();
        size_t getSize();
        void printMatches();
        double peek(std::uintptr_t);

        class InvalidSearchBounds{};
        class InvalidIndex{};
        class InvalidPID{};

};

#endif