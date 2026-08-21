これまでに構築したCMakeとCMakePresets.jsonを活用した、ビルド・実行・クリーンアップ手順の完全まとめをご用意いたしました。

今後は複雑なコマンドオプションを覚える必要はなく、すべてこの構成で一元管理できます。

プロジェクト構成（前提）
CMakePresets.json をプロジェクトルートに配置している状態です。

Plaintext
raylib_tetris/
├── CMakeLists.txt        # ビルド設定
├── CMakePresets.json     # 各環境用プリセット (win / win-native / linux / web)
└── ...
1. ビルドコマンド（Configure & Build）
cmake --preset <名前> で構成し、cmake --build build/<フォルダ> でコンパイルします。

対象プラットフォーム	実行環境	構成（Configure）	構築（Build）
Linux (WSL)	WSL	cmake --preset linux	cmake --build build/linux
Windows (.exe)	WSL	cmake --preset win	cmake --build build/win
Windows (.exe)	Windows (w64devkit)	cmake --preset win-native	cmake --build build/win-native
Web (HTML/WASM)	WSL / Windows	cmake --preset web	cmake --build build/web
2. 実行コマンド（Run）
ビルドされた成果物を起動する手順です。

Linux (WSL) 版

Bash
./build/linux/raylib_tetris
Windows (.exe) 版（WSLのターミナルから起動する場合）

Bash
cmd.exe /c ".\raylib_tetris.exe"
Windows (.exe) 版（Windowsネイティブ環境から起動する場合）

DOS
.\raylib_tetris.exe
Web (Emscripten) 版
簡易Webサーバーを起動し、ブラウザで http://localhost:8000/raylib_tetris.html にアクセスします。

Bash
python3 -m http.server -d build/web 8000
3. ワンコマンドで「ビルド ＆ 実行」
コンパイルから起動までを一括で行う方法です。

Linux (WSL) 版
CMakeLists.txt 内の run ターゲットを呼び出します。

Bash
cmake --build build/linux --target run
Windows (.exe) 版（WSLから）

Bash
cmake --build build/win && cmd.exe /c ".\raylib_tetris.exe"
4. クリーンアップ（Clean & Wipe）
状況に合わせて以下のいずれかを選択します。

生成されたオブジェクトファイル（.o や .exe など）のみ消去する場合
CMakeの構成（キャッシュ）を残したまま成果物だけを削除します。

Bash
cmake --build build/linux --target clean
ビルド環境を完全クリア（初期化）する場合【推奨】
キャッシュを含めて build ディレクトリごと削除します。構成をやり直したい時はこれが確実です。

Bash
rm -rf build/
5. Neovim（nvim）からのショートカット実行
Neovim 内からワンキーでビルド＆実行を行う場合の init.lua 設定例です。

Lua
-- F5キー: Linux版を画面下ターミナルでビルド＆実行
vim.keymap.set('n', '<F5>', ':sp | terminal cmake --build build/linux --target run<CR>', { noremap = true, silent = true })

-- F6キー: Windows版を画面下ターミナルでビルド＆cmd経由実行
vim.keymap.set('n', '<F6>', ':sp | terminal cmake --build build/win && cmd.exe /c ".\\raylib_tetris.exe"<CR>', { noremap = true, silent = true })


-- 実行時の注意：
音が出なかったりノイズっぽく聞こえたりしていたのは、PulseAudioの不具合ではなく、「作業ディレクトリ（カレントディレクトリ）のズレ」によってBGMやSEのファイルが見つかっていなかったからですね！

原因のメカニズム
ログにある通り、プログラム実行時の Working Directory（カレントディレクトリ）が build/linux になっています。

そのため、コード内の LoadMusicStream("Sounds/music.mp3") などの相対パスが、プロジェクトルート（raylib_tetris/）からではなく build/linux/Sounds/music.mp3 を探しに行ってしまい、ファイルが開けず無音になっていました。

解決方法（2つのアプローチ）
最新の main ブランチ（git switch main）に戻った上で、以下のいずれかで対処できます。

方法1: 実行時のワーキングディレクトリをプロジェクトルートにする（推奨）
build/linux フォルダの中から直接 ./raylib_tetris を叩くのではなく、プロジェクトのルートディレクトリから実行します。

Bash
# プロジェクトルートに移動
cd ~/bin/c/raylib_projects/raylib_tetris

# ルートから build 内の実行ファイルを指定して起動
./build/linux/raylib_tetris
これだけで、Sounds/ や Font/ の相対パスが正しく認識され、音がクリアに鳴るようになります！

方法2: CMakeLists.txt の make run ターゲットを使う
さきほど作成した CMakeLists.txt のカスタムターゲット make run は、以下のように作業ディレクトリをプロジェクトルート（CMAKE_CURRENT_SOURCE_DIR）に固定する設定になっています。

CMake
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
そのため、今後は直接実行ファイルを踏むのではなく、ルートから make run を使って起動すればこの問題は絶対に起きなくなります。
