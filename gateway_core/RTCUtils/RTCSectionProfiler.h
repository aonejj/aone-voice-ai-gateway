#ifndef __RTC_SECTION_PROFILER_H__
#define __RTC_SECTION_PROFILER_H__

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <time.h>

namespace rtcprof {

inline int64_t WallUs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000LL + ts.tv_nsec / 1000;
}

inline int64_t ThreadCpuUs() {
    timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000LL + ts.tv_nsec / 1000;
}

}  // namespace rtcprof

class RTCSectionStat {
public:
    explicit RTCSectionStat(const char* name, size_t report_every = 500)
        : _name(name), _report_every(report_every) {
        _wall.reserve(report_every);
        _cpu.reserve(report_every);
    }

    void Add(int64_t wall_us, int64_t cpu_us = -1) {
        _wall.push_back(wall_us);
        if (cpu_us >= 0) _cpu.push_back(cpu_us);
    }

    bool ReportDue() const { return _wall.size() >= _report_every; }

    std::string TakeReport() {
        char buf[320];
        if (_wall.empty()) {
            std::snprintf(buf, sizeof(buf), "[prof] %s: no samples", _name);
            return buf;
        }

        std::vector<int64_t> w(_wall);
        std::sort(w.begin(), w.end());
        size_t over1 = 0, over5 = 0;
        for (int64_t v : w) {
             if (v > 1000) ++over1; 
             if (v > 5000) ++over5; 
        }

        int n = std::snprintf(buf, sizeof(buf),
            "[prof] %s n=%zu wall(us) p50= %lld p99= %lld max= %lld  >1ms= %zu >5ms= %zu",
            _name, w.size(), (long long)Pct(w, 0.50), (long long)Pct(w, 0.99),
            (long long)w.back(), over1, over5);

        if (!_cpu.empty() && n > 0 && static_cast<size_t>(n) < sizeof(buf)) {
            std::vector<int64_t> c(_cpu);
            std::sort(c.begin(), c.end());
            std::snprintf(buf + n, sizeof(buf) - n,
                "  | cpu(us) p50= %lld p99= %lld max= %lld",
                (long long)Pct(c, 0.50), (long long)Pct(c, 0.99), (long long)c.back());
        }

        _wall.clear();
        _cpu.clear();
        return buf;
    }

private:
    static int64_t Pct(const std::vector<int64_t>& sorted, double p) {
        size_t idx = static_cast<size_t>(p * static_cast<double>(sorted.size()));
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    const char* _name;
    size_t _report_every;
    std::vector<int64_t> _wall;
    std::vector<int64_t> _cpu;
};

class RTCScopedSectionTimer {
public:
    explicit RTCScopedSectionTimer(RTCSectionStat& stat)
        : _stat(stat), _w0(rtcprof::WallUs()), _c0(rtcprof::ThreadCpuUs()) {}
    ~RTCScopedSectionTimer() {
        _stat.Add(rtcprof::WallUs() - _w0, rtcprof::ThreadCpuUs() - _c0);
    }
private:
    RTCSectionStat& _stat;
    int64_t _w0, _c0;
};

#endif