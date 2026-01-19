#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "chess_game.h"
#include "chess_ai.h"

#define BOARD_SIZE 8
#define SQUARE_SIZE 80
#define WINDOW_WIDTH (BOARD_SIZE * SQUARE_SIZE + 350)
#define WINDOW_HEIGHT (BOARD_SIZE * SQUARE_SIZE + 120)

// Animation speed: Lower = faster, Higher = slower
// 150 = very fast, 300 = normal (default), 500 = slow, 1000 = very slow
#define MOVE_ANIMATION_DURATION 300 // milliseconds

// Unicode chess piece symbols
static const char* pieceSymbols[2][7] = {
    // White pieces
    {"", "♙", "♖", "♘", "♗", "♕", "♔"},
    // Black pieces
    {"", "♟", "♜", "♞", "♝", "♛", "♚"}
};

// Initialize the chess board
void initBoard(ChessGame* game) {
    // Clear the board
    memset(game->board, 0, sizeof(game->board));

    // Initialize captured pieces counters
    memset(game->capturedWhite, 0, sizeof(game->capturedWhite));
    memset(game->capturedBlack, 0, sizeof(game->capturedBlack));

    // Place black pieces (top)
    PieceType blackOrder[] = {PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
                                PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK};
    for (int col = 0; col < 8; col++) {
        game->board[0][col] = (Piece){blackOrder[col], COLOR_BLACK};
        game->board[1][col] = (Piece){PIECE_PAWN, COLOR_BLACK};
    }

    // Place white pieces (bottom)
    PieceType whiteOrder[] = {PIECE_ROOK, PIECE_KNIGHT, PIECE_BISHOP, PIECE_QUEEN,
                               PIECE_KING, PIECE_BISHOP, PIECE_KNIGHT, PIECE_ROOK};
    for (int col = 0; col < 8; col++) {
        game->board[7][col] = (Piece){whiteOrder[col], COLOR_WHITE};
        game->board[6][col] = (Piece){PIECE_PAWN, COLOR_WHITE};
    }

    game->enPassantRow = -1;
    game->enPassantCol = -1;
    game->moveHistoryCount = 0;
    game->halfMoveClock = 0;
    
    // Initialize castling rights - no pieces have moved yet
    game->whiteKingMoved = 0;
    game->whiteRookKingsideMoved = 0;
    game->whiteRookQueensideMoved = 0;
    game->blackKingMoved = 0;
    game->blackRookKingsideMoved = 0;
    game->blackRookQueensideMoved = 0;
    
    // Save initial board state
    saveBoardState(game);
}

// Check if coordinates are valid
int isValidSquare(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// Save current board state for repetition detection
void saveBoardState(ChessGame* game) {
    if (game->moveHistoryCount >= 200) {
        // Shift history if full
        for (int i = 0; i < 199; i++) {
            game->moveHistory[i] = game->moveHistory[i + 1];
        }
        game->moveHistoryCount = 199;
    }
    
    BoardState* state = &game->moveHistory[game->moveHistoryCount++];
    memcpy(state->board, game->board, sizeof(game->board));
    state->enPassantRow = game->enPassantRow;
    state->enPassantCol = game->enPassantCol;
}

// Compare two board states
int compareBoardStates(BoardState* state1, BoardState* state2) {
    if (state1->enPassantRow != state2->enPassantRow || 
        state1->enPassantCol != state2->enPassantCol) {
        return 0;
    }
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (state1->board[row][col].type != state2->board[row][col].type ||
                state1->board[row][col].color != state2->board[row][col].color) {
                return 0;
            }
        }
    }
    return 1;
}

// Check for threefold repetition
int checkThreefoldRepetition(ChessGame* game) {
    if (game->moveHistoryCount < 5) return 0;
    
    BoardState* currentState = &game->moveHistory[game->moveHistoryCount - 1];
    int repetitionCount = 1;
    
    // Check previous positions (only check every other move since we need same player)
    for (int i = game->moveHistoryCount - 3; i >= 0; i -= 2) {
        if (compareBoardStates(currentState, &game->moveHistory[i])) {
            repetitionCount++;
            if (repetitionCount >= 3) {
                return 1;
            }
        }
    }
    
    return 0;
}

// Check if a square is attacked by opponent (without recursion)
int isSquareAttacked(ChessGame* game, int targetRow, int targetCol, PieceColor attackerColor) {
    int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = game->board[row][col];
            if (piece.type == PIECE_NONE || piece.color != attackerColor) continue;
            
            // Check pawn attacks
            if (piece.type == PIECE_PAWN) {
                int direction = (piece.color == COLOR_WHITE) ? -1 : 1;
                for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
                    if (row + direction == targetRow && col + colOffset == targetCol) {
                        return 1;
                    }
                }
            }
            // Check knight attacks
            else if (piece.type == PIECE_KNIGHT) {
                for (int i = 0; i < 8; i++) {
                    if (row + knightMoves[i][0] == targetRow && 
                        col + knightMoves[i][1] == targetCol) {
                        return 1;
                    }
                }
            }
            // Check king attacks
            else if (piece.type == PIECE_KING) {
                for (int i = 0; i < 8; i++) {
                    if (row + directions[i][0] == targetRow && 
                        col + directions[i][1] == targetCol) {
                        return 1;
                    }
                }
            }
            // Check sliding pieces (rook, bishop, queen)
            else {
                int isRook = (piece.type == PIECE_ROOK);
                int isBishop = (piece.type == PIECE_BISHOP);
                
                for (int d = 0; d < 8; d++) {
                    if (isRook && (d == 0 || d == 2 || d == 5 || d == 7)) continue;
                    if (isBishop && (d == 1 || d == 3 || d == 4 || d == 6)) continue;
                    
                    for (int i = 1; i < 8; i++) {
                        int newRow = row + directions[d][0] * i;
                        int newCol = col + directions[d][1] * i;
                        
                        if (!isValidSquare(newRow, newCol)) break;
                        if (newRow == targetRow && newCol == targetCol) return 1;
                        if (game->board[newRow][newCol].type != PIECE_NONE) break;
                    }
                }
            }
        }
    }
    return 0;
}

