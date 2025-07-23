#ifndef CARD_H
#define CARD_H

#include <string>
#include <cstdlib>
#include <iostream>

using namespace std;

typedef enum cardSuits {
    SISI     = -1,
    HEARTS   = 0,
    SPADES   = 1,
    CLUBS    = 2,
    DIAMONDS = 3,
} CardSuit;

typedef enum cardValues {
    VIVI  = -1,
    TWO   = 2,
    THREE = 3,
    FOUR  = 4,
    FIVE  = 5,
    SIX   = 6,
    SEVEN = 7,
    EIGHT = 8,
    NINE  = 9,
    TEN   = 10,
    JACK  = 11,
    QUEEN = 12,
    KING  = 13,
    ACE   = 14,
} CardValue;

class Card {
public:
    //
    explicit Card();
    //
    explicit Card(CardSuit s, CardValue v);
    //
    explicit Card(const Card *card);
    //
    Card(int face);
    //
    ~Card();
    //
    void init();
    //
    bool setCard(const CardSuit s, const CardValue v);
    //
    bool setCard(const string &strCard);
    //
    bool setCard(const Card *card);
    //
    const CardSuit getSuit();
    //
    const CardValue getValue();
    //
    const char *str();
    //
    const char *strCluster();
    //
    bool isValid();
    //
    int hashCode();
    //
    void setHashCode(int code);

public:
    //
    static std::string suitToStr(int suit);
    //
    static std::string faceToStr(int face);
    //
    static std::string cardToStr(int card);
    //
    static int suitToInt(std::string suit);
    //
    static int faceToInt(std::string face);

private:
    //
    const char *getSuitSymbol();
    //
    const char *getValueSymbol();

private:
    //
    char desc[32];
    //
    CardSuit suit;
    //
    CardValue value;
};

typedef Card *CardPtr;
#endif //CARD_H
