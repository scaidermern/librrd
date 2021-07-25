#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>

#include "librrd.h"

template <class T>
T stringToNumber(const std::string& str) {
    assert(!str.empty());

    T number = 0;
    std::stringstream sstr(str);
    sstr >> number;
    assert(sstr);

    return number;
}

/// return memory usage
using meminfo_t = std::tuple<double, double, double>;
meminfo_t get_meminfo() {
    std::ifstream file("/proc/meminfo");

    rrd_data_point::data_point mem_available(0.0);
    rrd_data_point::data_point mem_buffers(0.0);
    rrd_data_point::data_point mem_cached(0.0);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream sstr(line);
        std::string key, value, unit;
        sstr >> key >> value >> unit;

        if (key == "MemAvailable:") {
            mem_available = stringToNumber<rrd_data_point::data_point>(value);
        } else if (key == "Buffers:") {
            mem_buffers = stringToNumber<rrd_data_point::data_point>(value);
        } else if (key == "Cached:") {
            mem_cached = stringToNumber<rrd_data_point::data_point>(value);
            break;
        }
    }

    return std::make_tuple(mem_available, mem_buffers, mem_cached);
}

/// return CPU usage
/// note: first value is bogus (average since startup)
double get_cpu_usage() {
    using value_t = unsigned long long;
    static value_t last_total = 0.0;
    static value_t last_work = 0.0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);

    std::stringstream sstr(line);
    std::string tmp; // "cpu" in first line
    value_t user, nice, system, idle, iowait, irq, softirq;
    sstr >> tmp >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

    value_t cur_total = user + nice + system + idle + iowait + irq + softirq;
    value_t cur_work = cur_total - idle;

    value_t total = cur_total - last_total;
    value_t work = cur_work - last_work;

    last_total = cur_total;
    last_work = cur_work;

    return (work / (double)total) * 100.0;
}

int main() {
    // measurement parameters
    std::chrono::seconds interval(1);
    std::chrono::minutes duration(1);

    // rrd parameters
    int steps      = 5;      // create new RRA entry every 5 seconds
    int rows_all   = 30;     // 30 seconds coverage of RRA that holds all data points (each RRA entry equals one data point)
    int rows_other = 60 * 24; // 1 day coverage of all other RRAs (each RRA entry spans 60 seconds)

    rrd_data cpu_usage("cpu", std::list<rrd_archive>{
        // all data points, max 3 minutes
        rrd_archive("all", 1, rows_all, rrd_archive::AVG),
        // minutely minimum, max 1 day
        rrd_archive("min", steps, rows_other, rrd_archive::MIN),
        // minutely maximum, max 1 day
        rrd_archive("max", steps, rows_other, rrd_archive::MAX),
        // minutely average, max 1 day
        rrd_archive("avg", steps, rows_other, rrd_archive::AVG)
    });

    rrd_data mem_available("mem_available", std::list<rrd_archive>{
        // all data points, max 3 minutes
        rrd_archive("all", 1, rows_all, rrd_archive::AVG),
        // minutely minimum, max 1 day
        rrd_archive("min", steps, rows_other, rrd_archive::MIN),
        // minutely maximum, max 1 day
        rrd_archive("max", steps, rows_other, rrd_archive::MAX),
        // minutely average, max 1 day
        rrd_archive("avg", steps, rows_other, rrd_archive::AVG)
    });
    rrd_data mem_buffers("mem_buffers", std::list<rrd_archive>{
        // all data points, max 3 minutes
        rrd_archive("all", 1, rows_all, rrd_archive::AVG),
        // minutely minimum, max 1 day
        rrd_archive("min", steps, rows_other, rrd_archive::MIN),
        // minutely maximum, max 1 day
        rrd_archive("max", steps, rows_other, rrd_archive::MAX),
        // minutely average, max 1 day
        rrd_archive("avg", steps, rows_other, rrd_archive::AVG)
    });
    rrd_data mem_cached("mem_cached", std::list<rrd_archive>{
        // all data points, max 3 minutes
        rrd_archive("all", 1, rows_all, rrd_archive::AVG),
        // minutely minimum, max 1 day
        rrd_archive("min", steps, rows_other, rrd_archive::MIN),
        // minutely maximum, max 1 day
        rrd_archive("max", steps, rows_other, rrd_archive::MAX),
        // minutely average, max 1 day
        rrd_archive("avg", steps, rows_other, rrd_archive::AVG)
    });

    (void)get_cpu_usage(); // throw away first value
    std::this_thread::sleep_for(interval);

    std::cout << "creating " << (duration / interval) << " data points, this will take "
	      << std::chrono::duration_cast<std::chrono::seconds>(duration).count()
	      << " seconds" << std::endl;
    for (int i = 0; i < (duration / interval); ++i) {
        const auto now = rrd_data_point::clock::now();
        std::cout << "." << std::flush;

        double cpu = get_cpu_usage();
        cpu_usage.add(cpu, now);

        meminfo_t mem_info = get_meminfo();
        mem_available.add(std::get<0>(mem_info), now);
        mem_buffers.add(std::get<1>(mem_info), now);
        mem_cached.add(std::get<2>(mem_info), now);

        const auto end = rrd_data_point::clock::now();
        std::this_thread::sleep_for(interval - (end - now));
    }
    std::cout << std::endl;

    cpu_usage.dump("cpu_usage_");
    mem_available.dump("mem_available_");
    mem_buffers.dump("mem_buffers_");
    mem_cached.dump("mem_cached_");
}
