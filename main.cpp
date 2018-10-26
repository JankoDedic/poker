#include <iostream>

#include <poker/card.hpp>
#include <poker/hand.hpp>
#include <poker/span.hpp>

int
main()
{
    using namespace poker;
    using namespace poker::debug;
    card cards[] = {
        {card_rank::A, card_suit::clubs},
        {card_rank::A, card_suit::clubs},
        {card_rank::A, card_suit::clubs},
        {card_rank::J, card_suit::clubs},
        {card_rank::K, card_suit::clubs},
        {card_rank::_2, card_suit::clubs},
        {card_rank::_2, card_suit::clubs}
    };
    const auto h = hand(cards);
    std::cout << h.ranking() << std::endl;
    std::cout << h.strength() << std::endl;
    for (auto card : h.cards()) {
        std::cout << card << ' ';
    }
    std::cout << std::endl;
}
