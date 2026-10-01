#include <iomanip>
#include <iostream>
#include <string>

#include "arbitrage.hpp"       
#include "market_maker.hpp"
#include "simulator.hpp"

int main() {
    const int steps = 1000;
    MultiOutcomeMarket market;
    DutchBookScanner scanner(0.001);
    InventoryMarketMaker mm(0.35, 1.5, "Candidate_A");  // gamma, kappa, target symbol

    int arb_triggers = 0;
    double total_arb_profit = 0.0;

    const std::string bar(65, '=');
    const std::string dash(65, '-');

    std::cout << bar << "\n"
              << "RUNNING MULTI-OUTCOME PREDICTION MARKET ENGINE\n"
              << bar << "\n";

    for (int step = 0; step < steps; ++step) {
        const double t_remaining = 1.0 - (static_cast<double>(step) / steps);
        market.step(0.08);  // jump probability

        const auto fair_probs = market.get_fair_probabilities();
        const auto books = market.get_order_books(0.035);  // inefficiency

        if (auto arb = scanner.evaluate(books, 100.0)) {
            ++arb_triggers;
            total_arb_profit += arb->locked_profit;
        }

        const auto [my_bid, my_ask] = mm.quote(fair_probs.at(mm.target), t_remaining);
        mm.process_fills(my_bid, my_ask, books.at(mm.target));
    }

    const double fair_terminal = market.get_fair_probabilities().at(mm.target);
    const double mm_mtm = mm.cash + (mm.inventory * fair_terminal);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Discrete Simulation Ticks   : " << steps << "\n"
              << dash << "\n"
              << "ARBITRAGE STRATEGY (Combinatorial Dutch-Book):\n"
              << "  Opportunities Captured    : " << arb_triggers << "\n"
              << "  Total Locked-in Profit    : $" << total_arb_profit << "\n"
              << dash << "\n"
              << "MARKET MAKING STRATEGY (" << mm.target << "):\n"
              << "  Terminal Inventory        : " << mm.inventory << " contracts\n"
              << "  Cash Flow Balance         : $" << mm.cash << "\n"
              << "  Mark-to-Market Total PnL  : $" << mm_mtm << "\n"
              << bar << "\n";

    return 0;
}