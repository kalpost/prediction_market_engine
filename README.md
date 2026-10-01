# Prediction Market Engine

A small C++ simulator for a prediction market (e.g., Kalshi, Polymarket), with two strategies running on top of it: a Dutch-book scanner and an Avellaneda-Stoikov style market maker.

It began as a Python project and was later ported to C++ one-to-one. The goal was to keep behavior identical, so the two versions can be compared directly.

## Why C++

The original prototype was written in Python (you can find it at github.com/kalpost/prediction_market_engine_python). I switched it to C++ for three main reasons. First, I wanted to scale the Monte Carlo: the Python version spends most of its time on overhead from tiny operations, which is fine for a few hundred paths but gets really painful for large runs. Second, I wanted to learn how execution-style code is written, especially with strategy logic that looks like what runs closer to a trading system. Lastly, a C++ core is a lot easier to embed into something larger than a Python script is.

On a 500-path, 1,000-tick Monte Carlo run, the C++ version finishes in about 0.56 s versus about 21 s for the Python original (roughly 37x faster, on a MacBook Pro).

## What it does

Picture an election with three candidates. Exactly one of them wins, so a fair set of prices for "Candidate X wins" contracts should add up to 1. But real quotes don't always follow that trend: sometimes the best asks across all outcomes add up to less than 1. Buy one contract of every outcome and you've paid less than the guaranteed payout of 1. That's a Dutch book, and it's what the scanner looks for.

The engine has three moving parts:

1. The market (`simulator.hpp`) holds a hidden "true" probability for each outcome. The probabilities come from a vector of logits, so they always sum to 1. Every tick the logits take a small Gaussian step, and every now and then a "news shock" hits one candidate. From those true probabilities it builds bid/ask quotes.
2. The arbitrage scanner (`arbitrage.hpp`) adds up the best asks across every outcome, subtracts taker fees, and reports whenever there's a positive edge.
3. The market maker (`market_maker.hpp`) quotes a bid and ask on one outcome, shifting its prices away from whatever inventory it's holding.

## Building

You need a C++17 compiler and CMake 3.14 or newer. Everything is header-only, and there are no other dependencies.

```bash
cmake -S . -B build
cmake --build build
```

This produces two executables.

| Binary | What it runs |
|---|---|
| `engine` | One 1,000-tick path, prints a summary of both strategies |
| `mc` | 500 independent paths, prints distribution statistics |

No CMake? Plain g++ works too:

```bash
g++ -std=c++17 -O2 main.cpp -o engine
g++ -std=c++17 -O2 monte_carlo.cpp -o mc
```

## Running it

```bash
./build/engine
./build/mc
```

The Monte Carlo prints something like this:

```
======================================================================
RUNNING MONTE CARLO HARNESS (500 PATHS x 1000 TICKS)
======================================================================
Combinatorial Arbitrage Yield (Mean +/- Std): $174.11 +/- $55.51
Expected Arb Frequency per 1k Ticks       : 106.9 events (10.7% capture rate)
----------------------------------------------------------------------
MM Terminal Inventory (Mean +/- Std)      : 6.14 +/- 9.81 contracts
MM Mark-to-Market PnL (Mean +/- Std)      : $0.65 +/- $1.73
MM 95% Historical Value-at-Risk (VaR_95)   : $-0.14
======================================================================
```

Nothing is seeded by default, so your numbers will differ from run to run.

### Reproducible runs

To get the same output every time, seed the generator at the top of `main()`:

```cpp
rng::seed(42);
```

## How it works

### Market model

Each outcome has a logit, all starting at zero. On every tick:

- Each logit takes a Gaussian step with standard deviation 0.05.
- With probability `jump_probability` (0.08 in the demos), one randomly chosen outcome gets a "news shock" of +0.4 or -0.4.

The visible order book for each outcome is the true probability plus or minus a fixed half-spread, shifted by uniform noise whose size is set by `retail inefficiency`. Quotes are clipped to stay inside `[0.01, 0.99]`.

### Arbitrage scanner

For a set of mutually exclusive outcomes, the arbitrage scanner computes:

```
net_edge = 1 - sum(best asks) - (number of outcomes * taker fee)
```

If `net_edge` is above 0.001, it reports an opportunity:

```
net_margin_pct = net_edge / sum(best asks) * 100
locked_profit  = capital * net_edge / sum(best asks)
```

The size of the trade is a fixed `capital_per_arb`.

### Market maker

Quoting follows the Avellaneda-Stoikov idea. The reservation price is the fair price pushed against inventory, so a long position makes the maker quote lower to encourage selling it back, and a short position does the opposite:

```
reservation = fair - inventory * gamma * sigma^2 * max(t_remaining, 0.01)
half_spread = max((1 / gamma) * ln(1 + gamma / kappa), 0.01)

bid = reservation - half_spread
ask = reservation + half_spread
```

`t_remaining` runs from 1 down to 0 over the simulation, which makes the inventory skew fade as the horizon approaches. Here `sigma` is fixed at 0.2.

Fills are simulated. If the maker's bid is at or above the market bid, it buys one contract with 40% probability, and the same logic applies on the ask side. Inventory is capped at 15 contracts in either direction.

At the end, PnL is marked to market as cash + inventory * fair probability (final).

### Parameters

| Parameter | Where | Default | Meaning |
|---|---|---|---|
| `taker_fee_per_contract` | `DutchBookScanner` | 0.001 | Fee charged per contract on each leg |
| `capital_per_arb` | `evaluate()` | 100 | Notional deployed per arbitrage |
| `gamma` | `InventoryMarketMaker` | 0.35 | Risk aversion; higher means a stronger inventory skew |
| `kappa` | `InventoryMarketMaker` | 1.5 | Order-book liquidity; higher means a tighter spread |
| `jump probability` | `step()` | 0.05 | Per-tick chance of a news shock (demos use 0.08) |
| `retail inefficiency` | `get_order_books()` | 0.035 | Size of the noise added to quotes |

## Project layout

```
.
├── CMakeLists.txt
├── main.cpp            # single-path run
├── monte_carlo.cpp     # multi-path harness
├── simulator.hpp       # MultiOutcomeMarket
├── arbitrage.hpp       # DutchBookScanner
├── market_maker.hpp    # InventoryMarketMaker
└── rng.hpp             # numpy-compatible random number generation
```

## Things worth knowing

- The default market maker parameters give a very wide spread. With `gamma = 0.35` and `kappa = 1.5`, the half-spread works out to roughly 0.6, so quotes mostly sit at the 0.01 / 0.99 clamps and fills are rare. That's why its PnL hovers near zero. Lowering `gamma` or raising `kappa` gives a more realistic spread.
- The arbitrage numbers are idealized. The scanner assumes you can buy every leg at the best ask in full size at the same instant, with no slippage, or leg risk. Treat the profit as an upper bound.
