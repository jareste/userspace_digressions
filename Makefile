NAME = initramfs.cpio
INIT_BIN = init
ROOTFS = /tmp/rootfs
KERNEL = /boot/vmlinuz-$(shell uname -r)
BUSYBOX = /tmp/busybox/_install/bin/busybox

CC = gcc
CFLAGS = -static -Wall -Wextra -Werror -DDEBUG
RM = rm -rf

DIRS = bin dev proc sys etc run tmp sbin lib lib/modules
COMMANDS = sh fsck cat mount swapon getty ls login ifconfig ip udhcpc ping insmod syslogd crond

#########
FILES = main ud_hostname ud_mount ud_cli ud_consoles ud_network ud_modules ft_list ud_daemon ud_signals ud_log

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
	cp $(BUSYBOX) $(ROOTFS)/bin/. || true
	cp /sbin/fsck.ext4 $(ROOTFS)/sbin/fsck.ext4 || true
	mv $(ROOTFS)/bin/crond $(ROOTFS)/sbin/crond || true
	dos2unix files/*
	cp files/fstab $(ROOTFS)/etc/fstab
	cp files/passwd $(ROOTFS)/etc/passwd
	cp /usr/share/zoneinfo/Europe/Madrid $(ROOTFS)/etc/localtime
	cp files/net.conf $(ROOTFS)/etc/net.conf
	cp files/modules $(ROOTFS)/etc/modules
	cp files/daemons.conf $(ROOTFS)/etc/daemons.conf
	cc files/cli_client.c -o $(ROOTFS)/bin/initctl -static
	mkdir -p $(ROOTFS)/lib/modules/
	cp /lib/modules/$(shell uname -r)/kernel/drivers/net/ethernet/intel/e1000/e1000.ko $(ROOTFS)/lib/modules/
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
	qemu-system-x86_64 -kernel $(KERNEL) -initrd $(ROOTFS)/$(NAME) -append "console=ttyS0 init=/init noapic nolapic" -nographic -nic user,model=e1000

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
		echo "$(INIT_BIN)" >> .gitignore; \
		echo "$(OBJ_DIR)" >> .gitignore; \
		echo .gitignore >> .gitignore; \
	else \
		echo ".gitignore already exists."; \
	fi


re: fclean all

.PHONY: all clean fclean re run create_dirs ibusybox create_commands
