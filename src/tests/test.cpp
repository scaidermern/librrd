#include <cassert>
#include <chrono>
#include <cmath>
#include <iterator>
#include <list>
#include <sstream>
#include <vector>

#ifdef DEBUG
#include <iostream>
#endif

#define private public
#include "librrd.h"

/// print content of all RRAs
void print(rrd_data const& data) {
    LOG("archives of rrd " << data.name());
    for (auto const& a : data.archives()) {
        assert(a.archive().size() <= a.rows());
        LOG("archive " << a.name() << " with cf " << a.cf_to_str());
        LOGNL("  t: ");
        for (auto const& b : a.archive()) {
            (void)b;
            LOGNL(b.time().time_since_epoch().count() << " ");
        }
        LOGNL(std::endl << "  v: ");
        for (auto const& b : a.archive()) {
            (void)b;
            LOGNL(b.value() << " ");
        }
        LOG("");
    }
}

/// very simplified floating point comparison, enough for our unit tests
bool almost_equal(rrd_data_point::data_point d1, rrd_data_point::data_point d2) {
    return std::abs(d1 - d2) < 0.01;
}

/// compare RRA content against expected content, element by element
void assert_equal_content(std::vector<rrd_data_point::data_point> const& expected, rrd_archive const& actual) {
    assert(expected.size() == actual.archive().size());
    auto it1 = expected.begin();
    auto it2 = actual.archive().begin();
    while (it1 != expected.end() && it2 != actual.archive().end()) {
        LOG("PDP " << *it1 << " =? " << it2->value());
        assert(almost_equal(*it1, it2->value()));
        ++it1;
        ++it2;
    }
}

/// compare RRA content (dumped version) against expected content
void assert_equal_dump_content(std::string const& expected, rrd_archive const& actual) {
    std::stringstream ss;
    actual.dump(ss);
    LOG("expected: " << expected << "\nactual: " << ss.str());
    assert(expected == ss.str());
}

/// simple test with two PDPs
void test_01() {
    const int pdps = 2;
    const int steps = 2;
    const int rra_size = 1;
    rrd_data data("foo", std::list<rrd_archive>{
        // archive containing every data point with a maximum size of 2
        rrd_archive("all", 1, pdps, rrd_archive::AVG),
        // archive containing the minimum of every 2 data points with a maximum size of 1
        rrd_archive("min", steps, rra_size, rrd_archive::MIN),
        // archive containing the maximum of every 2 data points with a maximum size of 1
        rrd_archive("max", steps, rra_size, rrd_archive::MAX),
        // archive containing the average of every 2 data points with a maximum size of 1
        rrd_archive("avg", steps, rra_size, rrd_archive::AVG)
    });

    // populate with two PDPs,
    // the consolidation will create a single RRA entry consisting of all PDPs
    const double c = 1.2;
    const rrd_data_point::time_point t;
    for (int i = 0; i < pdps; ++i) {
        data.add((rrd_data_point::data_point)i * c, t + rrd_archive::dump_resolution(i));

    }
    print(data);

    // check RRA all
    std::list<rrd_archive>::const_iterator all_it = data.archives().begin();
    assert(all_it->archive().size() == pdps);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        (rrd_data_point::data_point)1 * c, (rrd_data_point::data_point)0 * c
        }, *all_it);
    assert_equal_dump_content("1 1.2\n0 0\n", *all_it);

    // check RRA min
    std::list<rrd_archive>::const_iterator min_it = data.archives().begin();
    std::advance(min_it, 1);
    assert(min_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        0.0
        }, *min_it);
    assert_equal_dump_content("0 0\n", *min_it);

    // check RRA max
    std::list<rrd_archive>::const_iterator max_it = data.archives().begin();
    std::advance(max_it, 2);
    assert(max_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        1.2
        }, *max_it);
    assert_equal_dump_content("1 1.2\n", *max_it);

    // check RRA avg
    std::list<rrd_archive>::const_iterator avg_it = data.archives().begin();
    std::advance(avg_it, 3);
    assert(avg_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        0.6 // (0 + 1.2) / 2
        }, *avg_it);
    assert_equal_dump_content("1 0.6\n", *avg_it);
}

