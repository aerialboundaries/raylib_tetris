.PHONY: all linux win win-native web web-win run run-win run-web re-linux re-win re-web clean fclean info help


# デフォルトターゲット (単に make と打った場合は Linux ビルド)
all: linux


# OS環境の自動判定 (Windows cmd/PowerShell か WSL/Linux か)
ifeq ($(OS),Windows_NT)
    IS_WIN := 1
    RM := rmdir /s /q
else
    IS_WIN := 0
    RM := rm -rf
endif

# ==============================================================================
# 1. WSL (Linux) 用ビルド
# ==============================================================================

# Linux Native ビルド

linux:
	@echo "=== [Linux Native] ビルドを開始します ==="
	cmake --preset linux
	cmake --build build/linux

	@chmod +x build/linux/* 2>/dev/null || true
	@echo "=== [Linux Native] ビルド完了 ==="

# WSL から Windows 用 .exe にクロスコンパイル (+ 実行権限の付与)
win:
	@echo "=== [Windows Cross-Compile] ビルドを開始します ==="

	cmake --preset win
	cmake --build build/win
	@chmod +x build/win/*.exe 2>/dev/null || true

	@echo "=== [Windows Cross-Compile] ビルド完了 ==="

# WSL から Emscripten (Web) 用ビルド
web:
	@echo "=== [Web (Emscripten / WSL)] ビルドを開始します ==="
	emcmake cmake --preset web
	cmake --build build/web
	@echo "=== [Web (Emscripten / WSL)] ビルド完了 ==="


# ==============================================================================
# 2. Windows ネイティブ (cmd / PowerShell / w64devkit) 用ビルド
# ==============================================================================

# Windows ネイティブビルド (w64devkit / MinGW)
win-native:
	@echo "=== [Windows Native] ビルドを開始します ==="
	cmake --preset win-native
	cmake --build build/win-native
	@echo "=== [Windows Native] ビルド完了 ==="


# Windows ネイティブ環境からの Web(Emscripten) ビルド
web-win:
	@echo "=== [Web (Emscripten / Windows)] ビルドを開始します ==="
	call emcmake cmake --preset web-win || emcmake cmake --preset web-win
	cmake --build build/web-win
	@echo "=== [Web (Emscripten / Windows)] ビルド完了 ==="

# ==============================================================================
# 3. 実行 (Run) 機能
# ==============================================================================

# Linux版のビルド ＆ 実行
run: linux

	cmake --build build/linux --target run

# Windows版(.exe)をビルド ＆ WSLから直接Windows側で起動
run-win: win
	@echo "=== Windows 実行ファイルを起動します ==="
	@cmd.exe /c "start /b $$(wslpath -w build/win/*.exe)" 2>/dev/null || ./build/win/*.exe

# Web版をビルド ＆ Pythonローカルサーバーで起動 (http://localhost:8000)
run-web: web
	@echo "=== Web版ローカルサーバーを起動します (http://localhost:8000) ==="
	@python3 -m http.server 8000 --directory build/web || python -m http.server 8000 --directory build/web

# ==============================================================================

# 4. リコンパイル (Recompile / フルリビルド) 機能
# ==============================================================================

re-linux:
	@echo "=== [Linux] クリーンリビルド ==="
	$(RM) build/linux

	$(MAKE) linux


re-win:
	@echo "=== [Windows Cross] クリーンリビルド ==="
	$(RM) build/win
	$(MAKE) win


re-web:
	@echo "=== [Web] クリーンリビルド ==="
	$(RM) build/web
	$(MAKE) web

# ==============================================================================
# 5. クリーンアップ・情報表示・ヘルプ
# ==============================================================================

# 一部のビルド生成物を削除

clean:
	@echo "=== 生成物を整理します ==="
	$(RM) build/linux build/win build/web build/web-win build/win-native 2>/dev/null || true

# build ディレクトリ全体を完全削除 (fclean)
fclean:

	@echo "=== [Full Clean] build ディレクトリを完全に削除します ==="
	$(RM) build

# ビルド済みバイナリのサイズ・更新日時を表示
info:
	@echo "=== ビルド成果物一覧 ==="
	@if [ -d "build" ]; then ls -lh build/*/* 2>/dev/null || dir /s build; else echo "build ディレクトリが存在しません。"; fi

# 使い方ヘルプ
help:
	@echo "利用可能なターゲット一覧:"
	@echo "  make linux      : WSL Native Linux ビルド"

	@echo "  make win        : WSLからのWindows(.exe)クロスコンパイル (+ chmod +x)"
	@echo "  make win-native : Windowsネイティブ(w64devkit)用ビルド"

	@echo "  make web        : WSL環境でのWeb(Emscripten) ビルド"
	@echo "  make web-win    : Windows環境でのWeb(Emscripten) ビルド"
	@echo "  --------------------------------------------------------"
	@echo "  make run        : Linux版をビルドして実行"

	@echo "  make run-win    : Windows版(.exe)をビルドしてWSLから直接起動"
	@echo "  make run-web    : Web版をビルドして http://localhost:8000 で起動"
	@echo "  --------------------------------------------------------"
	@echo "  make re-linux   : Linux版をフルリビルド"
	@echo "  make re-win     : Windows(.exe)版をフルリビルド"
	@echo "  make re-web     : Web版をフルリビルド"
	@echo "  --------------------------------------------------------"
	@echo "  make clean      : ビルド生成物の整理"
	@echo "  make fclean     : build ディレクトリの完全削除"
	@echo "  make info       : ビルド成果物のサイズ・パス一覧を表示"
