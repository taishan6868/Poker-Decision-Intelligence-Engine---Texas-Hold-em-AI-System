#include "Game.hpp"
#include "CardUtil.hpp"
#include "Player.hpp"
#include "Pluribus.hpp"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <string>
#include "ShareMem.hpp"
#include "NumPy.hpp"
#include <stdlib.h>
#include "Configure.hpp"

Game::Game() {
    numPlayers = 0;
    deckOffset = 0;
    gameOver = false;
    round = GAME_ROUND_PREFLOP;
    players = nullptr;
    deck = nullptr;
    raiseCount = 0;
    raiseNumber = 0;
    seedindex = 0;
    potPreflop = 2000;
    multi_enter = 0;
    researchPlayer = -1;
    isShuffle = 1;
    lastRaiserIndex[0] = -1;
    lastRaiserIndex[1] = -1;
    lastRaiserIndex[2] = -1;
    lastRaiserIndex[3] = -1;
    std::random_device mRd;
    srand(mRd());
}

Game::~Game() {
    //
    if (players != nullptr) {
        delete []players;
        players = nullptr;
    }
    //
    if (deck != nullptr) {
        delete []deck;
        deck = nullptr;
    }
}

void Game::terminal() {
    while (!isGameOver()) {
        auto cur = getTurnPlayer();
        if (cur->isFold()) {
            LOG_DEBUG("Player" << cur->getIndex() << " skip(fold)");
        } else if (cur->isAllIn()) {
            LOG_DEBUG("Player" << cur->getIndex() << " skip(allin)");
        } else {
            cur->doAction();
        }
        //
        doGameSchedule();
    }
}

int Game::doTurnSchedule() {
    while (!checkAllin()) {
        auto player = getNextTurnPlayer();
        if (player->isFold()) {
            LOG_DEBUG("@State: Mover(skip fold). turn=" << player->getIndex() << ", round=" << getRound());
            continue;
        } else if (player->isAllIn()) {
            LOG_DEBUG("@State: Mover(skip allin). turn=" << player->getIndex() << ", round=" << getRound());
            continue;
        } else {
            LOG_DEBUG("@State: Mover(next player). turn=" << player->getIndex() << ", round=" << getRound());
            player->doAction();
            break;
        }
    }
    //
    return 0;
}

int Game::getOppoRelativePosition() {
    if (Configure::GetInstance().mOppoPosition == 0) {
        return 0;
    }
    //
    if ((Configure::GetInstance().mOppoPosition == 2) && (round != GAME_ROUND_PREFLOP)) {
        return 0;
    }
    //
    int beforAllPeople = getBeforeAllPlayerNum(getTurnIndex());
    int beforActivePeople = getBeforeActivePlayerNum(getTurnIndex());
    int behindActivePeople =  getBehindActivePlayerNum(getTurnIndex());
    if (((beforActivePeople + behindActivePeople == 1) && round != GAME_ROUND_PREFLOP) || (round == GAME_ROUND_PREFLOP && beforActivePeople != 0)) {
        int oppoPosition = -1;
        std::vector<int > playerLutRaw;
        std::vector<int > playerLut;
        int beforActivePeople = 0 ;
        for (int s = 0 ; s < numPlayers; s++) {
            playerLutRaw.push_back(s);
        }
        //
        if (round == GAME_ROUND_PREFLOP) {
            vector temp(playerLutRaw.begin() + 2, playerLutRaw.begin() + numPlayers);
            temp.push_back(0);
            temp.push_back(1);
            playerLut = temp;
        } else {
            playerLut =  playerLutRaw;
        }
        //
        if (round != GAME_ROUND_PREFLOP) {
            oppoPosition = 0;
            for (int s = 0 ; s < numPlayers; s++ ) {
                if (players[playerLut[s]].isFold() == false and s != beforAllPeople) {
                    oppoPosition = s;
                }
            }
            //
            if (oppoPosition == -1) {
                printf("oppo position error");
            }
            //
            if (abs(beforAllPeople - oppoPosition ) > 2) {
                return 3;
            } else if (abs(beforAllPeople - oppoPosition ) == 2) {
                return 2;
            } else if (abs(beforAllPeople - oppoPosition ) == 1) {
                return 1;
            }
            //printf("other %d %d\n",beforAllPeople,oppoPosition);
        } else if (round == GAME_ROUND_PREFLOP) {
            oppoPosition = 0;
            for (int s = 0 ; s < beforAllPeople; s++) {
                if (players[playerLut[s]].isFold() == false) {
                    oppoPosition = s;
                }
            }
            if (oppoPosition == -1 ) {
                printf("oppo position error");
            }
            if (abs(beforAllPeople - oppoPosition ) > 2 ) {
                return 3;
            } else if (abs(beforAllPeople - oppoPosition ) == 2 ) {
                return 2;
            } else if (abs(beforAllPeople - oppoPosition ) == 1 ) {
                return 1;
            }
            //printf("preflop %d %d\n",beforAllPeople,oppoPosition);
        }
    }
    //printf("final %d %d %d n",beforAllPeople,beforActivePeople,behindActivePeople);
    return 0 ;
}

int Game::doGameSchedule() {
    if (!isGameOver() && !isRoundOver()) {
        doTurnSchedule();
    }
    //
    while (isRoundOver() && !isGameOver()) {
        addBetsToPot();
        setPlayerStates();
        gotoNextRound();
    }
    //
    return 0;
}

int Game::ctor(int num) {
    if (num <= 0) {
        LOG_FATAL("data invalid, num=" << num);
        exit(0);
    }
    //
    if (-1 == initDeckCardList()) {
        exit(0);
    }
    //
    resetCards();
    //
    shuffleDeck(0);
    //The player list changes dynamically
    if ((numPlayers > 0) && (num != numPlayers)) {
        if (players != nullptr) {
            delete []players;
            players = nullptr;
        }
    }
    //
    numPlayers = num;
    LOG_DEBUG("@NewGame: create game begin, num=" << numPlayers);
    //
    if (-1 == initPlayerList()) {
        exit(0);
    }
    //
    resetPlayers();
    //
    resetDeck();
    //
    lastRaiserIndex[0] = -1;
    lastRaiserIndex[1] = -1;
    lastRaiserIndex[2] = -1;
    lastRaiserIndex[3] = -1;
    //
    clearPlayerStates();
    setDealerPosition();
    //
    setPlayerOrder(true);
    //
    LOG_DEBUG("Small Blind: $"  << SMALL_BLIND);
    LOG_DEBUG("Big Blind: $"    << BIG_BLIND);
    LOG_DEBUG("Dealer:  Player" << getDealer()->getIndex());
    LOG_DEBUG("Smaller: Player" << getSmallBlindPlayer()->getIndex());
    LOG_DEBUG("Biger:   Player" << getBigBlindPlayer()->getIndex());
    //
    sbPlayerBetting();
    //
    bbPlayerBetting();
    //
    distributeCards();
    //
    if (!checkPlayerHandValid()) {
        printHand();
        exit(0);
    }
    potPreflop = 0;
    multi_enter = 0;
    //
    //userDefined();
    //
    printHand();
    //
    round = GAME_ROUND_PREFLOP;
    //
    researchPlayer = -1;
    //
    isShuffle = 1;
    //
    LOG_DEBUG("@NewGame: create game over!");
    return 0;
}

