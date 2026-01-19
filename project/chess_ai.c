#include "chess_ai.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define BOARD_SIZE 8

// Piece values for evaluation (centipawns)
static const int PIECE_VALUES[] = {
    0,   // NONE
    100, // PAWN
    500, // ROOK
    320, // KNIGHT
    330, // BISHOP
    900, // QUEEN
    20000 // KING
};

// Position bonus tables
static const int PAWN_POSITION_BONUS[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

static const int KNIGHT_POSITION_BONUS[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

static const int BISHOP_POSITION_BONUS[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

static const int ROOK_POSITION_BONUS[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {0,  0,  0,  5,  5,  0,  0,  0}
};

static const int QUEEN_POSITION_BONUS[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    {-5,  0,  5,  5,  5,  5,  0, -5},
    {0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

static const int KING_POSITION_BONUS[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    {20, 20,  0,  0,  0,  0, 20, 20},
    {20, 30, 10,  0,  0, 10, 30, 20}
};

// Helper function to check if a square is attacked (works on search board)
int isSquareAttackedAI(ChessAI* ai, int targetRow, int targetCol, PieceColor attackerColor) {
    int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type == PIECE_NONE || piece.color != attackerColor) continue;
            
            if (piece.type == PIECE_PAWN) {
                int direction = (piece.color == COLOR_WHITE) ? -1 : 1;
                for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
                    if (row + direction == targetRow && col + colOffset == targetCol) {
                        return 1;
                    }
                }
            }
            else if (piece.type == PIECE_KNIGHT) {
                for (int i = 0; i < 8; i++) {
                    if (row + knightMoves[i][0] == targetRow && 
                        col + knightMoves[i][1] == targetCol) {
                        return 1;
                    }
                }
            }
            else if (piece.type == PIECE_KING) {
                for (int i = 0; i < 8; i++) {
                    if (row + directions[i][0] == targetRow && 
                        col + directions[i][1] == targetCol) {
                        return 1;
                    }
                }
            }
            else {
                int isRook = (piece.type == PIECE_ROOK);
                int isBishop = (piece.type == PIECE_BISHOP);
                
                for (int d = 0; d < 8; d++) {
                    if (isRook && (d == 0 || d == 2 || d == 5 || d == 7)) continue;
                    if (isBishop && (d == 1 || d == 3 || d == 4 || d == 6)) continue;
                    
                    for (int i = 1; i < 8; i++) {
                        int newRow = row + directions[d][0] * i;
                        int newCol = col + directions[d][1] * i;
                        
                        if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
                        if (newRow == targetRow && newCol == targetCol) return 1;
                        if (ai->searchBoard[newRow][newCol].type != PIECE_NONE) break;
                    }
                }
            }
        }
    }
    return 0;
}

// Check if king is in check (on search board)
int isInCheckAI(ChessAI* ai, PieceColor color) {
    int kingRow = -1, kingCol = -1;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type == PIECE_KING && piece.color == color) {
                kingRow = row;
                kingCol = col;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    if (kingRow == -1) return 0;
    
    PieceColor opponent = (color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return isSquareAttackedAI(ai, kingRow, kingCol, opponent);
}

// Create AI instance
ChessAI* createChessAI(ChessGame* game, AIDifficulty difficulty) {
    if (!game) return NULL;
    
    ChessAI* ai = (ChessAI*)malloc(sizeof(ChessAI));
    if (!ai) return NULL;

    ai->game = game;
    ai->difficulty = difficulty;
    ai->maxDepth = difficulty * 2;
    ai->nodesSearched = 0;
    ai->searchHistoryCount = 0;
    ai->stopSearch = 0;
    
    // Initialize search board state
    memset(ai->searchBoard, 0, sizeof(ai->searchBoard));
    ai->searchEnPassantRow = -1;
    ai->searchEnPassantCol = -1;
    ai->searchWhiteKingMoved = 0;
    ai->searchWhiteRookKingsideMoved = 0;
    ai->searchWhiteRookQueensideMoved = 0;
    ai->searchBlackKingMoved = 0;
    ai->searchBlackRookKingsideMoved = 0;
    ai->searchBlackRookQueensideMoved = 0;
    ai->searchCurrentPlayer = COLOR_WHITE;
    
    // Allocate transposition table
    ai->transpositionTable = (TTEntry*)calloc(TT_SIZE, sizeof(TTEntry));
    if (!ai->transpositionTable) {
        free(ai);
        return NULL;
    }

    return ai;
}

// Destroy AI instance
void destroyChessAI(ChessAI* ai) {
    if (ai) {
        if (ai->transpositionTable) {
            free(ai->transpositionTable);
        }
        free(ai);
    }
}

// Set AI difficulty
void setAIDifficulty(ChessAI* ai, AIDifficulty difficulty) {
    ai->difficulty = difficulty;
    ai->maxDepth = difficulty * 2;
}

// Zobrist hashing for position (uses search board)
unsigned long long hashPosition(ChessAI* ai) {
    unsigned long long hash = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece p = ai->searchBoard[row][col];
            if (p.type != PIECE_NONE) {
                hash = hash * 31 + (p.type * 13 + p.color * 7 + row * 11 + col * 17);
            }
        }
    }
    
    hash ^= (ai->searchWhiteKingMoved * 23);
    hash ^= (ai->searchBlackKingMoved * 29);
    hash ^= (ai->searchEnPassantCol * 37 + ai->searchEnPassantRow * 41);
    hash ^= (ai->searchCurrentPlayer * 43);
    
    return hash;
}

