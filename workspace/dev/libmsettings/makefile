# Stub libmsettings for dev platform
# Create an empty library and copy the header
# The actual functions are implemented in platform.c

# Set default PREFIX if not provided
PREFIX ?= /tmp/dev_prefix

all:
	mkdir -p $(PREFIX)/include $(PREFIX)/lib
	cp ../platform/msettings.h $(PREFIX)/include/
	# Create empty stub library (functions are in platform.c)
	ar rcs $(PREFIX)/lib/libmsettings.a

clean:
	rm -f $(PREFIX)/include/msettings.h $(PREFIX)/lib/libmsettings.a

.PHONY: all clean
