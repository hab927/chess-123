#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    // make bitmasks for the pawns
    rank2       = 0x000000000000FF00ULL;
    rank7       = 0x00FF000000000000ULL;
    notFileA    = 0xFEFEFEFEFEFEFEFEULL;
    notFileH    = 0x7F7F7F7F7F7F7F7FULL;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    initMagicBitboards();

    setBitboards();
    _moves = generateAllMoves(stateString(), -1);

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    int index = 0;

    for (char c : fen) {
        if (c == '/') {
            continue;
        }
        if (std::isdigit(c)) { // skip ahead
            index += c - '0';
            continue;
        }
        int rank = 7 - (index / 8);
        int file = index % 8;

        Bit* b;
        switch (c) {
            // lowercase = black pieces, 0pnbrqk
            case ('p'):
                b = PieceForPlayer(1, Pawn);
                b->setGameTag(129);
                break;
            case ('n'):
                b = PieceForPlayer(1, Knight);
                b->setGameTag(130);
                break;
            case ('b'):
                b = PieceForPlayer(1, Bishop);
                b->setGameTag(131);
                break;
            case ('r'):
                b = PieceForPlayer(1, Rook);
                b->setGameTag(132);
                break;
            case ('q'):
                b = PieceForPlayer(1, Queen);
                b->setGameTag(133);
                break;
            case ('k'):
                b = PieceForPlayer(1, King);
                b->setGameTag(134);
                break;
            // upperacse = player 0, white pieces
            case ('P'):
                b = PieceForPlayer(0, Pawn);
                b->setGameTag(1);
                break;
            case ('N'):
                b = PieceForPlayer(0, Knight);
                b->setGameTag(2);
                break;
            case ('B'):
                b = PieceForPlayer(0, Bishop);
                b->setGameTag(3);
                break;
            case ('R'):
                b = PieceForPlayer(0, Rook);
                b->setGameTag(4);
                break;
            case ('Q'):
                b = PieceForPlayer(0, Queen);
                b->setGameTag(5);
                break;
            case ('K'):
                b = PieceForPlayer(0, King);
                b->setGameTag(6);
                break;
            default: 
                std::cout << "mystery piece" << std::endl;
        }
        b->setPosition(_grid->getSquare(file,rank)->getPosition());
        _grid->getSquare(file,rank)->setBit(b);
        index++;
    }
}

BitboardMap Chess::getBitboards(std::string s) {

    std::map<std::string, Bitboard> bitboardMap;

    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;
    Bitboard blackPieces;
    Bitboard whitePieces;
    Bitboard occupiedSquares;
    Bitboard emptySquares;

    int count = 0; // debugging variable
    uint64_t bitMask = (uint64_t)1; // move this through while we iterate
    for (char c : s) {
        count++;
        if (c == '0') { 
            emptySquares |= bitMask;
            bitMask <<= 1;
            continue;
        }
        switch(c) {
            // lowercase = black pieces, 0pnbrqk
            case 'p':
            case 'P':
                pawns |= bitMask;
                break;
            case 'n':
            case 'N':
                knights |= bitMask;
                break;
            case 'b':
            case 'B':
                bishops |= bitMask;
                break;
            case 'r':
            case 'R':
                rooks |= bitMask;
                break;
            case 'q':
            case 'Q':
                queens |= bitMask;
                break;
            case 'k':
            case 'K':
                kings |= bitMask;
                break;
            default: 
                std::cout << "bitboard set fail at index " << count << std::endl;
        }
        islower(c) ? blackPieces |= bitMask : whitePieces |= bitMask;
        occupiedSquares |= bitMask;
        bitMask <<= 1;
    }

    // assign each one inside of the map
    // it's a bit clunky, but it should work!
    bitboardMap["pawns"] = pawns;
    bitboardMap["knights"] = knights;
    bitboardMap["rooks"] = rooks;
    bitboardMap["queens"] = queens;
    bitboardMap["kings"] = kings;
    bitboardMap["black"] = blackPieces;
    bitboardMap["white"] = whitePieces;
    bitboardMap["occupied"] = occupiedSquares;
    bitboardMap["empty"] = emptySquares;

    return bitboardMap;
}

