# dev platform

###########################################################

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
$(error please specify PLATFORM, eg. PLATFORM=dev make)
endif

###########################################################

all: minui minarch

early:
	@echo "No early build steps needed for dev platform"

minui:
	cd ../all/minui && PLATFORM=dev make

minarch:
	cd ../all/minarch && PLATFORM=dev make

clean:
	cd ../all/minui && PLATFORM=dev make clean
	cd ../all/minarch && PLATFORM=dev make clean

test: minui
	@echo "Testing minui..."
	@./build/dev/minui.elf || echo "Note: minui requires display and input"

.PHONY: all early minui minarch clean test