// Check if castling is possible
int canCastle(ChessGame* game, PieceColor color, int kingSide) {
    int kingRow = (color == COLOR_WHITE) ? 7 : 0;
    int rookCol = kingSide ? 7 : 0;
    
    // Check if king or rook has moved
    if (color == COLOR_WHITE) {
        if (game->whiteKingMoved) return 0;
        if (kingSide && game->whiteRookKingsideMoved) return 0;
        if (!kingSide && game->whiteRookQueensideMoved) return 0;
    } else {
        if (game->blackKingMoved) return 0;
        if (kingSide && game->blackRookKingsideMoved) return 0;
        if (!kingSide && game->blackRookQueensideMoved) return 0;
    }
    
    // Check if king is in check
    if (isInCheck(game, color)) return 0;
    
    // Check if pieces are in the way and squares are not under attack
    PieceColor opponent = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    
    if (kingSide) {
        // Kingside castling (O-O)
        // Check f and g files are empty
        if (game->board[kingRow][5].type != PIECE_NONE || 
            game->board[kingRow][6].type != PIECE_NONE) {
            return 0;
        }
        // Check king doesn't move through or into check
        if (isSquareAttacked(game, kingRow, 5, opponent) ||
            isSquareAttacked(game, kingRow, 6, opponent)) {
            return 0;
        }
    } else {
        // Queenside castling (O-O-O)
        // Check b, c, and d files are empty
        if (game->board[kingRow][1].type != PIECE_NONE || 
            game->board[kingRow][2].type != PIECE_NONE ||
            game->board[kingRow][3].type != PIECE_NONE) {
            return 0;
        }
        // Check king doesn't move through or into check
        if (isSquareAttacked(game, kingRow, 2, opponent) ||
            isSquareAttacked(game, kingRow, 3, opponent)) {
            return 0;
        }
    }
    
    // Verify rook is actually there
    if (game->board[kingRow][rookCol].type != PIECE_ROOK ||
        game->board[kingRow][rookCol].color != color) {
        return 0;
    }
    
    return 1;
}

// Add castling moves to possible moves
void addCastlingMoves(ChessGame* game, int row, int col, PieceColor color, int moves[64][2], int* count) {
    // Only check castling if piece is a king at starting position
    int startRow = (color == COLOR_WHITE) ? 7 : 0;
    if (row != startRow || col != 4) return;
    
    // Kingside castling
    if (canCastle(game, color, 1)) {
        moves[*count][0] = row;
        moves[*count][1] = 6; // King moves to g-file
        (*count)++;
    }
    
    // Queenside castling
    if (canCastle(game, color, 0)) {
        moves[*count][0] = row;
        moves[*count][1] = 2; // King moves to c-file
        (*count)++;
    }
}

