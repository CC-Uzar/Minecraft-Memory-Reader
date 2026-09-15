#include "MemoryScan.h"

#include <iostream>

using namespace std;

int main() {

    MemoryScan memscan;
    memscan.setPID(12900);
    memscan.setBounds(0.0, 1000.0);

    memscan.createScan();
    cout << memscan.getMatchCount() << endl;

    getchar();
    
    return 0;
}