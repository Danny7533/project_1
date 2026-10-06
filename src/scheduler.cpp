//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//

#include <queue>
#include "scheduler.hpp"

std::queue<ProcessId_t> readyQ;
// scheduler grabs from C1 queue
std::queue<ProcessId_t> C1Q;
ProcessId_t running = InvalidProcessId();


void setCState (CPUId_t core_id, CState_t state) {
    //if C1 add to queue and alert scheduler
    //if ()
}

void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);
    if(running == InvalidProcessId()) {
        running = pid;
        LoadContext(running, 4);
        RunCore(4);
    }
    else {  // There is already a running process
        readyQ.push(pid);
    }
}

void ExitProcess(ProcessId_t pid) {
    // Process finished running. Update the scheduler's data structures and decisions accordingly.
    //here we move p-state down since we keep finishing
    if(running != pid) {
        ThrowException("A process that was not running is calling exit!!!");
    }
    if(!readyQ.empty()){
        running = readyQ.front();
        readyQ.pop();
        LoadContext(running, 4);
        RunCore(4);
    }
    else {
        running = InvalidProcessId();   // Nothing is running right now
    }
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions
    if(running == InvalidProcessId())       // Nothing to do
        return;
    // adjust c-state and p-state potentially on workload
    // Someone was running
    if(readyQ.empty())                      // We have a running process but no other processes are waiting
        return;
    //either maintain or up p-state if we keep hitting interrupt
    SaveContext(running, 4);
    readyQ.push(running);
    running = readyQ.front();
    readyQ.pop();
    LoadContext(running, 4);
    RunCore(4);
}

//grab c1 cores 
void schedule() {
}

void CStateTransitionComplete(CPUId_t core_id){
    
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
}
