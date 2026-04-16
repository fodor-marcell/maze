SDK_DIR := c_sdk_220203/MinGW
CC := $(SDK_DIR)/bin/gcc.exe
TARGET := maze.exe

SRCS := src/main.c src/config.c
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
	$(RM) $(OBJS) $(TARGET)
