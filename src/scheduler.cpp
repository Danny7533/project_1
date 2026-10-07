//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//

#include <queue>
#include "scheduler.hpp"
#include "sim_types.h"
#include <cassert>

std::queue<ProcessId_t> readyQ;
const CPUId_t NUM_CORES = 8;
// scheduler grabs from C1 queue
std::queue<CPUId_t> C1Q;
//List of processes core is running
std::vector<ProcessId_t> coreRunning(NUM_CORES, InvalidProcessId());
// cStates of cores
std::vector<CState_t> coreCState(NUM_CORES, C1);
// keeps track of cState change
std::vector<bool> coreCStatePending(NUM_CORES, false);
// number of ticks core idles
std::vector<Time_t> idleTicks(NUM_CORES, 0);
ProcessId_t running = InvalidProcessId();

int coresRunning() {
    int count = 0; 
    for (CPUId_t core_id = 0; core_id < NUM_CORES; core_id++) {
        if (coreRunning[core_id] != InvalidProcessId()) {
            count++;
        }
    }
    return count;
}

PState_t selectPState(int workload) {
    if (workload > 16) {
        return P0;
    } else if (workload > 12) {
        return P1;
    } else if (workload > 10) {
        return P2;
    } else if (workload > 8) {
        return P3;
    }
    return P4;
    //some criteria to choose pState
}

void idlePolicy(CPUId_t core) {
    if (coreCStatePending[core]) {
        return;
    }
    switch (idleTicks[core]) {
        case 2:
            // std::cout<<"set "<<core<<" "<<"C1\n";
            SetCState(core, C1);
            coreCState[core] = C1;
            break;
        case 4:
            // std::cout<<"set "<<core<<" "<<"C2\n";
            SetCState(core, C2);
            coreCState[core] = C2;
            break;
        case 6:
            // std::cout<<"set "<<core<<" "<<"C3\n";
            SetCState(core, C3);
            coreCState[core] = C3;
            break;
        case 8:
            // std::cout<<"set "<<core<<" "<<"C4\n";
            SetCState(core, C4);
            coreCState[core] = C4;
            break;
        case 10:
            // std::cout<<"set "<<core<<" "<<"C6\n";
            SetCState(core, C6);
            coreCState[core] = C6;
            break;
        default:
            break;
    }
    //some criteria to chnage cState
}

void schedule() {
    if (readyQ.empty())
        return;
    // for (CPUId_t core = 0; core < NUM_CORES; core++) {
    //     std::cout << "idleTicks[" << core << "] = " << idleTicks[core] << "\n";
    // }
    int workload = readyQ.size() + coresRunning();
    PState_t pState = selectPState(workload);
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] != InvalidProcessId()) {
            continue;
        }
        if (readyQ.empty())
            break;
        if (coreCState[core]!=C1&&coreCState[core]!=C2||coreCStatePending[core]) {
            idleTicks[core]=0;
            if(!coreCStatePending[core]){
                // std::cout<<"Wake up "<<core<<" from C"<<coreCState[core]<<"\n";
                coreCState[core]=C1;
                SetCState(core, C1);
                coreCStatePending[core]=true;
            }
            continue;
        }
        // std::cout<<"run "<<core<<"\n";
        if (!coreCStatePending[core]&&coreCState[core]!=C2) {
            ProcessId_t pid = readyQ.front();
            readyQ.pop();
            coreRunning[core] = pid;
            idleTicks[core] = 0;
            SetPState(core, pState);
            // std::cout<<"run "<<core<<" with pid "<<coreRunning[core]<<"\n";
            LoadContext(coreRunning[core], core);
            RunCore(core);
        }

    }

}


void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    // SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);
    // if(running == InvalidProcessId()) {
    //     running = pid;
    //     LoadContext(running, 4);
    //     RunCore(4);
    // }
    // else {  // There is already a running process
    //     readyQ.push(pid);
    // }
    readyQ.push(pid);
    schedule();
}

void ExitProcess(ProcessId_t pid) {
    // Process finished running. Update the scheduler's data structures and decisions accordingly.
    //here we move p-state down since we keep finishing
    bool found = false;
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] == pid) {
            coreRunning[core] = InvalidProcessId();
            found = true;
            break;
        }
    }
    if (!found) {
        ThrowException("A process that was not running is calling exit!!!");
    }
    schedule();

    // if(running != pid) {
    //     ThrowException("A process that was not running is calling exit!!!");
    // }
    // for (CPUId_t core = 0; core < NUM_CORES; core++) {
    //     if (coreRunning[core] == pid) {
    //         coreRunning[core] = InvalidProcessId();
    //         break;
    //     }
    // }
    // if(!readyQ.empty()){
    //     running = readyQ.front();
    //     readyQ.pop();
    //     LoadContext(running, 4);
    //     RunCore(4);
    // }
    // else {
    //     running = InvalidProcessId();   // Nothing is running right now
    // }
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions
    // if (readyQ.empty())       // Nothing to do if no processes are waiting
    //     return;
    
    int cores_free = (int) NUM_CORES - (int)coresRunning();
    int num_to_schedule = std::min((int) readyQ.size()-cores_free, (int) NUM_CORES);
    for (CPUId_t core = 0; core < NUM_CORES && num_to_schedule > 0; core++) {
        if (coreRunning[core] != InvalidProcessId()) {
            SaveContext(coreRunning[core], core);
            readyQ.push(coreRunning[core]);
            coreRunning[core] = InvalidProcessId();
        }
        num_to_schedule--;
        
    }
    // for (CPUId_t core = 0; core < NUM_CORES; core++) {
    //     if (coreRunning[core] == InvalidProcessId()) {
    //         idlePolicy(core);
    //     }
    // }
    schedule();
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] == InvalidProcessId()&&!coreCStatePending[core]) {
            idleTicks[core]++;
            idlePolicy(core);
        }
    }
    // bool allC6 = true;
    // for (CPUId_t core = 0; core < NUM_CORES; core++) {
    //     if (coreCState[core] != C6) {
    //         allC6 = false;
    //         break;
    //     }
    // }
    // if (allC6) {
    //     std::cout << "All cores are in C6, transitioning core 0 to C7" << std::endl;
    //     SetCState(0, C7);
    //     for (CPUId_t core = 0; core < NUM_CORES; core++) {
    //         coreCState[core]=C7;
    //     }
    // }


    // if(running == InvalidProcessId())       // Nothing to do
    //     return;
    // // adjust c-state and p-state potentially on workload
    // // Someone was running
    // if(readyQ.empty())                      // We have a running process but no other processes are waiting
    //     return;
    // //either maintain or up p-state if we keep hitting interrupt
    // SaveContext(running, 4);
    // readyQ.push(running);
    // running = readyQ.front();
    // readyQ.pop();
    // LoadContext(running, 4);
    // RunCore(4);
}

void CStateTransitionComplete(CPUId_t core_id){
    // std::cout<<"Awake "<<core_id<<"\n";
    coreCStatePending[core_id]=false;
    schedule();
    // coreCStatePending[core_id] = false;
    // if (coreRunning[core_id] == InvalidProcessId()) {
    //     LoadContext(coreRunning[core_id], core_id);
    //     RunCore(core_id);
    // }
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
}
