#ifndef CARD_UTIL_H
#define CARD_UTIL_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <random>
#include "Card.hpp"
#include "Log.hpp"

using namespace std;

//
typedef enum Hands {
    NOTHING         = 0,
    HIGH_CARD       = 1,
    ONE_PAIR        = 2,
    TWO_PAIR        = 3,
    THREE_OF_A_KIND = 4,
    STRAIGHT        = 5,
    FLUSH           = 6,
    FULL_HOUSE      = 7,
    FOUR_OF_A_KIND  = 8,
    STRAIGHT_FLUSH  = 9,
    ROYAL_FLUSH     = 10,
} Hand;

//
class CardUtil {
public:
    //
    static void printCardArray(Card *cards[], int count);
    //
    static int compareHand(Card *source[], Card *descr[], Hand maxValue);
    //
    static void sortCardArray(Card *cards[], int left, int right);
    //
    static Hand evaluateHand(Card *hand[]);
    //
    static string getTypeSymbol(Hand value);
    //
    static vector<Card> buildDeckCards();
};

#endif //CARD_UTIL_H
#