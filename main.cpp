#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include <poker/card.hpp>
#include <poker/debug/card.hpp>
#include <poker/hand.hpp>
#include <poker/debug/hand.hpp>
#include <poker/player.hpp>

#include <poker/dealer.hpp>
#include <poker/debug/dealer.hpp>
#include <poker/deck.hpp>

int
main()
{
    using namespace poker;
    using namespace poker::debug;

    const auto the_blinds = blinds{10, 20, 5};
    auto rng = std::mt19937(std::random_device{}());
    auto the_deck = deck(rng);
    auto the_community_cards = community_cards();
    player players[] = {{500}, {500}, {500}, {500}};
    auto button = &players[0];
    auto the_dealer = dealer(
        &players,
        &players[0],
        &the_community_cards,
        &the_deck,
        the_blinds
    );
    the_dealer.post_blinds();
    do {
        do {
            std::cout << std::string(80, '_') << '\n';
            // Player index
            std::cout << "player index:\t\t";
            for (auto i = 0; i < std::size(players); ++i) {
                std::cout << std::setw(10) << i;
            }
            std::cout << '\n';
            // Player stack
            std::cout << "player stack:\t\t";
            for (const auto& p : players) {
                std::cout << std::setw(10) << p.stack();
            }
            std::cout << '\n';
            // Player bet
            std::cout << "player bet:\t\t";
            for (const auto& p : players) {
                std::cout << std::setw(10) << p.bet_size();
            }
            std::cout << '\n';
            // Active/non-active players
            std::cout << "active:\t\t\t";
            for (auto p : the_dealer.players()) {
                if (p) {
                    std::cout << std::setw(10) << "(Y)";
                } else {
                    std::cout << std::setw(10) << "(N)";
                }
            }
            std::cout << '\n';
            // Button
            std::cout << "button:\t\t\t";
            for (auto p : the_dealer.players()) {
                if (p == button) {
                    std::cout << std::setw(10) << "(D)";
                } else {
                    std::cout << std::setw(10) << " ";
                }
            }
            std::cout << '\n';
            // Player to act
            std::cout << "player to act:\t\t";
            for (auto p : the_dealer.players()) {
                if (p == &the_dealer.player_to_act()) {
                    std::cout << std::setw(10) << "(A)";
                } else {
                    std::cout << std::setw(10) << " ";
                }
            }
            std::cout << '\n';
            std::cout << std::string(80, '-') << '\n';
            // Last aggressive actor
            std::cout << "last aggro actor:\t";
            for (auto p : the_dealer.players()) {
                if (p == *the_dealer._last_aggressive_actor) {
                    std::cout << std::setw(10) << "(*)";
                } else {
                    std::cout << std::setw(10) << " ";
                }
            }
            std::cout << '\n';
            // Last raise
            std::cout << "last raise:\t\t" << the_dealer._last_raise << '\n';
            // Biggest bet
            std::cout << "biggest bet:\t\t" << the_dealer._biggest_bet << '\n';
            // Community cards
            std::cout << "community:\t\t";
            for (const auto& card : the_dealer._community_cards->cards()) {
                std::cout << '[' << card << "] ";
            }
            for (auto i = 0; i < 5 - the_dealer._community_cards->cards().size(); ++i) {
                std::cout << "[..] ";
            }
            std::cout << '\n';
            // Round of betting
            std::cout << "round of betting:\t" << the_dealer._rob << '\n';
            std::cout << std::string(80, '-') << '\n';
            // Actions
            std::cout << "actions:\t\t";
            const auto& ac = the_dealer._action_criteria;
            if (ac.action_types() == valid_action_types::check_bet) {
                std::cout << "[check] [bet (" << ac.min() << ", " << ac.max() << ")] ";
            } else if (ac.action_types() == valid_action_types::check_raise) {
                std::cout << "[check] " << "[raise (" << ac.min() << ", " << ac.max() << ")] ";
            } else if (ac.action_types() == valid_action_types::call_raise) {
                std::cout << "[call] " << "[raise (" << ac.min() << ", " << ac.max() << ")] ";
            } else if (ac.action_types() == valid_action_types::call_only) {
                std::cout << "[call] ";
            }
            std::cout << "[fold]\n";
            std::cout << std::string(80, '=') << '\n';
            // Process input command
            auto command = std::string();
            std::cin >> command;
            if (command == "quit") {
                std::exit(0);
            } else if (command == "fold") {
                the_dealer.action_taken(action(action_type::fold));
            } else if (command == "check") {
                the_dealer.action_taken(action(action_type::check));
            } else if (command == "call") {
                the_dealer.action_taken(action(action_type::call));
            } else if (command == "bet") {
                auto bet = chips();
                std::cin >> bet;
                the_dealer.action_taken(action(action_type::bet, bet));
            } else if (command == "raise") {
                auto bet = chips();
                std::cin >> bet;
                the_dealer.action_taken(action(action_type::raise, bet));
            } else {
                std::cout << "Unknown command\n";
            }

            /* if (the_dealer.betting_round_done()) { */
            /*     the_dealer.end_betting_round(); */
            /* } */
        } while (!the_dealer.betting_round_done());
        // end_betting_round, but actually do something meaningful with it
    } while (!the_dealer.done());
}