void Chess::setBitboards() {  // set these inside of the internal chess class
    BitboardMap bbm = getBitboards(stateString());
    pawns = bbm["pawns"];
    knights = bbm["knights"];
    rooks = bbm["rooks"];
    queens = bbm["queens"];
    kings = bbm["kings"];
    blackPieces = bbm["black"];
    whitePieces = bbm["white"];
    occupiedSquares = bbm["occupied"];
    emptySquares = bbm["empty"];
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);

    int piece = bit.gameTag();
    if (piece > 128) {
        piece -= 128;
    }

    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();

    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();

    int srcIndex = srcX + srcY*8;
    int dstIndex = dstX + dstY*8;

    int player = getCurrentPlayer()->playerNumber();

    // pawn move generation
    if (piece == Pawn) {
        const int dir = (player == 0) ? 8 : -8;
        const uint64_t startRank = (player == 0) ? rank2 : rank7;
        Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;

        if (dstIndex == srcIndex + dir) {
            return emptySquares.isOn(dstIndex); 
        }

        if (dstIndex == srcIndex + 2 * dir) {
            bool onHomeRank = ((1ULL << srcIndex) & startRank);
            bool pathClear = emptySquares.isOn(srcIndex + dir) && 
                            emptySquares.isOn(dstIndex);
            return onHomeRank && pathClear;
        }

        if (dstIndex == srcIndex - 1 + dir && (uint64_t)1 << srcIndex & notFileA) { // capture on left
            return enemyPieces.isOn(dstIndex);
        }

        if (dstIndex == srcIndex + 1 + dir && (uint64_t)1 << srcIndex & notFileH) { // capture on right
            return enemyPieces.isOn(dstIndex);
        }
    }

    // king move generation
    if (piece == King) {
        Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;
        int offsets[8] = {9, 8, 7, 1, -1, -7, -8, -9};

        for (int offset : offsets) {
            if (dstIndex == srcIndex + offset) {
                return enemyPieces.isOn(dstIndex) || emptySquares.isOn(dstIndex);
            }
        }
    }

    // knight move generation
    if (piece == Knight) {
        Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;
        int offsets[8] = {17, 15, 10, 6, -6, -10, -15, -17};

        for (int offset : offsets) {
            if (dstIndex == srcIndex + offset) {
                return enemyPieces.isOn(dstIndex) || emptySquares.isOn(dstIndex);
            }
        }
    }

    // rook move generation
    if (piece == Rook) {
        uint64_t attacks = getRookAttacks(srcIndex, occupiedSquares.getData());

        if ((attacks >> dstIndex) & 1ULL) {
            Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;
            return enemyPieces.isOn(dstIndex) || emptySquares.isOn(dstIndex);
        }
    }

    // bishop move generation
    if (piece == Bishop) {
        uint64_t attacks = getBishopAttacks(srcIndex, occupiedSquares.getData());

        if ((attacks >> dstIndex) & 1ULL) {
            Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;
            return enemyPieces.isOn(dstIndex) || emptySquares.isOn(dstIndex);
        }
    }

    // queen move generation
    if (piece == Queen) {
        uint64_t attacks = getQueenAttacks(srcIndex, occupiedSquares.getData());

        if ((attacks >> dstIndex) & 1ULL) {
            Bitboard enemyPieces = (player == 0) ? blackPieces : whitePieces;
            return enemyPieces.isOn(dstIndex) || emptySquares.isOn(dstIndex);
        }
    }
    
    return false;
}

