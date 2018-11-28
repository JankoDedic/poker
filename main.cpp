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

int
main()
{
    using namespace poker;
    using namespace poker::debug;

    test_hand("As 2c 3c 5c 4c 8s 3s");
    test_hand("As Ks Qs Js Ts 2c 3c");
    test_hand("As Kc Qd Jh Ts 2c 3c");
    test_hand("As Ks Qs Ts 9s 2c 3c");
    test_hand("Ah 2s 3d 4c 5s 2c 3c");
}
