CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -ldl -lm

SRC     = src/main.c src/game.c src/render.c
OBJ     = $(SRC:.c=.o)
TARGET  = splash

PLAYERS = players/player1.so players/player2.so players/player3.so players/player4.so

.PHONY: all clean players run

all: $(TARGET) players

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

players: $(PLAYERS)

players/%.so: players/%.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

run: all
	./$(TARGET) players/player1.so players/player2.so players/player3.so players/player4.so

clean:
	rm -f $(OBJ) $(TARGET) $(PLAYERS)
