CC = gcc
NAME = git-check
SRC = src/$(NAME).c
EXEC = bin/$(NAME)
INSTALL_PATH = /usr/local/bin
MAN = $(NAME).1
MAN_INSTALL_PATH = /usr/local/share/man/man1

RESET = \033[0m
YELLOW = \033[0;33m
GREEN = \033[0;32m
RED = \033[0;31m
BLUE = \033[1;34m

all: install

compile: clear
	mkdir -p bin
	$(CC) $(SRC) -o $(EXEC)
	printf "$(YELLOW)•$(RESET) Compiled\n"

clear:
	rm -rf bin
	printf "$(YELLOW)•$(RESET) Removed executable and objects\n"

install: compile
	sudo cp $(EXEC) $(INSTALL_PATH)
	sudo mkdir -p $(MAN_INSTALL_PATH)
	sudo cp man/$(MAN) $(MAN_INSTALL_PATH)
	sudo mandb >/dev/null 2>&1
	printf "$(GREEN)•$(RESET) Installed \033[1;32m$(NAME)$(RESET) to $(BLUE)$(INSTALL_PATH)$(RESET)\n"

uninstall:
	sudo rm -f $(INSTALL_PATH)/$(NAME)
	sudo rm -f $(MAN_INSTALL_PATH)/$(MAN)
	printf "$(RED)•$(RESET) Uninstalled \033[1;32m$(NAME)$(RESET) from $(BLUE)$(INSTALL_PATH)$(RESET)\n"

.PHONY: all compile clear install uninstall
.SILENT: all compile clear install uninstall
