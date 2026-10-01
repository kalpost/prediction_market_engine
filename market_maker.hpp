#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>

#include "rng.hpp"

class InventoryMarketMaker {
public:
    double gamma;        // risk aversion penalty
    double kappa;        // order book liquidity parameter
    std::string target;
    int inventory = 0;
    double cash = 0.0;

    explicit InventoryMarketMaker(double gamma_ = 0.35,
                                  double kappa_ = 1.5,
                                  std::string target_symbol = "Candidate_A")
        : gamma(gamma_), kappa(kappa_), target(std::move(target_symbol)) {}

    std::pair<double, double> quote(double fair_price, double t_remaining) const {
        const double skew = inventory * gamma * (0.2 * 0.2) * std::max(t_remaining, 0.01);
        const double reservation_price = std::clamp(fair_price - skew, 0.02, 0.98);

        double half_spread = (1.0 / gamma) * std::log(1.0 + gamma / kappa);
        half_spread = std::max(half_spread, 0.01);

        const double my_bid = round_to(std::max(0.01, reservation_price - half_spread), 3);
        const double my_ask = round_to(std::min(0.99, reservation_price + half_spread), 3);
        return {my_bid, my_ask};
    }

    void process_fills(double my_bid, double my_ask,
                       const std::map<std::string, double>& market_book) {
        if (my_bid >= market_book.at("bid") && inventory < 15) {
            if (rng::uniform() < 0.40) {
                inventory += 1;
                cash -= my_bid;
            }
        }

        if (my_ask <= market_book.at("ask") && inventory > -15) {
            if (rng::uniform() < 0.40) {
                inventory -= 1;
                cash += my_ask;
            }
        }
    }

private:
    static double round_to(double x, int digits) {
        const double scale = std::pow(10.0, digits);
        return std::round(x * scale) / scale;
    }
};