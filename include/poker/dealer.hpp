#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <new>
#include <random>

#include <doctest/doctest.h>

#include <poker/deck.hpp>
#include <poker/player.hpp>
#include <poker/hand.hpp>
#include <poker/community_cards.hpp>
#include "poker/detail/utility.hpp"

#include "poker/detail/betting_round.hpp"
#include "poker/detail/pot.hpp"
#include "poker/detail/pot_manager.hpp"

namespace poker {

struct blinds {
    chips small;
    chips big = 2*small;
    chips ante = 0;
};

class dealer {
    static constexpr auto max_players = 9; // for the time being
    using player_container = std::array<player *, max_players>;

    using betting_round = poker::detail::betting_round;

    using pot_manager = poker::detail::pot_manager;

    // Data members
    player_container _players{};
    player_container::iterator _button;

    betting_round _betting_round;
    blinds _blinds;

    deck *_deck = nullptr;
    community_cards *_community_cards = nullptr;

    poker::round_of_betting _round_of_betting;
    bool _betting_round_ended = false;
    pot_manager _pot_manager{};
    // store legal action range?

    auto next_or_wrap(player_container::iterator it) noexcept -> player_container::iterator {
        do {
            ++it;
            if (it == _players.end()) it = _players.begin();
        } while (!*it);
        return it;
    }

    void collect_ante() noexcept {
        for (auto p : _players) {
            if (p) {
                p->take_from_stack(std::min(_blinds.ante, p->total_chips()));
            }
        }
    }

    auto post_blinds() noexcept -> player_container::iterator {
        auto it = _button;
        const auto num_players = std::count_if(std::begin(_players), std::end(_players), [] (player *p) { return p != nullptr; });
        if (num_players != 2) it = next_or_wrap(it);
        (**it).bet(std::min(_blinds.small, (**it).total_chips()));
        it = next_or_wrap(it);
        (**it).bet(std::min(_blinds.big, (**it).total_chips()));
        return it;
    }

    void deal_hole_cards() noexcept {
        for (auto p : _players) {
            if (p) p->hole_cards = {_deck->draw(), _deck->draw()};
        }
    }

    // Deals community cards up until the current round of betting.
    void deal_community_cards() noexcept {
        using poker::detail::to_underlying;
        auto cards = std::vector<card>();
        const auto num_cards_to_deal = to_underlying(_round_of_betting) - _community_cards->cards().size();
        std::generate_n(std::back_inserter(cards), num_cards_to_deal, [&] { return _deck->draw(); });
        _community_cards->deal(cards);
    }

public:
    dealer() /*noexcept*/ = default;

    template<typename PlayerRange, typename = std::enable_if_t<std::is_same_v<poker::detail::range_value_t<PlayerRange>, player>>>
    dealer(PlayerRange &players, decltype(std::begin(players)) button, blinds b, deck &d, community_cards &cc) noexcept
        : _blinds(b)
        , _deck(&d)
        , _community_cards(&cc)
        , _round_of_betting(round_of_betting::preflop)
    {
        constexpr auto addressof = [] (player &p) { return &p; };
        std::transform(std::begin(players), std::end(players), std::begin(_players), addressof);
        _button = _players.begin() + std::distance(std::begin(players), button);
    }

    dealer(const player_container &players, player_container::const_iterator button, blinds b, deck &d, community_cards &cc) noexcept
        : _blinds(b)
        , _deck(&d)
        , _community_cards(&cc)
        , _round_of_betting(round_of_betting::preflop)
        , _players(players)
        , _button(_players.begin() + std::distance(players.begin(), button))
    {
    }

    dealer(const dealer &) = delete;
    auto operator=(const dealer &) -> dealer & = delete;

    dealer(dealer &&) = delete;
    auto operator=(dealer &&) -> dealer & = delete;

    auto player_to_act() const noexcept -> player_container::const_iterator {
        return _betting_round.player_to_act();
    }

    auto players() const noexcept -> const player_container & { return _betting_round.players(); }

