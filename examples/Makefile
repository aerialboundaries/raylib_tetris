NAME        := a.out
CC          := gcc
CFLAGS      := -Wall -Wextra -std=c99 -pedantic-errors -MMD -MP


# raylibに必要なリンクオプション

LIBS        := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


OBJDIR      := obj

# 【修正ポイント】カレントディレクトリ直下の.cファイルだけをワイルドカードで安全に取得

SRCS        := $(wildcard *.c)

# オブジェクトファイルと依存関係ファイルのパスをobj/以下に変更
OBJS        := $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS        := $(OBJS:.o=.d)

.PHONY: all
all: $(NAME)


# リンク工程
$(NAME): $(OBJS)

	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


# コンパイル工程（ディレクトリ作成も同時に行う）
# ターゲットに直接カレントディレクトリのファイルを結びつけます
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# 依存関係ファイルの読み込み
-include $(DEPS)


.PHONY: clean
clean:
	rm -rf $(OBJDIR)


.PHONY: fclean
fclean: clean
	rm -f $(NAME)

.PHONY: re
re: fclean all

# デバッグ用ターゲット
.PHONY: debug
debug: CFLAGS += -g -fsanitize=integer -fsanitize=address -fsanitize=leak

debug: re


# 動作確認用
.PHONY: test

test: all
	./$(NAME)

.PHONY: run

run: all
	./$(NAME)
