#include <iostream>

#include <poker/card.hpp>
#include <poker/hand.hpp>
#include <poker/span.hpp>

int
main()
{
    using namespace poker;
    using namespace poker::debug;
    auto cards = make_cards<7>("Ac Ad Ah Js Kc 2s 2s");
    /* card cards[] = { */
    /*     make_card("Ac"), */
    /*     make_card("Ad"), */
    /*     make_card("Ah"), */
    /*     make_card("Js"), */
    /*     make_card("Kc"), */
    /*     make_card("2c"), */
    /*     make_card("2c") */
    /* }; */
    const auto h = hand(cards);
    std::cout << h.ranking() << std::endl;
    std::cout << h.strength() << std::endl;
    for (auto card : h.cards()) {
        std::cout << card << ' ';
    }
    std::cout << std::endl;
}
