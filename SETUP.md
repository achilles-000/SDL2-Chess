# How to Build and Run the Chess Game

## Option 1: Install MSYS2 (Recommended - Easiest)

1. **Download and Install MSYS2:**
   - Go to https://www.msys2.org/
   - Download and run the installer
   - Follow the installation wizard

2. **Open MSYS2 UCRT64 terminal** (from Start Menu)

3. **Install required packages:**
   ```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc
   pacman -S mingw-w64-ucrt-x86_64-SDL2
   ```

4. **Add MSYS2 to your PATH:**
   - Add `C:\msys64\ucrt64\bin` to your Windows PATH environment variable
   - Or use the MSYS2 terminal directly

5. **Build the game:**
   ```bash
   cd /c/Users/Stefan/Documents/project1/project
   gcc -Wall -Wextra -std=c99 -o chess.exe chess.c -lSDL2
   ```

6. **Run:**
   ```bash
   ./chess.exe
   ```

## Option 2: Install MinGW-w64 Standalone

1. **Download MinGW-w64:**
   - Go to https://www.mingw-w64.org/downloads/
   - Or use WinLibs: https://winlibs.com/
   - Download the UCRT runtime version

2. **Extract and add to PATH:**
   - Extract to a folder (e.g., `C:\mingw64`)
   - Add `C:\mingw64\bin` to your Windows PATH

3. **Download SDL2:**
   - Go to https://github.com/libsdl-org/SDL/releases
   - Download `SDL2-devel-2.x.x-mingw.tar.gz`
   - Extract and copy:
     - `include` folder → `C:\mingw64\include\SDL2`
     - `lib` folder contents → `C:\mingw64\lib`
     - `bin` folder DLLs → `C:\mingw64\bin`

4. **Build:**
   ```bash
   cd C:\Users\Stefan\Documents\project1\project
   gcc -Wall -Wextra -std=c99 -o chess.exe chess.c -lSDL2
   ```

## Option 3: Use Visual Studio (if you have it)

1. **Install Visual Studio** with C++ development tools

2. **Download SDL2:**
   - Get SDL2-devel from https://www.libsdl.org/download-2.0.php
   - Extract to a folder

3. **Create a Visual Studio project** and configure SDL2 paths

## Quick Test

After installing, test if GCC works:
```bash
gcc --version
```

If it works, navigate to the project folder and build:
```bash
cd C:\Users\Stefan\Documents\project1\project
gcc -Wall -Wextra -std=c99 -o chess.exe chess.c -lSDL2
```

Then run:
```bash
chess.exe
```

## Troubleshooting

- **"gcc: command not found"** - GCC is not in your PATH. Add the bin folder to PATH or use the full path.
- **"cannot find -lSDL2"** - SDL2 libraries are not found. Make sure SDL2 is installed and in the correct location.
- **"SDL2/SDL.h: No such file"** - SDL2 headers are not found. Check include paths.