int Game::clone(Game *game) {
    LOG_DEBUG("@CloneGame: enter, num=" << game->numPlayers);
    //
    if (-1 == initDeckCardList()) {
        exit(0);
    }
    //
    std::ostringstream oss;
    deckOffset = game->deckOffset;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto oldCard = &(game->deck[i]);
        auto newCard = &deck[i];
        newCard->init();
        newCard->setCard(oldCard);
        //
        if (i >= deckOffset) {
            oss << newCard->str() << " ";
        }
    }
    //
    if (!checkDeckCardValid()) {
        LOG_FATAL("invalid card data!");
        exit(0);
    }
    //
    LOG_DEBUG("@CloneGame: card_list_init, deckOffset=" << deckOffset << ", card_list=" << oss.str());
    //
    if (game->numPlayers <= 0) {
        LOG_FATAL("Invalid game object!");
        exit(0);
    }
    //
    LOG_DEBUG("@CloneGame: check game params.");
    round = game->round;
    numPlayers = game->numPlayers;
    pot = game->pot;
    LOG_DEBUG("@CloneGame: numPlayers=" << numPlayers << ", pot=" << pot << ", round=" << round);
    //
    LOG_DEBUG("@CloneGame: init player_list begin, numPlayers=" << numPlayers);
    //
    if (-1 == initPlayerList()) {
        exit(0);
    }
    //
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        player->setGame(this);
        player->init(i);
        player->clone(&(game->players[i]));
    }
    //
    if (!checkPlayerHandValid()) {
        exit(0);
    }
    //
    printHand();
    LOG_DEBUG("@CloneGame: init player_list over, numPlayers=" << numPlayers);
    //
    flop[0] = getCard(game->flop[0]);
    flop[1] = getCard(game->flop[1]);
    flop[2] = getCard(game->flop[2]);
    //
    turn  = getCard(game->turn);
    //
    river = getCard(game->river);
    raiseCount = game->raiseCount;
    raiseNumber = game->raiseNumber;
    potPreflop  = game->potPreflop;
    multi_enter = game->multi_enter;
    lastRaiserIndex[0] = game->lastRaiserIndex[0];
    lastRaiserIndex[1] = game->lastRaiserIndex[1];
    lastRaiserIndex[2] = game->lastRaiserIndex[2];
    lastRaiserIndex[3] = game->lastRaiserIndex[3];
    LOG_DEBUG("Board: round=" << round << ", deckOffset=" << deckOffset);
    LOG_DEBUG("Board: flop=" << flop[0] << " " << flop[1] << " " << flop[2]);
    LOG_DEBUG("Board: turn=" << river << ", river=" << river);
    if (GAME_ROUND_PREFLOP == round) {
        flop[0] = nullptr;
        flop[1] = nullptr;
        flop[2] = nullptr;
        turn = nullptr;
        river = nullptr;
    } else if (GAME_ROUND_FLOP == round) {
        turn = nullptr;
        river = nullptr;
    } else if (GAME_ROUND_TURN == round) {
        river = nullptr;
    }
    //
    printBoard();
    //
    raiseCount = game->raiseCount;
    raiseNumber = game->raiseNumber;
    gameOver   = game->gameOver;
    turnIndex  = game->turnIndex;
    lastIndex  = game->lastIndex;
    LOG_DEBUG("@CloneGame: process cards, numPlayers=" << numPlayers
              << ", raiseCount=" << raiseCount
              << ", gameOver="   << gameOver
              << ", turnIndex="  << turnIndex
              << ", lastIndex="  << lastIndex);
    //
    memset(payOffs, 0, sizeof(payOffs));
    for (int i = 0; i < game->numPlayers; i++) {
        payOffs[i] = game->payOffs[i];
    }
    //
    researchPlayer = game->researchPlayer;
    //
    isShuffle = game->isShuffle;
    //
    LOG_DEBUG("@CloneGame: exit.");
    return 0;
}

int Game::reinit(int num) {
    if (num <= 0) {
        LOG_FATAL("data invalid, num=" << num);
        exit(0);
    }
    //
    resetCards();
    //
    shuffleDeck(0);
    //
    numPlayers = num;
    LOG_DEBUG("@NewGame: create game begin, num=" << numPlayers);
    //
    resetPlayers();
    //
    resetDeck();
    //
    lastRaiserIndex[0] = -1;
    lastRaiserIndex[1] = -1;
    lastRaiserIndex[2] = -1;
    lastRaiserIndex[3] = -1;
    clearPlayerStates();
    setDealerPosition();
    //
    setPlayerOrder(true);
    //
    LOG_DEBUG("Small Blind: $"  << SMALL_BLIND);
    LOG_DEBUG("Big Blind: $"    << BIG_BLIND);
    LOG_DEBUG("Dealer:  Player" << getDealer()->getIndex());
    LOG_DEBUG("Smaller: Player" << getSmallBlindPlayer()->getIndex());
    LOG_DEBUG("Biger:   Player" << getBigBlindPlayer()->getIndex());
    //
    sbPlayerBetting();
    //
    bbPlayerBetting();
    //
    distributeCards();
    //
    if (!checkPlayerHandValid()) {
        printHand();
        exit(0);
    }
    potPreflop = 0;
    multi_enter = 0;
    //
    //userDefined();
    //
    printHand();
    //
    round = GAME_ROUND_PREFLOP;
    //
    LOG_DEBUG("@NewGame: create game over!");
    return 0;
}

int Game::initPlayerList() {
    if ((numPlayers > 0) && (nullptr == players)) {
        players = new Player[numPlayers];
        LOG_DEBUG("init player list succ, numPlayers=" << numPlayers);
        return 0;
    }
    //
    LOG_ERROR("@FuckYou: init player list fail, numPlayers=" << numPlayers);
    return -1;
}

int Game::initDeckCardList() {
    deckOffset = 0;
    if (nullptr == deck) {
        deck = new Card[TOTAL_CARDS];
        LOG_DEBUG("new card_list, num=" << TOTAL_CARDS);
        //
        LOG_DEBUG("init deck cards.");
        return 0;
    }
    //
    LOG_ERROR("@FuckYou: deck-pointer is invalid");
    return -1;
}

int Game::cloneDeck(Game *game) {
    std::ostringstream oss;
    deckOffset = game->deckOffset;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto oldCard = &(game->deck[i]);
        auto newCard = &deck[i];
        newCard->init();
        newCard->setCard(oldCard->getSuit(), oldCard->getValue());
        //
        if (i >= deckOffset) {
            oss << newCard->str() << " ";
        }
    }
    return 0;
}

int Game::cloneDeck2(Game *game) {
    std::ostringstream oss;
    // deckOffset = game->deckOffset;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto oldCard = &(game->deck[i]);
        auto newCard = &deck[i];
        newCard->init();
        newCard->setCard(oldCard->getSuit(), oldCard->getValue());
        //
        if (i >= deckOffset) {
            oss << newCard->str() << " ";
        }
    }
    //
    TEST_I(oss.str());
    return 0;
}

int Game::userDefined() {
    LOG_DEBUG("-------------------- user-defined(1) --------------------");
    // // define the hand
    // for (int i = 0; i < numPlayers; i++) {
    //     auto player = &players[i];
    //     player->getHand(0).setCard(x);
    //     player->getHand(1).setCard(y);
    // }
    // // define a set of public cards
    // deck[deckOffset + 0].setCard(s, v);
    // deck[deckOffset + 1].setCard(s, v);
    // deck[deckOffset + 2].setCard(s, v);
    // deck[deckOffset + 3].setCard(s, v);
    // deck[deckOffset + 4].setCard(s, v);
    LOG_DEBUG("-------------------- user-defined(2) --------------------");
    int j = rand() % (2);
    if (j == 0) {
        //
        for (int c = 0 ; c < 52; c++) {
            if (deck[c].getSuit() == CardSuit(0) and deck[c].getValue() == ACE) {
                players[0].addCardToHand(&deck[c], 0);
                break;
            }
        }
        //
        for (int c = 0 ; c < 52; c++) {
            if (deck[c].getSuit() == CardSuit(1) and deck[c].getValue() == ACE) {
                players[0].addCardToHand(&deck[c], 1);
                break;
            }
        }
    } else {
        //
        for (int c = 0 ; c < 52; c++) {
            if (deck[c].getSuit() == CardSuit(0) and deck[c].getValue() == QUEEN) {
                players[0].addCardToHand(&deck[c], 0);
                break;
            }
        }
        //
        for (int c = 0 ; c < 52; c++) {
            if (deck[c].getSuit() == CardSuit(1) and deck[c].getValue() == QUEEN) {
                players[0].addCardToHand(&deck[c], 1);
                break;
            }
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(0) and deck[c].getValue() == KING) {
            players[1].addCardToHand(&deck[c], 0);
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(1) and deck[c].getValue() == KING) {
            players[1].addCardToHand(&deck[c], 1);
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(0) and deck[c].getValue() == THREE) {
            flop[0] = &deck[c];
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(1) and deck[c].getValue() == THREE) {
            flop[1] = &deck[c];
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(2) and deck[c].getValue() == THREE) {
            flop[2] = &deck[c];
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(3) and deck[c].getValue() == THREE) {
            turn = &deck[c];
            break;
        }
    }
    //
    for (int c = 0 ; c < 52; c ++) {
        if (deck[c].getSuit() == CardSuit(1) and deck[c].getValue() == TWO) {
            river = &deck[c];
            break;
        }
    }
    //
    return 0;
}

