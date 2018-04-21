CC = gcc

INCLUDES = -I$(inc_dir)

CCFLAGS = -Wall -ggdb --std=c99 $(shell sdl2-config --cflags) $(INCLUDES)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image

src_dir = src
inc_dir = inc
obj_dir = obj
target_dir = bin

src = puzzle.c my_math.c game.c state.c

obj = $(patsubst %.c,$(obj_dir)/%.o,$(src))
dep = $(patsubst %.c,$(obj_dir)/%.od,$(src))

programs = main.c
prog_deps = $(patsubst %.c,$(obj_dir)/%.pd,$(programs))
targets   = $(patsubst %.c,$(target_dir)/%,$(programs))

obj_dirs = $(sort $(dir $(obj)))

all: $(targets)

.PHONY: all clean

clean:
	-rm -r -- $(obj_dir)
	-rm -- $(targets)

ifeq ($(MAKECMDGOALS),all)
-include $(dep)
-include $(prog_deps)
endif
ifeq ($(MAKECMDGOALS),)
-include $(dep)
-include $(prog_deps)
endif

$(target_dir)/%: $(src_dir)/%.c $(obj) | $(target_dir)
	$(CC) $(CCFLAGS) $< -o $@ $(obj) $(LDFLAGS)

$(target_dir) $(obj_dirs):
	mkdir -p $@

$(obj_dir)/%.od: $(src_dir)/%.c | $(obj_dirs)
	$(CC) $(INCLUDES) -MM -MT "$(obj_dir)/$*.o $@" -o $@ -c $<

$(obj_dir)/%.pd: $(src_dir)/%.c | $(obj_dirs)
	$(CC) $(INCLUDES) -MM -MT "$(target_dir)/$* $@" -o $@ -c $<

$(obj_dir)/%.o: $(src_dir)/%.c | $(obj_dirs)
	$(CC) $(CCFLAGS) -c -o $@ $<

%.h:
	grep $@ -l -R $(obj_dir) | grep 'd$$' | xargs rm
