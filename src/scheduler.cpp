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
}

bool ascendCores(int workload) {
    return workload > NUM_CORES * 3;
}

void idlePolicy(CPUId_t core) {
    if (coreCStatePending[core]) {
        return;
    }
    switch (idleTicks[core]) {
        case 2:
            SetCState(core, C1);
            coreCState[core] = C1;
            break;
        case 4:
            SetCState(core, C2);
            coreCState[core] = C2;
            break;
        case 6:
            SetCState(core, C3);
            coreCState[core] = C3;
            break;
        case 8:
            SetCState(core, C4);
            coreCState[core] = C4;
            break;
        case 10:
            SetCState(core, C6);
            coreCState[core] = C6;
            break;
        default:
            break;
    }
}

void ascendScheduler(PState_t pState) {
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] != InvalidProcessId()) {
            continue;
        }
        if (readyQ.empty())
            break;
        if (coreCState[core]!=C1&&coreCState[core]!=C2||coreCStatePending[core]) {
            idleTicks[core]=0;
            if(!coreCStatePending[core]){
                coreCState[core]=C1;
                SetCState(core, C1);
                coreCStatePending[core]=true;
            }
            continue;
        }
        if (!coreCStatePending[core]&&coreCState[core]!=C2) {
            ProcessId_t pid = readyQ.front();
            readyQ.pop();
            coreRunning[core] = pid;
            idleTicks[core] = 0;
            SetPState(core, pState);
            LoadContext(coreRunning[core], core);
            RunCore(core);
        }

    }
}

void descendScheduler(PState_t pState) {
    for (CPUId_t core = NUM_CORES - 1; core >= 0; core--) {
        if (core > 8)
            break;
        if (coreRunning[core] == InvalidProcessId()) {
            if (readyQ.empty())
                break;
            if (coreCState[core]!=C1&&coreCState[core]!=C2||coreCStatePending[core]) {
                idleTicks[core]=0;
                if(!coreCStatePending[core]){
                    coreCState[core]=C1;
                    SetCState(core, C1);
                    coreCStatePending[core]=true;
                }
            }
            if (!coreCStatePending[core]&&coreCState[core]!=C2) {
                ProcessId_t pid = readyQ.front();
                readyQ.pop();
                coreRunning[core] = pid;
                idleTicks[core] = 0;
                SetPState(core, pState);
                LoadContext(coreRunning[core], core);
                RunCore(core);
            }

        }

    }

}

void schedule() {
    if (readyQ.empty())
        return;
    int workload = readyQ.size() + coresRunning();
    PState_t pState = selectPState(workload);
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] != InvalidProcessId()) {
            SetPState(core, pState);
        }
    }
    
    if (ascendCores(workload)) {
        ascendScheduler(pState);
    } else {
        descendScheduler(pState);
    }

}


void CreateProcess(ProcessId_t pid) {
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
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions
    
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
    schedule();
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreRunning[core] == InvalidProcessId()&&!coreCStatePending[core]) {
            idleTicks[core]++;
            idlePolicy(core);
        }
    }
    bool allC6 = true;
    for (CPUId_t core = 0; core < NUM_CORES; core++) {
        if (coreCState[core] != C6) {
            allC6 = false;
            break;
        }
    }
    if (allC6) {
        SetCState(0, C7);
        for (CPUId_t core = 0; core < NUM_CORES; core++) {
            coreCState[core]=C7;
        }
    }
}

void CStateTransitionComplete(CPUId_t core_id){
    coreCStatePending[core_id]=false;
    schedule();
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
}
