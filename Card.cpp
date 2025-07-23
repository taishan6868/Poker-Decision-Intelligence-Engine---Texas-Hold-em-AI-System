#include "Card.hpp"
#include "Log.hpp"

static std::string suits[4] = {"♥", "♠", "♣", "♦"};
static std::string symbols[4] = {"h", "s", "c", "d"};
static std::string faces[13] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};

Card::Card() {
    this->suit = SISI;
    this->value = VIVI;
    this->str();
}

Card::Card(const CardSuit s, const CardValue v) {
    this->suit = s;
    this->value = v;
    this->str();
}

Card::Card(const Card *card) {
    this->suit = card->suit;
    this->value = card->value;
    this->str();
}

Card::~Card() {

}

void Card::init() {
    this->suit = SISI;
    this->value = VIVI;
    this->str();
}

bool Card::setCard(const Card *card) {
    this->suit = card->suit;
    this->value = card->value;
    this->str();
    return true;
}

bool Card::setCard(const CardSuit s, const CardValue v) {
    this->suit = s;
    this->value = v;
    this->str();
    return true;
}

bool Card::setCard(const string &strCard) {
    std::string v = strCard.substr(0, 1);
    std::string s = strCard.substr(1, 1);
    //
    int suit = -1;
    for (int i = 0; i < 4; i++) {
        if (symbols[i] == s) {
            suit = i;
            break;
        }
    }
    //
    int value = -1;
    for (int j = 0; j < 13; j++) {
        if (faces[j] == v) {
            value = j;
            break;
        }
    }
    //
    if ((-1 == suit) || (-1 == value)) {
        LOG_ERROR("invalid card(0): " << strCard << " s=" << suit << " v=" << value);
        return false;
    }
    //
    if ((suit < 0) || (suit > 3) || (value < 0) || (value > 12)) {
        LOG_ERROR("invalid card(1): " << strCard << " s=" << suit << " v=" << value);
        return false;
    }
    //
    this->suit = (CardSuit)(suit);
    this->value = (CardValue)(value + 2);
    this->str();
    // LOG_DEBUG(strCard << " " << this->str());
    return true;
}

const CardSuit Card::getSuit() {
    return this->suit;
}

const CardValue Card::getValue() {
    return this->value;
}

const char *Card::getSuitSymbol() {
    return suits[suit].c_str();
}

const char *Card::getValueSymbol() {
    return faces[value - 2].c_str();
}

const char *Card::str() {
    memset(desc, 0, sizeof(desc));
    //
    if (VIVI != getValue()) {
        sprintf(desc, "%s", getValueSymbol());
    } else {
        sprintf(desc, "%s", "★");
    }
    //
    if (SISI != getSuit()) {
        sprintf(desc + strlen(desc), "%s", getSuitSymbol());
    } else {
        sprintf(desc + strlen(desc), "%s", "☆");
    }
    //
    return &desc[0];
}

const char *Card::strCluster() {
    // desc = "";
    // desc += getValueSymbol();
    // desc += symbols[suit];
    // return desc;
    memset(desc, 0, sizeof(desc));
    //
    if (VIVI != getValue()) {
        sprintf(desc, "%s", getValueSymbol());
    } else {
        sprintf(desc, "%s", "★");
    }
    //
    if (SISI != getSuit()) {
        sprintf(desc + strlen(desc), "%s", symbols[suit].c_str());
    } else {
        sprintf(desc + strlen(desc), "%s", "☆");
    }
    //
    return &desc[0];
}

bool Card::isValid() {
    if ((getSuit() >= HEARTS)
            && (getSuit() <= DIAMONDS)
            && (getValue() >= TWO)
            && (getValue() <= ACE)) {
        return true;
    }
    //
    LOG_ERROR("invalid card: " << str() << ", suit=" << getSuit() << ", value=" << getValue());
    return false;
}

std::string Card::suitToStr(int suit) {
    std::string strSuit;
    if ((suit >= 0) && (suit < 4)) {
        strSuit = symbols[suit];
    }
    return strSuit;
}

std::string Card::faceToStr(int face) {
    std::string strFace;
    if ((face >= 0) && (face < 13)) {
        strFace = faces[face];
    }
    return strFace;
}

std::string Card::cardToStr(int card) {
    int rank = (card & 0x0f) - 2;
    int suit = (card & 0xf0) >> 4;
    return faces[rank] + symbols[suit];
}

int Card::suitToInt(std::string suit) {
    int res = -1;
    for (int i = 0; i < 4; i++) {
        auto s = symbols[i];
        if (s == suit) {
            res = i;
            break;
        }
    }
    return res;
}

int Card::faceToInt(std::string face) {
    int res = -1;
    for (int i = 0; i < 13; i++) {
        auto v = faces[i];
        if (v == face) {
            res = i + 2;
            break;
        }
    }
    return res;
}

int Card::hashCode() {
    return (this->suit) * 13 + (this->value - 2);
}

void Card::setHashCode(int code) {
    this->suit = (CardSuit)(code / 13);
    this->value = (CardValue)(code % 13 + 2);
}