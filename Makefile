SDK_DIR := c_sdk_220203/MinGW
CC := $(SDK_DIR)/bin/gcc.exe
TARGET := maze.exe

SRCS := src/main.c src/game.c src/render.c src/model.c src/config.c
OBJS := $(SRCS:.c=.o)

CFLAGS := -fdiagnostics-color=always -g -I$(SDK_DIR)/include/SDL2
LDFLAGS := -L$(SDK_DIR)/lib
LDLIBS := -lSDL2 -lSDL2_image -lopengl32 -lglu32

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	@if exist src\main.o del /F /Q src\main.o
	@if exist src\game.o del /F /Q src\game.o
	@if exist src\render.o del /F /Q src\render.o
	@if exist src\model.o del /F /Q src\model.o
	@if exist src\config.o del /F /Q src\config.o
	@if exist $(TARGET) del /F /Q $(TARGET)