int Game::Game::sbPlayerBetting() {
    getSmallBlindPlayer()->setState(BET, SMALL_BLIND);
    getSmallBlindPlayer()->deductChips(SMALL_BLIND);
    getSmallBlindPlayer()->getState().move = INRESET;
    return 0;
}

int Game::bbPlayerBetting() {
    getBigBlindPlayer()->setState(BET, BIG_BLIND);
    getBigBlindPlayer()->deductChips(BIG_BLIND);
    getBigBlindPlayer()->getState().move = INRESET;
    return 0;
}

void Game::gotoNextRound() {
    if (GAME_ROUND_PREFLOP == round) {
        LOG_DEBUG("==================== GAME_ROUND_FLOP ====================");
        flipFlop();
        setPlayerOrder();
        round = GAME_ROUND_FLOP;
        potPreflop = getPot();
        LOG_DEBUG("set potPreflop =" << potPreflop);
    } else if (GAME_ROUND_FLOP == round) {
        LOG_DEBUG("==================== GAME_ROUND_TURN ====================");
        flipTurn();
        setPlayerOrder();
        round = GAME_ROUND_TURN;
    } else if (GAME_ROUND_TURN == round) {
        LOG_DEBUG("==================== GAME_ROUND_RIVER ====================");
        flipRiver();
        setPlayerOrder();
        round = GAME_ROUND_RIVER;
    } else if (GAME_ROUND_RIVER == round) {
        LOG_DEBUG("==================== GAME_ROUND_SETTLE ====================");
        determineWinner();
    }
    //
    // if (!isGameOver()) {
    //     LOG_DEBUG("@State: round=" << getRound()
    //               << ", turn=" << getTurnIndex()
    //               << ", last=" << getLastIndex()
    //               << ", over=" << isGameOver());
    // }
}

bool Game::isRoundOver() {
    if (checkAllin()) {
        // LOG_DEBUG("@Round: end condition1");
        return true;
    }
    //
    if (getTurnIndex() == getLastIndex()) {
        if (checkAllAct() && checkAllSame()) {
            // LOG_DEBUG("@Round: end condition2");
            return true;
        }
    }
    //
    if (checkLastPlayerRemaining(getTurnPlayer())) {
        // LOG_DEBUG("@Round: end condition3");
        return true;
    }
    //
    return false;
}

bool Game::isGameOver() {
    return gameOver;
}

void Game::resetDeck() {
    //
    pot = 0;
    //
    round = GAME_ROUND_PREFLOP;
    //
    deckOffset = 0;
    //
    gameOver = false;
    //
    turnIndex = 0;
    //
    lastIndex = 0;
    //
    flop[0] = nullptr;
    flop[1] = nullptr;
    flop[2] = nullptr;
    //
    turn = nullptr;
    river = nullptr;
    //
    memset(payOffs, 0, sizeof(payOffs));
}

void Game::resetCards() {
    deckOffset = 0;
    if (deck == nullptr) {
        deck = new Card[TOTAL_CARDS];
    }
    //
    bool isValid = true;
    std::ostringstream oss;
    int offset = 0;
    for (int suit = 0; suit <= 3; suit++) {
        for (int value = 2; value <= 14; value++) {
            auto card = &(deck[offset++]);
            card->init();
            card->setCard((CardSuit)suit, (CardValue)value);
            oss << card->str() << " ";
            //
            if ((card->getSuit() == SISI) || (card->getValue() == VIVI)) {
                isValid = false;
            }
        }
    }
    //
    if (!isValid) {
        LOG_ERROR("reset_cards_list(" << offset << "): " << oss.str());
    }
    //
    flop[0] = nullptr;
    flop[1] = nullptr;
    flop[2] = nullptr;
    //
    turn = nullptr;
    river = nullptr;
}

void Game::resetPlayers() {
    if ((numPlayers > 0) && (players == nullptr)) {
        players = new Player[numPlayers];
        LOG_DEBUG("@NewGame: new player_list, num=" << numPlayers);
    }
    //
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        player->setGame(this);
        player->init(i);
    }
}

Player *Game::getPlayers(int index) {
    return &players[index];
}

int Game::getPlayerNum() {
    return numPlayers;
}

int Game::getPot() {
    return pot;
}

void Game::rotateDealerChip() {
    int dealer = 0;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isDealer()) {
            dealer = i;
            break;
        }
    }
    //
    players[dealer].setDealer(false);
    //
    if (dealer == 0) {
        players[numPlayers - 1].setDealer(true);
    } else {
        players[dealer - 1].setDealer(true);
    }
}

void Game::setDealerPosition() {
    for (int i = 0; i < numPlayers; ++i) {
        players[i].setDealer(false);
    }
    //
    if (numPlayers == 2) {
        players[1].setDealer(true);
    } else {
        players[numPlayers - 1].setDealer(true);
    }
}

Player *Game::getDealer() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isDealer()) {
            return player;
        }
    }
    //never happens
    LOG_FATAL("Invalid dealer!");
    return nullptr;
}

Player *Game::getBigBlindPlayer() {
    int dealer = 0;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isDealer()) {
            dealer = i;
            break;
        }
    }
    return &players[(dealer + 2) % numPlayers];
}

Player *Game::getSmallBlindPlayer() {
    int dealer = 0;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isDealer()) {
            dealer = i;
            break;
        }
    }
    return &players[(dealer + 1) % numPlayers];
}

Player *Game::getNextTurnPlayer(bool isFold) {
    //
    int numCall = 0;
    int index = getTurnIndex();
    //
    do {
        //
        numCall = numCall + 1;
        if (numCall >= numPlayers) {
            break;
        }
        //
        index = (index + 1) % numPlayers;
        //
        auto player = &players[index];
        if (isFold && player->isFold()) {
            continue;
        }
        //
        break;
    } while (true);
    //
    turnIndex = index;
    return &players[index];
}

Player *Game::getNextPlayer(int index, bool isFold, bool isAllIn) {
    //
    int numCall = 0;
    do {
        numCall = numCall + 1;
        if (numCall >= numPlayers) {
            break;
        }
        //
        index = (index + 1) % numPlayers;
        auto player = &players[index];
        if ((isFold && player->isFold()) || (isAllIn && player->isAllIn())) {
            continue;
        }
        //
        break;
    } while (true);
    //
    return &players[index];
}

Player *Game::getPreviousPlayer(int index, bool isFold) {
    int numCall = 0;
    do {
        //
        numCall = numCall + 1;
        if (numCall >= numPlayers) {
            break;
        }
        //
        index = index - 1;
        if (index < 0) {
            index = (index + numPlayers) % numPlayers;
        }
        //
        auto player = &players[index];
        if (isFold && player->isFold())
            continue;
        //
        break;
    } while (true);
    return &players[index];
}

Player *Game::getNextPlayer(Player *player, bool isFold, bool isAllIn) {
    //
    int index = player->getIndex();
    //
    int numCall = 0;
    do {
        numCall = numCall + 1;
        if (numCall >= numPlayers) {
            break;
        }
        //
        index = (index + 1) % numPlayers;
        //
        auto player = &players[index];
        if (isFold && player->isFold())
            continue;
        if (isAllIn && player->isAllIn())
            continue;
        //
        break;
    } while (true);
    return &players[index];
}

Player *Game::getPreviousPlayer(Player *player, bool isFold) {
    //
    int index = player->getIndex();
    //
    int numCall = 0;
    do {
        //
        numCall = numCall + 1;
        if (numCall >= numPlayers) {
            break;
        }
        //
        index = index - 1;
        if (index < 0) {
            index = (index + numPlayers) % numPlayers;
        }
        //
        auto player = &players[index];
        if (isFold && player->isFold())
            continue;
        //
        break;
    } while (true);
    //
    return &players[index];
}

Player *Game::getPlayer(int index) {
    for (int i = 0; i < numPlayers; ++i) {
        if (i == index) {
            return &players[i];
        }
    }
    LOG_FATAL("invalid index: index=" << index);
    return nullptr;
}

int Game::getPlayerIndex(Player *player) {
    for (int i = 0; i < numPlayers; ++i) {
        auto pp = &players[i];
        if (player == pp) {
            return i;
        }
    }
    //never happens
    return -1;
}

void Game::flipFlop(bool verbose) {
    discardNextCard();
    //
    flop[0] = flipNextCard();
    flop[1] = flipNextCard();
    flop[2] = flipNextCard();
    //
    printHand();
    printBoard();
}

