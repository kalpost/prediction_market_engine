#pragma once

#include <cmath>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

struct ArbResult {
    double sum_asks;
    double net_margin_pct;
    double locked_profit;
};

class DutchBookScanner {
public:
    
    using Books = std::map<std::string, std::map<std::string, double>>;

    explicit DutchBookScanner(double taker_fee_per_contract = 0.001)
        : taker_fee_(taker_fee_per_contract) {}

    std::optional<ArbResult> evaluate(const Books& books,
                                      double capital_per_arb = 100.0) const {
        double total_ask_cost = 0.0;
        for (const auto& [name, book] : books) {
            total_ask_cost += book.at("ask");  
        }

        const double total_fees = static_cast<double>(books.size()) * taker_fee_;
        const double net_edge = 1.0 - total_ask_cost - total_fees;

        if (net_edge > 0.001) {  
            if (total_ask_cost == 0.0) {
                throw std::domain_error("division by zero: total_ask_cost == 0");
            }
            const double margin_pct = (net_edge / total_ask_cost) * 100.0;
            const double profit = capital_per_arb * (net_edge / total_ask_cost);
            return ArbResult{
                round_to(total_ask_cost, 4),
                round_to(margin_pct, 2),
                round_to(profit, 2)
            };
        }
        return std::nullopt;
    }

private:
    double taker_fee_;

    static double round_to(double x, int digits) {
        const double scale = std::pow(10.0, digits);
        return std::round(x * scale) / scale;
    }
};