NAME = initramfs.cpio
INIT_BIN = init
ROOTFS = /tmp/rootfs
KERNEL = /boot/vmlinuz-$(shell uname -r)
BUSYBOX = /tmp/busybox/_install/bin/busybox

CC = gcc
CFLAGS = -static
RM = rm -rf
SRCS = init.c

all: .gitignore ibusybox $(NAME)

$(INIT_BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^


$(NAME): $(INIT_BIN)
	$(RM) $(ROOTFS) $@
	mkdir -p $(ROOTFS)/bin
	mkdir -p $(ROOTFS)/dev
	mkdir -p $(ROOTFS)/proc
	mkdir -p $(ROOTFS)/sys
	cp $(INIT_BIN) $(ROOTFS)/
	cp $(BUSYBOX) $(ROOTFS)/bin/ || true
	(cd $(ROOTFS)/bin && ln -sf busybox sh)
	(cd $(ROOTFS)/bin && ln -sf busybox fsck)
	(cd $(ROOTFS) && find . -print0 | cpio --null -ov --format=newc > $(NAME))
	cpio -t < $(ROOTFS)/$(NAME) | head

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

.PHONY: all clean fclean re run