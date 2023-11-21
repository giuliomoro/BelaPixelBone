#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += examples/rgb-test
TARGETS += examples/matrix-test
TARGETS += examples/tile-test
TARGETS += examples/scroll
TARGETS += examples/clear
TARGETS += examples/game_of_life
TARGETS += examples/clock
TARGETS += examples/binary_clock
# TARGETS += examples/test
TARGETS += examples/2048
# TARGETS += examples/fade-test
# TARGETS += examples/fire
# TARGETS += network/udp-rx
# TARGETS += network/opc-rx

PIXELBONE_OBJS = pixel.o gfx.o matrix.o pru.o util.o
PIXELBONE_LIB := libpixelbone.a 

all: $(TARGETS) $(PIXELBONE_LIB) ws281x.bin

#CXXFLAGS += -I /root/Bela/include
#LDLIBS += /root/Bela/lib/libbela.a
CFLAGS += \
	-std=c99 \
	-W \
	-Wall \
	-D_BSD_SOURCE \
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \
	-I. \
	-O2 \
	-mtune=cortex-a8 \
	-march=armv7-a \

LDFLAGS += \

LDLIBS += \
	-lpthread \

export CROSS_COMPILE:=


$(PIXELBONE_LIB): $(PIXELBONE_OBJS)  #let's link library files into a static library
	ar rcs $(PIXELBONE_LIB) $(PIXELBONE_OBJS)

libs: $(PIXELBONE_LIB) $(APP_LOADER_LIB)

LDLIBS += $(PIXELBONE_LIB)

#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ./am335x/app_loader
APP_LOADER_LIB := $(APP_LOADER_DIR)/lib/libprussdrv.a
CFLAGS += -I$(APP_LOADER_DIR)/include
LDLIBS += $(APP_LOADER_LIB)

#####
#
# The TI PRU assembler looks like it has macros and includes,
# but it really doesn't.  So instead we use cpp to pre-process the
# file and then strip out all of the directives that it adds.
# PASM also doesn't handle multiple statements per line, so we
# insert hard newline characters for every ; in the file.
#
PASM_DIR ?= ./am335x/pasm
PASM := $(PASM_DIR)/pasm

%.bin: %.p $(PASM)
	$(CPP) - < $< | perl -p -e 's/^#.*//; s/;/\n/g; s/BYTE\((\d+)\)/t\1/g' > $<.i
	$(PASM) -V3 -b $<.i $(basename $@)
	$(RM) $<.i

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


$(foreach O,$(TARGETS),$(eval $O: $O.o $(APP_LOADER_LIB)))

$(TARGETS):$(PIXELBONE_LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)


.PHONY: clean

clean:
	rm -rf \
		**/*.o \
		*.o \
		ws281x.hp.i \
		.*.o.d \
		*~ \
		$(INCDIR_APP_LOADER)/*~ \
		$(TARGETS) \
		$(PIXELBONE_LIB)\
		*.bin \

###########
# 
# PRU Libraries and PRU assembler are build from their own trees.
# 
$(APP_LOADER_LIB):
	$(MAKE) -C $(APP_LOADER_DIR)/interface

$(PASM):
	$(MAKE) -C $(PASM_DIR)

# Include all of the generated dependency files
-include .*.o.d
