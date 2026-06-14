CC     = gcc
CFLAGS = -Wall -Wextra -g
SRCS   = main.c chef.c waiter.c diner.c ipc.c
TARGET = canteen

all: $(TARGET)

$(TARGET): $(SRCS) shared.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lrt -lpthread

clean:
	rm -f $(TARGET)
	@echo "Dọn IPC còn sót (nếu có):"
	-rm -f /dev/shm/canteen_shm
	-rm -f /dev/shm/sem.canteen_*

.PHONY: all clean
