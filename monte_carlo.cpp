#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

#include "arbitrage.hpp"       
#include "market_maker.hpp"
#include "simulator.hpp"

struct PathResult {
    int arb_count;
    double arb_profit;
    int inventory;
    double mtm;
};

static PathResult run_single_path(int steps = 1000) {
    MultiOutcomeMarket market;
    DutchBookScanner scanner(0.001);
    InventoryMarketMaker mm(0.35, 1.5, "Candidate_A");  // gamma, kappa (default), target symbol

    int arb_count = 0;
    double arb_profit = 0.0;

    for (int step = 0; step < steps; ++step) {
        const double t_remaining = 1.0 - (static_cast<double>(step) / steps);
        market.step(0.08);
        const auto fair_probs = market.get_fair_probabilities();
        const auto books = market.get_order_books(0.035);

        if (auto arb = scanner.evaluate(books, 100.0)) {
            ++arb_count;
            arb_profit += arb->locked_profit;
        }

        const auto [my_bid, my_ask] = mm.quote(fair_probs.at(mm.target), t_remaining);
        mm.process_fills(my_bid, my_ask, books.at(mm.target));
    }

    const double fair_terminal = market.get_fair_probabilities().at(mm.target);
    const double mm_mtm = mm.cash + (mm.inventory * fair_terminal);
    return {arb_count, arb_profit, mm.inventory, mm_mtm};
}


static double np_mean(const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
}

static double np_std(const std::vector<double>& v) {
    const double m = np_mean(v);
    double acc = 0.0;
    for (double x : v) acc += (x - m) * (x - m);
    return std::sqrt(acc / static_cast<double>(v.size()));
}

static double np_percentile(std::vector<double> v, double q) {
    std::sort(v.begin(), v.end());
    const double idx = (q / 100.0) * static_cast<double>(v.size() - 1);
    const std::size_t lo = static_cast<std::size_t>(std::floor(idx));
    const std::size_t hi = std::min(lo + 1, v.size() - 1);
    const double frac = idx - static_cast<double>(lo);
    return v[lo] + (v[hi] - v[lo]) * frac;
}

static void run_monte_carlo(int n_simulations = 500, int steps_per_sim = 1000) {
    const std::string bar(70, '=');
    const std::string dash(70, '-');

    std::cout << bar << "\n"
              << "RUNNING MONTE CARLO HARNESS (" << n_simulations << " PATHS x "
              << steps_per_sim << " TICKS)\n"
              << bar << "\n";

    std::vector<double> arb_counts, arb_profits, mm_inventories, mm_pnls;
    arb_counts.reserve(n_simulations);
    arb_profits.reserve(n_simulations);
    mm_inventories.reserve(n_simulations);
    mm_pnls.reserve(n_simulations);

    for (int i = 0; i < n_simulations; ++i) {
        const PathResult r = run_single_path(steps_per_sim);
        arb_counts.push_back(r.arb_count);
        arb_profits.push_back(r.arb_profit);
        mm_inventories.push_back(r.inventory);
        mm_pnls.push_back(r.mtm);
    }

    std::cout << std::fixed;

    std::cout << std::setprecision(2)
              << "Combinatorial Arbitrage Yield (Mean +/- Std): $" << np_mean(arb_profits)
              << " +/- $" << np_std(arb_profits) << "\n";

    std::cout << std::setprecision(1)
              << "Expected Arb Frequency per 1k Ticks       : " << np_mean(arb_counts)
              << " events (" << np_mean(arb_counts) / 10 << "% capture rate)\n";

    std::cout << dash << "\n";

    std::cout << std::setprecision(2)
              << "MM Terminal Inventory (Mean +/- Std)      : " << np_mean(mm_inventories)
              << " +/- " << np_std(mm_inventories) << " contracts\n"
              << "MM Mark-to-Market PnL (Mean +/- Std)      : $" << np_mean(mm_pnls)
              << " +/- $" << np_std(mm_pnls) << "\n"
              << "MM 95% Historical Value-at-Risk (VaR_95)   : $" << np_percentile(mm_pnls, 5.0)
              << "\n"
              << bar << "\n";
}

int main() {
    run_monte_carlo(500, 1000);
    return 0;
}