// Store position in transposition table
void storeTTEntry(ChessAI* ai, unsigned long long hash, int depth, int score, int flag) {
    unsigned int index = hash % TT_SIZE;
    TTEntry* entry = &ai->transpositionTable[index];
    
    if (entry->hash == 0 || entry->depth <= depth) {
        entry->hash = hash;
        entry->depth = depth;
        entry->score = score;
        entry->flag = flag;
    }
}

// Probe transposition table
TTEntry* probeTTEntry(ChessAI* ai, unsigned long long hash) {
    unsigned int index = hash % TT_SIZE;
    TTEntry* entry = &ai->transpositionTable[index];
    
    if (entry->hash == hash) {
        return entry;
    }
    return NULL;
}

// Reset node count
void resetNodeCount(ChessAI* ai) {
    ai->nodesSearched = 0;
}

// Get nodes searched
int getNodesSearched(ChessAI* ai) {
    return ai->nodesSearched;
}

// Position evaluation (uses search board)
int evaluatePosition(ChessAI* ai, PieceColor color) {
    int score = 0;

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type != PIECE_NONE) {
                int pieceValue = PIECE_VALUES[piece.type];
                int positionBonus = 0;

                int boardRow = (piece.color == COLOR_WHITE) ? row : (7 - row);
                int boardCol = col;

                switch (piece.type) {
                    case PIECE_PAWN:
                        positionBonus = PAWN_POSITION_BONUS[boardRow][boardCol];
                        break;
                    case PIECE_KNIGHT:
                        positionBonus = KNIGHT_POSITION_BONUS[boardRow][boardCol];
                        break;
                    case PIECE_BISHOP:
                        positionBonus = BISHOP_POSITION_BONUS[boardRow][boardCol];
                        break;
                    case PIECE_ROOK:
                        positionBonus = ROOK_POSITION_BONUS[boardRow][boardCol];
                        break;
                    case PIECE_QUEEN:
                        positionBonus = QUEEN_POSITION_BONUS[boardRow][boardCol];
                        break;
                    case PIECE_KING:
                        positionBonus = KING_POSITION_BONUS[boardRow][boardCol];
                        break;
                    default:
                        break;
                }

                if (piece.color == color) {
                    score += pieceValue + positionBonus;
                } else {
                    score -= pieceValue + positionBonus;
                }
            }
        }
    }

    return score;
}