std::vector<BitMove> Chess::generateAllMoves(std::string state, int player) {

    std::vector<BitMove> moves;
    moves.reserve(32);

    BitboardMap bbm = getBitboards(state);

    // for pawns
    const int dir = (player == 0) ? 8 : -8;
    const uint64_t startRank = (player == 0) ? rank2 : rank7;
    Bitboard enemyPieces = (player == 0) ? bbm["black"] : bbm["white"];
    Bitboard empty = bbm["empty"];
    Bitboard occupied = bbm["occupied"];

    // offsets
    int kingOffsets[8] = {9, 8, 7, 1, -1, -7, -8, -9};
    int knightOffsets[8] = {17, 15, 10, 6, -6, -10, -15, -17};

    bbm["pawns"].forEachBit([&](int fromSquare) {
        uint64_t fromBB = (1ULL << fromSquare);

        int singlePush = fromSquare + dir;
        if (singlePush >= 0 && singlePush < 64 && empty.isOn(singlePush)) {
            moves.push_back({fromSquare, singlePush, Pawn});
            int doublePush = fromSquare + 2 * dir;
            if ((fromBB & startRank) && empty.isOn(doublePush)) {
                moves.push_back({fromSquare, doublePush, Pawn});
            }
        }
        if (fromBB & notFileA) {
            int capLeft = fromSquare + dir - 1;
            if (capLeft >= 0 && capLeft < 64 && enemyPieces.isOn(capLeft)) {
                moves.push_back({fromSquare, capLeft, Pawn});
            }
        }
        if (fromBB & notFileH) {
            int capRight = fromSquare + dir + 1;
            if (capRight >= 0 && capRight < 64 && enemyPieces.isOn(capRight)) {
                moves.push_back({fromSquare, capRight, Pawn});
            }
        }
    });

    bbm["kings"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();

        for (int offset : kingOffsets) {
            if (movableSquares.isOn(fromSquare + offset)) {
                moves.push_back({fromSquare, fromSquare + offset, King});
            }
        }
    });

    bbm["knights"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();

        for (int offset : knightOffsets) {
            if (movableSquares.isOn(fromSquare + offset)) {
                moves.push_back({fromSquare, fromSquare + offset, Knight});
            }
        }
    });

    bbm["rooks"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();
        Bitboard attacks = getRookAttacks(fromSquare, occupied.getData());
        Bitboard rookMoves = Bitboard(movableSquares.getData() & attacks.getData());

        rookMoves.forEachBit([&](int destSquare) {
            if (movableSquares.isOn(destSquare)) {
                moves.push_back({fromSquare, destSquare, Rook});
            }
        });
    });

    bbm["bishops"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();
        Bitboard attacks = getBishopAttacks(fromSquare, occupied.getData());
        Bitboard bishopMoves = Bitboard(movableSquares.getData() & attacks.getData());

        bishopMoves.forEachBit([&](int destSquare) {
            if (movableSquares.isOn(destSquare)) {
                moves.push_back({fromSquare, destSquare, Bishop});
            }
        });
    });

    bbm["queens"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();
        Bitboard attacks = getQueenAttacks(fromSquare, occupied.getData());
        Bitboard queenMoves = Bitboard(movableSquares.getData() & attacks.getData());

        queenMoves.forEachBit([&](int destSquare) {
            if (movableSquares.isOn(destSquare)) {
                moves.push_back({fromSquare, destSquare, Queen});
            }
        });
    });

    return moves;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    cleanupMagicBitboards();
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    setBitboards();
    _moves = generateAllMoves(stateString(), getCurrentPlayer()->playerNumber());
    whitePieces.printBitboard();
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::updateAI() {

    int maxDepth = 2;
    char baseState[65];
    int bestMoveScore = -10000000;
    BitMove bestMove;
    std::string currentState = stateString();

    for (BitMove move : _moves) {
        strcpy(&baseState[0], currentState.c_str());
        char temp = baseState[move.to];
        baseState[move.to] = baseState[move.from];
        baseState[move.from] = '0';

        int score = -negamax(baseState, maxDepth, -10000000, 10000000, 1);

        if (score > bestMoveScore) {
            bestMove = move;
            bestMoveScore = score;
        }
    }

    if (bestMoveScore != -10000000) {
        BitHolder& src = getHolderAt(bestMove.from & 7, bestMove.from / 8);
        BitHolder& dst = getHolderAt(bestMove.to   & 7, bestMove.to   / 8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0,0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}

int Chess::evaluate(std::string state) {
    int score = 0;

    std::map<char, int> pieceValues = {
        {'P', 100}, {'N', 320}, {'B', 330}, {'R', 500}, {'Q', 900}, {'K', 20000},
        {'p', -100}, {'n', -320}, {'b', -330}, {'r', -500}, {'q', -900}, {'k', -20000}
    };
    
    for (char c : state) {
        if (pieceValues.find(c) != pieceValues.end()) {
            score += pieceValues[c];
        }
    }

    return score;
}

int Chess::negamax(std::string state, int depth, int alpha, int beta, int playerColor) {

    if (playerColor == 0) {
        std::puts("This is not supposed to happen.");
    }

    if (depth == 0) {
        int score = evaluate(state);
        return playerColor * score;
    }

    int bestVal = -10000000;
    char baseState[65];

    std::vector<BitMove> negatedMoves = generateAllMoves(state, playerColor);

    for (BitMove move : negatedMoves) {
        strcpy(&baseState[0], state.c_str());
        char temp = state[move.to];
        baseState[move.to] = baseState[move.from];
        baseState[move.from] = '0';

        int score = -negamax(baseState, depth-1, -10000000, 10000000, 1);

        state[move.from] = state[move.to];
        state[move.to] = temp;

        alpha == std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }
    return bestVal;
}