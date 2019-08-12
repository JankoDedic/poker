# poker
[![Build Status](https://travis-ci.com/JankoDedic/poker.svg?branch=master)](https://travis-ci.com/JankoDedic/poker)
[![Build status](https://ci.appveyor.com/api/projects/status/y59am3sj5pqtfxik?svg=true)](https://ci.appveyor.com/project/JankoDedic/poker)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/jankodedic/poker/master/LICENSE)

# Introduction
This component provides a fully functional No-Limit Hold'em poker model with hand evaluation. API is **not stable**. Support for other games is planned and in development.

# Usage
A normal use case of playing a game with user input should look something like this:
```cpp
auto dealer = poker::dealer(players, button, blinds, deck, community_cards);
dealer.start_hand();
while (not dealer.done()) {
    while (not dealer.betting_round_over())
        dealer.action_taken(get_user_action());
    dealer.end_betting_round();
}
dealer.showdown();
```