void Game::flipTurn(bool verbose) {
    discardNextCard();
    //
    turn = flipNextCard();
    //
    printHand();
    printBoard();
}

void Game::flipRiver(bool verbose) {
    discardNextCard();
    //
    river = flipNextCard();
    //
    printHand();
    printBoard();
}

void Game::distributeCards() {
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < numPlayers; i++) {
            auto player = &players[i];
            player->addCardToHand(flipNextCard(), j);
        }
    }
}

void Game::distributeCards2() {
    deckOffset = 0;
    for (int i = 0; i <= TOTAL_CARDS ; i++) {
        if (((deck[i].getValue() == ACE) || (deck[i].getValue() == KING)) && (deckOffset < 4)) {
            //
            swap(&deck[i], &deck[deckOffset]);
            //
            if (deckOffset == 0)
                players[0].addCardToHand(&deck[deckOffset], 0);
            if (deckOffset == 1)
                players[0].addCardToHand(&deck[deckOffset], 1);
            if (deckOffset == 2)
                players[1].addCardToHand(&deck[deckOffset], 0);
            if (deckOffset == 3)
                players[1].addCardToHand(&deck[deckOffset], 1);
            //
            deckOffset++;
        }
    }
    //
    for (int i = 0; i <= TOTAL_CARDS; i++) {
        if ((deck[i].getValue() == TWO) && (deckOffset < 7)) {
            //
            swap(&deck[i], &deck[deckOffset]);
            //
            if (deckOffset == 4)
                flop[0] = &deck[deckOffset];
            if (deckOffset == 5)
                flop[1] = &deck[deckOffset];
            if (deckOffset == 6)
                flop[2] = &deck[deckOffset];
            //
            deckOffset++;
        }
    }
    //
    for (int i = 0; i <= TOTAL_CARDS; i++) {
        if ((deck[i].getValue() == FOUR)
                && (deckOffset == 7)
                && (deck[i].getSuit() != flop[0]->getSuit())
                && (deck[i].getSuit() != flop[1]->getSuit())
                && (deck[i].getSuit() != flop[2]->getSuit())) {
            //
            swap(&deck[i], &deck[deckOffset]);
            //
            if (deckOffset == 7) {
                turn = &deck[deckOffset];
            }
            //
            deckOffset++ ;
        }
    }
    deckOffset = 16;
    //}
    //
    /*
    for (int i = 0; i <= TOTAL_CARDS; i++) {
        if ((deck[i].getValue() == THREE)
                && (deckOffset == 16)
                && (deck[i].getSuit() != flop[0]->getSuit())
                && (deck[i].getSuit() != flop[1]->getSuit())
                && (deck[i].getSuit() != flop[2]->getSuit())) {
            //
            swap(&deck[i], &deck[deckOffset]);
<<<<<<< Updated upstream
            //
            if (deckOffset == 8) {
=======
            if (deckOffset == 16)
>>>>>>> Stashed changes
                river = &deck[deckOffset];
            }
            //
            deckOffset++ ;
        }
    }*/
    /*
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < numPlayers; ++i) {
            players[i].addCardToHand(flipNextCard(), j);
        }
    }*/
}

bool Game::distributeCards3(int pos, const std::vector<string> &hands, const std::vector<string> &boards) {
    if (2 != hands.size()) {
        LOG_ERROR("invalid hands_size");
        return false;
    }
    //
    std::vector<string> remains;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto card = &deck[i];
        bool isAdd = true;
        for (auto iter = hands.begin(); iter != hands.end(); iter++) {
            if (card->strCluster() == (*iter)) {
                isAdd = false;
                break;
            }
        }
        //
        for (auto iter2 = boards.begin(); iter2 != boards.end(); iter2++) {
            if (card->strCluster() == (*iter2)) {
                isAdd = false;
                break;
            }
        }
        //
        if (isAdd) {
            remains.push_back(card->strCluster());
        }
    }
    LOG_DEBUG("player_num=" << hands.size() << ", remains_size=" << remains.size() << ", boards_size=" << boards.size());
    //
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(remains.begin(), remains.end(), std::default_random_engine(seed));
    //
    std::vector<int> vOffset;
    std::vector<string> vCards(52, "");
    //hole cards
    int handsOffset = numPlayers;
    if ((vCards[pos] != "") || (vCards[pos + handsOffset] != "")) {
        LOG_ERROR("Player" << pos << " wrong data, handsOffset=" << handsOffset);
        return false;
    }
    //
    vOffset.push_back(pos);
    vOffset.push_back(pos + handsOffset);
    //
    vCards[pos] = hands[0];
    vCards[pos + handsOffset] = hands[1];
    //board cards
    int boardOffset = numPlayers * 2;
    for (int j = 0; j < boards.size(); j++) {
        vOffset.push_back(j + boardOffset);
        vCards[j + boardOffset] = boards[j];
    }
    //
    std::ostringstream ss;
    ss << "vOffset:";
    for (auto iter = vOffset.begin(); iter != vOffset.end(); iter++) {
        ss << " " << *iter;
    }
    //remaining cards
    ss << " - ";
    for (int k = 0; k < remains.size(); k++) {
        auto found = std::find(vOffset.begin(), vOffset.end(), k);
        if (found != vOffset.end())
            continue;
        //
        ss << " " << k;
    }
    //
    for (int z = 0; z < vCards.size(); z++) {
        auto v = vCards[z];
        if (v != "")
            continue;
        //
        if (remains.empty()) {
            LOG_ERROR("remains wrong data!");
            return false;
        }
        //
        auto iter = remains.begin();
        vCards[z] = *iter;
        remains.erase(iter);
    }
    //
    LOG_DEBUG(ss.str());
    //
    if (!remains.empty()) {
        LOG_ERROR("vCards wrong data!");
        return false;
    }
    // Update pointer
    for (int i = 0; i < numPlayers; i++) {
        auto card1 = &deck[i];
        auto card2 = &deck[i + handsOffset];
        auto player = getPlayers(i);
        player->setHand(card1, 0);
        player->setHand(card2, 1);
    }
    //
    std::ostringstream ss1;
    for (int i = 0; i < vCards.size(); i++) {
        if (i == boardOffset)
            ss1 << "-" << vCards[i];
        else if (i == numPlayers)
            ss1 << "~" << vCards[i];
        else
            ss1 << " " << vCards[i];
        //
        deck[i].setCard(vCards[i]);
        // Update pointer
        int k = i - boardOffset;
        if ((k >= 0) && (k < boards.size())) {
            if (k < 3) {
                flop[k] = &deck[i];
            } else if (k == 3) {
                turn = &deck[i];
            } else if (k == 4) {
                river = &deck[i];
            }
        }
        //
        ss1 << "(" << deck[i].str() << ")";
    }
    LOG_DEBUG("SUCC: " << ss1.str());
    return true;
}

