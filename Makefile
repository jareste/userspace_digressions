NAME = initramfs.cpio
INIT_BIN = init
ROOTFS = /tmp/rootfs
KERNEL = /boot/vmlinuz-$(shell uname -r)
BUSYBOX = /tmp/busybox/_install/bin/busybox

CC = gcc
CFLAGS = -static -Wall -Wextra -Werror -DDEBUG
RM = rm -rf

DIRS = bin dev proc sys etc run tmp
COMMANDS = sh fsck cat

# SRCS = init.c

#########
FILES = main ud_hostname ud_mount ud_cli

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs
#########

OBJ_DIR = objs

#########
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -Isrcs -c $< -o $@



all: .gitignore ibusybox $(NAME)

$(INIT_BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^


$(NAME): $(INIT_BIN) create_dirs create_commands
	$(RM) $@
	echo "jareste-" > $(ROOTFS)/etc/hostname
	cp $(INIT_BIN) $(ROOTFS)/
	cp $(BUSYBOX) $(ROOTFS)/bin/ || true
# 	(cd $(ROOTFS)/bin && ln -sf busybox sh)
# 	(cd $(ROOTFS)/bin && ln -sf busybox fsck)
# 	(cd $(ROOTFS)/bin && ln -sf busybox cat)
	(cd $(ROOTFS) && find . -print0 | cpio --null -ov --format=newc > $(NAME))
	cpio -t < $(ROOTFS)/$(NAME) | head

create_dirs:
	mkdir -p $(addprefix $(ROOTFS)/, $(DIRS))

create_commands:
	@for cmd in $(COMMANDS); do \
		if [ ! -f $(ROOTFS)/bin/$$cmd ]; then \
			ln -s busybox $(ROOTFS)/bin/$$cmd; \
		fi; \
	done

run: $(NAME)
	qemu-system-x86_64 -kernel $(KERNEL) -initrd $(ROOTFS)/$(NAME) -append "console=ttyS0 init=/init" -nographic

clean:
	@echo "[+] Limpiando archivos temporales..."
	$(RM) $(INIT_BIN) $(SRCS) $(ROOTFS)

fclean: clean
	@echo "[+] Eliminando initramfs..."
	$(RM) $(NAME)

ibusybox:
	@if [ ! -f $(BUSYBOX) ]; then \
		echo "BusyBox not found, building it..."; \
		sh install_busybox.sh; \
	else \
		echo "BusyBox already built."; \
	fi

.gitignore:
	@if [ ! -f .gitignore ]; then \
		echo ".gitignore not found, creating it..."; \
		echo "$(NAME)" > .gitignore; \
		echo "$(INIT_BIN)" >> .gitignore \
		echo .gitignore >> .gitignore \
	else \
		echo ".gitignore already exists."; \
	fi


re: fclean all

.PHONY: all clean fclean re run create_dirs
