#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include "MagicBitboards.h"

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    Grid* _grid;
    int _currentPlayer;
    std::vector<BitMove> _moves;

    std::vector<BitMove> generateAllMoves();
    int negamax(std::string state, int maxDepth, int alpha, int beta, int player);

    void updateAI();

    // bitboards
    void setBitboards();
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

    // bitboard masks
    uint64_t rank2;
    uint64_t rank7;
    uint64_t notFileA;
    uint64_t notFileH;
};