bool Game::distributeCards4(int i) {
    if (-1 == researchPlayer) {
        LOG_ERROR("no set researchPlayer!");
        return false;
    }
    //
    LOG_DEBUG("distributeCards4(): i=" << i << ", researchPlayer=" << researchPlayer);
    std::vector<int> vOffset;
    vOffset.clear();
    //
    int holeOffset = researchPlayer;
    vOffset.push_back(holeOffset);
    vOffset.push_back(holeOffset + numPlayers);
    //
    int boardNum = 0;
    int boardOffset = numPlayers * 2;
    if (GAME_ROUND_FLOP == getRound()) {
        vOffset.push_back(boardOffset + 0);
        vOffset.push_back(boardOffset + 1);
        vOffset.push_back(boardOffset + 2);
        boardNum = 3;
    } else if (GAME_ROUND_TURN == getRound()) {
        vOffset.push_back(boardOffset + 0);
        vOffset.push_back(boardOffset + 1);
        vOffset.push_back(boardOffset + 2);
        vOffset.push_back(boardOffset + 3);
        boardNum = 4;
    } else if (GAME_ROUND_RIVER == getRound()) {
        vOffset.push_back(boardOffset + 0);
        vOffset.push_back(boardOffset + 1);
        vOffset.push_back(boardOffset + 2);
        vOffset.push_back(boardOffset + 3);
        vOffset.push_back(boardOffset + 4);
        boardNum = 5;
    }
    //
    std::ostringstream os;
    for (auto iter = vOffset.begin(); iter != vOffset.end(); iter++) {
        os << " " << *iter << "-" << deck[*iter].str();
    }
    LOG_DEBUG("vOffset: " << os.str());
    //
    std::ostringstream os1;
    std::ostringstream os3;
    std::vector<int> vCards;
    vCards.clear();
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto card = &deck[i];
        if (std::find(vOffset.begin(), vOffset.end(), i) == vOffset.end()) {
            int face = card->getSuit() * 15 + card->getValue();
            vCards.push_back(face);
            os1 << " " << card->str();
        }
        os3 << " " << card->str();
    }
    //
    if ((vCards.size() + vOffset.size()) != TOTAL_CARDS) {
        LOG_ERROR("numPlayers:" << numPlayers << " iRound:" << round);
        LOG_ERROR("vOffset: " << os.str());
        LOG_ERROR("vDeck: " << os3.str());
        LOG_ERROR("Wrong number of cards: vCards_size=" << vCards.size() << ",vOffset_size=" << vOffset.size());
        return false;
    }
    //
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(vCards.begin(), vCards.end(), rng);
    //
    std::ostringstream os2;
    std::ostringstream os4;
    int offset = 0;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto card = &deck[i];
        //
        if (std::find(vOffset.begin(), vOffset.end(), i) == vOffset.end()) {
            auto face = vCards[offset++];
            auto suit = face / 15;
            auto vals = face % 15;
            card->setCard((CardSuit)suit, (CardValue)vals);
            os2 << " " << card->str();
        }
        //
        os4 << " " << card->str();
    }
    //
    LOG_DEBUG("shuffle(1)" << os1.str());
    LOG_DEBUG("shuffle(2)" << os2.str());
    //
    LOG_DEBUG("deck(1)" << os3.str());
    LOG_DEBUG("deck(2)" << os4.str());
    // Update pointer
    int handsOffset = numPlayers;
    for (int i = 0; i < numPlayers; i++) {
        auto card1 = &deck[i];
        auto card2 = &deck[i + handsOffset];
        auto player = getPlayers(i);
        player->setHand(card1, 0);
        player->setHand(card2, 1);
    }
    // Update pointer
    for (int k = 0; k < boardNum; k++) {
        if (k < 3) {
            flop[k] = &deck[i + boardOffset];
        } else if (k == 3) {
            turn = &deck[i + boardOffset];
        } else if (k == 4) {
            river = &deck[i + boardOffset];
        }
    }
    //
    return true;
}

bool Game::distributeCards5(const std::unordered_map<int, vector<string>> &hands, const std::vector<string> &boards) {
    std::vector<string> remains;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto card = &deck[i];
        bool isAdd = true;
        for (auto iter1 = hands.begin(); iter1 != hands.end(); iter1++) {
            for (auto iter = (*iter1).second.begin(); iter != (*iter1).second.end(); iter++) {
                if (card->strCluster() == (*iter)) {
                    isAdd = false;
                    break;
                }
            }
        }
        //
        for (auto iter2 = boards.begin(); iter2 != boards.end(); iter2++) {
            if (card->strCluster() == (*iter2)) {
                isAdd = false;
                break;
            }
        }
        //
        if (isAdd) {
            remains.push_back(card->strCluster());
        }
    }
    LOG_DEBUG("player_num=" << hands.size()
              << ", remains_size=" << remains.size()
              << ", boards_size=" << boards.size());
    //
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(remains.begin(), remains.end(), std::default_random_engine(seed));
    //
    std::vector<int> vOffset;
    std::vector<string> vCards(52, "");
    //hole cards
    int handsOffset = numPlayers;
    for (auto iter = hands.begin(); iter != hands.end(); iter++) {
        auto first = (*iter).first;
        auto second = (*iter).second;
        //
        if ((vCards[first] != "") || (vCards[first + handsOffset] != "")) {
            LOG_ERROR("Player" << first << " wrong data, handsOffset=" << handsOffset);
            return false;
        }
        //
        vOffset.push_back(first);
        vOffset.push_back(first + handsOffset);
        //
        vCards[first] = second[0];
        vCards[first + handsOffset] = second[1];
    }
    //board cards
    int boardOffset = numPlayers * 2;
    for (int j = 0; j < boards.size(); j++) {
        vOffset.push_back(j + boardOffset);
        vCards[j + boardOffset] = boards[j];
    }
    //
    std::ostringstream ss;
    ss << "vOffset:";
    for (auto iter = vOffset.begin(); iter != vOffset.end(); iter++) {
        ss << " " << *iter;
    }
    //remaining cards
    ss << " - ";
    for (int k = 0; k < remains.size(); k++) {
        auto found = std::find(vOffset.begin(), vOffset.end(), k);
        if (found != vOffset.end())
            continue;
        //
        ss << " " << k;
    }
    //
    for (int z = 0; z < vCards.size(); z++) {
        auto v = vCards[z];
        if (v != "")
            continue;
        //
        if (remains.empty()) {
            LOG_ERROR("remains wrong data!");
            return false;
        }
        //
        auto iter = remains.begin();
        vCards[z] = *iter;
        remains.erase(iter);
    }
    //
    LOG_DEBUG(ss.str());
    //
    if (!remains.empty()) {
        LOG_ERROR("vCards wrong data!");
        return false;
    }
    // Update pointer
    for (int i = 0; i < numPlayers; i++) {
        auto card1 = &deck[i];
        auto card2 = &deck[i + handsOffset];
        auto player = getPlayers(i);
        player->setHand(card1, 0);
        player->setHand(card2, 1);
    }
    //
    std::ostringstream ss1;
    for (int i = 0; i < vCards.size(); i++) {
        if (i == boardOffset)
            ss1 << "-" << vCards[i];
        else if (i == numPlayers)
            ss1 << "~" << vCards[i];
        else
            ss1 << " " << vCards[i];
        //
        deck[i].setCard(vCards[i]);
        // Update pointer
        int k = i - boardOffset;
        if ((k >= 0) && (k < boards.size())) {
            if (k < 3) {
                flop[k] = &deck[i];
            } else if (k == 3) {
                turn = &deck[i];
            } else if (k == 4) {
                river = &deck[i];
            }
        }
        //
        ss1 << "(" << deck[i].str() << ")";
    }
    LOG_DEBUG("SUCC: " << ss1.str());
    return true;
}

void Game::swap(Card *a, Card *b) {
    Card temp;
    temp.init();
    temp.setCard(a);
    //
    a->setCard(b);
    b->setCard(&temp);
}

void Game::shuffleDeck(int start) {
    if (isShuffle) {
        srand(time(NULL));
        for (int i = TOTAL_CARDS - 1; i >= start; i--) {
            int j = rand() % (TOTAL_CARDS - start) + start;
            if (i == j) {
                continue;
            }
            //
            swap(&deck[i], &deck[j]);
        }
    } else {
        LOG_DEBUG("no shuffle Deck!!!!");
    }
    //
    // std::ostringstream oss;
    // for (int i = start; i < TOTAL_CARDS; i++) {
    //     oss << deck[i].str() << " ";
    // }
    //LOG_DEBUG("shuffle: deckOffset=" << deckOffset << ", cards=" << oss.str());
}

bool Game::checkDeckCardValid() {
    bool isValid = true;
    std::ostringstream oss;
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto card = &deck[i];
        if ((card->getSuit() == SISI) || (card->getValue() == VIVI)) {
            isValid = false;
        }
        //
        oss << card->str() << " ";
    }
    //
    if (!isValid) {
        LOG_ERROR("cards: " << oss.str());
    }
    //
    return isValid;
}

bool Game::checkPlayerHandValid() {
    bool isValid = true;
    //
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        if ((nullptr == player->getHand(0)) || (nullptr == player->getHand(1))) {
            LOG_ERROR("Player" << player->getIndex() << "hand error!!");
            isValid = false;
        }
    }
    //
    return isValid;
}

void Game::discardNextCard() {
    // deckOffset++;
    shuffleDeck(deckOffset);
}

Card *Game::flipNextCard() {
    auto card = &deck[deckOffset];
    deckOffset++;
    return card;
}

