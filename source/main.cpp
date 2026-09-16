#include "MemoryScan.h"

#include <iostream>

using namespace std;

int main() {

    double min, max;
    MemoryScan memscan;
    memscan.setPID(18372);
    memscan.setBounds(0.0, 1000.0);

    memscan.createScan();
    // cout << "Match count: " << memscan.getMatchCount() << endl;
    cout << "Memory block count: " << memscan.getSize() << endl;

    while (min < 100000) {
        cout << "Enter min value: ";
        cin >> min;

        cout << "Enter max value: ";
        cin >> max;
        if (min < max) {
            memscan.setBounds(min, max);
            memscan.searchScan(0, memscan.getSize());
            memscan.clearMisses();
        } else {
            memscan.printMatches();
        }
            

        cout << "Match count: " << memscan.getMatchCount() << endl;
        cout << "Memory block count: " << memscan.getSize() << endl;

    }

    getchar();
    
    return 0;
}