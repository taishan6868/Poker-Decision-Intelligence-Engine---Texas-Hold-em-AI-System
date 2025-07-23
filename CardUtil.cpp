#include "CardUtil.hpp"

Hand CardUtil::evaluateHand(Card *hand[]) {
    //
    int straight = 0;
    int flush = 0;
    int three = 0;
    int four = 0;
    int full = 0;
    int pairs = 0;
    int k = 0;
    //checks for flush
    while ((k < 4) && (hand[k]->getSuit() == hand[k + 1]->getSuit())) {
        k++;
    }
    //
    if (k == 4) {
        flush = 1;
    }
    //checks for straight
    k = 0;
    while ((k < 4) && (hand[k]->getValue() == (hand[k + 1]->getValue() - 1))) {
        k++;
    }
    //
    if (k == 4) {
        straight = 1;
    }
    //checks for fours
    for (int i = 0; i < 2; i++) {
        k = i;
        while ((k < i + 3) && (hand[k]->getValue() == hand[k + 1]->getValue())) {
            k++;
        }
        //
        if (k == (i + 3)) {
            four = 1;
        }
    }
    //checks for threes and fullhouse
    if (!four) {
        for (int i = 0; i < 3; i++) {
            //
            k = i;
            while ((k < i + 2) && (hand[k]->getValue() == hand[k + 1]->getValue())) {
                k++;
            }
            //
            if (k == (i + 2)) {
                three = 1;
                if (i == 0) {
                    if (hand[3]->getValue() == hand[4]->getValue())
                        full = 1;
                } else if (i == 1) {
                    if (hand[0]->getValue() == hand[4]->getValue())
                        full = 1;
                } else {
                    if (hand[0]->getValue() == hand[1]->getValue())
                        full = 1;
                }
            }
        }
    }
    //
    if (straight && flush)
        return STRAIGHT_FLUSH;
    else if (four)
        return FOUR_OF_A_KIND;
    else if (full)
        return FULL_HOUSE;
    else if (flush)
        return FLUSH;
    else if (straight)
        return STRAIGHT;
    else if (three)
        return THREE_OF_A_KIND;
    //checks for pairs
    for (k = 0; k < 4; k++) {
        if (hand[k]->getValue() == hand[k + 1]->getValue())
            pairs++;
    }
    //
    if (pairs == 2)
        return TWO_PAIR;
    else if (pairs)
        return ONE_PAIR;
    else
        return HIGH_CARD;
}

void CardUtil::sortCardArray(Card *cards[], int left, int right) {
    int i = left;
    int j = right;
    //
    Card *tmp = nullptr;
    Card *pivot = cards[(left + right) / 2];
    //
    while (i <= j) {
        //
        while ((i <= right) && (cards[i]->getValue() < pivot->getValue())) {
            i++;
        }
        //
        while ((j >= left) && (cards[j]->getValue() > pivot->getValue())) {
            j--;
        }
        //
        if (i <= j) {
            tmp = cards[i];
            cards[i] = cards[j];
            cards[j] = tmp;
            //
            i++;
            j--;
        }
    }
    //
    if (left < j) {
        sortCardArray(cards, left, j);
    }
    //
    if (i < right) {
        sortCardArray(cards, i, right);
    }
}

void CardUtil::printCardArray(Card *cards[], int count) {
    std::ostringstream oss;
    for (int i = 0; i < count; i++) {
        oss << cards[i]->str() << " ";
    }
    LOG_DEBUG(oss.str());
}

