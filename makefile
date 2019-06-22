BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

PREFIX = /usr

CPPFLAGS += -fno-rtti -fno-exceptions  -fno-threadsafe-statics \
 -pipe -Wconversion-null -O2
CC = gcc
CFLAGS += -O3 -flto
CPP = gcc

all :  $(BUILD_DIR) $(BUILD_DIR)/evdoublebind $(BUILD_DIR)/evdoublebind-inspector $(BUILD_DIR)/evdoublebind-make-config

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(BUILD_DIR)/evdoublebind: $(SRC_DIRS)/evdoublebind.c
	$(CC) $(CFLAGS) $(SRC_DIRS)/evdoublebind.c -o $@

$(BUILD_DIR)/evdoublebind-inspector: $(SRC_DIRS)/inspector.cc
	$(CPP) $(CPPFLAGS) $(SRC_DIRS)/inspector.cc -o $@ -lxkbcommon

$(BUILD_DIR)/evdoublebind-make-config: $(SRC_DIRS)/make_config.cc
	$(CPP) $(CPPFLAGS) $(SRC_DIRS)/make_config.cc -o $@ -lxkbcommon

$(BUILD_DIR)/%.cc.o: %.cc
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	strip $(BUILD_DIR)/*
	cp -f $(BUILD_DIR)/* $(DESTDIR)$(PREFIX)/bin/
	chmod 'g+s' $(DESTDIR)$(PREFIX)/bin/evdoublebind

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/evdoublebind $(DESTDIR)$(PREFIX)/bin/evdoublebind-inspector $(DESTDIR)$(PREFIX)/bin/evdoublebind-make-config

.PHONY: musl-static

musl-static: CC = musl-gcc -O3 -static -Wl,-z,noseparate-code
musl-static: all

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
