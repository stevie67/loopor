#!/usr/bin/make -f
# Makefile for loopor.lv2 #
# ----------------------- #
# Created by falkTX, Stevie
#

include Makefile.mk

# --------------------------------------------------------------

PREFIX  ?= /usr/local
DESTDIR ?=

# --------------------------------------------------------------
# Default target is to build all plugins

all: build

# --------------------------------------------------------------
# loopor build rules

build: loopor.lv2/loopor$(LIB_EXT) loopor.lv2/manifest.ttl

loopor.lv2/loopor$(LIB_EXT): loopor.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lm $(SHARED) -o $@

loopor.lv2/manifest.ttl: loopor.lv2/manifest.ttl.in
	sed -e "s|@LIB_EXT@|$(LIB_EXT)|" $< > $@

# --------------------------------------------------------------

clean:
	rm -f loopor.lv2/looper$(LIB_EXT) loopor.lv2/manifest.ttl

# --------------------------------------------------------------

install: build
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/loopor.lv2

	install -m 644 loopor.lv2/*.so  $(DESTDIR)$(PREFIX)/lib/lv2/loopor.lv2/
	install -m 644 loopor.lv2/*.ttl $(DESTDIR)$(PREFIX)/lib/lv2/loopor.lv2/

# --------------------------------------------------------------
