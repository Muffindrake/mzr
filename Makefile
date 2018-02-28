PKGC_CFLAGS := `pkg-config glib-2.0 pango gtk+-3.0 vte-2.91 --cflags`
PKGC_LIBS   := `pkg-config glib-2.0 pango gtk+-3.0 vte-2.91 --libs`
CFLAGS := $(CFLAGS) $(PKGC_CFLAGS) -std=c11 -Wall -Wextra -Wpedantic -pipe
LDFLAGS ?= -Wl,-O1 -Wl,--as-needed -Wl,-flto
LDFLAGS := $(LDFLAGS) $(PKGC_LIBS)

VERSION := \"dog\"
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

OPTLEVEL ?= -O3 -march=native -flto -ffast-math
prefix ?= /usr/local

%.o: %.c
	$(CC) $(CFLAGS) $(OPTLEVEL) -D VERSION=$(VERSION) -c $<

mzr: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OPTLEVEL) $(OBJS) $(LDFLAGS)

.PHONY: all
all: mzr

.PHONY: install
install: all
	install -m 0755 mzr $(prefix)/bin

.PHONY: clean
clean:
	$(RM) mzr $(OBJS)
