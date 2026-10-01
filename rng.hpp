#pragma once

#include <cmath>
#include <cstdint>
#include <random>


namespace rng {

struct State {
    std::mt19937 gen{std::random_device{}()};
    bool has_gauss = false;
    double gauss = 0.0;
};

inline State& state() {
    static State s;
    return s;
}

inline void seed(std::uint32_t s) {
    state().gen.seed(s);
    state().has_gauss = false;
    state().gauss = 0.0;
}

inline std::uint32_t next_uint32() { return state().gen(); }

inline double uniform() {
    const std::uint32_t a = next_uint32() >> 5;
    const std::uint32_t b = next_uint32() >> 6;
    return (a * 67108864.0 + b) / 9007199254740992.0;
}

inline double uniform(double low, double high) {
    return low + (high - low) * uniform();
}

inline double standard_normal() {
    State& st = state();
    if (st.has_gauss) {
        st.has_gauss = false;
        const double tmp = st.gauss;
        st.gauss = 0.0;
        return tmp;
    }
    double x1, x2, r2;
    do {
        x1 = 2.0 * uniform() - 1.0;
        x2 = 2.0 * uniform() - 1.0;
        r2 = x1 * x1 + x2 * x2;
    } while (r2 >= 1.0 || r2 == 0.0);
    const double f = std::sqrt(-2.0 * std::log(r2) / r2);
    st.gauss = f * x1;
    st.has_gauss = true;
    return f * x2;
}

inline double normal(double loc, double scale) {
    return loc + scale * standard_normal();
}

inline int randint(int low, int high) {
    const std::uint32_t range = static_cast<std::uint32_t>(high - low - 1);
    if (range == 0) return low;
    std::uint32_t mask = range;
    mask |= mask >> 1; mask |= mask >> 2; mask |= mask >> 4;
    mask |= mask >> 8; mask |= mask >> 16;
    std::uint32_t val;
    while ((val = (next_uint32() & mask)) > range) {}
    return low + static_cast<int>(val);
}

}  