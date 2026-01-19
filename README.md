# Chess Game - Windows C Application with AI

A complete chess game implemented in C using SDL2 for Windows, featuring multiple game modes including AI opponents with adjustable difficulty.

## 🎯 Features

- **Complete Chess Rules**: All standard chess piece movements, check/checkmate detection, stalemate detection, pawn promotion, en passant
- **Multiple Game Modes**:
  - Human vs Human
  - Human vs AI (4 difficulty levels)
- **Advanced AI**: Minimax algorithm with alpha-beta pruning, position evaluation with piece values and positional bonuses
- **Visual Feedback**: Move indicators (dots for moves, circles for captures), piece highlighting, game status display
- **Captured Pieces Display**: Shows what pieces each side has lost during the game
- **Professional UI**: Clean interface with game statistics and controls

## Requirements

- **MSYS2** (recommended for easy installation)
- **GCC compiler**
- **SDL2** development libraries
- **SDL2_ttf** (for text rendering)
- **SDL2_image** (for piece images, optional)

## Installation & Building

### Quick Setup (MSYS2)

1. **Install MSYS2**: Download from https://www.msys2.org/
2. **Install dependencies**:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc
   pacman -S mingw-w64-ucrt-x86_64-SDL2
   pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
   pacman -S mingw-w64-ucrt-x86_64-SDL2_image
   ```
3. **Build the game**:
   ```bash
   cd your-project-directory
   .\build.bat
   ```

## Game Modes & Controls

### Starting Different Game Modes

Use **Ctrl + Number** to switch game modes:

- **Ctrl+1**: Human vs Human
- **Ctrl+2**: Human vs AI (Easy - depth 2)
- **Ctrl+3**: Human vs AI (Medium - depth 4)
- **Ctrl+4**: Human vs AI (Hard - depth 6)
- **Ctrl+5**: Human vs AI (Expert - depth 8)

### Gameplay Controls

- **Left Click**: Select piece / Make move
- **R Key**: Reset game to starting position
- **ESC**: Exit game

### Move Indicators

- **Small dots**: Available move squares
- **Circles**: Squares where you can capture opponent pieces
- **Blue highlight**: Selected piece

## AI Engine Details

### Algorithm
- **Minimax** with **alpha-beta pruning** for efficiency
- **Depth control** for adjustable difficulty (2-8 ply)
- **Move ordering** for better pruning performance

### Position Evaluation
- **Material balance**: Piece values (Pawn=100, Rook=500, Knight/Bishop=320/330, Queen=900, King=20000)
- **Positional bonuses**: Pieces score better on active squares
- **Checkmate/Stalemate detection**: Immediate win/loss/draw evaluation

### Performance
- **Node counting**: Tracks positions evaluated per move
- **Thinking delay**: 1-second pause between AI moves for better UX

## File Structure

```
chess.c          # Main game logic and SDL rendering
chess_ai.h       # AI engine header
chess_ai.c       # AI implementation (minimax, evaluation)
pieces/          # Optional piece image files
build.bat        # Windows build script
Makefile         # Alternative build configuration
```

## Piece Images (Optional)

Place PNG files in the `pieces/` folder for custom piece graphics:
- `white_pawn.png`, `white_rook.png`, etc.
- `black_pawn.png`, `black_rook.png`, etc.

If no images are found, the game uses Unicode chess symbols or simple shapes.

## Building from Source

### Windows (MSYS2)
```bash
gcc -Wall -Wextra -std=c99 -o chess.exe chess.c chess_ai.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
```

### Linux/Mac
```bash
gcc -Wall -Wextra -std=c99 -o chess chess.c chess_ai.c -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
```

## Chess Rules Implemented

✅ **All standard rules**:
- Piece movements (pawn, rook, knight, bishop, queen, king)
- Check and checkmate detection
- Stalemate detection
- Pawn promotion (auto-promotes to queen)
- En passant captures
- No illegal moves allowed (moves that would leave king in check)

## AI Difficulty Levels

- **Easy (Depth 2)**: Basic tactical play, good for beginners
- **Medium (Depth 4)**: Decent positional understanding
- **Hard (Depth 6)**: Strong tactical and positional play
- **Expert (Depth 8)**: Very challenging, requires precise play to win

## Troubleshooting

- **"gcc command not found"**: Add MSYS2 to your PATH or use full path to gcc.exe
- **Missing DLLs**: Run `build.bat` which copies required DLLs automatically
- **No piece images**: Game falls back to Unicode symbols or shapes
- **AI too slow**: Reduce difficulty level or increase depth limit in code

## Technical Notes

- **Written in C**: No external dependencies except SDL2 libraries
- **Cross-platform**: Code works on Windows, Linux, and macOS
- **Efficient AI**: Alpha-beta pruning reduces search space significantly
- **Memory safe**: No dynamic memory allocation during gameplay
- **Extensible**: Easy to add features like castling, 50-move rule, etc.

Enjoy playing chess against a challenging AI opponent! ♟️🤖
