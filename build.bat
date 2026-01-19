@echo off
echo Building Chess Game...
set PATH=C:\msys64\ucrt64\bin;%PATH%
gcc -Wall -Wextra -std=c99 -o chess.exe chess.c chess_ai.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
if %ERRORLEVEL% EQU 0 (
    echo Build successful! Copying DLLs...
    copy C:\msys64\ucrt64\bin\SDL2.dll SDL2.dll >nul 2>&1
    copy C:\msys64\ucrt64\bin\SDL2_ttf.dll SDL2_ttf.dll >nul 2>&1
    copy C:\msys64\ucrt64\bin\SDL2_image.dll SDL2_image.dll >nul 2>&1
    echo.
    echo Piece images should be placed in a 'pieces' folder:
    echo   pieces/white_pawn.png
    echo   pieces/white_rook.png
    echo   pieces/white_knight.png
    echo   pieces/white_bishop.png
    echo   pieces/white_queen.png
    echo   pieces/white_king.png
    echo   pieces/black_pawn.png
    echo   pieces/black_rook.png
    echo   pieces/black_knight.png
    echo   pieces/black_bishop.png
    echo   pieces/black_queen.png
    echo   pieces/black_king.png
    echo.
    echo Run chess.exe to play.
) else (
    echo Build failed! Make sure SDL2, SDL2_ttf, and SDL2_image are installed via MSYS2.
    echo Install with: pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_image
)
pause
