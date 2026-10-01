#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "rng.hpp"

class MultiOutcomeMarket {
public:
    using Books = std::map<std::string, std::map<std::string, double>>;

    std::vector<std::string> outcomes;

    explicit MultiOutcomeMarket(std::vector<std::string> outcomes_ = {})
        : outcomes(outcomes_.empty()
                       ? std::vector<std::string>{"Candidate_A", "Candidate_B", "Candidate_C"}
                       : std::move(outcomes_)),
          n(static_cast<int>(outcomes.size())),
          logits(outcomes.size(), 0.0) {}

    std::map<std::string, double> get_fair_probabilities() const {
        const double mx = *std::max_element(logits.begin(), logits.end());
        std::vector<double> e(n);
        double sum = 0.0;
        for (int i = 0; i < n; ++i) {
            e[i] = std::exp(logits[i] - mx);
            sum += e[i];
        }
        std::map<std::string, double> probs;
        for (int i = 0; i < n; ++i) probs[outcomes[i]] = e[i] / sum;
        return probs;
    }

    void step(double jump_probability = 0.05) {
        for (int i = 0; i < n; ++i) logits[i] += rng::normal(0.0, 0.05);

        if (rng::uniform() < jump_probability) {
            const int shock_target = rng::randint(0, n);
            const double shock = (rng::randint(0, 2) == 0) ? -0.4 : 0.4;
            logits[shock_target] += shock;
        }
    }

    Books get_order_books(double retail_inefficiency = 0.035) const {
        const auto fair = get_fair_probabilities();
        Books books;
        for (const auto& name : outcomes) {  
            const double p = fair.at(name);
            const double half_spread = 0.012;
            const double noise = rng::uniform(-retail_inefficiency, retail_inefficiency);

            const double bid = std::clamp(p - half_spread + noise, 0.01, 0.97);
            const double ask = std::clamp(std::max(bid + 0.01, p + half_spread + noise), 0.02, 0.99);

            books[name] = {{"bid", round_to(bid, 3)}, {"ask", round_to(ask, 3)}};
        }
        return books;
    }

private:
    int n;
    std::vector<double> logits;

    static double round_to(double x, int digits) {
        const double scale = std::pow(10.0, digits);
        return std::round(x * scale) / scale;
    }
};