/// same data as test_01 but with three PDPs
void test_02() {
    const int pdps = 3;
    const int steps = 2;
    const int rra_size = 1;
    rrd_data data("foo", std::list<rrd_archive>{
        // archive containing every data point with a maximum size of 3
        rrd_archive("all", 1, pdps, rrd_archive::AVG),
        // archive containing the minimum of every 2 data points with a maximum size of 1
        rrd_archive("min", steps, rra_size, rrd_archive::MIN),
        // archive containing the maximum of every 2 data points with a maximum size of 1
        rrd_archive("max", steps, rra_size, rrd_archive::MAX),
        // archive containing the average of every 2 data points with a maximum size of 1
        rrd_archive("avg", steps, rra_size, rrd_archive::AVG)
    });

    // populate with three PDPs,
    // the consolidation will create a single RRA entry consisting of the first two PDPs,
    // the resulting RRAs are expected to be the same as in test_01
    // because the third PDP doesn't lead to a new consolidation
    const double c = 1.2;
    const rrd_data_point::time_point t;
    for (int i = 0; i < pdps; ++i) {
        data.add((rrd_data_point::data_point)i * c, t + rrd_archive::dump_resolution(i));
    }
    print(data);

    // check RRA all
    std::list<rrd_archive>::const_iterator all_it = data.archives().begin();
    assert(all_it->archive().size() == pdps);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        (rrd_data_point::data_point)2 * c, (rrd_data_point::data_point)1 * c, (rrd_data_point::data_point)0 * c
        }, *all_it);
    assert_equal_dump_content("2 2.4\n1 1.2\n0 0\n", *all_it);

    // check RRA min
    std::list<rrd_archive>::const_iterator min_it = data.archives().begin();
    std::advance(min_it, 1);
    assert(min_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        0.0
        }, *min_it);
    assert_equal_dump_content("0 0\n", *min_it);

    // check RRA max
    std::list<rrd_archive>::const_iterator max_it = data.archives().begin();
    std::advance(max_it, 2);
    assert(max_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        1.2
        }, *max_it);
    assert_equal_dump_content("1 1.2\n", *max_it);

    // check RRA avg
    std::list<rrd_archive>::const_iterator avg_it = data.archives().begin();
    std::advance(avg_it, 3);
    assert(avg_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        0.6 // (0 + 1.2) / 2
        }, *avg_it);
    assert_equal_dump_content("1 0.6\n", *avg_it);
}

/// test with many PDPs
void test_03() {
    const int pdps = 100;
    const int steps = 10;
    const int rra_size = 5;
    rrd_data data("foo", std::list<rrd_archive>{
        // archive containing every data point with a maximum size of 50
        rrd_archive("all", 1, pdps, rrd_archive::AVG),
        // archive containing the minimum of every 10 data points with a maximum size of 5
        rrd_archive("min", steps, rra_size, rrd_archive::MIN),
        // archive containing the maximum of every 10 data points with a maximum size of 5
        rrd_archive("max", steps, rra_size, rrd_archive::MAX),
        // archive containing the average of every 10 data points with a maximum size of 5
        rrd_archive("avg", steps, rra_size, rrd_archive::AVG)
    });

    const rrd_data_point::time_point t;
    for (int i = 0; i < pdps; ++i) {
        data.add(i % 10, t + rrd_archive::dump_resolution(i));
    }
    print(data);

    // check RRA all
    std::list<rrd_archive>::const_iterator all_it = data.archives().begin();
    assert(all_it->archive().size() == pdps);

    // check RRA min
    std::list<rrd_archive>::const_iterator min_it = data.archives().begin();
    std::advance(min_it, 1);
    assert(min_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        0.0, 0.0, 0.0, 0.0, 0.0
        }, *min_it);
    assert_equal_dump_content("90 0\n80 0\n70 0\n60 0\n50 0\n", *min_it);

    // check RRA max
    std::list<rrd_archive>::const_iterator max_it = data.archives().begin();
    std::advance(max_it, 2);
    assert(max_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        9.0, 9.0, 9.0, 9.0, 9.0,
        }, *max_it);
    assert_equal_dump_content("99 9\n89 9\n79 9\n69 9\n59 9\n", *max_it);

    // check RRA avg
    std::list<rrd_archive>::const_iterator avg_it = data.archives().begin();
    std::advance(avg_it, 3);
    assert(avg_it->archive().size() == rra_size);
    assert_equal_content(std::vector<rrd_data_point::data_point>{
        4.5, 4.5, 4.5, 4.5, 4.5,
        }, *avg_it);
    assert_equal_dump_content("99 4.5\n89 4.5\n79 4.5\n69 4.5\n59 4.5\n", *avg_it);
}

int main() {
    test_01();
    test_02();
    test_03();
}