int Game::determineWinner() {
    if (gameOver) {
        LOG_DEBUG("Double settlement！");
        return 0;
    }
    //
    if ((flop[0] == nullptr) || (flop[1] == nullptr) || (flop[2] == nullptr)) {
        flipFlop(true);
    }
    //
    if (turn == nullptr) {
        flipTurn(true);
    }
    //
    if (river == nullptr) {
        flipRiver(true);
    }
    //
    printStatus("GameOver");
    //
    std::vector<int> winners;
    winners.clear();
    //
    int numOther = 0;
    int numAllin = 0;
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        if (player->isFold())
            continue;
        //
        if (player->isAllIn())
            numAllin++;
        else
            numOther++;
        //
        winners.push_back(i);
    }
    LOG_DEBUG("GameOver: numOther=" << numOther << ", numAllin=" << numAllin);
    //
    if (!flop[0] || !flop[1] || !flop[2]) {
        LOG_DEBUG("GameOver: no card, flop=" << flop[0] << " " << flop[1] << " " << flop[2]);
    } else if (!turn) {
        LOG_DEBUG("GameOver: no card, turn=" << turn);
    } else if (!river) {
        LOG_DEBUG("GameOver: no card, rive=" << river);
    }
    //
    if (winners.size() > 1) {
        winners.clear();
        //
        Hand hands[numPlayers];
        Hand best = NOTHING;
        //
        int curr = 0;
        for (int i = 0; i < numPlayers; ++i) {
            auto player = &players[i];
            if (player->isFold()) {
                continue;
            }
            //
            hands[i] = player->bestHand(flop, turn, river, true);
            if (hands[i] > best) {
                best = hands[i];
                curr = i;
                winners.clear();
                winners.push_back(i);
            } else if (hands[i] == best) {
                auto target = &players[curr];
                int ret = CardUtil::compareHand(player->getSelect(), target->getSelect(), best);
                if (ret == 0) {
                    winners.push_back(i);
                } else if (ret == 1) {
                    curr = i;
                    winners.clear();
                    winners.push_back(i);
                }
            }
        }
    }
    //
    givePotToWinner(winners);
    //
    round = GAME_ROUND_RIVER;
    //
    gameOver = true;
    //
    for (auto item : winners) {
        players[item].incHandsWon();
    }
    LOG_DEBUG("@Game: gameOver=" << gameOver << ", winNum=" << winners.size());
    //
    return winners.size();
}

void Game::givePotToWinner(vector<int> winners) {
    //
    int numEach = winners.size();
    int each = 0;
    if (numEach > 0) {
        each = (int)floor((float)pot / (float)numEach);
        LOG_DEBUG("Split pot winner(succ): numEach=" << numEach << ", pot=$" << pot << ", each=$" << each);
    } else {
        LOG_DEBUG("Split pot winner(fail): numEach=" << numEach << ", pot=$" << pot << ", each=$" << each);
    }
    //
    printHand();
    //
    memset(payOffs, 0, sizeof(payOffs));
    for (auto item : winners) {
        auto player = &players[item];
        int have = player->getChips();
        int win = have + each - PLAYER_CHIPS;
        player->addChips(each);
        payOffs[item] = win;
        LOG_DEBUG("Player" << item << ": win=$" << win << ", have=$" << have);
    }
    //
    for (int item = 0; item < numPlayers; item++) {
        auto search = std::find(winners.begin(), winners.end(), item);
        if (search != winners.end()) {
            continue;
        }
        //
        auto player = &players[item];
        int have = player->getChips();
        int lose = player->getChips() - PLAYER_CHIPS;
        payOffs[item] = lose;
        LOG_DEBUG("Player" << item << ": lose=$" << lose << ", have=$" << have);
    }
    //
    pot = 0;
}

void Game::printPot() {
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        LOG_DEBUG("Player" << i
                  << ": fold=" << player->isFold()
                  << ", chips=" << player->getChips());
    }
    LOG_DEBUG("Pot: $" << pot);
}

void Game::printBoard() {
    std::ostringstream oss;
    oss << "@Board cards:";
    //
    int numCombo = 0;
    Card *combo[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    //flop
    if (flop[0] != nullptr) {
        combo[numCombo++] = flop[0];
        oss << "-" << flop[0]->str();
    }
    //
    if (flop[1] != nullptr) {
        combo[numCombo++] = flop[1];
        oss << "-" << flop[1]->str();
    }
    //
    if (flop[2] != nullptr) {
        combo[numCombo++] = flop[2];
        oss << "-" << flop[2]->str();
    }
    //turn
    if (turn != nullptr) {
        combo[numCombo++] = turn;
        oss << "-" << turn->str();
    }
    //river
    if (river != nullptr) {
        combo[numCombo++] = river;
        oss << "-" << river->str();
    }
    //
    if (numCombo > 0) {
        LOG_DEBUG(oss.str());
    }
}

void Game::printStatus(std::string tag) {
    LOG_DEBUG("------- Status@" << tag << " enter -------");
    //
    LOG_DEBUG("@Stage: round=" << getRound() << ", pot=" << getCurrentPot());
    LOG_DEBUG("@Turn: index="         << getTurnIndex()
              << ", activtedNum="     << getActivtedNum()
              << ", raiseCount="      << getRaiseCount()
              << ", potCluster="      << getPotCluster()
              << ", handCluster="     << getPlayer(getTurnIndex())->getCluster()
              << ", cardCluster="     << getCardCluster(getTurnIndex())
              << ", firstBet="        << getFirstBet()
              << ", beforePlayerNum=" << getBeforeAllPlayerNum(getTurnIndex())
              << ", beforePlayerNum=" << getBeforeActivePlayerNum(getTurnIndex())
              << ", behindPlayerNum=" << getBehindAllPlayerNum(getTurnIndex())
              << ", behindPlayerNum=" << getBehindActivePlayerNum(getTurnIndex())
              << ", validActionId="   << std::bitset<11>(getTurnPlayer()->getActionSequence()));
    //
    printHand();
    printBoard();
    //
    LOG_DEBUG("------- Status@" << tag << " exit -------");
}

void Game::setPlayerStates() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold()) {
            player->setState(FOLD, 0);
        } else {
            if (!player->isAllIn()) {
                player->setState(INRESET, 0);
            }
        }
    }
}

void Game::clearPlayerStates() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        player->setState(INRESET, 0);
        player->setIsAdd(false);
    }
}

void Game::addBetsToPot() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isAllIn()) {
            if (player->getState().isAdd) {
                continue;
            } else {
                player->setIsAdd(true);
            }
        }
        //
        pot += player->getState().bet;
    }
}

int Game::getCurrentPot() {
    int curPot = pot;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        curPot += player->getState().bet;
    }
    return curPot;
}

bool Game::checkLastPlayerRemaining(Player *player) {
    if (player->isFold()) {
        return false;
    }
    //
    bool ret = true;
    for (int i = 0; i < numPlayers; ++i) {
        auto pp = &players[i];
        if (pp != player) {
            ret &= (pp->isFold());
        }
    }
    //
    return ret;
}

Card **Game::getFlopCard() {
    return flop;
}

Card *Game::getTurnCard() {
    return turn;
}

Card *Game::getRiverCard() {
    return river;
}

Card *Game::getCard(Card *card) {
    if (card == nullptr) {
        return nullptr;
    }
    //
    for (int i = 0; i < TOTAL_CARDS; ++i) {
        auto cc = &deck[i];
        if ((card->getSuit() == cc->getSuit()) && (card->getValue() == cc->getValue())) {
            return cc;
        }
    }
    //never happens
    LOG_FATAL("invalid data: card=" << card->str());
    return nullptr;
}

Card *Game::getCard(int rank, int suit) {
    for (int i = 0; i < TOTAL_CARDS; i++) {
        auto cc = &deck[i];
        if ((suit == cc->getSuit()) && (rank == cc->getValue())) {
            return cc;
        }
    }
    //never happens
    LOG_FATAL("invalid data: rank=" << rank << ", suit=" << suit);
    return nullptr;
}

void Game::printHand() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        player->printHand();
    }
}

void Game::resetChips() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        player->resetChips();
    }
}

bool Game::checkAllin() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold())
            continue;
        if (!player->isAllIn())
            return false;
    }
    return true;
}

bool Game::checkAllAct() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (!player->isAct())
            return false;
    }
    return true;
}

bool Game::checkAllSame() {
    int maxBetChips = getRoundMaxBet();
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold())
            continue;
        if (player->isAllIn())
            continue;
        if (maxBetChips != player->getState().bet)
            return false;
    }
    //
    return true;
}

//
bool Game::checkAllFold() {
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold())
            continue;
        //
        return false;
    }
    //
    return true;
}

