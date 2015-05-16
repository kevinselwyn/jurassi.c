NAME    := jurassi
BIN_DIR := /usr/local/bin

all: build

build: $(NAME).c
	gcc -o $(NAME)c $< -lao
	printf "magic.wav" >> $(NAME)c
	cat magic.wav >> $(NAME)c

test: build
	./$(NAME)c

install: $(NAME)c
	install -m 0755 $(NAME)c $(BIN_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(NAME)c

clean:
	rm -f $(NAME)c