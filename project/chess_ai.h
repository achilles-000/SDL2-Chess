#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <stdlib.h>
#include "chess_game.h"

// Move structure for AI
typedef struct {
    int fromRow, fromCol;
    int toRow, toCol;
    int score; // For move ordering
} AIMove;

// Move history for AI search (separate from game history)
typedef struct {
    int fromRow, fromCol, toRow, toCol;
    Piece capturedPiece;
    Piece movedPiece; // Store the piece that was moved
    int enPassantRow, enPassantCol;
    int wasEnPassantCapture; // Flag for en passant captures
    int enPassantCaptureRow; // Row where en passant pawn was
    
    // Castling rights before the move
    int whiteKingMoved;
    int whiteRookKingsideMoved;
    int whiteRookQueensideMoved;
    int blackKingMoved;
    int blackRookKingsideMoved;
    int blackRookQueensideMoved;
    
    int wasCastling; // Flag if this was a castling move
    int castlingRookFromCol, castlingRookToCol; // For undoing castling
} AIMoveHistory;

// Transposition table entry
typedef struct {
    unsigned long long hash;
    int depth;
    int score;
    int flag; // 0=exact, 1=alpha, 2=beta
} TTEntry;

#define TT_SIZE 524288  // 512K entries (about 12MB memory)
#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA 2

// AI Game structure (defined in chess_game.h as forward declaration)
struct ChessAI {
    ChessGame* game;
    AIDifficulty difficulty;
    int maxDepth;
    int nodesSearched; // For performance tracking
    AIMoveHistory searchHistory[100]; // Separate history for AI search
    int searchHistoryCount;
    TTEntry* transpositionTable; // Hash table for positions
    int stopSearch; // Flag to stop search early
    
    // Separate board state for AI search (doesn't affect visual board)
    Piece searchBoard[8][8];
    int searchEnPassantRow;
    int searchEnPassantCol;
    int searchWhiteKingMoved;
    int searchWhiteRookKingsideMoved;
    int searchWhiteRookQueensideMoved;
    int searchBlackKingMoved;
    int searchBlackRookKingsideMoved;
    int searchBlackRookQueensideMoved;
    PieceColor searchCurrentPlayer;
};

// Function declarations
ChessAI* createChessAI(ChessGame* game, AIDifficulty difficulty);
void destroyChessAI(ChessAI* ai);
void setAIDifficulty(ChessAI* ai, AIDifficulty difficulty);

// Main AI function - finds best move for given color
void findBestMove(ChessAI* ai, PieceColor color, int* bestFromRow, int* bestFromCol, int* bestToRow, int* bestToCol);

// Core minimax algorithm
int minimax(ChessAI* ai, int depth, int alpha, int beta, PieceColor maximizingPlayer);

// Position evaluation function
int evaluatePosition(ChessAI* ai, PieceColor color);

// Generate all possible moves for a color
int generateAllMoves(ChessAI* ai, PieceColor color, AIMove* moves);

// Order moves for better alpha-beta pruning
void orderMoves(ChessAI* ai, AIMove* moves, int count);

// Make/unmake move for search (without UI updates)
void makeMoveForAI(ChessAI* ai, int fromRow, int fromCol, int toRow, int toCol);
void unmakeMoveForAI(ChessAI* ai);

// Performance tracking
void resetNodeCount(ChessAI* ai);
int getNodesSearched(ChessAI* ai);

// Optimization functions
unsigned long long hashPosition(ChessAI* ai);
void storeTTEntry(ChessAI* ai, unsigned long long hash, int depth, int score, int flag);
TTEntry* probeTTEntry(ChessAI* ai, unsigned long long hash);
int quiescenceSearch(ChessAI* ai, int alpha, int beta, PieceColor maximizingPlayer);

#endif // CHESS_AI_H