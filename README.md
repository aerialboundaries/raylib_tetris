#1. ビルド用の構成（Makefile生成）
build フォルダの作成と Makefile や compile_commands.json の生成を一括で行います。

Bash
cmake -B build
2
2. コンパイル（ビルドの実行）
-S .（ソースの場所）や -B build を自動で認識してコンパイルします。

Bash
cmake --build build
3
3. 実行（`make run` 相当）
作成しておいた run ターゲットを呼び出します。

Bash
cmake --build build --target run
なぜ cmake -B build がおすすめなのか？
cd 移動の手間がなくなる
cd build して cmake .. し、実行時に cd .. で戻ってくる…といった移動の必要が一切ありません。

Neovim やターミナルとの相性が良い
常にプロジェクトのルートディレクトリに居続けられるため、Neovim 内でターミナルを開いてコマンドを打ったり、キーバインドでビルドコマンドを呼び出したりするのが非常に簡単になります。 raylib_tetris
