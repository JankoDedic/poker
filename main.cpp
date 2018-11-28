#include <iostream>

#include <poker/card.hpp>
#include <poker/hand.hpp>
#include <poker/span.hpp>

void
test_hand(std::string_view str)
{
    using namespace poker;
    using namespace poker::debug;
    auto cards = make_cards<7>(str);
    if (auto h = hand::_straight_flush_eval(cards)) {
        std::cout << h->ranking() << " | ";
        std::cout << h->strength() << " | ";
        for (auto card : h->cards()) {
            std::cout << card << ' ';
        }
        std::cout << '\n';
    } else {
        std::cout << "No straight/flush found!\n";
    }
}

void
print_hand(poker::hand h) noexcept
{
    using namespace poker;
    using namespace poker::debug;

    std::cout << h.ranking() << " | " << h.strength() << '\n';
}

int
main()
{
    using namespace poker;
    using namespace poker::debug;

    const hand hands[] = {
        make_hand("As Ks Qs Js Ts 9s 8s"),
        make_hand("As Qs Js Ts 9s 8s 7s"),
        make_hand("As Ah Ad Ac 9s 8s 7s"),
        make_hand("As Ah Ad Kc Ks 8s 7s"),
        make_hand("As 3s 5s 7s 9s Tc Qc"),
        make_hand("As 2h 3d 4c 5s 8s 7s"),
        make_hand("Ac Ad Ah 7s 9s Tc Qc"),
        make_hand("Ac Ad Kh Ks 9s Tc Qc"),
        make_hand("Ac Ad 2h Js 9s Tc 3c"),
        make_hand("Ac 7d 2h Js 9s Tc 3c"),
    };
    for (const auto& hand : hands) {
        print_hand(hand);
    }
}
