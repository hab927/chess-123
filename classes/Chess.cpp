#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>

#define FLIP(i) (i ^ 56)

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

BitboardMap Chess::getBitboards(std::string s, int player) {

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

    // players: 1 is white, -1 is black

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
                if (player == 1 && isupper(c)) pawns |= bitMask;
                else if (player == -1 && islower(c)) pawns |= bitMask;
                break;
            case 'n':
            case 'N':
                if (player == 1 && isupper(c)) knights |= bitMask;
                else if (player == -1 && islower(c)) knights |= bitMask;
                break;
            case 'b':
            case 'B':
                if (player == 1 && isupper(c)) bishops |= bitMask;
                else if (player == -1 && islower(c)) bishops |= bitMask;
                break;
            case 'r':
            case 'R':
                if (player == 1 && isupper(c)) rooks |= bitMask;
                else if (player == -1 && islower(c)) rooks |= bitMask;
                break;
            case 'q':
            case 'Q':
                if (player == 1 && isupper(c)) queens |= bitMask;
                else if (player == -1 && islower(c)) queens |= bitMask;
                break;
            case 'k':
            case 'K':
                if (player == 1 && isupper(c)) kings |= bitMask;
                else if (player == -1 && islower(c)) kings |= bitMask;
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
    BitboardMap bbm = getBitboards(stateString(), (getCurrentPlayer()->playerNumber() == 0) ? 1 : -1);
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

    std::string testState = stateString();
    char srcPiece = testState[srcIndex];
    char dstPiece = testState[dstIndex];
    
    testState[dstIndex] = srcPiece;
    testState[srcIndex] = '0';
    
    int playerSign = (player == 0) ? 1 : -1;
    bool isLegal = !kingInCheck(testState, playerSign);

    if (!isLegal) {
        return false;
    }

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
            if ((offset == 9 || offset == 1 || offset == -7) && (srcIndex % 8 == 7)) continue; // skip right moves if on file H
            if ((offset == 7 || offset == -1 || offset == -9) && (srcIndex % 8 == 0)) continue; // skip left moves if on file A
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

            int file = srcIndex % 8;
            if (file == 7) {
                if (offset == 17 || offset == 10 || offset == -6 || offset == -15) continue; // skip moves that wrap right
            } else if (file == 0) {
                if (offset == 15 || offset == 6 || offset == -10 || offset == -17) continue; // skip moves that wrap left
            } else if (file == 6) {
                if (offset == -6 || offset == 10) continue; // skip moves that wrap right
            } else if (file == 1) {
                if (offset == -10 || offset == 6) continue; // skip moves that wrap left
            }
            
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

    BitboardMap bbm = getBitboards(state, player);

    // for pawns
    const int dir = (player == 1) ? 8 : -8;
    const uint64_t startRank = (player == 1) ? rank2 : rank7;
    Bitboard enemyPieces = (player == 1) ? bbm["black"] : bbm["white"];
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

        for (int i = 0; i < 8; i++) {
            if ((i == 2 || i == 4) && (fromSquare % 8 == 0)) continue;
            if ((i == 0 || i == 6) && (fromSquare % 8 == 7)) continue;
            int dest = fromSquare + kingOffsets[i];
            if (movableSquares.isOn(dest)) {
                moves.push_back({fromSquare, dest, King});
            }
        }
    });

    bbm["knights"].forEachBit([&](int fromSquare) {
        Bitboard movableSquares = enemyPieces.getData() | empty.getData();

        for (int i = 0; i < 8; i++) {
            int file = fromSquare % 8;
            if (file == 7) {
                if (i == 0 || i == 2 || i == 4 || i == 6) continue; // skip moves that wrap right
            } else if (file == 0) {
                if (i == 1 || i == 3 || i == 5 || i == 7) continue; // skip moves that wrap left
            } else if (file == 6) {
                if (i == 2 || i == 4) continue; // skip moves that wrap right
            } else if (file == 1) {
                if (i == 3 || i == 5) continue; // skip moves that wrap left
            }
            if (movableSquares.isOn(fromSquare + knightOffsets[i])) {
                moves.push_back({fromSquare, fromSquare + knightOffsets[i], Knight});
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

std::vector<BitMove> Chess::generateLegalMoves(std::string state, int player) {
    std::vector<BitMove> pseudoLegal = generateAllMoves(state, player);
    std::vector<BitMove> legal;
    legal.reserve(pseudoLegal.size());
    
    for (BitMove move : pseudoLegal) {
        std::string testState = state;
        testState[move.to] = testState[move.from];
        testState[move.from] = '0';
        
        if (!kingInCheck(testState, player)) {
            legal.push_back(move);
        }
    }
    
    return legal;
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
    setBitboards();
    _moves = generateLegalMoves(stateString(), getCurrentPlayer()->playerNumber() == 0 ? 1 : -1);
    if (_moves.size() == 0) {
        return getCurrentPlayer()->nextPlayer();
    }
    return nullptr;
}

bool Chess::checkForDraw()
{
    setBitboards();
    _moves = generateLegalMoves(stateString(), getCurrentPlayer()->playerNumber() == 0 ? 1 : -1);
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

    int maxDepth = 3;
    char baseState[65];
    int bestMoveScore = -100000000;
    BitMove bestMove;
    std::string currentState = stateString();

    int aiPlayerSign = (getCurrentPlayer()->playerNumber() == 0) ? 1 : -1;
    std::vector<BitMove> AIMoves = generateLegalMoves(currentState, aiPlayerSign);
    
    for (BitMove move : AIMoves) {
        std::string pieceString;
        switch (move.piece) {
            case Pawn: pieceString = "Pawn"; break;
            case Knight: pieceString = "Knight"; break;
            case Bishop: pieceString = "Bishop"; break;
            case Rook: pieceString = "Rook"; break;
            case Queen: pieceString = "Queen"; break;
            case King: pieceString = "King"; break;
            default: pieceString = "Unknown";
        }
        std::cout << "AI Move: " << std::to_string(move.from).c_str() << " to " << std::to_string(move.to).c_str() << " with " << pieceString << std::endl;
    }

    for (BitMove move : AIMoves) {
        strcpy(&baseState[0], currentState.c_str());
        char temp = baseState[move.to];
        baseState[move.to] = baseState[move.from];
        baseState[move.from] = '0';

        int score = -negamax(baseState, maxDepth, -100000000, 100000000, -aiPlayerSign);

        if (score > bestMoveScore) {
            bestMove = move;
            bestMoveScore = score;

            std::cout << bestMoveScore << std::endl;
        }
    }

    if (bestMoveScore != -10000000) {
        std::cout << "Best Move: " + std::to_string(bestMove.from) + " to " + std::to_string(bestMove.to) + " with piece " + std::to_string(bestMove.piece) + " with score " + std::to_string(bestMoveScore) << std::endl;
        BitHolder& src = getHolderAt(bestMove.from & 7, bestMove.from / 8);
        BitHolder& dst = getHolderAt(bestMove.to   & 7, bestMove.to   / 8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0,0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}

int Chess::negamax(std::string state, int depth, int alpha, int beta, int playerColor) {

    if (playerColor == 0) {
        std::puts("This is not supposed to happen.");
    }

    if (depth == 0) {
        int score = evaluate(state);
        return playerColor == 1 ? score : -score;
    }

    int bestVal = -10000000;
    char baseState[65];

    std::vector<BitMove> negatedMoves = generateLegalMoves(state, playerColor);

    std::sort(negatedMoves.begin(), negatedMoves.end(), [&state](const BitMove& a, const BitMove& b) {
        // incentivise capturing
        char targetA = state[a.to];
        char targetB = state[b.to];
        bool isCapureA = (targetA != '0');
        bool isCaptureB = (targetB != '0');
        
        if (isCapureA != isCaptureB) {
            return isCapureA; 
        }
        
        int pieceValueA = a.piece > 2 ? a.piece : 0;
        int pieceValueB = b.piece > 2 ? b.piece : 0;
        return pieceValueA > pieceValueB;
    });

    for (BitMove move : negatedMoves) {
        strcpy(&baseState[0], state.c_str());
        char temp = baseState[move.to];
        baseState[move.to] = baseState[move.from];
        baseState[move.from] = '0';

        bestVal = std::max(bestVal, -negamax(baseState, depth-1, -beta, -alpha, -playerColor));

        baseState[move.from] = baseState[move.to];
        baseState[move.to] = temp;

        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }
    return bestVal;
}

bool Chess::kingInCheck(const std::string &state, int player) {
    // 1 = white, -1 = black
    BitboardMap bbm = getBitboards(state, player);
    Bitboard playerKing = bbm["kings"].getData() & bbm[(player == 1) ? "white" : "black"].getData();
    int kingIndex = playerKing.getIndexOfLSB();

    std::vector<BitMove> enemyMoves = generateAllMoves(state, -player);
    
    for (BitMove move : enemyMoves) {
        if (move.to == kingIndex) {
            return true;
        }
    }
    return false;
}

int Chess::evaluate(const std::string &state) {
    // material values for each piece type (uppercase assumed white)
    int materialValue[128] = {0};
    materialValue['P'] = 100;
    materialValue['N'] = 320;
    materialValue['B'] = 330;
    materialValue['R'] = 500;
    materialValue['Q'] = 900;
    materialValue['K'] = 20000;

    const int pawnTable[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5, 5, 10, 25, 25, 10, 5, 5,
        0, 0, 0, 20, 20, 0, 0, 0,
        5, -5, -10, 0, 0, -10, -5, 5,
        5, 10, 10, -20, -20, 10, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0};
    const int knightTable[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20, 0, 0, 0, 0, -20, -40,
        -30, 0, 10, 15, 15, 10, 0, -30,
        -30, 5, 15, 20, 20, 15, 5, -30,
        -30, 0, 15, 20, 20, 15, 0, -30,
        -30, 5, 10, 15, 15, 10, 5, -30,
        0, 5, 10, 10, 10, 10, 5, 0,
        -50, -40, -30, -30, -30, -30, -40, -50};
    const int rookTable[64] = {
        0, 0, 0, 5, 5, 0, 0, 0,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        5, 10, 10, 10, 10, 10, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0};
    const int queenTable[64] = {
        -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20};
    const int kingTable[64] = {
        20, 30, 10, 0, 0, 10, 30, 20,
        20, 20, 0, 0, 0, 0, 20, 20,
        -10, -20, -20, -20, -20, -20, -20, -10,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30};
    const int bishopTable[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 10, 10, 5, 0, -10,
        -10, 5, 5, 10, 10, 5, 5, -10,
        -10, 0, 10, 10, 10, 10, 0, -10,
        -10, 10, 10, 10, 10, 10, 10, -10,
        -10, 5, 0, 0, 0, 0, 5, -10,
        -20, -10, -10, -10, -10, -10, -10, -20};

    int score = 0;

    for (int i=0; i<64; i++) {
        char piece = state[i];
        int j = FLIP(i);
        switch (piece) {
            case 'P':
                score += materialValue['P'] + pawnTable[j];
                break;
            case 'p':
                score -= materialValue['P'] + pawnTable[FLIP(j)];
                break;
            case 'N':
                score += materialValue['N'] + knightTable[j];
                break;
            case 'n':
                score -= materialValue['N'] + knightTable[FLIP(j)];
                break;
            case 'B':
                score += materialValue['B'] + bishopTable[j];
                break;
            case 'b':
                score -= materialValue['B'] + bishopTable[FLIP(j)];
                break;
            case 'R':
                score += materialValue['R'] + rookTable[j];
                break;
            case 'r':
                score -= materialValue['R'] + rookTable[FLIP(j)];
                break;
            case 'Q':
                score += materialValue['Q'] + queenTable[j];
                break;
            case 'q':
                score -= materialValue['Q'] + queenTable[FLIP(j)];
                break;
            case 'K':
                score += materialValue['K'] + kingTable[j];
                break;
            case 'k':
                score -= materialValue['K'] + kingTable[FLIP(j)];
                break;
        }
    }
    return score;
}