int Game::getRoundMaxBet() {
    int bet = 0;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold())
            continue;
        if (bet >= player->getState().bet)
            continue;
        bet = player->getState().bet;
    }
    //
    return bet;
}

GameRound Game::getRound() {
    return round;
}

void Game::setRound(int r) {
    round = (GameRound)r;
}

int Game::getRaiseIndex() {
    if (raiseCount == 0)
        return 0;
    //
    if (raiseCount == 1)
        return 1;
    //
    if (raiseCount > 1)
        return 2;
    //
    return 0;
}
int Game::getRaiseCount() {
    return raiseCount;
}

int Game::incRaiseCount() {
    return (++raiseCount);
}

int Game::setRaiseCount(int count) {
    raiseCount = count;
    return raiseCount;
}

int Game::getActivtedNum() {
    int cnt = 0;
    //
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        if (!player->isFold()) {
            cnt++;
        }
    }
    //
    return cnt;
}

int Game::getBeforeAllPlayerNum(int playerIndex) {
    if (round == GAME_ROUND_PREFLOP) {
        if ((playerIndex != 0) && (playerIndex != 1)) {
            return playerIndex - 2;
        } else {
            return numPlayers - 1 - (1 - playerIndex);
        }
    }
    //
    return playerIndex;
}

int Game::getBeforeActivePlayerNum(int playerIndex) {
    std::vector<int> playerLutRaw;
    std::vector<int> playerLut;
    int beforActivePeople = 0 ;
    int beforAllPeople = getBeforeAllPlayerNum(playerIndex);;
    for (int s = 0 ; s < numPlayers; s++) {
        playerLutRaw.push_back(s);
    }
    //
    if (round == GAME_ROUND_PREFLOP) {
        vector temp(playerLutRaw.begin() + 2, playerLutRaw.begin() + numPlayers);
        temp.push_back(0);
        temp.push_back(1);
        playerLut = temp;
    } else {
        playerLut =  playerLutRaw;
    }
    //
    for (int s = 0 ; s < numPlayers; s++ ) {
        if (s < beforAllPeople) {
            if (players[playerLut[s]].isFold() == false) {
                beforActivePeople += 1;
            }
        }
    }
    //
    return beforActivePeople;
}

int Game::getBehindActivePlayerNum(int playerIndex) {
    std::vector<int > playerLutRaw;
    std::vector<int > playerLut;
    int afterActivePeople = 0;
    int beforAllPeople = getBeforeAllPlayerNum(playerIndex);;
    //
    for (int s = 0 ; s < numPlayers; s++) {
        playerLutRaw.push_back(s);
    }
    //
    if (round == GAME_ROUND_PREFLOP) {
        vector temp(playerLutRaw.begin() + 2, playerLutRaw.begin() + numPlayers);
        temp.push_back(0);
        temp.push_back(1);
        playerLut = temp;
    } else {
        playerLut =  playerLutRaw;
    }
    //
    for (int s = 0 ; s < numPlayers; s++ ) {
        if (s > beforAllPeople) {
            if (!players[playerLut[s]].isFold()) {
                afterActivePeople += 1;
            }
        }
    }
    //
    return afterActivePeople;
}

int Game::getLastRaiseIndex() {
    // LOG_DEBUG("getLastRaiserIndex" << "round" << round);
    //
    if (round != 1 ) {
        // LOG_DEBUG("getLastRaiserIndex" << lastRaiserIndex[round - 2] << "round" << round );
        if (getTurnIndex() == lastRaiserIndex[round - 2]) {
            return 1;
        } else {
            return 0;
        }
    }
    //
    return 0;
}

int Game::getBehindAllPlayerNum(int playerIndex) {
    if (round == GAME_ROUND_PREFLOP) {
        return numPlayers - 1 - getBeforeAllPlayerNum(playerIndex);
    }
    //
    return numPlayers - 1 - playerIndex;
}

int Game::getCardCluster(int playerIndex) {
    int mRound = getRound();
    if ( mRound == GAME_ROUND_PREFLOP ) {
        bool suited = false;
        auto player = &players[playerIndex];
        if (player->getHand()[0]->getSuit() ==  player->getHand()[1]->getSuit()) {
            suited = true;
        }
        float centerrank = 0;
        float rankcha = 0;
        if (player->getHand()[0]->getValue() >= player->getHand()[1]->getValue()) {
            centerrank = player->getHand()[1]->getValue() - 2;
            rankcha = player->getHand()[0]->getValue() - player->getHand()[1]->getValue();
        } else {
            centerrank = player->getHand()[0]->getValue() - 2;
            rankcha = player->getHand()[1]->getValue() - player->getHand()[0]->getValue();
        }
        if (suited) {
            return centerrank * 13 + centerrank + rankcha * 13;
        } else {
            return centerrank * 13 + centerrank + rankcha;
        }
    } else {
        std::string cardStr = "";
        auto player = &players[playerIndex];
        cardStr += player->getHand(0)->strCluster();
        cardStr += player->getHand(1)->strCluster();
        if (mRound > 1) {
            cardStr += flop[0]->strCluster();
            cardStr += flop[1]->strCluster();
            cardStr += flop[2]->strCluster();
        }
        if (mRound > 2) {
            cardStr += turn->strCluster();
        }
        if (mRound > 3) {
            cardStr += river->strCluster();
        }
        int mCardCluster = NumPy::GetInstance().ShmPointer()->TestCard(cardStr);
        //LOG_DEBUG("cardCluster" << " " << cardStr << " " << mCardCluster);
	//tempset for test
        if(player->getHand(0)->strCluster()[0] == 'K' and player->getHand(1)->strCluster()[0] == 'K' and mRound == GAME_ROUND_TURN  )
	{
            return 100; 
	}
	
        return mCardCluster;
    }
    //
    return 0;
}

int Game::getBehindPlayerNum(int playerIndex) {
    int cnt = 0;
    for (int i = playerIndex + 1; i < numPlayers; i++) {
        auto player = &players[i];
        if (!player->isFold()) {
            cnt++;
        }
    }
    return cnt;
}

int Game::getFirstBet() {
    auto player = &players[turnIndex];
    int dif = getRoundMaxBet() - player->getState().bet;
    int biggest_bet_i = PLAYER_CHIPS  - player->getChips() + dif; // warning!!!!!!!!!
    if (biggest_bet_i == BIG_BLIND and getRound() == 1 )
        return 1;
    else
        return 0;
}

