RM=rf -f

SRCDIR=src
HEADERS=include
OUTPUTDIR=bin
BUILDDIR=build

INC=-I ./$(HEADERS) -I/usr/X11R6/include -I/usr/local/include

CC=clang
CFLAGS=-std=gnu11 -pedantic-errors $(INC) -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -g
LDFLAGS=-L/usr/X11R6/lib -lGL -lGLU -lglut -lm


MAIN_SRC=ex1

SOURCES=$(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:%.c=%.o))
TARGET=$(OUTPUTDIR)/ex1

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
