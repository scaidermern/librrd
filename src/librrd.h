#ifndef LIBRRD_H_
#define LIBRRD_H_

#include <chrono>
#include <deque>
#include <list>
#include <memory>
#include <string>

#ifdef DEBUG
#include <iostream>
#define LOG(msg)   do { std::cout << msg << std::endl; } while (0)
#define LOGNL(msg) do { std::cout << msg; } while (0)
#else
#define LOG(msg) do {} while (0)
#define LOGNL(msg) do {} while (0)
#endif

/// single data point
class rrd_data_point {
public:
    using data_point = double;
    using clock = std::chrono::system_clock;
    using time_point = std::chrono::time_point<clock>;

    rrd_data_point(data_point value, time_point time);

    /// return value of data point
    data_point value() const { return value_; }
    /// return time of data point
    time_point time()  const { return time_; }

private:
    data_point value_;
    time_point time_;
};

/// round robin archive (RRA) of consolidated data points (CDPs)
class rrd_archive {
public:
    /// consolidate function for aggregating PDPs to RRA entries
    enum consolidate_function {
        AVG,
        MIN,
        MAX
    };

    /// duration resolution for dumping archive content
    using dump_resolution = std::chrono::milliseconds;

    rrd_archive(std::string name, unsigned int steps, unsigned int rows, int cf);

    /// add new primary data point (PDPs)
    void add(std::shared_ptr<rrd_data_point> data);

    /// return name of the archive
    std::string const& name()   const { return name_; }
    /// return number of primary data points (PDPs) to consolidate for one RRA entry
    unsigned int steps() const { return steps_; }
    /// return maximum number of RRA entries until the oldest gets overwritten
    unsigned int rows()  const { return rows_; }
    /// return all RRA entries
    std::deque<rrd_data_point> const& archive() const { return archive_; }

    /// return consolidation function
    int cf() const { return cf_; }
    /// return human readable description of consolidation function
    std::string cf_to_str() const;

    /// dump RRA content to stream
    void dump(std::ostream& out) const;

private:
    /// consolidate PDPs to a new RRA entry
    void consolidate();
    /// aggregate PDPs with the configured consolidation function
    rrd_data_point aggregate();

    /// return sum of all CDPs
    rrd_data_point::data_point sum() const;
    /// return minimum of all CDPs
    std::shared_ptr<rrd_data_point> min() const;
    /// return maximum of all CDPs
    std::shared_ptr<rrd_data_point> max() const;

    /// name of the round robin archive (RRA)
    std::string name_;
    /// number of primary data points (PDPs) to consolidate for one RRA entry
    unsigned int steps_;
    /// maximum number of RRA entries until the oldest gets overwritten
    unsigned int rows_;
    /// consolidate function to use for aggregating PDPs to RRA entries
    int cf_;
    /// primary data points (PDPs)
    std::deque<std::shared_ptr<rrd_data_point>> datapoints_;
    /// round robin archive (RRA) of consolidated data points (CDPs)
    std::deque<rrd_data_point> archive_;
};

/// database of multiple RRAs
class rrd_data {
public:
    rrd_data(std::string name, std::list<rrd_archive> archives);

    /// add new primary data point (PDP) to all archives
    void add(rrd_data_point::data_point value, rrd_data_point::time_point time);

    /// return name of the database
    std::string const& name() const { return name_; }
    /// return all archives
    std::list<rrd_archive> const& archives() const { return archives_; }

    /// dump all RRAs to a file
    void dump(std::string const& prefix = "") const;

private:
    /// name of the data
    std::string name_;
    /// multiple round robin archives (RRA) with consolidated data points (CDPs)
    std::list<rrd_archive> archives_;
};

#endif // LIBRRD_H_
