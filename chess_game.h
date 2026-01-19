#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_thread.h>

// Basic chess types
typedef enum {
    PIECE_NONE = 0,
    PIECE_PAWN,
    PIECE_ROOK,
    PIECE_KNIGHT,
    PIECE_BISHOP,
    PIECE_QUEEN,
    PIECE_KING
} PieceType;

typedef enum {
    COLOR_NONE = 0,
    COLOR_WHITE,
    COLOR_BLACK
} PieceColor;

typedef enum {
    GAME_MODE_HUMAN_VS_HUMAN = 0,
    GAME_MODE_HUMAN_VS_AI = 1
} GameMode;

// Forward declarations for AI
struct ChessAI;
typedef struct ChessAI ChessAI;

typedef enum {
    DIFFICULTY_EASY = 1,
    DIFFICULTY_MEDIUM = 2,
    DIFFICULTY_HARD = 3,
    DIFFICULTY_EXPERT = 4
} AIDifficulty;

// Chess piece structure
typedef struct {
    PieceType type;
    PieceColor color;
} Piece;

// Move history for detecting repetition
typedef struct {
    Piece board[8][8];
    int enPassantRow;
    int enPassantCol;
} BoardState;

// Animation structure
typedef struct {
    int active;
    int fromRow, fromCol;
    int toRow, toCol;
    float progress; // 0.0 to 1.0
    Uint32 startTime;
    int duration; // milliseconds
    Piece movingPiece;
} MoveAnimation;

// Promotion dialog structure
typedef struct {
    int active;
    int row, col;
    PieceColor color;
} PromotionDialog;

// Main game structure
typedef struct ChessGame {
    Piece board[8][8];
    PieceColor currentPlayer;
    int selectedRow;
    int selectedCol;
    int possibleMoves[64][2];
    int possibleMovesCount;
    int gameOver;
    int enPassantRow;
    int enPassantCol;
    int capturedWhite[16];
    int capturedBlack[16];
    GameMode gameMode;
    ChessAI* ai;
    AIDifficulty aiDifficulty;
    int aiThinking;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* pieceTextures[2][7];
    
    // Castling rights
    int whiteKingMoved;
    int whiteRookKingsideMoved;
    int whiteRookQueensideMoved;
    int blackKingMoved;
    int blackRookKingsideMoved;
    int blackRookQueensideMoved;
    
    // Move history for repetition detection
    BoardState moveHistory[200];
    int moveHistoryCount;
    int halfMoveClock; // For 50-move rule
    
    // Animation
    MoveAnimation animation;
    
    // Promotion dialog
    PromotionDialog promotionDialog;
    
    // AI threading
    SDL_Thread* aiThread;
    SDL_mutex* aiMutex;
    int aiMoveReady;
    int aiBestFromRow, aiBestFromCol, aiBestToRow, aiBestToCol;
} ChessGame;

// Function declarations
void initBoard(ChessGame* game);
int isValidSquare(int row, int col);
void getPossibleMoves(ChessGame* game, int row, int col, int moves[64][2], int* count);
int isPossibleMove(int moves[64][2], int count, int row, int col);
void makeMove(ChessGame* game, int fromRow, int fromCol, int toRow, int toCol);
void makeMoveAnimated(ChessGame* game, int fromRow, int fromCol, int toRow, int toCol);
int checkGameEnd(ChessGame* game, PieceColor color);
int isInCheck(ChessGame* game, PieceColor color);
int checkThreefoldRepetition(ChessGame* game);
void saveBoardState(ChessGame* game);
int compareBoardStates(BoardState* state1, BoardState* state2);
int canCastle(ChessGame* game, PieceColor color, int kingSide);
void addCastlingMoves(ChessGame* game, int row, int col, PieceColor color, int moves[64][2], int* count);

#endif // CHESS_GAME_H