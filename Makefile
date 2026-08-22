.PHONY: all linux win win-native web web-win run run-win run-native run-web re-linux re-win re-web clean fclean info help

# OS環境の自動判定 (Windows cmd/PowerShell か WSL/Linux か)
ifeq ($(OS),Windows_NT)
    IS_WIN := 1
    RM := rmdir /s /q
else
    IS_WIN := 0

    RM := rm -rf
endif

# デフォルトターゲット (単に make と打った場合)
all:
ifeq ($(IS_WIN),1)
	$(MAKE) win-native
else

	$(MAKE) linux
endif


# ==============================================================================
# 1. WSL (Linux) 用ビルド
# ==============================================================================

linux:
	@echo "=== [Linux Native] ビルドを開始します ==="

	cmake --preset linux

	cmake --build build/linux
	@chmod +x build/linux/* 2>/dev/null || true
	@echo "=== [Linux Native] ビルド完了 ==="

win:
	@echo "=== [Windows Cross-Compile] ビルドを開始します ==="
	cmake --preset win
	cmake --build build/win
	@chmod +x build/win/*.exe 2>/dev/null || true
	@echo "=== [Windows Cross-Compile] ビルド完了 ==="

web:
	@echo "=== [Web (Emscripten / WSL)] ビルドを開始します ==="
	emcmake cmake --preset web
	emmake make -C build/web

	@echo "=== [Web (Emscripten / WSL)] ビルド完了 ==="

# ==============================================================================
# 2. Windows ネイティブ (w64devkit) 用ビルド
# ==============================================================================

win-native:
	@echo "=== [Windows Native] ビルドを開始します ==="
	cmake --preset win-native
	cmake --build build/win-native
	@echo "=== [Windows Native] ビルド完了 ==="

web-win:
	@echo "=== [Web (Emscripten / Windows)] ビルドを開始します ==="
	emcmake cmake --preset web
	cmake --build build/web 
	@echo "=== [Web (Emscripten / Windows)] ビルド完了 ==="


# ==============================================================================
# 3. 実行 (Run) 機能
# ==============================================================================

# OS自動判別の run ターゲット
run:
ifeq ($(IS_WIN),1)
	$(MAKE) run-native

else
	$(MAKE) linux
	cmake --build build/linux --target run
endif

run-native: win-native
	@echo "=== [Windows Native] 実行ファイルを起動します ==="
	cmake --build build/win-native --target run

run-win: win
	@echo "=== Windows 実行ファイルを起動します ==="
	@cmd.exe /c "start /b $$(wslpath -w build/win/*.exe)" 2>/dev/null || ./build/win/*.exe


run-web: web
	@echo "=== Web版ローカルサーバーを起動します (http://localhost:8000) ==="
	@python3 -m http.server 8000 --directory build/web || python -m http.server 8000 --directory build/web

# ==============================================================================
# 4. リコンパイル 機能
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
# 5. クリーンアップ・ヘルプ
# ==============================================================================

clean:

	@echo "=== 生成物を整理します ==="

	$(RM) build/linux build/win build/web build/web-win build/win-native 2>/dev/null || true

fclean:
	@echo "=== [Full Clean] build ディレクトリを完全に削除します ==="
	$(RM) build

info:
	@echo "=== ビルド成果物一覧 ==="
	@if [ -d "build" ]; then ls -lh build/*/* 2>/dev/null || dir /s build; else echo "build ディレクトリが存在しません。"; fi


help:
	@echo "利用可能なターゲット一覧:"
	@echo "  make run        : OSを自動判定してビルド＆実行 (WinならNative、WSLならLinux)"
	@echo "  make win-native : Windowsネイティブ(w64devkit)用ビルド"
	@echo "  make run-native : Windowsネイティブ(w64devkit)用ビルド＆実行"
	@echo "  make linux      : WSL Native Linux ビルド"

	@echo "  make win        : WSLからのWindows(.exe)クロスコンパイル"
	@echo "  make clean      : ビルド生成物の整理"
	@echo "  make fclean     : build ディレクトリの完全削除"