// Helper to check if move would put own king in check (on search board)
int wouldBeInCheckAI(ChessAI* ai, int fromRow, int fromCol, int toRow, int toCol, PieceColor color) {
    Piece originalPiece = ai->searchBoard[toRow][toCol];
    Piece movingPiece = ai->searchBoard[fromRow][fromCol];
    ai->searchBoard[toRow][toCol] = movingPiece;
    ai->searchBoard[fromRow][fromCol] = (Piece){PIECE_NONE, COLOR_NONE};
    
    Piece enPassantPawn = {PIECE_NONE, COLOR_NONE};
    int enPassantCaptureRow = -1;
    if (movingPiece.type == PIECE_PAWN && toRow == ai->searchEnPassantRow && toCol == ai->searchEnPassantCol) {
        enPassantCaptureRow = (movingPiece.color == COLOR_WHITE) ? toRow + 1 : toRow - 1;
        enPassantPawn = ai->searchBoard[enPassantCaptureRow][toCol];
        ai->searchBoard[enPassantCaptureRow][toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    }
    
    int inCheck = isInCheckAI(ai, color);
    
    ai->searchBoard[fromRow][fromCol] = movingPiece;
    ai->searchBoard[toRow][toCol] = originalPiece;
    if (enPassantCaptureRow != -1) {
        ai->searchBoard[enPassantCaptureRow][toCol] = enPassantPawn;
    }
    
    return inCheck;
}

// Get possible moves for a piece (on search board)
void getPossibleMovesAI(ChessAI* ai, int row, int col, int moves[64][2], int* count) {
    *count = 0;
    Piece piece = ai->searchBoard[row][col];
    
    if (piece.type == PIECE_NONE) return;
    
    int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    
    if (piece.type == PIECE_PAWN) {
        int direction = (piece.color == COLOR_WHITE) ? -1 : 1;
        int startRow = (piece.color == COLOR_WHITE) ? 6 : 1;
        
        if (row + direction >= 0 && row + direction < 8 && ai->searchBoard[row + direction][col].type == PIECE_NONE) {
            if (!wouldBeInCheckAI(ai, row, col, row + direction, col, piece.color)) {
                moves[*count][0] = row + direction;
                moves[*count][1] = col;
                (*count)++;
            }
            
            if (row == startRow && ai->searchBoard[row + 2 * direction][col].type == PIECE_NONE) {
                if (!wouldBeInCheckAI(ai, row, col, row + 2 * direction, col, piece.color)) {
                    moves[*count][0] = row + 2 * direction;
                    moves[*count][1] = col;
                    (*count)++;
                }
            }
        }
        
        for (int colOffset = -1; colOffset <= 1; colOffset += 2) {
            int newCol = col + colOffset;
            if (newCol >= 0 && newCol < 8 && row + direction >= 0 && row + direction < 8) {
                Piece target = ai->searchBoard[row + direction][newCol];
                if (target.type != PIECE_NONE && target.color != piece.color) {
                    if (!wouldBeInCheckAI(ai, row, col, row + direction, newCol, piece.color)) {
                        moves[*count][0] = row + direction;
                        moves[*count][1] = newCol;
                        (*count)++;
                    }
                }
            }
        }
        
        if (ai->searchEnPassantRow != -1 && ai->searchEnPassantCol != -1) {
            if (row + direction == ai->searchEnPassantRow && 
                (col + 1 == ai->searchEnPassantCol || col - 1 == ai->searchEnPassantCol)) {
                if (!wouldBeInCheckAI(ai, row, col, ai->searchEnPassantRow, ai->searchEnPassantCol, piece.color)) {
                    moves[*count][0] = ai->searchEnPassantRow;
                    moves[*count][1] = ai->searchEnPassantCol;
                    (*count)++;
                }
            }
        }
    }
    else if (piece.type == PIECE_KNIGHT) {
        for (int i = 0; i < 8; i++) {
            int newRow = row + knightMoves[i][0];
            int newCol = col + knightMoves[i][1];
            
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                Piece target = ai->searchBoard[newRow][newCol];
                if (target.type == PIECE_NONE || target.color != piece.color) {
                    if (!wouldBeInCheckAI(ai, row, col, newRow, newCol, piece.color)) {
                        moves[*count][0] = newRow;
                        moves[*count][1] = newCol;
                        (*count)++;
                    }
                }
            }
        }
    }
    else if (piece.type == PIECE_KING) {
        for (int i = 0; i < 8; i++) {
            int newRow = row + directions[i][0];
            int newCol = col + directions[i][1];
            
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                Piece target = ai->searchBoard[newRow][newCol];
                if (target.type == PIECE_NONE || target.color != piece.color) {
                    if (!wouldBeInCheckAI(ai, row, col, newRow, newCol, piece.color)) {
                        moves[*count][0] = newRow;
                        moves[*count][1] = newCol;
                        (*count)++;
                    }
                }
            }
        }
        
        // Castling
        int kingRow = (piece.color == COLOR_WHITE) ? 7 : 0;
        if (row == kingRow && col == 4) {
            int canKingside = 0, canQueenside = 0;
            
            if (piece.color == COLOR_WHITE) {
                canKingside = !ai->searchWhiteKingMoved && !ai->searchWhiteRookKingsideMoved;
                canQueenside = !ai->searchWhiteKingMoved && !ai->searchWhiteRookQueensideMoved;
            } else {
                canKingside = !ai->searchBlackKingMoved && !ai->searchBlackRookKingsideMoved;
                canQueenside = !ai->searchBlackKingMoved && !ai->searchBlackRookQueensideMoved;
            }
            
            PieceColor opponent = (piece.color == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
            
            if (canKingside && !isInCheckAI(ai, piece.color)) {
                if (ai->searchBoard[kingRow][5].type == PIECE_NONE && 
                    ai->searchBoard[kingRow][6].type == PIECE_NONE &&
                    !isSquareAttackedAI(ai, kingRow, 5, opponent) &&
                    !isSquareAttackedAI(ai, kingRow, 6, opponent)) {
                    moves[*count][0] = kingRow;
                    moves[*count][1] = 6;
                    (*count)++;
                }
            }
            
            if (canQueenside && !isInCheckAI(ai, piece.color)) {
                if (ai->searchBoard[kingRow][1].type == PIECE_NONE && 
                    ai->searchBoard[kingRow][2].type == PIECE_NONE &&
                    ai->searchBoard[kingRow][3].type == PIECE_NONE &&
                    !isSquareAttackedAI(ai, kingRow, 2, opponent) &&
                    !isSquareAttackedAI(ai, kingRow, 3, opponent)) {
                    moves[*count][0] = kingRow;
                    moves[*count][1] = 2;
                    (*count)++;
                }
            }
        }
    }
    else {
        int isRook = (piece.type == PIECE_ROOK);
        int isBishop = (piece.type == PIECE_BISHOP);
        
        for (int d = 0; d < 8; d++) {
            if (isRook && (d == 0 || d == 2 || d == 5 || d == 7)) continue;
            if (isBishop && (d == 1 || d == 3 || d == 4 || d == 6)) continue;
            
            for (int i = 1; i < 8; i++) {
                int newRow = row + directions[d][0] * i;
                int newCol = col + directions[d][1] * i;
                
                if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
                
                Piece target = ai->searchBoard[newRow][newCol];
                if (target.type == PIECE_NONE) {
                    if (!wouldBeInCheckAI(ai, row, col, newRow, newCol, piece.color)) {
                        moves[*count][0] = newRow;
                        moves[*count][1] = newCol;
                        (*count)++;
                    }
                } else {
                    if (target.color != piece.color) {
                        if (!wouldBeInCheckAI(ai, row, col, newRow, newCol, piece.color)) {
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
}

// Generate all possible moves (uses search board)
int generateAllMoves(ChessAI* ai, PieceColor color, AIMove* moves) {
    int count = 0;

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type != PIECE_NONE && piece.color == color) {
                int tempMoves[64][2];
                int tempCount;
                getPossibleMovesAI(ai, row, col, tempMoves, &tempCount);

                for (int i = 0; i < tempCount; i++) {
                    moves[count].fromRow = row;
                    moves[count].fromCol = col;
                    moves[count].toRow = tempMoves[i][0];
                    moves[count].toCol = tempMoves[i][1];
                    moves[count].score = 0;
                    count++;
                }
            }
        }
    }

    return count;
}

// Order moves for better pruning (uses search board)
void orderMoves(ChessAI* ai, AIMove* moves, int count) {
    for (int i = 0; i < count; i++) {
        moves[i].score = 0;

        Piece victim = ai->searchBoard[moves[i].toRow][moves[i].toCol];
        Piece attacker = ai->searchBoard[moves[i].fromRow][moves[i].fromCol];

        if (victim.type != PIECE_NONE) {
            moves[i].score = PIECE_VALUES[victim.type] * 100 - PIECE_VALUES[attacker.type];
        }
        
        int toCenter = abs(3 - moves[i].toRow) + abs(3 - moves[i].toCol);
        int fromCenter = abs(3 - moves[i].fromRow) + abs(3 - moves[i].fromCol);
        if (toCenter < fromCenter) {
            moves[i].score += 10;
        }
        
        if (attacker.type == PIECE_PAWN) {
            if (attacker.color == COLOR_WHITE) {
                moves[i].score += (6 - moves[i].toRow) * 5;
            } else {
                moves[i].score += (moves[i].toRow - 1) * 5;
            }
        }
        
        if (attacker.type == PIECE_KING && abs(moves[i].toCol - moves[i].fromCol) == 2) {
            moves[i].score += 50;
        }
    }

    for (int i = 1; i < count; i++) {
        AIMove key = moves[i];
        int j = i - 1;
        while (j >= 0 && moves[j].score < key.score) {
            moves[j + 1] = moves[j];
            j--;
        }
        moves[j + 1] = key;
    }
}

// Make move for AI search (ONLY modifies search board, NOT game->board)
void makeMoveForAI(ChessAI* ai, int fromRow, int fromCol, int toRow, int toCol) {
    if (ai->searchHistoryCount >= 100) return;

    AIMoveHistory* hist = &ai->searchHistory[ai->searchHistoryCount++];
    
    hist->fromRow = fromRow;
    hist->fromCol = fromCol;
    hist->toRow = toRow;
    hist->toCol = toCol;
    hist->movedPiece = ai->searchBoard[fromRow][fromCol];
    hist->capturedPiece = ai->searchBoard[toRow][toCol];
    hist->enPassantRow = ai->searchEnPassantRow;
    hist->enPassantCol = ai->searchEnPassantCol;
    hist->wasEnPassantCapture = 0;
    hist->enPassantCaptureRow = -1;
    hist->wasCastling = 0;
    
    hist->whiteKingMoved = ai->searchWhiteKingMoved;
    hist->whiteRookKingsideMoved = ai->searchWhiteRookKingsideMoved;
    hist->whiteRookQueensideMoved = ai->searchWhiteRookQueensideMoved;
    hist->blackKingMoved = ai->searchBlackKingMoved;
    hist->blackRookKingsideMoved = ai->searchBlackRookKingsideMoved;
    hist->blackRookQueensideMoved = ai->searchBlackRookQueensideMoved;

    Piece piece = ai->searchBoard[fromRow][fromCol];

    if (piece.type == PIECE_KING) {
        if (piece.color == COLOR_WHITE) {
            ai->searchWhiteKingMoved = 1;
        } else {
            ai->searchBlackKingMoved = 1;
        }
        
        if (abs(toCol - fromCol) == 2) {
            hist->wasCastling = 1;
            hist->castlingRookFromCol = (toCol > fromCol) ? 7 : 0;
            hist->castlingRookToCol = (toCol > fromCol) ? 5 : 3;
            
            ai->searchBoard[toRow][hist->castlingRookToCol] = ai->searchBoard[fromRow][hist->castlingRookFromCol];
            ai->searchBoard[fromRow][hist->castlingRookFromCol] = (Piece){PIECE_NONE, COLOR_NONE};
        }
    } else if (piece.type == PIECE_ROOK) {
        if (piece.color == COLOR_WHITE) {
            if (fromRow == 7 && fromCol == 7) ai->searchWhiteRookKingsideMoved = 1;
            else if (fromRow == 7 && fromCol == 0) ai->searchWhiteRookQueensideMoved = 1;
        } else {
            if (fromRow == 0 && fromCol == 7) ai->searchBlackRookKingsideMoved = 1;
            else if (fromRow == 0 && fromCol == 0) ai->searchBlackRookQueensideMoved = 1;
        }
    }
    
    if (hist->capturedPiece.type == PIECE_ROOK) {
        if (hist->capturedPiece.color == COLOR_WHITE) {
            if (toRow == 7 && toCol == 7) ai->searchWhiteRookKingsideMoved = 1;
            else if (toRow == 7 && toCol == 0) ai->searchWhiteRookQueensideMoved = 1;
        } else {
            if (toRow == 0 && toCol == 7) ai->searchBlackRookKingsideMoved = 1;
            else if (toRow == 0 && toCol == 0) ai->searchBlackRookQueensideMoved = 1;
        }
    }

    if (piece.type == PIECE_PAWN && toRow == ai->searchEnPassantRow && toCol == ai->searchEnPassantCol) {
        int capturedRow = (piece.color == COLOR_WHITE) ? toRow + 1 : toRow - 1;
        Piece enPassantPawn = ai->searchBoard[capturedRow][toCol];
        if (enPassantPawn.type != PIECE_NONE) {
            hist->wasEnPassantCapture = 1;
            hist->enPassantCaptureRow = capturedRow;
            hist->capturedPiece = enPassantPawn;
        }
        ai->searchBoard[capturedRow][toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    }

    ai->searchEnPassantRow = -1;
    ai->searchEnPassantCol = -1;

    if (piece.type == PIECE_PAWN && abs(toRow - fromRow) == 2) {
        ai->searchEnPassantRow = (fromRow + toRow) / 2;
        ai->searchEnPassantCol = toCol;
    }

    ai->searchBoard[toRow][toCol] = piece;
    ai->searchBoard[fromRow][fromCol] = (Piece){PIECE_NONE, COLOR_NONE};

    if (piece.type == PIECE_PAWN && (toRow == 0 || toRow == 7)) {
        ai->searchBoard[toRow][toCol].type = PIECE_QUEEN;
    }

    ai->searchCurrentPlayer = (ai->searchCurrentPlayer == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}

// Unmake move for AI search (ONLY modifies search board)
void unmakeMoveForAI(ChessAI* ai) {
    if (ai->searchHistoryCount <= 0) return;

    AIMoveHistory* hist = &ai->searchHistory[--ai->searchHistoryCount];

    ai->searchBoard[hist->fromRow][hist->fromCol] = hist->movedPiece;
    
    if (hist->wasCastling) {
        ai->searchBoard[hist->fromRow][hist->castlingRookFromCol] = ai->searchBoard[hist->toRow][hist->castlingRookToCol];
        ai->searchBoard[hist->toRow][hist->castlingRookToCol] = (Piece){PIECE_NONE, COLOR_NONE};
        ai->searchBoard[hist->toRow][hist->toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    } else if (hist->wasEnPassantCapture) {
        ai->searchBoard[hist->enPassantCaptureRow][hist->toCol] = hist->capturedPiece;
        ai->searchBoard[hist->toRow][hist->toCol] = (Piece){PIECE_NONE, COLOR_NONE};
    } else {
        ai->searchBoard[hist->toRow][hist->toCol] = hist->capturedPiece;
    }

    ai->searchEnPassantRow = hist->enPassantRow;
    ai->searchEnPassantCol = hist->enPassantCol;
    ai->searchWhiteKingMoved = hist->whiteKingMoved;
    ai->searchWhiteRookKingsideMoved = hist->whiteRookKingsideMoved;
    ai->searchWhiteRookQueensideMoved = hist->whiteRookQueensideMoved;
    ai->searchBlackKingMoved = hist->blackKingMoved;
    ai->searchBlackRookKingsideMoved = hist->blackRookKingsideMoved;
    ai->searchBlackRookQueensideMoved = hist->blackRookQueensideMoved;

    ai->searchCurrentPlayer = (ai->searchCurrentPlayer == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}

// Check if player has any legal moves (on search board)
int hasLegalMovesAI(ChessAI* ai, PieceColor color) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type != PIECE_NONE && piece.color == color) {
                int moves[64][2];
                int count;
                getPossibleMovesAI(ai, row, col, moves, &count);
                if (count > 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Check for game end conditions (on search board)
int checkGameEndAI(ChessAI* ai, PieceColor color) {
    int hasMoves = hasLegalMovesAI(ai, color);
    int inCheck = isInCheckAI(ai, color);

    if (!hasMoves) {
        if (inCheck) {
            return 1; // Checkmate
        } else {
            return 2; // Stalemate
        }
    }

    return 0;
}

// Quiescence search (uses search board)
int quiescenceSearch(ChessAI* ai, int alpha, int beta, PieceColor maximizingPlayer) {
    ai->nodesSearched++;
    
    int standPat = evaluatePosition(ai, maximizingPlayer);
    
    if (standPat >= beta) {
        return beta;
    }
    if (alpha < standPat) {
        alpha = standPat;
    }
    
    AIMove moves[256];
    int moveCount = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece piece = ai->searchBoard[row][col];
            if (piece.type != PIECE_NONE && piece.color == ai->searchCurrentPlayer) {
                int tempMoves[64][2];
                int tempCount;
                getPossibleMovesAI(ai, row, col, tempMoves, &tempCount);
                
                for (int i = 0; i < tempCount; i++) {
                    Piece target = ai->searchBoard[tempMoves[i][0]][tempMoves[i][1]];
                    if (target.type != PIECE_NONE) {
                        moves[moveCount].fromRow = row;
                        moves[moveCount].fromCol = col;
                        moves[moveCount].toRow = tempMoves[i][0];
                        moves[moveCount].toCol = tempMoves[i][1];
                        moves[moveCount].score = 0;
                        moveCount++;
                    }
                }
            }
        }
    }
    
    orderMoves(ai, moves, moveCount);
    
    for (int i = 0; i < moveCount; i++) {
        AIMove move = moves[i];
        
        makeMoveForAI(ai, move.fromRow, move.fromCol, move.toRow, move.toCol);
        int score = -quiescenceSearch(ai, -beta, -alpha, maximizingPlayer);
        unmakeMoveForAI(ai);
        
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

// Minimax with alpha-beta pruning (uses search board)
int minimax(ChessAI* ai, int depth, int alpha, int beta, PieceColor maximizingPlayer) {
    ai->nodesSearched++;
    
    if (ai->stopSearch) return 0;
    
    unsigned long long hash = hashPosition(ai);
    TTEntry* ttEntry = probeTTEntry(ai, hash);
    if (ttEntry && ttEntry->depth >= depth) {
        if (ttEntry->flag == TT_EXACT) {
            return ttEntry->score;
        } else if (ttEntry->flag == TT_ALPHA && ttEntry->score <= alpha) {
            return alpha;
        } else if (ttEntry->flag == TT_BETA && ttEntry->score >= beta) {
            return beta;
        }
    }

    int gameEnd = checkGameEndAI(ai, ai->searchCurrentPlayer);
    if (gameEnd > 0) {
        if (gameEnd == 1) {
            int score = (maximizingPlayer == ai->searchCurrentPlayer) ? -999999 + (ai->maxDepth - depth) : 999999 - (ai->maxDepth - depth);
            storeTTEntry(ai, hash, depth, score, TT_EXACT);
            return score;
        } else {
            storeTTEntry(ai, hash, depth, 0, TT_EXACT);
            return 0;
        }
    }
    
    if (depth == 0) {
        int score = quiescenceSearch(ai, alpha, beta, maximizingPlayer);
        storeTTEntry(ai, hash, depth, score, TT_EXACT);
        return score;
    }

    AIMove moves[256];
    int moveCount = generateAllMoves(ai, ai->searchCurrentPlayer, moves);
    orderMoves(ai, moves, moveCount);
    
    int originalAlpha = alpha;

    if (maximizingPlayer == ai->searchCurrentPlayer) {
        int maxEval = INT_MIN;
        for (int i = 0; i < moveCount; i++) {
            if (ai->stopSearch) break;
            
            AIMove move = moves[i];

            makeMoveForAI(ai, move.fromRow, move.fromCol, move.toRow, move.toCol);
            int eval = minimax(ai, depth - 1, alpha, beta, maximizingPlayer);
            unmakeMoveForAI(ai);

            maxEval = (eval > maxEval) ? eval : maxEval;
            alpha = (alpha > eval) ? alpha : eval;

            if (beta <= alpha) break;
        }
        
        int flag = (maxEval <= originalAlpha) ? TT_ALPHA : (maxEval >= beta) ? TT_BETA : TT_EXACT;
        storeTTEntry(ai, hash, depth, maxEval, flag);
        
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for (int i = 0; i < moveCount; i++) {
            if (ai->stopSearch) break;
            
            AIMove move = moves[i];

            makeMoveForAI(ai, move.fromRow, move.fromCol, move.toRow, move.toCol);
            int eval = minimax(ai, depth - 1, alpha, beta, maximizingPlayer);
            unmakeMoveForAI(ai);

            minEval = (eval < minEval) ? eval : minEval;
            beta = (beta < eval) ? beta : eval;

            if (beta <= alpha) break;
        }
        
        int flag = (minEval <= originalAlpha) ? TT_ALPHA : (minEval >= beta) ? TT_BETA : TT_EXACT;
        storeTTEntry(ai, hash, depth, minEval, flag);
        
        return minEval;
    }
}

// Find best move - COPIES game state to search board before searching
void findBestMove(ChessAI* ai, PieceColor color, int* bestFromRow, int* bestFromCol, int* bestToRow, int* bestToCol) {
    // Copy game state to AI's search board (this is the key change!)
    memcpy(ai->searchBoard, ai->game->board, sizeof(ai->searchBoard));
    ai->searchEnPassantRow = ai->game->enPassantRow;
    ai->searchEnPassantCol = ai->game->enPassantCol;
    ai->searchWhiteKingMoved = ai->game->whiteKingMoved;
    ai->searchWhiteRookKingsideMoved = ai->game->whiteRookKingsideMoved;
    ai->searchWhiteRookQueensideMoved = ai->game->whiteRookQueensideMoved;
    ai->searchBlackKingMoved = ai->game->blackKingMoved;
    ai->searchBlackRookKingsideMoved = ai->game->blackRookKingsideMoved;
    ai->searchBlackRookQueensideMoved = ai->game->blackRookQueensideMoved;
    ai->searchCurrentPlayer = color;
    
    AIMove moves[256];
    int moveCount = generateAllMoves(ai, color, moves);
    orderMoves(ai, moves, moveCount);

    resetNodeCount(ai);
    ai->searchHistoryCount = 0;
    ai->stopSearch = 0;

    int bestScore = INT_MIN;
    AIMove bestMove = {-1, -1, -1, -1, 0};

    for (int i = 0; i < moveCount; i++) {
        if (ai->stopSearch) break;
        
        AIMove move = moves[i];

        makeMoveForAI(ai, move.fromRow, move.fromCol, move.toRow, move.toCol);
        int score = minimax(ai, ai->maxDepth - 1, INT_MIN, INT_MAX, color);
        unmakeMoveForAI(ai);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    if (bestMove.fromRow != -1) {
        *bestFromRow = bestMove.fromRow;
        *bestFromCol = bestMove.fromCol;
        *bestToRow = bestMove.toRow;
        *bestToCol = bestMove.toCol;
    }
}