// Check if a move would put own king in check (used for validation)
int wouldBeInCheck(ChessGame* game, int fromRow, int fromCol, int toRow, int toCol, PieceColor color) {
    // Make a temporary move
    Piece originalPiece = game->board[toRow][toCol];
    Piece movingPiece = game->board[fromRow][fromCol];
    game->board[toRow][toCol] = movingPiece;
    game->board[fromRow][fromCol] = (Piece){PIECE_NONE, COLOR_NONE};
    
    // Handle en passant capture temporarily
    Piece enPassantPawn = {PIECE_NONE, COLOR_NONE};
    int enPassantCaptureRow = -1;
    if (movingPiece.type == PIECE_PAWN && toRow == game->enPassantRow && toCol == game->enPassantCol) {
        enPassantCaptureRow = (movingPiece.color == COLOR_WHITE) ? toRow + 1 : toRow - 1;
        enPassantPawn = game->board[enPassantCaptureRow][toCol];
        game->board[enPassantCaptureRow][toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    }
    
    // Find the king (might have moved)
    int kingRow = -1, kingCol = -1;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = game->board[row][col];
            if (piece.type == PIECE_KING && piece.color == color) {
                kingRow = row;
                kingCol = col;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    int inCheck = 0;
    if (kingRow != -1) {
        PieceColor opponent = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        inCheck = isSquareAttacked(game, kingRow, kingCol, opponent);
    }
    
    // Undo the temporary move
    game->board[fromRow][fromCol] = movingPiece;
    game->board[toRow][toCol] = originalPiece;
    if (enPassantCaptureRow != -1) {
        game->board[enPassantCaptureRow][toCol] = enPassantPawn;
    }
    
    return inCheck;
}

// Get pawn moves including en passant
void getPawnMoves(ChessGame* game, int row, int col, PieceColor color, int moves[64][2], int* count) {
    int direction = (color == COLOR_WHITE) ? -1 : 1;
    int startRow = (color == COLOR_WHITE) ? 6 : 1;
    
    // Move forward one square
    if (isValidSquare(row + direction, col) && game->board[row + direction][col].type == PIECE_NONE) {
        if (!wouldBeInCheck(game, row, col, row + direction, col, color)) {
            moves[*count][0] = row + direction;
            moves[*count][1] = col;
            (*count)++;
        }
        
        // Move forward two squares from starting position
        if (row == startRow && game->board[row + 2 * direction][col].type == PIECE_NONE) {
            if (!wouldBeInCheck(game, row, col, row + 2 * direction, col, color)) {
                moves[*count][0] = row + 2 * direction;
                moves[*count][1] = col;
                (*count)++;
            }
        }
    }
    
    // Capture diagonally
    for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
        int newCol = col + colOffset;
        if (isValidSquare(row + direction, newCol)) {
            Piece target = game->board[row + direction][newCol];
            if (target.type != PIECE_NONE && target.color != color) {
                if (!wouldBeInCheck(game, row, col, row + direction, newCol, color)) {
                    moves[*count][0] = row + direction;
                    moves[*count][1] = newCol;
                    (*count)++;
                }
            }
        }
    }
    
    // En passant
    if (game->enPassantRow != -1 && game->enPassantCol != -1) {
        if (row + direction == game->enPassantRow && 
            (col + 1 == game->enPassantCol || col - 1 == game->enPassantCol)) {
            if (!wouldBeInCheck(game, row, col, game->enPassantRow, game->enPassantCol, color)) {
                moves[*count][0] = game->enPassantRow;
                moves[*count][1] = game->enPassantCol;
                (*count)++;
            }
        }
    }
}

// Get sliding piece moves (rook, bishop, queen)
void getSlidingMoves(ChessGame* game, int row, int col, PieceColor color, 
                     int directions[][2], int dirCount, int moves[64][2], int* count) {
    for (int d = 0; d < dirCount; d++) {
        int dr = directions[d][0];
        int dc = directions[d][1];
        
        for (int i = 1; i < 8; i++) {
            int newRow = row + dr * i;
            int newCol = col + dc * i;
            
            if (!isValidSquare(newRow, newCol)) break;
            
            Piece target = game->board[newRow][newCol];
            if (target.type == PIECE_NONE) {
                if (!wouldBeInCheck(game, row, col, newRow, newCol, color)) {
                    moves[*count][0] = newRow;
                    moves[*count][1] = newCol;
                    (*count)++;
                }
            } else {
                if (target.color != color) {
                    if (!wouldBeInCheck(game, row, col, newRow, newCol, color)) {
                        moves[*count][0] = newRow;
                        moves[*count][1] = newCol;
                        (*count)++;
                    }
                }
                break;
            }
        }
    }
}

// Get knight moves
void getKnightMoves(ChessGame* game, int row, int col, PieceColor color, int moves[64][2], int* count) {
    int knightMoves[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, 
                             {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + knightMoves[i][0];
        int newCol = col + knightMoves[i][1];
        
        if (!isValidSquare(newRow, newCol)) continue;
        
        Piece target = game->board[newRow][newCol];
        if (target.type == PIECE_NONE || target.color != color) {
            if (!wouldBeInCheck(game, row, col, newRow, newCol, color)) {
                moves[*count][0] = newRow;
                moves[*count][1] = newCol;
                (*count)++;
            }
        }
    }
}

// Get king moves
void getKingMoves(ChessGame* game, int row, int col, PieceColor color, int moves[64][2], int* count) {
    int kingMoves[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, 
                           {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    
    for (int i = 0; i < 8; i++) {
        int newRow = row + kingMoves[i][0];
        int newCol = col + kingMoves[i][1];
        
        if (!isValidSquare(newRow, newCol)) continue;
        
        Piece target = game->board[newRow][newCol];
        if (target.type == PIECE_NONE || target.color != color) {
            if (!wouldBeInCheck(game, row, col, newRow, newCol, color)) {
                moves[*count][0] = newRow;
                moves[*count][1] = newCol;
                (*count)++;
            }
        }
    }
    
    // Add castling moves
    addCastlingMoves(game, row, col, color, moves, count);
}

// Get all possible moves for a piece
void getPossibleMoves(ChessGame* game, int row, int col, int moves[64][2], int* count) {
    *count = 0;
    Piece piece = game->board[row][col];
    
    if (piece.type == PIECE_NONE) return;
    
    int rookDirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int bishopDirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    int queenDirs[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, 
                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    
    switch (piece.type) {
        case PIECE_PAWN:
            getPawnMoves(game, row, col, piece.color, moves, count);
            break;
        case PIECE_ROOK:
            getSlidingMoves(game, row, col, piece.color, rookDirs, 4, moves, count);
            break;
        case PIECE_BISHOP:
            getSlidingMoves(game, row, col, piece.color, bishopDirs, 4, moves, count);
            break;
        case PIECE_QUEEN:
            getSlidingMoves(game, row, col, piece.color, queenDirs, 8, moves, count);
            break;
        case PIECE_KNIGHT:
            getKnightMoves(game, row, col, piece.color, moves, count);
            break;
        case PIECE_KING:
            getKingMoves(game, row, col, piece.color, moves, count);
            break;
        default:
            break;
    }
}

// Check if a move is in the possible moves list
int isPossibleMove(int moves[64][2], int count, int row, int col) {
    for (int i = 0; i < count; i++) {
        if (moves[i][0] == row && moves[i][1] == col) {
            return 1;
        }
    }
    return 0;
}

// Check if a color is in check
int isInCheck(ChessGame* game, PieceColor color) {
    // Find the king
    int kingRow = -1, kingCol = -1;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = game->board[row][col];
            if (piece.type == PIECE_KING && piece.color == color) {
                kingRow = row;
                kingCol = col;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return 0;
    
    // Check if king's square is attacked
    PieceColor opponent = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return isSquareAttacked(game, kingRow, kingCol, opponent);
}

// Check if a player has any legal moves
int hasLegalMoves(ChessGame* game, PieceColor color) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = game->board[row][col];
            if (piece.type != PIECE_NONE && piece.color == color) {
                int moves[64][2];
                int count;
                getPossibleMoves(game, row, col, moves, &count);
                if (count > 0) {
                    return 1; // Found a legal move
                }
            }
        }
    }
    return 0; // No legal moves
}

// Check for game end conditions
// Returns: 0=game continues, 1=checkmate, 2=stalemate, 3=draw by repetition, 4=draw by 50-move rule
int checkGameEnd(ChessGame* game, PieceColor color) {
    // Check for 50-move rule
    if (game->halfMoveClock >= 100) {
        return 4; // Draw by 50-move rule
    }
    
    // Check for threefold repetition
    if (checkThreefoldRepetition(game)) {
        return 3; // Draw by repetition
    }
    
    int hasMoves = hasLegalMoves(game, color);
    int inCheck = isInCheck(game, color);

    if (!hasMoves) {
        if (inCheck) {
            return 1; // Checkmate
        } else {
            return 2; // Stalemate
        }
    }

    return 0; // Game continues
}

// Make a move (without animation - for AI)
void makeMove(ChessGame* game, int fromRow, int fromCol, int toRow, int toCol) {
    Piece piece = game->board[fromRow][fromCol];
    Piece capturedPiece = game->board[toRow][toCol];

    // Update half-move clock for 50-move rule
    if (piece.type == PIECE_PAWN || capturedPiece.type != PIECE_NONE) {
        game->halfMoveClock = 0; // Reset on pawn move or capture
    } else {
        game->halfMoveClock++;
    }

    // Track castling rights - check if king or rooks move
    if (piece.type == PIECE_KING) {
        if (piece.color == COLOR_WHITE) {
            game->whiteKingMoved = 1;
        } else {
            game->blackKingMoved = 1;
        }
        
        // Check if this is a castling move
        if (abs(toCol - fromCol) == 2) {
            // This is castling!
            int rookFromCol = (toCol > fromCol) ? 7 : 0;
            int rookToCol = (toCol > fromCol) ? 5 : 3;
            
            // Move the rook
            game->board[toRow][rookToCol] = game->board[fromRow][rookFromCol];
            game->board[fromRow][rookFromCol] = (Piece){PIECE_NONE, COLOR_NONE};
        }
    } else if (piece.type == PIECE_ROOK) {
        // Track which rook moved
        if (piece.color == COLOR_WHITE) {
            if (fromRow == 7 && fromCol == 7) {
                game->whiteRookKingsideMoved = 1;
            } else if (fromRow == 7 && fromCol == 0) {
                game->whiteRookQueensideMoved = 1;
            }
        } else {
            if (fromRow == 0 && fromCol == 7) {
                game->blackRookKingsideMoved = 1;
            } else if (fromRow == 0 && fromCol == 0) {
                game->blackRookQueensideMoved = 1;
            }
        }
    }
    
    // Check if a rook was captured (affects castling rights)
    if (capturedPiece.type == PIECE_ROOK) {
        if (capturedPiece.color == COLOR_WHITE) {
            if (toRow == 7 && toCol == 7) {
                game->whiteRookKingsideMoved = 1;
            } else if (toRow == 7 && toCol == 0) {
                game->whiteRookQueensideMoved = 1;
            }
        } else {
            if (toRow == 0 && toCol == 7) {
                game->blackRookKingsideMoved = 1;
            } else if (toRow == 0 && toCol == 0) {
                game->blackRookQueensideMoved = 1;
            }
        }
    }

    // Track captured pieces
    if (capturedPiece.type != PIECE_NONE) {
        if (capturedPiece.color == COLOR_WHITE) {
            game->capturedWhite[capturedPiece.type]++;
        } else {
            game->capturedBlack[capturedPiece.type]++;
        }
    }

    // Handle en passant capture
    if (piece.type == PIECE_PAWN && toRow == game->enPassantRow && toCol == game->enPassantCol) {
        // Remove the captured pawn
        int capturedRow = (piece.color == COLOR_WHITE) ? toRow + 1 : toRow - 1;
        Piece enPassantPawn = game->board[capturedRow][toCol];
        if (enPassantPawn.type != PIECE_NONE) {
            if (enPassantPawn.color == COLOR_WHITE) {
                game->capturedWhite[enPassantPawn.type]++;
            } else {
                game->capturedBlack[enPassantPawn.type]++;
            }
            game->halfMoveClock = 0; // Reset on capture
        }
        game->board[capturedRow][toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    }

    // Clear en passant target
    game->enPassantRow = -1;
    game->enPassantCol = -1;

    // Set en passant target if pawn moves two squares
    if (piece.type == PIECE_PAWN && abs(toRow - fromRow) == 2) {
        game->enPassantRow = (fromRow + toRow) / 2;
        game->enPassantCol = toCol;
    }

    game->board[toRow][toCol] = piece;
    game->board[fromRow][fromCol] = (Piece){PIECE_NONE, COLOR_NONE};

    // Pawn promotion (auto-promote to queen for AI, dialog for human)
    if (piece.type == PIECE_PAWN && (toRow == 0 || toRow == 7)) {
        game->board[toRow][toCol].type = PIECE_QUEEN; // Auto-promote for AI
    }

    // Save board state for repetition detection
    saveBoardState(game);

    // Switch turns
    game->currentPlayer = (game->currentPlayer == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    game->selectedRow = -1;
    game->selectedCol = -1;
    game->possibleMovesCount = 0;

    // Check for game end conditions for the NEW current player (who just received the turn)
    int gameEnd = checkGameEnd(game, game->currentPlayer);
    if (gameEnd > 0) {
        game->gameOver = gameEnd; // 1=checkmate, 2=stalemate, 3=repetition, 4=50-move
    }
}

// Make a move with animation (for human players)
void makeMoveAnimated(ChessGame* game, int fromRow, int fromCol, int toRow, int toCol) {
    Piece piece = game->board[fromRow][fromCol];
    
    // Check if pawn promotion is needed
    if (piece.type == PIECE_PAWN && (toRow == 0 || toRow == 7)) {
        // Show promotion dialog
        game->promotionDialog.active = 1;
        game->promotionDialog.row = toRow;
        game->promotionDialog.col = toCol;
        game->promotionDialog.color = piece.color;
        
        // Temporarily make the move to set up promotion
        game->board[toRow][toCol] = piece;
        game->board[fromRow][fromCol] = (Piece){PIECE_NONE, COLOR_NONE};
        
        // Start animation
        game->animation.active = 1;
        game->animation.fromRow = fromRow;
        game->animation.fromCol = fromCol;
        game->animation.toRow = toRow;
        game->animation.toCol = toCol;
        game->animation.progress = 0.0f;
        game->animation.startTime = SDL_GetTicks();
        game->animation.duration = MOVE_ANIMATION_DURATION;
        game->animation.movingPiece = piece;
        
        return; // Wait for promotion selection
    }
    
    // Start animation
    game->animation.active = 1;
    game->animation.fromRow = fromRow;
    game->animation.fromCol = fromCol;
    game->animation.toRow = toRow;
    game->animation.toCol = toCol;
    game->animation.progress = 0.0f;
    game->animation.startTime = SDL_GetTicks();
    game->animation.duration = MOVE_ANIMATION_DURATION;
    game->animation.movingPiece = piece;
    
    // Make the actual move
    makeMove(game, fromRow, fromCol, toRow, toCol);
}

// AI thread function - runs in background
int aiThreadFunction(void* data) {
    ChessGame* game = (ChessGame*)data;
    
    if (!game->ai) return 0;
    
    int bestFromRow, bestFromCol, bestToRow, bestToCol;
    findBestMove(game->ai, game->currentPlayer, &bestFromRow, &bestFromCol, &bestToRow, &bestToCol);
    
    // Lock mutex to safely update shared data
    SDL_LockMutex(game->aiMutex);
    game->aiBestFromRow = bestFromRow;
    game->aiBestFromCol = bestFromCol;
    game->aiBestToRow = bestToRow;
    game->aiBestToCol = bestToCol;
    game->aiMoveReady = 1;
    SDL_UnlockMutex(game->aiMutex);
    
    return 0;
}

// Start AI thinking in background thread
void startAIThinking(ChessGame* game) {
    if (!game->ai || game->gameOver > 0 || game->aiThinking) return;
    
    game->aiThinking = 1;
    game->aiMoveReady = 0;
    
    // Create AI thread
    game->aiThread = SDL_CreateThread(aiThreadFunction, "AIThread", (void*)game);
    if (!game->aiThread) {
        printf("Failed to create AI thread: %s\n", SDL_GetError());
        game->aiThinking = 0;
    }
}

// Check if AI has finished thinking and execute move
void checkAIMove(ChessGame* game) {
    if (!game->aiThinking) return;
    
    // Check if AI has a move ready (thread-safe)
    SDL_LockMutex(game->aiMutex);
    int moveReady = game->aiMoveReady;
    int fromRow = game->aiBestFromRow;
    int fromCol = game->aiBestFromCol;
    int toRow = game->aiBestToRow;
    int toCol = game->aiBestToCol;
    SDL_UnlockMutex(game->aiMutex);
    
    if (moveReady) {
        // Wait for thread to complete
        SDL_WaitThread(game->aiThread, NULL);
        game->aiThread = NULL;
        game->aiThinking = 0;
        
        // Execute the move
        if (fromRow != -1) {
            Piece piece = game->board[fromRow][fromCol];
            
            // Start animation
            game->animation.active = 1;
            game->animation.fromRow = fromRow;
            game->animation.fromCol = fromCol;
            game->animation.toRow = toRow;
            game->animation.toCol = toCol;
            game->animation.progress = 0.0f;
            game->animation.startTime = SDL_GetTicks();
            game->animation.duration = MOVE_ANIMATION_DURATION;
            game->animation.movingPiece = piece;
            
            makeMove(game, fromRow, fromCol, toRow, toCol);
        }
    }
}

// Legacy function for compatibility (now uses threading)
void makeAIMove(ChessGame* game) {
    startAIThinking(game);
}

// Handle square click
void handleSquareClick(ChessGame* game, int row, int col) {
    if (game->gameOver > 0 || game->aiThinking || game->animation.active || game->promotionDialog.active) return;

    // If it's AI's turn, don't allow human input
    if (game->gameMode == GAME_MODE_HUMAN_VS_AI && game->currentPlayer != COLOR_WHITE) {
        return;
    }

    Piece piece = game->board[row][col];

    // If a square is already selected
    if (game->selectedRow != -1) {
        // If clicking the same square, deselect
        if (game->selectedRow == row && game->selectedCol == col) {
            game->selectedRow = -1;
            game->selectedCol = -1;
            game->possibleMovesCount = 0;
            return;
        }

        // If clicking a possible move, make the move
        if (isPossibleMove(game->possibleMoves, game->possibleMovesCount, row, col)) {
            makeMoveAnimated(game, game->selectedRow, game->selectedCol, row, col);
            return;
        }

        // If clicking own piece, select it
        if (piece.type != PIECE_NONE && piece.color == game->currentPlayer) {
            game->selectedRow = row;
            game->selectedCol = col;
            getPossibleMoves(game, row, col, game->possibleMoves, &game->possibleMovesCount);
            return;
        }
    }

    // Select piece if it's the current player's turn
    if (piece.type != PIECE_NONE && piece.color == game->currentPlayer) {
        game->selectedRow = row;
        game->selectedCol = col;
        getPossibleMoves(game, row, col, game->possibleMoves, &game->possibleMovesCount);
    }
}

// Load piece images
void loadPieceImages(ChessGame* game) {
    // Initialize all textures to NULL
    memset(game->pieceTextures, 0, sizeof(game->pieceTextures));
    
    // File names: pieces/white_pawn.png, pieces/black_rook.png, etc.
    const char* pieceNames[7] = {"", "pawn", "rook", "knight", "bishop", "queen", "king"};
    const char* colorNames[2] = {"white", "black"};
    
    for (int color = 0; color < 2; color++) {
        for (int type = 1; type < 7; type++) {
            char filename[256];
            sprintf(filename, "pieces/%s_%s.png", colorNames[color], pieceNames[type]);
            
            SDL_Surface* surface = IMG_Load(filename);
            if (surface) {
                game->pieceTextures[color][type] = SDL_CreateTextureFromSurface(game->renderer, surface);
                SDL_FreeSurface(surface);
                if (!game->pieceTextures[color][type]) {
                    printf("Warning: Could not create texture for %s\n", filename);
                }
            } else {
                printf("Warning: Could not load image %s: %s\n", filename, IMG_GetError());
            }
        }
    }
}

// Render text using SDL2_ttf
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect rect = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

// Helper function to draw a filled circle
void drawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

// Helper function to draw a circle outline
void drawCircleOutline(SDL_Renderer* renderer, int centerX, int centerY, int radius, int thickness) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            int distSq = x*x + y*y;
            int outerRadiusSq = radius * radius;
            int innerRadiusSq = (radius - thickness) * (radius - thickness);
            if (distSq <= outerRadiusSq && distSq >= innerRadiusSq) {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

// Render promotion dialog
void renderPromotionDialog(ChessGame* game) {
    if (!game->promotionDialog.active) return;
    
    // Semi-transparent overlay
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(game->renderer, &overlay);
    
    // Dialog box
    int dialogWidth = 400;
    int dialogHeight = 150;
    int dialogX = (BOARD_SIZE * SQUARE_SIZE - dialogWidth) / 2;
    int dialogY = (BOARD_SIZE * SQUARE_SIZE - dialogHeight) / 2;
    
    SDL_SetRenderDrawColor(game->renderer, 50, 50, 50, 255);
    SDL_Rect dialogRect = {dialogX, dialogY, dialogWidth, dialogHeight};
    SDL_RenderFillRect(game->renderer, &dialogRect);
    
    SDL_SetRenderDrawColor(game->renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(game->renderer, &dialogRect);
    
    // Title
    if (game->font) {
        SDL_Color white = {255, 255, 255, 255};
        renderText(game->renderer, game->font, "Choose Promotion Piece", 
                  dialogX + 80, dialogY + 10, white);
    }
    
    // Draw piece options (Queen, Rook, Bishop, Knight)
    PieceType options[] = {PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT};
    int optionSize = 70;
    int startX = dialogX + 30;
    int startY = dialogY + 50;
    
    for (int i = 0; i < 4; i++) {
        int optionX = startX + i * (optionSize + 20);
        
        // Draw background
        SDL_SetRenderDrawColor(game->renderer, 100, 100, 100, 255);
        SDL_Rect optionRect = {optionX, startY, optionSize, optionSize};
        SDL_RenderFillRect(game->renderer, &optionRect);
        
        SDL_SetRenderDrawColor(game->renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(game->renderer, &optionRect);
        
        // Draw piece
        int colorIdx = game->promotionDialog.color - 1;
        SDL_Texture* texture = game->pieceTextures[colorIdx][options[i]];
        if (texture) {
            SDL_Rect pieceRect = {optionX + 5, startY + 5, optionSize - 10, optionSize - 10};
            SDL_RenderCopy(game->renderer, texture, NULL, &pieceRect);
        }
    }
    
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);
}

// Render the chess board
void renderBoard(ChessGame* game) {
    // Update animation
    if (game->animation.active) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsed = currentTime - game->animation.startTime;
        game->animation.progress = (float)elapsed / game->animation.duration;
        
        if (game->animation.progress >= 1.0f) {
            game->animation.active = 0;
            game->animation.progress = 1.0f;
        }
    }
    
    // Clear screen
    SDL_SetRenderDrawColor(game->renderer, 40, 40, 40, 255);
    SDL_RenderClear(game->renderer);
    
    // Draw board squares
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            SDL_Rect rect = {col * SQUARE_SIZE, row * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE};
            
            // Normal square colors
            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(game->renderer, 240, 217, 181, 255); // Light
            } else {
                SDL_SetRenderDrawColor(game->renderer, 181, 136, 99, 255); // Dark
            }
            SDL_RenderFillRect(game->renderer, &rect);
            
            // Highlight selected square
            if (game->selectedRow == row && game->selectedCol == col) {
                SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(game->renderer, 100, 149, 237, 180);
                SDL_RenderFillRect(game->renderer, &rect);
                SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);

                SDL_SetRenderDrawColor(game->renderer, 70, 130, 180, 255);
                SDL_RenderDrawRect(game->renderer, &rect);
            }
            
            // Draw piece (skip if it's the animating piece)
            Piece piece = game->board[row][col];
            if (piece.type != PIECE_NONE) {
                int isAnimating = (game->animation.active && 
                                  row == game->animation.toRow && 
                                  col == game->animation.toCol);
                
                if (!isAnimating) {
                    int centerX = col * SQUARE_SIZE + SQUARE_SIZE / 2;
                    int centerY = row * SQUARE_SIZE + SQUARE_SIZE / 2;
                    int colorIdx = piece.color - 1;
                    SDL_Texture* texture = game->pieceTextures[colorIdx][piece.type];
                    
                    if (texture) {
                        int imgSize = SQUARE_SIZE - 10;
                        SDL_Rect dstRect = {
                            col * SQUARE_SIZE + 5,
                            row * SQUARE_SIZE + 5,
                            imgSize,
                            imgSize
                        };
                        SDL_RenderCopy(game->renderer, texture, NULL, &dstRect);
                    } else {
                        const char* symbol = pieceSymbols[colorIdx][piece.type];
                        if (symbol && strlen(symbol) > 0) {
                            SDL_Color textColor = (piece.color == COLOR_WHITE) ? 
                                (SDL_Color){255, 255, 255, 255} : (SDL_Color){0, 0, 0, 255};
                            
                            if (game->font) {
                                renderText(game->renderer, game->font, symbol, 
                                          col * SQUARE_SIZE + SQUARE_SIZE / 2 - 20,
                                          row * SQUARE_SIZE + SQUARE_SIZE / 2 - 25, textColor);
                            } else {
                                int radius = SQUARE_SIZE / 3;
                                SDL_SetRenderDrawColor(game->renderer, textColor.r, textColor.g, textColor.b, textColor.a);
                                drawFilledCircle(game->renderer, centerX, centerY, radius);
                            }
                        }
                    }
                }
            }
            
            // Draw move indicators
            if (isPossibleMove(game->possibleMoves, game->possibleMovesCount, row, col)) {
                SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
                int centerX = col * SQUARE_SIZE + SQUARE_SIZE / 2;
                int centerY = row * SQUARE_SIZE + SQUARE_SIZE / 2;
                Piece target = game->board[row][col];
                
                if (target.type != PIECE_NONE) {
                    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
                    drawCircleOutline(game->renderer, centerX, centerY, SQUARE_SIZE / 2 - 2, 4);
                    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 200);
                    drawCircleOutline(game->renderer, centerX, centerY, SQUARE_SIZE / 2 - 2, 2);
                } else {
                    SDL_SetRenderDrawColor(game->renderer, 100, 100, 100, 200);
                    drawFilledCircle(game->renderer, centerX, centerY, 8);
                }
                SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);
            }
        }
    }
    
    // Draw animating piece on top
    if (game->animation.active) {
        float fromX = game->animation.fromCol * SQUARE_SIZE;
        float fromY = game->animation.fromRow * SQUARE_SIZE;
        float toX = game->animation.toCol * SQUARE_SIZE;
        float toY = game->animation.toRow * SQUARE_SIZE;
        
        float currentX = fromX + (toX - fromX) * game->animation.progress;
        float currentY = fromY + (toY - fromY) * game->animation.progress;
        
        int colorIdx = game->animation.movingPiece.color - 1;
        SDL_Texture* texture = game->pieceTextures[colorIdx][game->animation.movingPiece.type];
        
        if (texture) {
            int imgSize = SQUARE_SIZE - 10;
            SDL_Rect dstRect = {(int)currentX + 5, (int)currentY + 5, imgSize, imgSize};
            SDL_RenderCopy(game->renderer, texture, NULL, &dstRect);
        }
    }
    
    // Draw the UI panel (side panel with game info)
    int panelX = BOARD_SIZE * SQUARE_SIZE + 15;
    int panelY = 15;
    int panelWidth = 320;
    int panelHeight = WINDOW_HEIGHT - 30;

    SDL_SetRenderDrawColor(game->renderer, 35, 35, 35, 255);
    SDL_Rect mainPanel = {panelX, panelY, panelWidth, panelHeight};
    SDL_RenderFillRect(game->renderer, &mainPanel);

    SDL_SetRenderDrawColor(game->renderer, 100, 100, 100, 255);
    SDL_Rect borderRect = {panelX - 2, panelY - 2, panelWidth + 4, panelHeight + 4};
    SDL_RenderDrawRect(game->renderer, &borderRect);

    SDL_SetRenderDrawColor(game->renderer, 70, 70, 70, 255);
    SDL_Rect innerRect = {panelX, panelY, panelWidth, panelHeight};
    SDL_RenderDrawRect(game->renderer, &innerRect);

    // Draw status text
    char statusText[200];
    SDL_Color statusColor = {255, 255, 255, 255};

    if (game->aiThinking) {
        sprintf(statusText, "AI Thinking...");
        statusColor = (SDL_Color){100, 200, 255, 255};
    } else if (game->gameOver == 1) {
        PieceColor winner = (game->currentPlayer == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
        sprintf(statusText, "Checkmate! %s wins!",
            (winner == COLOR_WHITE) ? "White" : "Black");
        statusColor = (SDL_Color){255, 215, 0, 255};
    } else if (game->gameOver == 2) {
        sprintf(statusText, "Stalemate! Draw!");
        statusColor = (SDL_Color){200, 200, 200, 255};
    } else if (game->gameOver == 3) {
        sprintf(statusText, "Draw by Repetition!");
        statusColor = (SDL_Color){200, 200, 200, 255};
    } else if (game->gameOver == 4) {
        sprintf(statusText, "Draw by 50-Move Rule!");
        statusColor = (SDL_Color){200, 200, 200, 255};
    } else {
        const char* modeStr = "";
        switch (game->gameMode) {
            case GAME_MODE_HUMAN_VS_HUMAN: modeStr = "Human vs Human"; break;
            case GAME_MODE_HUMAN_VS_AI: modeStr = "Human vs AI"; break;
        }

        const char* diffStr = "";
        switch (game->aiDifficulty) {
            case DIFFICULTY_EASY: diffStr = "(Easy)"; break;
            case DIFFICULTY_MEDIUM: diffStr = "(Medium)"; break;
            case DIFFICULTY_HARD: diffStr = "(Hard)"; break;
            case DIFFICULTY_EXPERT: diffStr = "(Expert)"; break;
        }

        sprintf(statusText, "%s - %s %s",
            (game->currentPlayer == COLOR_WHITE) ? "White" : "Black",
            modeStr, game->gameMode == GAME_MODE_HUMAN_VS_AI ? diffStr : "");

        if (isInCheck(game, game->currentPlayer)) {
            strcat(statusText, " - CHECK!");
            statusColor = (SDL_Color){255, 100, 100, 255};
        }
    }

    if (game->font) {
        renderText(game->renderer, game->font, statusText,
                   panelX + 20, panelY + 20, statusColor);

        SDL_Color infoColor = {180, 180, 180, 255};
        SDL_Color whiteColor = {255, 255, 255, 255};
        SDL_Color blackColor = {200, 200, 200, 255};

        // Captured pieces section
        int captureY = panelY + 70;
        renderText(game->renderer, game->font, "Captured Pieces:",
                  panelX + 20, captureY, infoColor);

        captureY += 30;
        renderText(game->renderer, game->font, "White Lost:",
                  panelX + 20, captureY, whiteColor);
        
        captureY += 25;
        int captureX = panelX + 20;
        int pieceSize = 30;
        int hasWhiteCaptured = 0;
        
        PieceType pieceOrder[] = {PIECE_PAWN, PIECE_KNIGHT, PIECE_BISHOP, PIECE_ROOK, PIECE_QUEEN, PIECE_KING};
        for (int i = 0; i < 6; i++) {
            PieceType type = pieceOrder[i];
            int count = game->capturedWhite[type];
            
            for (int j = 0; j < count; j++) {
                SDL_Texture* texture = game->pieceTextures[0][type];
                if (texture) {
                    SDL_Rect dstRect = {captureX, captureY, pieceSize, pieceSize};
                    SDL_RenderCopy(game->renderer, texture, NULL, &dstRect);
                }
                captureX += pieceSize + 5;
                
                if (captureX > panelX + panelWidth - pieceSize - 20) {
                    captureX = panelX + 20;
                    captureY += pieceSize + 5;
                }
                hasWhiteCaptured = 1;
            }
        }
        
        if (!hasWhiteCaptured) {
            renderText(game->renderer, game->font, "None", captureX, captureY, whiteColor);
            captureY += 25;
        } else {
            captureY += pieceSize + 10;
        }

        captureY += 10;
        renderText(game->renderer, game->font, "Black Lost:",
                  panelX + 20, captureY, blackColor);
        
        captureY += 25;
        captureX = panelX + 20;
        int hasBlackCaptured = 0;
        
        for (int i = 0; i < 6; i++) {
            PieceType type = pieceOrder[i];
            int count = game->capturedBlack[type];
            
            for (int j = 0; j < count; j++) {
                SDL_Texture* texture = game->pieceTextures[1][type];
                if (texture) {
                    SDL_Rect dstRect = {captureX, captureY, pieceSize, pieceSize};
                    SDL_RenderCopy(game->renderer, texture, NULL, &dstRect);
                }
                captureX += pieceSize + 5;
                
                if (captureX > panelX + panelWidth - pieceSize - 20) {
                    captureX = panelX + 20;
                    captureY += pieceSize + 5;
                }
                hasBlackCaptured = 1;
            }
        }
        
        if (!hasBlackCaptured) {
            renderText(game->renderer, game->font, "None", captureX, captureY, blackColor);
            captureY += 25;
        } else {
            captureY += pieceSize + 10;
        }

        // Controls section
        int controlsY = captureY + 50;
        renderText(game->renderer, game->font, "Controls:",
                  panelX + 20, controlsY, infoColor);
        renderText(game->renderer, game->font, "Click to select/move",
                  panelX + 30, controlsY + 25, infoColor);
        renderText(game->renderer, game->font, "Press R to reset",
                  panelX + 30, controlsY + 50, infoColor);
        renderText(game->renderer, game->font, "ESC or close to exit",
                  panelX + 30, controlsY + 75, infoColor);

        renderText(game->renderer, game->font, "Game Modes (Ctrl+Key):",
                  panelX + 20, controlsY + 110, infoColor);
        renderText(game->renderer, game->font, "1: Human vs Human",
                  panelX + 30, controlsY + 135, infoColor);
        renderText(game->renderer, game->font, "2-5: Human vs AI (Easy-Expert)",
                  panelX + 30, controlsY + 160, infoColor);
    }
    
    // Draw promotion dialog on top if active
    renderPromotionDialog(game);
    
    SDL_RenderPresent(game->renderer);
}

// Initialize SDL and create window
int initSDL(ChessGame* game) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
    }
    
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image initialization failed: %s\n", IMG_GetError());
    }
    
    game->window = SDL_CreateWindow("Chess Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    
    if (!game->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 0;
    }
    
    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 0;
    }
    
    loadPieceImages(game);
    
    game->font = TTF_OpenFont("arial.ttf", 20);
    if (!game->font) {
        game->font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 20);
    }
    if (!game->font) {
        game->font = TTF_OpenFont("C:/Windows/Fonts/calibri.ttf", 20);
    }
    if (!game->font) {
        game->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    }
    
    // Create mutex for AI threading
    game->aiMutex = SDL_CreateMutex();
    if (!game->aiMutex) {
        printf("Failed to create AI mutex: %s\n", SDL_GetError());
        return 0;
    }
    
    game->aiThread = NULL;
    game->aiThinking = 0;
    game->aiMoveReady = 0;
    
    return 1;
}

