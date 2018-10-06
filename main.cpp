#include <iostream>

#include <poker/card.hpp>
#include <poker/span.hpp>

namespace poker {

std::ostream&
operator<<(std::ostream& os, card c)
{
    constexpr char rank_symbols[] = {
        '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
    };
    constexpr char suit_symbols[] = { 5, 4, 3, 6 };
    const auto rank_symbol = rank_symbols[static_cast<std::size_t>(c.rank)];
    const auto suit_symbol = suit_symbols[static_cast<std::size_t>(c.suit)];
    return os << rank_symbol << suit_symbol;
}

} // namespace poker

int
main()
{
    using namespace poker;
    const auto c1 = card{card_rank::A, card_suit::clubs};
    const auto c2 = card{card_rank::A, card_suit::diamonds};
    const auto c3 = card{card_rank::A, card_suit::hearts};
    const auto c4 = card{card_rank::A, card_suit::spades};
    std::cout << c1 << std::endl;
    std::cout << c2 << std::endl;
    std::cout << c3 << std::endl;
    std::cout << c4 << std::endl;
}
