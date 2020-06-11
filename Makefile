CXX = g++-9
SHELL = /bin/bash -o pipefail
ALGOROOT = ${ALGO}

CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -Wshadow -Wformat=2
CXXFLAGS += -Wfloat-equal -Wcast-qual -Wcast-align -fvisibility=hidden # -Wconversion

# By default sets to debug mode.
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -O0
	DBGFLAGS += -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -g
	DBGFLAGS += -DLOCAL
	# DBGFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
	# Since this flag will cause a AddressSantizer error on my debug
	# function `trace`, so here I just simply comment out this one.
	# -fstack-protector
else
	CXXFLAGS += -O2
endif

# For local debug purpose
CXXINCS = -I$(ALGOROOT) -I$(ALGOROOT)/third_party/jngen/includes
# CXXLIBS += -lgvc -lcgraph -lcdt

# byte-test config
CNT ?= 4

ELF := $(notdir $(CURDIR))
CMP := $(ELF)_mp
GEN := $(ELF)_ge
INP := $(ELF).in

all: curdir test

curdir:
	@echo $(CURDIR)

ifeq ($(DEBUG), 1)
% : %.cc
	@echo "cxx $<"
	@$(CXX) $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@ $(CXXLIBS) $(CXXINCS)
else
% : %.cl
	@echo "cxx $<"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@ $(CXXLIBS) $(CXXINCS)
endif

%.cl : %.cc
	@echo "byte-post"
	@byte-post $(ELF)
	@pbcopy < $(ELF).cl && pbpaste 2>&1 >/dev/null

%_mp : %.mp
	@echo "cxx $<"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@

%_ge : %.ge
	@echo "cxx $<"
	@$(CXX) -x c++ --std=c++17 -DLOCAL $< $(CXXINCS) -o $@ $(CXXLIBS)

clean:
	@-rm -rf $(ELF) *_mp *_ge

deepclean: clean
	@-rm -rf *.gg *.ga *.gb *_err_* *.gi

samples: clean
	@rm -rf *.inp
	@echo byte-sample $(INP)
	@byte-sample $(INP)

# Run with sample input.
test: samples $(ELF)
	@echo byte-run $(ELF)
	@byte-run $(ELF) $(DEBUG)

comp: $(GEN) $(CMP)
	@echo byte-test
	@byte-test $(CNT) $(LOG)

# Run with random generated input data.
run: $(GEN) $(ELF)
	@echo byte-test
	@byte-test $(CNT) $(LOG)

gen: $(GEN)
	./$(GEN)

memory: $(ELF)
	@echo byte-memory
	@byte-memory $(ELF)

.PHONY: all clean run test comp run

print-%:
	@echo $* = $($*)