// Cleanup SDL
void cleanupSDL(ChessGame* game) {
    // Stop AI thread if running
    if (game->aiThread) {
        SDL_WaitThread(game->aiThread, NULL);
        game->aiThread = NULL;
    }
    
    // Destroy mutex
    if (game->aiMutex) {
        SDL_DestroyMutex(game->aiMutex);
        game->aiMutex = NULL;
    }
    
    if (game->ai) {
        destroyChessAI(game->ai);
    }

    for (int color = 0; color < 2; color++) {
        for (int type = 0; type < 7; type++) {
            if (game->pieceTextures[color][type]) {
                SDL_DestroyTexture(game->pieceTextures[color][type]);
            }
        }
    }

    if (game->font) TTF_CloseFont(game->font);
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window) SDL_DestroyWindow(game->window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Initialize game with mode and AI settings
void initializeGame(ChessGame* game, GameMode mode, AIDifficulty difficulty) {
    // Stop any ongoing AI thinking
    game->aiThinking = 0;
    game->animation.active = 0;
    game->promotionDialog.active = 0;
    
    game->currentPlayer = COLOR_WHITE;
    game->selectedRow = -1;
    game->selectedCol = -1;
    game->possibleMovesCount = 0;
    game->gameOver = 0;
    game->enPassantRow = -1;
    game->enPassantCol = -1;
    game->gameMode = mode;
    game->aiDifficulty = difficulty;
    
    // Reset castling rights
    game->whiteKingMoved = 0;
    game->whiteRookKingsideMoved = 0;
    game->whiteRookQueensideMoved = 0;
    game->blackKingMoved = 0;
    game->blackRookKingsideMoved = 0;
    game->blackRookQueensideMoved = 0;

    // Safely destroy old AI
    if (game->ai) {
        destroyChessAI(game->ai);
        game->ai = NULL;
    }
    
    // Create new AI if needed
    if (mode == GAME_MODE_HUMAN_VS_AI) {
        game->ai = createChessAI(game, difficulty);
        if (!game->ai) {
            printf("Error: Failed to create AI\n");
            game->gameMode = GAME_MODE_HUMAN_VS_HUMAN; // Fallback
        }
    }
}

// Handle promotion piece selection
void handlePromotionClick(ChessGame* game, int mouseX, int mouseY) {
    if (!game->promotionDialog.active) return;
    
    int dialogWidth = 400;
    int dialogX = (BOARD_SIZE * SQUARE_SIZE - dialogWidth) / 2;
    int dialogY = (BOARD_SIZE * SQUARE_SIZE - 150) / 2;
    
    int optionSize = 70;
    int startX = dialogX + 30;
    int startY = dialogY + 50;
    
    PieceType options[] = {PIECE_QUEEN, PIECE_ROOK, PIECE_BISHOP, PIECE_KNIGHT};
    
    for (int i = 0; i < 4; i++) {
        int optionX = startX + i * (optionSize + 20);
        
        if (mouseX >= optionX && mouseX < optionX + optionSize &&
            mouseY >= startY && mouseY < startY + optionSize) {
            // Selected this piece
            game->board[game->promotionDialog.row][game->promotionDialog.col].type = options[i];
            game->promotionDialog.active = 0;
            
            // Now complete the move processing
            saveBoardState(game);
            game->currentPlayer = (game->currentPlayer == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
            game->selectedRow = -1;
            game->selectedCol = -1;
            game->possibleMovesCount = 0;
            
            int gameEnd = checkGameEnd(game, game->currentPlayer);
            if (gameEnd > 0) {
                game->gameOver = gameEnd;
            }
            break;
        }
    }
}

// Main function
int main(int argc, char* argv[]) {
    ChessGame game = {0};

    GameMode gameMode = GAME_MODE_HUMAN_VS_AI;
    AIDifficulty aiDifficulty = DIFFICULTY_MEDIUM;

    initializeGame(&game, gameMode, aiDifficulty);
    
    if (!initSDL(&game)) {
        return 1;
    }
    
    initBoard(&game);
    
    SDL_Event event;
    int running = 1;
    Uint32 lastAIMoveTime = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (game.promotionDialog.active) {
                        handlePromotionClick(&game, event.button.x, event.button.y);
                    } else {
                        int col = event.button.x / SQUARE_SIZE;
                        int row = event.button.y / SQUARE_SIZE;

                        if (isValidSquare(row, col)) {
                            handleSquareClick(&game, row, col);
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_r) {
                    initBoard(&game);
                    game.currentPlayer = COLOR_WHITE;
                    game.selectedRow = -1;
                    game.selectedCol = -1;
                    game.possibleMovesCount = 0;
                    game.gameOver = 0;
                    game.enPassantRow = -1;
                    game.enPassantCol = -1;
                    game.aiThinking = 0;
                    game.animation.active = 0;
                    game.promotionDialog.active = 0;
                    game.whiteKingMoved = 0;
                    game.whiteRookKingsideMoved = 0;
                    game.whiteRookQueensideMoved = 0;
                    game.blackKingMoved = 0;
                    game.blackRookKingsideMoved = 0;
                    game.blackRookQueensideMoved = 0;
                    lastAIMoveTime = 0;
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if (event.key.keysym.sym == SDLK_1 && event.key.keysym.mod & KMOD_CTRL) {
                    initializeGame(&game, GAME_MODE_HUMAN_VS_HUMAN, DIFFICULTY_MEDIUM);
                    initBoard(&game);
                } else if (event.key.keysym.sym == SDLK_2 && event.key.keysym.mod & KMOD_CTRL) {
                    initializeGame(&game, GAME_MODE_HUMAN_VS_AI, DIFFICULTY_EASY);
                    initBoard(&game);
                } else if (event.key.keysym.sym == SDLK_3 && event.key.keysym.mod & KMOD_CTRL) {
                    initializeGame(&game, GAME_MODE_HUMAN_VS_AI, DIFFICULTY_MEDIUM);
                    initBoard(&game);
                } else if (event.key.keysym.sym == SDLK_4 && event.key.keysym.mod & KMOD_CTRL) {
                    initializeGame(&game, GAME_MODE_HUMAN_VS_AI, DIFFICULTY_HARD);
                    initBoard(&game);
                } else if (event.key.keysym.sym == SDLK_5 && event.key.keysym.mod & KMOD_CTRL) {
                    initializeGame(&game, GAME_MODE_HUMAN_VS_AI, DIFFICULTY_EXPERT);
                    initBoard(&game);
                }
            }
        }

        // Handle AI moves with threading
        if (game.gameMode == GAME_MODE_HUMAN_VS_AI && game.currentPlayer != COLOR_WHITE && game.ai) {
            if (!game.gameOver && !game.animation.active) {
                if (!game.aiThinking) {
                    // Start AI thinking
                    Uint32 currentTime = SDL_GetTicks();
                    if (currentTime - lastAIMoveTime > 500) {
                        startAIThinking(&game);
                        lastAIMoveTime = currentTime;
                    }
                } else {
                    // Check if AI has finished thinking
                    checkAIMove(&game);
                }
            }
        }

        renderBoard(&game);
        SDL_Delay(16); // ~60 FPS
    }
    
    cleanupSDL(&game);
    return 0;
}