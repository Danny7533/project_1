#include "scheduler.hpp"
#include <iostream>

int main() {
    CPUId_t core_id = 0;
    setCState(0, CState_t::C0);
    std::cout << "Starting test..." << std::endl;
    return 0;
}