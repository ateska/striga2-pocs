CFLAGS:=-O0 -Wall -ggdb $(shell python3-config --cflags)
CFLAGS:=$(filter-out -O3 -g -DNDEBUG,$(CFLAGS)) # Filter out Python flags
LDLIBS+=-lev $(shell python3-config --libs --ldflags)

.PHONY: clean all rebuild subdirs

# Basic commands

subdirs:
	@$(foreach dir, $(SUBDIRS), $(MAKE) -C $(dir);)

clean:
	@echo " [RM] in" $(CURDIR)
	@$(RM) $(BIN) $(LIB) $(CLEAN) *.o
	@$(foreach dir, $(SUBDIRS) $(CLEANSUBDIRS), $(MAKE) -C $(dir) clean;)

# Compile command

.c.o:
	@echo " [CC]" $@
	@$(COMPILE.c) $(OUTPUT_OPTION) $<

# Link commands

${BIN}: ${OBJS}
	@echo " [LD]" $@
	@$(LINK.o) $^ ${UTOBJS} $(LOADLIBES) $(LDLIBS) -o $@

