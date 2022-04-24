PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

LDFLAGS += -liio -lpthread
ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
endif

SRC = test.c gyrophus.c gyrophus_device_thread.c
BIN = $(patsubst %.c,%.o,$(SRC))

all: 	$(BIN)
		$(CC) $(BIN) -o $@ $(LDFLAGS)

%.o:	$(PROJECT_ROOT)%.c
		$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
		rm -f $(BIN)
