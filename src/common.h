#ifndef __COMMON_H__
#define __COMMON_H__


#include <stdint.h>
#include <cmath>

enum class exchange_t: uint32_t {
    undefined=0, binance, kraken, cryptocom, total

};

enum class side_t: uint8_t {
    undefined=0, bid, ask
};

enum class updata_flag_t: uint64_t {
    underfined=0, snapshot
};

const double EPSILON = std::numeric_limits<double>::epsilon() * 100;

bool is_equal(double a, double b) {
    return std::fabs(a - b) < EPSILON;
}

bool is_less(double a, double b) {
    return (b - a) > EPSILON;
}

bool is_greater(double a, double b) {
    return (a - b) > EPSILON;
}


#endif // __COMMON_H__