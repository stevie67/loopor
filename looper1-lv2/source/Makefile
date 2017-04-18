#!/usr/bin/make -f
# Makefile for looper1.lv2 #
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
build: looper1

# --------------------------------------------------------------
# looper1 build rules

looper1: looper1.lv2/looper$(LIB_EXT) looper1.lv2/manifest.ttl

looper1.lv2/looper$(LIB_EXT): looper.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lm $(SHARED) -o $@

looper1.lv2/manifest.ttl: looper1.lv2/manifest.ttl.in
	sed -e "s|@LIB_EXT@|$(LIB_EXT)|" $< > $@

# --------------------------------------------------------------

clean:
	rm -f looper1.lv2/looper$(LIB_EXT) looper1.lv2/manifest.ttl

# --------------------------------------------------------------

install: build
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/looper1.lv2

	install -m 644 looper1.lv2/*.so  $(DESTDIR)$(PREFIX)/lib/lv2/looper1.lv2/
	install -m 644 looper1.lv2/*.ttl $(DESTDIR)$(PREFIX)/lib/lv2/looper1.lv2/

# --------------------------------------------------------------