    auto done() const noexcept -> bool {
        return _betting_round.over() && _betting_round_ended && _round_of_betting == round_of_betting::river;
    }

    auto round_of_betting() const noexcept -> poker::round_of_betting {
        return _round_of_betting;
    }

    // TODO: What happens when d.done() ? Do we assert or return a special value? Special value sounds bad.
    auto num_active_players() const noexcept -> std::size_t {
        return _betting_round.num_active_players();
    }

    auto biggest_bet() const noexcept -> chips {
        return _betting_round.biggest_bet();
    }

    TEST_CASE_CLASS("construction") {
        // auto players = std::vector<player>{player{1}, player{1}, player{1}};
        // auto rng = std::make_unique<std::mt19937>(std::random_device{}());
        // auto dck = deck{*rng};
        // auto cc = community_cards{};
        // auto dlr = dealer{players, players.begin()+1, {10, 20, 0}, dck, cc};
    }

    auto start_hand() noexcept -> void {
        collect_ante();
        const auto first_action = next_or_wrap(post_blinds());
        deal_hole_cards();
        if (std::count_if(_players.begin(), _players.end(), [] (auto p) { return p && p->stack() != 0; }) > 1) {
            new (&_betting_round) betting_round(_players, first_action, _blinds.big);
        }
    }

    TEST_CASE_CLASS("Starting the hand") {
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};

        GIVEN("A hand with two players who can cover their blinds") {
            player players[] = {player{100}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The button has posted the small blind") {
                    REQUIRE_EQ(players[0].bet_size(), 25);
                }

                THEN("The other player has posted the big blind") {
                    REQUIRE_EQ(players[1].bet_size(), 50);
                }

                THEN("The action is on the button") {
                    REQUIRE_EQ(*d.player_to_act(), &players[0]);
                }
            }
        }