int Game::getPotCluster() {
    //printf("in potCluster");
    int pot_size_id = 0;
    int n_chips_to_call_id = 0;
    int biggest_bet_id = 0 ;
    auto player = &players[turnIndex];
    int cur = player->getChips();
    int all_pot = getCurrentPot();
    int dif = getRoundMaxBet() - player->getState().bet;
    int n_chips_to_call_i = dif;
    int biggest_bet_i = PLAYER_CHIPS - player->getChips() + dif; // warning!!!!!!!!!
    float pot_size = (float)n_chips_to_call_i / all_pot;
    float n_chips_to_call = (float)n_chips_to_call_i / biggest_bet_i;
    float biggest_bet = biggest_bet_i / 200.0;
    int pot_afterflop_id = 0 ;
    float pot_afterflop = 0;
    // LOG_DEBUG("cur= " << cur << " all_pot=" << all_pot << " dif=" << dif << " n_chips_to_call_i=" << n_chips_to_call_i << " biggest_bet=" << biggest_bet);
    if (n_chips_to_call < 0.13)
        n_chips_to_call_id = 0;
    else if (n_chips_to_call <= 0.30)
        n_chips_to_call_id = 1;
    else if (n_chips_to_call <= 0.38)
        n_chips_to_call_id = 2;
    else if (n_chips_to_call <= 0.46)
        n_chips_to_call_id = 3;
    else if (n_chips_to_call <= 0.55)
        n_chips_to_call_id = 4;
    else if (n_chips_to_call <= 0.63)
        n_chips_to_call_id = 5;
    else if (n_chips_to_call <= 0.7)
        n_chips_to_call_id = 6;
    else
        n_chips_to_call_id = 7;
    //
    if (pot_size <= 0.13)  // 15% pot0.13
        pot_size_id = 0;
    else if ( pot_size <= 0.30)  // 30%pot 0.24
        pot_size_id = 1;
    else if ( pot_size <= 0.38)  // 50%pot 0.33
        pot_size_id = 2;
    else if (pot_size <= 0.46)  // 75 %pot 0.42
        pot_size_id = 3;
    else if (pot_size <= 0.55)  // 100 %pot 0.5
        pot_size_id = 4;
    else if (pot_size <= 0.63)  // 150%pot 0.6
        pot_size_id = 5;
    else if (pot_size <= 0.7)  // 200%pot 0.66
        pot_size_id = 6;
    else if (pot_size <= 0.775)  // 300%pot 0.75
        pot_size_id = 7;
    else if (pot_size <= 0.85)  // 500%pot 0.83
        pot_size_id = 9;
    else if (pot_size <= 0.925)  // 1100%pot 0.916
        pot_size_id = 11;
    else if (pot_size <= 0.96)  // 2000%pot 0.952
        pot_size_id = 13;
    else if (pot_size <= 0.9775)  // 4000%pot 0.975
        pot_size_id = 15;
    else if (pot_size <= 0.9875)  // 7000%pot 0.985
        pot_size_id = 17;
    else  // 10000pot 0.99
        pot_size_id = 18;
    //
    if (biggest_bet < 3)
        biggest_bet_id = 0;
    else if (biggest_bet < 6)
        biggest_bet_id = 1;
    else if (biggest_bet <= 12)
        biggest_bet_id = 2;
    else if (biggest_bet <= 24)
        biggest_bet_id = 3;
    else if (biggest_bet < 48)
        biggest_bet_id = 4;
    else if (biggest_bet < 72)
        biggest_bet_id = 5;
    else if (biggest_bet < 100)
        biggest_bet_id = 6;
    else if (biggest_bet == 100)
        biggest_bet_id = 7;
    //
    if (round !=  GAME_ROUND_PREFLOP) {
        pot_afterflop_id = 0;
        pot_afterflop = potPreflop / (all_pot * (1 - pot_size));
        if (pot_afterflop < 0.033)  // 1.65%  Ð¡ÓÚ40#Í¬»¨Ë³ Ë³×Ó
            pot_afterflop_id = 7;
        else if (pot_afterflop < 0.047)  // 大于21小于40 三条
            pot_afterflop_id = 6;
        else if (pot_afterflop < 0.076)  // 6.25% 大于13小于21 # 高两对
            pot_afterflop_id = 5;
        else if (pot_afterflop < 0.12)  // 12.5% 大于7.7 小于13#  超对 低两对
            pot_afterflop_id = 4;
        else if (pot_afterflop < 0.3)  // 25% 大于3.3 小于7.7# 顶对
            pot_afterflop_id = 3;
        else if (pot_afterflop < 0.45)  // 50% 大于2.2 小于3.3# 中对
            pot_afterflop_id = 2;
        else if (pot_afterflop < 0.55)  // 66% 大于1.8 小于2.2 小对
            pot_afterflop_id = 1;
        else
            pot_afterflop_id = 0; // 100%
        //
        biggest_bet_id = biggest_bet_id * 10 + pot_afterflop_id;
    }
    //
    // LOG_DEBUG("biggest_bet_id  " << biggest_bet_id  << " pot_size_id=" << pot_size_id << " n_chips_to_call_id=" << n_chips_to_call_id );
    //
    return biggest_bet_id * 1000  + pot_size_id * 10 + n_chips_to_call_id ;
    //printf("out potCluster");
    //return floor(getCurrentPot() / BIG_BLIND);
}

int Game::getTurnIndex() {
    return turnIndex;
}

void Game::setTurnIndex(int index) {
    turnIndex = index;
}

int Game::getLastIndex() {
    return lastIndex;
}

void Game::setLastIndex(int index) {
    lastIndex = index;
}

int Game::getNextIndex() {
    return (turnIndex + 1) % numPlayers;
}

void Game::setPlayerOrder(bool firstRound) {
    if (firstRound) {
        turnIndex = getNextPlayer(getBigBlindPlayer())->getIndex();
        lastIndex = getBigBlindPlayer()->getIndex();
    } else { //Otherwise its the small blind player who goes first
        turnIndex = getSmallBlindPlayer()->getIndex();
        lastIndex = getDealer()->getIndex();
    }
    //
    // LOG_DEBUG("@Order(before): round=" << getRound()
    //           << ", turn=" << getTurnIndex()
    //           << ", last=" << getLastIndex());
    //
    if (true) {
        auto turnPlayer = getTurnPlayer();
        while (turnPlayer->isFold() || turnPlayer->isAllIn()) {
            if (checkAllin()) {
                break;
            }
            //
            turnPlayer = getNextPlayer(turnIndex);
            turnIndex = turnPlayer->getIndex();
        }
    }
    //
    if (true) {
        auto lastPlayer = getLastPlayer();
        while (lastPlayer->isFold() || lastPlayer->isAllIn()) {
            if (checkAllin())
                break;
            lastPlayer = getPreviousPlayer(lastPlayer->getIndex());
            lastIndex = lastPlayer->getIndex();
        }
    }
    //
    raiseCount = 0;
    raiseNumber = 0;
    // LOG_DEBUG("@Order(behind): round=" << getRound()
    //           << ", turn=" << getTurnIndex()
    //           << ", last=" << getLastIndex());
}

Player *Game::getTurnPlayer() {
    return getPlayer(turnIndex);
}

Player *Game::getLastPlayer() {
    return getPlayer(lastIndex);
}

int *Game::getPayOffs() {
    return &payOffs[0];
}

int Game::getOtherPlayerNum(int type) {
    int numFold = 0;
    int numAllin = 0;
    int numOther = 0;
    for (int i = 0; i < numPlayers; i++) {
        auto player = &players[i];
        if (player->isFold()) {
            numFold++;
        } else if (player->isAllIn()) {
            numAllin++;
        } else {
            numOther++;
        }
    }
    LOG_DEBUG("numFold=" << numFold << ",numAllin=" << numAllin
              << ",numOther=" << numOther);
    if (type == 0)
        return numOther;
    else if (type == 1)
        return numAllin;
    else if (type == 2)
        return numFold;
    else
        return -1;
}

int Game::setLastPlayerToAct(int index) {
    // lastIndex = getPreviousPlayer(index, true)->getIndex();
    lastIndex = index;
    LOG_DEBUG("@Update round last player: index=" << lastIndex);
    return 0;
}

int Game::setLastPlayerToActor() {
    lastIndex = turnIndex;
    LOG_DEBUG("@Update round last player: index=" << lastIndex);
    return 0;
}


bool Game::checkGameOver() {
    int numOther = 0;
    int numAllIn = 0;
    int numFold = 0;
    int numNone = 0;
    for (int i = 0; i < numPlayers; ++i) {
        auto player = &players[i];
        if (player->isFold()) {
            numFold++;
        } else if (player->isAllIn()) {
            numAllIn++;
        } else if (player->isAct()) {
            numOther++;
        } else {
            numNone++;
        }
    }
    //
    if (numNone > 0) {
        // LOG_DEBUG("Not doing something: numNone=" << numNone);
        return false;
    }
    if (1 == numOther) {
        //
        if (checkAllAct() && checkAllSame()) {
            // LOG_DEBUG("@Cond1: numFold=" << numFold << ", numAllin=" << numAllIn << ", numOther=" << numOther);
            return true;
        }
    }
    //
    if (1 == numOther + numAllIn) {
        // LOG_DEBUG("@Cond2: numFold=" << numFold << ", numAllin=" << numAllIn << ", numOther=" << numOther);
        return true;
    }
    //
    return false;
}

int Game::quickGotoGameOver() {
    LOG_DEBUG("@GameOver: quick goto game over, round=" << round);
    determineWinner();
    return 0;
}

void Game::setLastRaiserIndex() {
    lastRaiserIndex[round - 1] = turnIndex;
    // LOG_DEBUG("setLastRaiserIndex=" << lastRaiserIndex[round - 1] << ", round=" << round - 1);
}

int Game::setPotPreflop(int potSize) {
    potPreflop = potSize;
    return potPreflop;
}

int Game::getPotPreflop() {
    return potPreflop;
}

int Game::setPotSize(int potSize) {
    pot = potSize;
    return pot;
}

int Game::getResearchPlayer() {
    return researchPlayer;
}

void Game::setResearchPlayer(int player) {
    researchPlayer = player;
}

int Game::getShuffle() {
    return isShuffle;
}

void Game::setShuffle(int flags) {
    isShuffle = flags;
}
