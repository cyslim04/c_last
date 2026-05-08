CC = gcc
SQLITE_ROOT = C:/Users/yce/.codegeex/mamba/envs/codegeex-agent/Library
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -I$(SQLITE_ROOT)/include
LDFLAGS = -L$(SQLITE_ROOT)/lib -lsqlite3 -lws2_32
SRC = src/server.c src/utils/sha256.c
TARGET = trust_work_backend

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

run: $(TARGET)
	powershell -ExecutionPolicy Bypass -File .\run.ps1

clean:
	del /Q $(TARGET).exe 2>NUL || exit 0