// 0 tie 1 win 2 lose
int CardUtil::compareHand(Card *source[], Card *descr[], Hand maxValue) {
    // std::ostringstream os;
    // os << "scompareHand before: source=[";
    // for (int i = 0; i <= 4; i++) {
    //     os << source[i]->str() << " ";
    // }
    // os << "]" << endl;
    // os << "scompareHand before: descr=[";
    // for (int i = 0; i <= 4; i++) {
    //     os << descr[i]->str() << " ";
    // }
    // os << "]" << endl;
    // cout << os.str() << endl;
    //
    int ret = 0;
    switch (maxValue) {
    case ROYAL_FLUSH: {
        //royal flush
    }
    break;
    case STRAIGHT_FLUSH:
    case FLUSH:
    case STRAIGHT:
    case HIGH_CARD: {
        for (int i = 4; i >= 0; i--) {
            if (source[i]->getValue() > descr[i]->getValue()) {
                ret = 1;
                break;
            } else if (source[i]->getValue() < descr[i]->getValue()) {
                ret = 2;
                break;
            }
        }
    }
    break;
    case FOUR_OF_A_KIND: {
        if (source[2]->getValue() > descr[2]->getValue()) {
            ret = 1;
        } else if (source[2]->getValue() < descr[2]->getValue()) {
            ret = 2;
        } else {
            CardValue value_1 = (source[2]->getValue() != source[0]->getValue())
                                ? source[0]->getValue()
                                : source[4]->getValue();
            CardValue value_2 = (descr[2]->getValue() != descr[0]->getValue())
                                ? descr[0]->getValue()
                                : descr[4]->getValue();
            if (value_1 > value_2) {
                ret = 1;
            } else if (value_1 < value_2) {
                ret = 2;
            }
        }
    }
    break;
    case FULL_HOUSE: {
        if (source[2]->getValue() > descr[2]->getValue()) {
            ret = 1;
        } else if (source[2]->getValue() < descr[2]->getValue()) {
            ret = 2;
        } else {
            //
            CardValue value_1 = (source[2]->getValue() != source[0]->getValue())
                                ? source[0]->getValue()
                                : source[4]->getValue();
            //
            CardValue value_2 = (descr[2]->getValue() != descr[0]->getValue())
                                ? descr[0]->getValue()
                                : descr[4]->getValue();
            if (value_1 > value_2) {
                ret = 1;
            } else if (value_1 < value_2) {
                ret = 2;
            } else {
                ret = 0;
            }
        }
    }
    case THREE_OF_A_KIND: {
        CardValue value_1 = source[2]->getValue();
        CardValue value_2 = descr[2]->getValue();
        if (value_1 > value_2) {
            ret = 1;
        } else if (value_1 < value_2) {
            ret = 2;
        } else {
            vector<CardValue> v_temp_1;
            vector<CardValue> v_temp_2;
            for (int i = 4; i >= 0; i--) {
                //
                if (i == 2) {
                    continue;
                }
                //
                if (source[i]->getValue() != value_1) {
                    v_temp_1.push_back(source[i]->getValue());
                }
                //
                if (descr[i]->getValue() != value_2) {
                    v_temp_2.push_back(descr[i]->getValue());
                }
            }
            //
            if ((v_temp_1.size() != (size_t)2) || (v_temp_1.size() != v_temp_2.size())) {
                // cout << "compareHand THREE_OF_A_KIND " << v_temp_1.size() << " != " << v_temp_2.size() << endl;
                break;
            }
            //
            for (size_t i = 0; i < v_temp_1.size(); i++) {
                if (v_temp_1[i] > v_temp_2[i]) {
                    ret = 1;
                    break;
                } else if (v_temp_1[i] < v_temp_2[i]) {
                    ret = 2;
                    break;
                }
            }
        }
    }
    break;
    case TWO_PAIR: {
        map<CardValue, int> m_temp_1;
        map<CardValue, int> m_temp_2;
        for (int i = 4; i >= 0; i--) {
            m_temp_1[source[i]->getValue()]++;
            m_temp_2[descr[i]->getValue()]++;
        }
        //
        CardValue value_1_1, value_1_2, value_1_3;
        CardValue value_2_1, value_2_2, value_2_3;
        int count = 0;
        for (auto item : m_temp_1) {
            if (item.second == 2) {
                if (count == 0) {
                    value_1_1 = item.first;
                    count++;
                } else {
                    value_1_2 = item.first;
                }
            } else {
                value_1_3 = item.first;
            }
        }
        //
        if (value_1_1 < value_1_2) {
            CardValue temp = value_1_1;
            value_1_1 = value_1_2;
            value_1_2 = temp;
        }
        //
        count = 0;
        for (auto item : m_temp_2) {
            if (item.second == 2) {
                if (count == 0) {
                    value_2_1 = item.first;
                    count++;
                } else {
                    value_2_2 = item.first;
                }
            } else {
                value_2_3 = item.first;
            }
        }
        //
        if (value_2_1 < value_2_2) {
            CardValue temp = value_2_1;
            value_2_1 = value_2_2;
            value_2_2 = temp;
        }
        //
        if (value_1_1 > value_2_1) {
            ret  = 1;
        } else if (value_1_1 < value_2_1) {
            ret = 2;
        } else {
            if (value_1_2 > value_2_2) {
                ret = 1;
            } else if (value_1_2 < value_2_2) {
                ret = 2;
            } else {
                if (value_1_3 > value_2_3) {
                    ret = 1;
                } else if (value_1_3 < value_2_3) {
                    ret = 2;
                }
            }
        }
    }
    break;
    case ONE_PAIR: {
        map<CardValue, int> m_temp_1;
        map<CardValue, int> m_temp_2;
        //
        for (int i = 4; i >= 0; i--) {
            m_temp_1[source[i]->getValue()]++;
            m_temp_2[descr[i]->getValue()]++;
        }
        //
        CardValue value_1;
        CardValue value_2;
        //
        for (auto item : m_temp_1) {
            if (item.second == 2)
                value_1 = item.first;
        }
        //
        for (auto item : m_temp_2) {
            if (item.second == 2)
                value_2 = item.first;
        }
        //
        if (value_1 > value_2) {
            ret = 1;
        } else if (value_1 < value_2) {
            ret = 2;
        } else {
            vector<CardValue> v_temp_1;
            vector<CardValue> v_temp_2;
            //
            for (int i = 4; i >= 0; i--) {
                //
                if (source[i]->getValue() != value_1) {
                    v_temp_1.push_back(source[i]->getValue());
                }
                //
                if (descr[i]->getValue() != value_1) {
                    v_temp_2.push_back(descr[i]->getValue());
                }
            }
            //
            for (size_t i = 0; i < v_temp_1.size(); i++) {
                if (v_temp_1[i] > v_temp_2[i]) {
                    ret = 1;
                    break;
                } else if (v_temp_1[i] < v_temp_2[i]) {
                    ret = 2;
                    break;
                }
            }
        }
    }
    break;
    default: {
        //
    }
    break;
    }

    return ret;
}

string CardUtil::getTypeSymbol(Hand value) {
    switch (value) {
    case HIGH_CARD:
        return "HIGH_CARD      ";
    case ONE_PAIR:
        return "ONE_PAIR       ";
    case TWO_PAIR:
        return "TWO_PAIR       ";
    case THREE_OF_A_KIND:
        return "THREE_OF_A_KIND";
    case STRAIGHT:
        return "STRAIGHT       ";
    case FLUSH:
        return "FLUSH          ";
    case FULL_HOUSE:
        return "FULL_HOUSE     ";
    case FOUR_OF_A_KIND:
        return "FOUR_OF_A_KIND ";
    case STRAIGHT_FLUSH:
        return "STRAIGHT_FLUSH ";
    case ROYAL_FLUSH:
        return "ROYAL_FLUSH    ";
    default:
        return "NOTHING        ";
    }
}

//
vector<Card> CardUtil::buildDeckCards() {
    std::vector<Card> vv;
    //
    for (int i = 0; i <= 3; i++) {
        for (int j = 2; j <= 14; j++) {
            vv.push_back(Card((CardSuit)i, (CardValue)j));
        }
    }
    //
    std::random_device rd;
    std::shuffle(vv.begin(), vv.end(), rd);
    return vv;
}
