#pragma once

#include <stdint.h>
#include <vector>
#include <deque>

#ifdef WIN32

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN 1
// windows defs
#include <windows.h>
#include <Psapi.h>
#include <tchar.h>
#include <pdh.h>
#else
// unix defs

#endif

struct Process_stats_snapshot {
    uint64_t ram_virt_total_bytes;
    uint64_t ram_virt_used_bytes;
    uint64_t ram_virt_process_used_bytes;
    uint64_t ram_phys_total_bytes;
    uint64_t ram_phys_used_bytes;
    uint64_t ram_phys_process_used_bytes;
    int64_t avg_tick_idle_time; // negative means we are lagging
    uint64_t num_good_ticks;
    uint64_t num_lag_ticks;

    double cpu_load_total;
    double cpu_load_process;
};


#ifdef WIN32

static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static HANDLE self;

struct Process_stats {
    std::deque<int64_t> tick_idle_timings;
    bool is_overloaded;
    uint64_t num_good_ticks;
    uint64_t num_lag_ticks;

    Process_stats() {
        is_overloaded = false;
        num_good_ticks = 0;
        num_lag_ticks = 0;

        init_process_cpu_query();
        init_global_cpu_query();
    }

    void add_tick_idle_timing(int64_t amount_of_idle_time) {
        tick_idle_timings.push_back(amount_of_idle_time);

        if (tick_idle_timings.size() > 100) {
            tick_idle_timings.erase(tick_idle_timings.begin());
        }

        if (amount_of_idle_time > 0) {
            num_good_ticks++;
        }
        else {
            num_lag_ticks++;
        }
    }

    int64_t calc_average_idle_time() {
        if (tick_idle_timings.size() == 0) {
            return 0;
        }

        int64_t total = 0;
        for (auto it = tick_idle_timings.begin(); it != tick_idle_timings.end(); ++it) {
            total += *it;
        }
        
        total = total / (int64_t)tick_idle_timings.size();

        return total;
    }

    // https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    bool gather_stats(Process_stats_snapshot& snapshot) {
        // the average time we are idle
        snapshot.avg_tick_idle_time = calc_average_idle_time();
        snapshot.num_good_ticks = num_good_ticks;
        snapshot.num_lag_ticks = num_lag_ticks;


        /// Total virtual memory
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        snapshot.ram_virt_total_bytes = memInfo.ullTotalPageFile;

        // virtual memory currently used
        snapshot.ram_virt_used_bytes = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;

        // virtual memory used by current process
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        snapshot.ram_virt_process_used_bytes = pmc.PrivateUsage;

        // total physical memory
        snapshot.ram_phys_total_bytes = memInfo.ullTotalPhys;

        // physical memory currently used
        snapshot.ram_phys_used_bytes = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

        // physical memory used by current process
        snapshot.ram_phys_process_used_bytes = pmc.WorkingSetSize;

        // the cpu load
        snapshot.cpu_load_total = query_global_cpu();

        // cpu load for process
        snapshot.cpu_load_process = query_process_cpu();

        return true;
    }

    void init_process_cpu_query() {
        SYSTEM_INFO sysInfo;
        FILETIME ftime, fsys, fuser;

        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;

        GetSystemTimeAsFileTime(&ftime);
        memcpy(&lastCPU, &ftime, sizeof(FILETIME));

        self = GetCurrentProcess();
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
        memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
    }

    void init_global_cpu_query() {
        PdhOpenQuery(NULL, NULL, &cpuQuery);
        // You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
        PdhAddEnglishCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
        PdhCollectQueryData(cpuQuery);
    }

    double query_global_cpu() {
        PDH_FMT_COUNTERVALUE counterVal;

        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
        return counterVal.doubleValue;
    }

    double query_process_cpu() {
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        double percent;

        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));

        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        percent = (sys.QuadPart - lastSysCPU.QuadPart) +
            (user.QuadPart - lastUserCPU.QuadPart);
        percent /= (now.QuadPart - lastCPU.QuadPart);
        //percent /= numProcessors;
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;

        return percent * 100;
    }
};

#else 

struct Process_stats {
    std::deque<int64_t> tick_idle_timings;
    bool is_overloaded;
    uint64_t num_good_ticks;
    uint64_t num_lag_ticks;
    
    Process_stats() {}

    // https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    bool gather_stats(Process_stats_snapshot& snapshot) {
        return false;        
    }

    void add_tick_idle_timing(int64_t amount_of_idle_time) {
        
    }

    int64_t calc_average_idle_time() {
        
        return 0;
    }

    void init_process_cpu_query() {
        
    }

    void init_global_cpu_query() {
        
    }

    double query_global_cpu() {
        
        return 0;
    }

    double query_process_cpu() {
        return 0;
    }
};
#endif