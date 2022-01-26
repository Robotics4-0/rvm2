
# install prefix either /usr or /usr/local on most unix systems
PREFIX ?= /usr

# comment out to disable SDL GUI
SDL_CFLAGS := -DHAVE_SDL $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf)
SDL_LDLIBS := $(shell sdl2-config --libs)   $(shell pkg-config --libs   SDL2_ttf) -lGL -lGLU -lpng

# comment out to disable Curses GUI
CURSES_CLFAGS := -DHAVE_CURSES
CURSES_LDLIBS := -lcurses

# comment out to disable LinuxCNC / Machinekit HAL component functionality
#HAL_CFLAGS := -DHAVE_HAL -I/usr/include/linuxcnc -DRTAPI -DTHREAD_FLAVOR_ID=RTAPI_POSIX_ID
#HAL_LDLIBS := -llinuxcnchal -lprotobuf

# comment out to disable ZeroMQ functionality
#ZMQ_CFLAGS := -DHAVE_ZMQ
#ZMQ_LDLIBS := -l:libzmq.so.5

# comment out to disable Mosquitto functionality
#MOSQ_CFLAGS := -DHAVE_MOSQUITTO
#MOSQ_LDLIBS := -lmosquitto

# comment out to disable trajectory calculation functionality
TRAJGEN_CFLAGS := -DHAVE_TRAJGEN
TRAJGEN_LDLIBS :=

CFLAGS  += $(SDL_CFLAGS) $(CURSES_CLFAGS) $(HAL_CFLAGS) $(ZMQ_CFLAGS) $(MOSQ_CFLAGS) $(TRAJGEN_CFLAGS) -O2 -g -Wall
LDLIBS  += $(SDL_LDLIBS) $(CURSES_LDLIBS) $(HAL_LDLIBS) $(ZMQ_LDLIBS) $(MOSQ_LDLIBS) $(TRAJGEN_LDLIBS) -lm
LDFLAGS += -Wl,--as-needed -lpaho-mqtt3c 

SUBDIRS = paho-mqtt-c/src



all: rvm2


%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $+


rvm2: rvm2.o trajgen.o mqtt_handler.o
	$(CC) -o $@ $+ $(LDFLAGS) $(LDLIBS)

rvm2.1: rvm2
	help2man -N ./$+ > $@

install: rvm2
	strip rvm2
	mkdir -p $(INSTALL_PREFIX)$(PREFIX)/bin
	install -m 0755 rvm2         $(INSTALL_PREFIX)$(PREFIX)/bin
	mkdir -p $(INSTALL_PREFIX)$(PREFIX)/share/icons/hicolor/512x512/apps
	install -m 0664 rvm2.png     $(INSTALL_PREFIX)$(PREFIX)/share/icons/hicolor/512x512/apps
	mkdir -p $(INSTALL_PREFIX)$(PREFIX)/share/applications
	install -m 0664 rvm2.desktop $(INSTALL_PREFIX)$(PREFIX)/share/applications

uninstall:
	rm $(INSTALL_PREFIX)$(PREFIX)/bin/rvm2
	rm $(INSTALL_PREFIX)$(PREFIX)/share/icons/hicolor/512x512/apps/rvm2.png
	rm $(INSTALL_PREFIX)$(PREFIX)/share/applications/rvm2.desktop

clean:
	rm -f *.o rvm2

pull:
	git pull origin master --rebase

push:
	git push origin HEAD:master

test:
	TRAVIS_DEBIAN_DISTRIBUTION=sid; wget -O- http://travis.debian.net/script.sh | sh -
