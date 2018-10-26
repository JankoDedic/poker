#include <iostream>

#include <poker/card.hpp>
#include <poker/hand.hpp>
#include <poker/span.hpp>

int
main()
{
    using namespace poker;
    using namespace poker::debug;
    const auto h = make_hand("Ac Ad Ah Js Kc 2s 2s");
    std::cout << h.ranking() << std::endl;
    std::cout << h.strength() << std::endl;
    for (auto card : h.cards()) {
        std::cout << card << ' ';
    }
    std::cout << std::endl;
}
