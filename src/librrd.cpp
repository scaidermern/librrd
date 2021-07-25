#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <fstream>
#include <list>
#include <memory>
#include <numeric>
#include <utility>

#include "librrd.h"

rrd_data_point::rrd_data_point(data_point value, time_point time) :
    value_(value),
    time_(time) {
}

rrd_archive::rrd_archive(std::string name, unsigned int steps, unsigned int rows, int cf) :
    name_(name),
    steps_(steps),
    rows_(rows),
    cf_(cf),
    archive_() {
}

void rrd_archive::add(std::shared_ptr<rrd_data_point> data) {
    if (steps_ == 1) {
        // shortcut in case we don't want to store each PDP
        // without performing any consolidation at all
        archive_.push_front(*data);
    } else {
        datapoints_.push_front(data);

        // do we need to consolidate our PDPs to an RRA entry?
        if (datapoints_.size() >= steps_) {
            LOG("reached max PDPs " << datapoints_.size() << ", consolidating.");
            consolidate();
        }
    }

    // remove oldest RRA entry if maximum size reached
    if (archive_.size() >= rows_) {
        archive_.pop_back();
    }
}

void rrd_archive::consolidate() {
    // aggregate all PDPs to a new RRA entry
    auto rra = aggregate();
    LOG("aggregated RRA entry for cf " << cf_to_str() << ": " << rra.value());
    archive_.push_front(rra);

    // clear list of PDPs after consolidation
    datapoints_.clear();
}

rrd_data_point rrd_archive::aggregate() {
    switch(cf_) {
    case AVG:
        // time of aggregated data points will equal the time of the newest data point
        return rrd_data_point(sum(), datapoints_.front()->time());
    case MIN:
        // time of aggregated data points will equal the time of the minimum data point
        return *min();
    case MAX:
        // time of aggregated data points will equal the time of the maximum data point
        return *max();
    default:
        assert(false && "unknown consolidation function");
        return rrd_data_point(0.0, datapoints_.front()->time());
    }
}

rrd_data_point::data_point rrd_archive::sum() const {
    return std::accumulate(datapoints_.begin(), datapoints_.end(), 0.0,
            [](rrd_data_point::data_point const& sum, std::shared_ptr<rrd_data_point> const& dp) {
        return sum + dp->value();
    }) / datapoints_.size();
}

std::shared_ptr<rrd_data_point> rrd_archive::min() const {
    return *std::min_element(datapoints_.begin(), datapoints_.end(),
            [](std::shared_ptr<rrd_data_point> const& dp1, std::shared_ptr<rrd_data_point> const& dp2) {
        return dp1->value() < dp2->value();
    });
}

std::shared_ptr<rrd_data_point> rrd_archive::max() const {
    return *std::max_element(datapoints_.begin(), datapoints_.end(),
            [](std::shared_ptr<rrd_data_point> const& dp1, std::shared_ptr<rrd_data_point> const& dp2) {
        return dp1->value() < dp2->value();
    });
}

std::string rrd_archive::cf_to_str() const {
    switch(cf_) {
    case AVG:
        return "average";
    case MIN:
        return "minimum";
    case MAX:
        return "maximum";
    default:
        assert(false && "unknown consolidation function");
        return "unknown";
    }
}

void rrd_archive::dump(std::ostream& out) const {
    for (auto const& data_point : archive_) {
        out << std::chrono::duration_cast<std::chrono::milliseconds>(data_point.time().time_since_epoch()).count()
            << " "
            << data_point.value()
            << std::endl;
    }
}

rrd_data::rrd_data(std::string name, std::list<rrd_archive> archives) :
    name_(name),
    archives_(archives) {
}

void rrd_data::add(rrd_data_point::data_point value, rrd_data_point::time_point time) {
    // update RRAs
    auto datapoint = std::make_shared<rrd_data_point>(value, time);
    for (rrd_archive& rra : archives_) {
        rra.add(datapoint);
    }
}

void rrd_data::dump(std::string const& prefix) const {
    for (rrd_archive const& rra : archives_) {
        std::string filename(prefix + rra.name() + ".rrd");
        std::ofstream out(filename);
        rra.dump(out);
    }
}
