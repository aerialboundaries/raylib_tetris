NAME        := a.out
CC          := gcc
CFLAGS      := -g -Wall -Wextra -std=c99 -pedantic-errors -MMD -MP


# WSL (Linux) 用のライブラリ
LIBS_WSL    := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# [追加] Windows用raylibのパス定義
RAYLIB_WIN_DIR := /mnt/c/raylib/w64devkit
CFLAGS_WIN     := -I $(RAYLIB_WIN_DIR)/include

OBJDIR      := obj
SRCS        := $(wildcard *.c)

# 通常ビルド用のオブジェクトと依存関係
OBJS        := $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS        := $(OBJS:.o=.d)

# Windowsビルド用の中間ファイル（混ざらないようにディレクトリを分けます）
OBJDIR_WIN  := obj_win

OBJS_WIN    := $(SRCS:%.c=$(OBJDIR_WIN)/%.o)
DEPS_WIN    := $(OBJS_WIN:.o=.d)

.PHONY: all
all: $(NAME)


# ----------------------------------------------------
# 1. WSL (Linux) 用ビルド
# ----------------------------------------------------
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_WSL)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# ----------------------------------------------------
# 依存関係ファイルの読み込み
# ----------------------------------------------------
-include $(DEPS)
-include $(DEPS_WIN)

# ----------------------------------------------------
# ユーティリティターゲット
# ----------------------------------------------------
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(OBJDIR_WIN)

.PHONY: fclean
fclean: clean
	rm -f $(NAME) $(NAME).exe


.PHONY: re
re: fclean all


# デバッグ用ターゲット

.PHONY: debug
debug: CFLAGS += -g -fsanitize=integer -fsanitize=address -fsanitize=leak
debug: re

# 動作確認用
.PHONY: test run
test: all
	./$(NAME)

run: all
	./$(NAME)

# --- 通常の Linux用 Makefile の末尾に追記 ---
.PHONY: win
win:
	make -f Makefile.win


