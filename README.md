1. Linux版（WSL）のビルド＆実行
Bash
# 構成 (Configure)
cmake --preset linux

# ビルド (Build)
cmake --build build/linux

# 実行
./raylib_tetris
# または cmake 経由で実行する場合:
cmake --build build/linux --target run
2. Windows版（MinGWクロスコンパイル）のビルド＆実行
Bash
# 構成 (Configure)
cmake --preset win

# ビルド (Build)
cmake --build build/win

# WSLのターミナルからWindowsアプリとして実行
chmod +x reylib_tetris.exe
./raylib_tetris.exe
