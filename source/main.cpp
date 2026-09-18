#include "MemoryScan.h"

#include <iostream>

using namespace std;

int main() {

    double min, max;
    MemoryScan memscan(3);
    int pid;
    cout << "Enter PID: ";
    cin >> pid;

    memscan.setPID(pid);
    memscan.createScan(true);
    memscan.setBounds(20.0,25.0);
    memscan.searchScan(true);
    memscan.clearMisses(true);

    return 0;
}