        GIVEN("A hand with two players who can't cover their blinds") {
            player players[] = {player{20}, player{20}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The hand has ended") {
                    REQUIRE(d.betting_round_over());
                    d.end_betting_round();
                    REQUIRE(d.done());
                }
            }
        }

        GIVEN("A hand with more than two players") {
            player players[] = {player{100}, player{100}, player{100}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The button+1 has posted the small blind") {
                    REQUIRE_EQ(players[1].bet_size(), 25);
                }

                THEN("The button+2 has posted the big blind") {
                    REQUIRE_EQ(players[2].bet_size(), 50);
                }

                THEN("The action is on the button+3") {
                    REQUIRE_EQ(*d.player_to_act(), &players[3]);
                }
            }
        }
    }

    enum class action : unsigned char {
        fold  = 1 << 0,
        check = 1 << 1,
        call  = 1 << 2,
        bet   = 1 << 3,
        raise = 1 << 4
    };
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(action)

    static auto is_valid(action a) noexcept -> bool {
        return std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1;
    }

    static constexpr auto is_aggressive(action a) noexcept -> bool {
        return bool(a & action::bet) || bool(a & action::raise);
    }

    struct action_range {
        dealer::action action = dealer::action::fold; // you can always fold
        chip_range chips;

        auto contains(dealer::action a, poker::chips bet = 0) const noexcept -> bool {
            assert(is_valid(a));
            return static_cast<bool>(a & action) && (is_aggressive(a) ? chips.contains(bet) : true);
        }
    };

    auto legal_actions() const noexcept -> action_range {
        const auto &player = **_betting_round.player_to_act();
        const auto actions = _betting_round.legal_actions();
        auto ar = action_range{};
        ar.chips = actions.chips;
        // Below we take care of differentiating between check/call and bet/raise,
        // which the betting_round treats as just "match" and "raise".
        if (_betting_round.biggest_bet() - player.bet_size() == 0) {
            ar.action |= action::check;
            assert(actions.can_raise); // If you can check, you can always bet or raise.
            // If this guy can check, with his existing bet_size, he is the big blind.
            if (player.bet_size() > 0) ar.action |= action::raise;
            else ar.action |= action::bet;
        } else {
            ar.action |= action::call;
            // If you can call, you may or may not be able to raise.
            if (actions.can_raise) ar.action |= action::raise;
        }
        return ar;
    }

    auto betting_round_over() const noexcept -> bool {
        return _betting_round.over();
    }

    void action_taken(action a, chips bet = 0) noexcept {
        assert(!betting_round_over());
        assert(legal_actions().contains(a, bet));

        if (bool(a & action::check) || bool(a & action::call)) {
            _betting_round.action_taken(betting_round::action::match);
        } else if (bool(a & action::bet) || bool(a & action::raise)) {
            _betting_round.action_taken(betting_round::action::raise, bet);
        } else {
            assert(bool(a & action::fold));
            _pot_manager.bet_folded((*player_to_act())->bet_size());
            /* const auto folded_player_index = std::distance(players().begin(), player_to_act()); */
            const auto folded_player_index = player_to_act() - players().begin();
            _players[folded_player_index] = nullptr;
            _betting_round.action_taken(betting_round::action::leave);
        }
    }

    void end_betting_round() noexcept {
        assert(!_betting_round_ended);
        assert(betting_round_over());
        _pot_manager.collect_bets_from(_players);
        if (_betting_round.num_active_players() <= 1) {
            _round_of_betting = round_of_betting::river;
            // If there is only one pot, and there is only one player in it...
            if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
                // ...there is no need to deal the undealt community cards.
            } else {
                deal_community_cards();
            }
            _betting_round_ended = true;
            // Now you call showdown()
        } else if (_round_of_betting < round_of_betting::river) {
            // Start the next betting round.
            _round_of_betting = next(_round_of_betting);
            const auto button_index = std::distance(_players.begin(), _button);
            _players = _betting_round.players();
            _button = _players.begin() + button_index;
            new (&_betting_round) betting_round(_players, next_or_wrap(_button), 0);
            deal_community_cards();
            // _betting_round_ended = false;
        } else {
            assert(_round_of_betting == round_of_betting::river);
            _betting_round_ended = true;
            // Now you call showdown()
        }
    }

    TEST_CASE_CLASS("Ending the betting round") {
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        player players[] = {player{1000}, player{1000}, player{1000}};
        auto d = dealer{players, &players[0], b, dck, cc};

        GIVEN("There is two or more active players at the end of any betting round except river") {
            d.start_hand();
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::check);

            REQUIRE(d.betting_round_over());
            REQUIRE_GE(d.num_active_players(), 2);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The next betting round begins") {
                    REQUIRE(!d.betting_round_over());
                    REQUIRE_EQ(d.round_of_betting(), poker::round_of_betting::flop);
                    REQUIRE_EQ(cc.cards().size(), 3);
                }
            }
        }

        GIVEN("There is two or more active players at the end of river") {
            d.start_hand();

            // preflop
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::check);
            d.end_betting_round();

            // flop
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            d.end_betting_round();

            // turn
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            d.end_betting_round();

            // river
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            d.action_taken(dealer::action::check);
            // not ended yet

            REQUIRE(d.betting_round_over());
            REQUIRE_EQ(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 5);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
                }
            }
        }

        GIVEN("There is one or less active players at the end of a betting round and more than one player in all pots") {
            d.start_hand();
            d.action_taken(dealer::action::raise, 1000);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::fold);

            REQUIRE(d.betting_round_over());
            REQUIRE_LE(d.num_active_players(), 1);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
                }
                THEN("The undealt community cards (if any) are dealt") {
                    REQUIRE_EQ(cc.cards().size(), 5);
                }
            }
        }

        GIVEN("There is one or less active players at the end of a betting round and a single player in a single pot") {
            d.start_hand();
            d.action_taken(dealer::action::raise, 1000);
            d.action_taken(dealer::action::fold);
            d.action_taken(dealer::action::fold);

            REQUIRE(d.betting_round_over());
            REQUIRE_LE(d.num_active_players(), 1);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
                }
                THEN("The undealt community cards (if any) are not dealt") {
                    REQUIRE_EQ(cc.cards().size(), 0);
                }
            }
        }
    }

    TEST_CASE_CLASS("flop, someone folded preflop, now others fold, when 1 remains, the hand should be over") {
        // A bug where we pass a container of pointers where some are null to the betting_round => round.
        // round initializes _num_active_players to .size() of the container, instead of counting non-null pointers.
        //
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        player players[] = {player{1000}, player{1000}, player{1000}};
        auto d = dealer{players, &players[0], b, dck, cc};

        d.start_hand();
        d.action_taken(dealer::action::fold);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::check);
        REQUIRE(d.betting_round_over());
        d.end_betting_round();

        d.action_taken(dealer::action::fold);
        REQUIRE(d.betting_round_over());
    }

    void showdown() noexcept {
        if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
            // No need to evaluate the hand. There is only one player.
            _pot_manager.pots()[0].eligible_players()[0]->add_to_stack(_pot_manager.pots()[0].size());
            return;

            // TODO: Also, no reveals in this case. Reveals are only necessary when there is >=2 players.
        }
        for (auto p : _pot_manager.pots()) {
            auto player_results = std::vector<std::pair<player *, hand>>();
            player_results.reserve(p.eligible_players().size());
            std::transform(p.eligible_players().begin(), p.eligible_players().end(), std::back_inserter(player_results), [&] (player *p) {
                return std::make_pair(p, hand{p->hole_cards, *_community_cards});
            });
            std::sort(player_results.begin(), player_results.end(), [] (auto &&lhs, auto &&rhs) {
                return lhs.second < rhs.second;
            });
            auto first_winner = player_results.begin();
            auto last_winner = std::adjacent_find(player_results.begin(), player_results.end(), [] (auto &&lhs, auto &&rhs) {
                return lhs.second != rhs.second;
            });
            if (last_winner != player_results.end()) ++last_winner;
            const auto payout = static_cast<chips>(p.size() / std::distance(first_winner, last_winner));
            std::for_each(first_winner, last_winner, [&] (auto &&winner) {
                winner.first->add_to_stack(payout);
            });
        }
    }

    TEST_CASE_CLASS("Showdown") {
        SUBCASE("single pot single player") {
            const auto b = blinds{25, 50};
            auto dck = deck{std::default_random_engine{std::random_device{}()}};
            auto cc = community_cards{};
            player players[] = {player{1000}, player{1000}, player{1000}};
            auto d = dealer{players, &players[0], b, dck, cc};

            d.start_hand();
            d.action_taken(dealer::action::raise, 1000);
            d.action_taken(dealer::action::fold);
            d.action_taken(dealer::action::fold);
            d.end_betting_round();

            REQUIRE(d.done());

            d.showdown();
            REQUIRE_EQ(players[0].stack(), 1075);
        }

        SUBCASE("multiple pots, multiple winners") {
            const auto b = blinds{25, 50};
            auto dck = deck{std::default_random_engine{std::random_device{}()}};
            auto cc = community_cards{};
            player players[] = {player{300}, player{200}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            d.start_hand();
            d.action_taken(dealer::action::raise, 300);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::call);
            d.end_betting_round();

            REQUIRE(d.done());

            cc = {};
            cc.deal(std::array{
                card{card_rank::A, card_suit::spades},
                card{card_rank::K, card_suit::spades},
                card{card_rank::Q, card_suit::spades},
                card{card_rank::J, card_suit::spades},
                card{card_rank::T, card_suit::spades}
            });
            for (auto &p : players) {
                p.hole_cards = {
                    {card_rank::_2, card_suit::clubs},
                    {card_rank::_3, card_suit::clubs}
                };
            }

            d.showdown();

            REQUIRE_EQ(players[0].stack(), 300);
            REQUIRE_EQ(players[1].stack(), 200);
            REQUIRE_EQ(players[2].stack(), 100);
        }
    }
};

} // namespace poker
