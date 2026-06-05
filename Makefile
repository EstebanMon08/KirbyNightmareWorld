# ══════════════════════════════════════════════════════
# Makefile — Kirby Dreamland (ncurses)
# Uso:  make          → compila
#       make run      → compila y ejecuta
#       make clean    → borra los .o y el ejecutable
# ══════════════════════════════════════════════════════

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS     = -lncursesw -lpthread -lm

TARGET = kirby

SRCS = main.cpp \
       sprites.cpp \
       level.cpp \
       enemy.cpp \
       projectile.cpp \
       player.cpp \
       render.cpp

OBJS = $(SRCS:.cpp=.o)

# ── Regla principal ──
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# ── Compilar cada .cpp en su .o ──
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Dependencias de headers ──
main.o:        main.cpp       sync.h player.h enemy.h projectile.h level.h render.h
sprites.o:     sprites.cpp    sprites.h
level.o:       level.cpp      sync.h level.h sprites.h
enemy.o:       enemy.cpp      sync.h enemy.h player.h projectile.h level.h sprites.h
projectile.o:  projectile.cpp sync.h projectile.h enemy.h player.h sprites.h
player.o:      player.cpp     sync.h player.h enemy.h projectile.h level.h sprites.h
render.o:      render.cpp     sync.h render.h player.h enemy.h projectile.h level.h sprites.h

# ── Targets extra ──
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: run clean
