CXX = g++
SHELL = /bin/bash -o pipefail
ALGOROOT = ${ALGO}

CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -O2 -Wshadow -Wformat=2
CXXFLAGS += -Wfloat-equal -Wcast-qual -Wcast-align -fvisibility=hidden # -Wconversion

# By default sets to debug mode.
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	DBGFLAGS += -fsanitize=address -fsanitize=undefined -fno-sanitize-recover
	# DBGFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
	# Since this flag will cause a AddressSantizer error on my debug
	# function `trace`, so here I just simply comment out this one.
	# -fstack-protector
endif

# For local debug purpose
CXXFLAGS += -I$(ALGOROOT)
ifeq ($(DEBUG), 1)
	DBGFLAGS += -DLOCAL
endif

# byte-test config
cnts ?= 10

ELF := $(notdir $(CURDIR))
CMP := $(ELF)_mp
GEN := $(ELF)_ge
INP := $(ELF).in

all: curdir test

curdir:
	@echo $(CURDIR)

% : %.cc
	@echo "cxx $<"
	@$(CXX) $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@

%_mp : %.mp
	@echo "cxx $<"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@

%_ge : %.ge
	@echo "cxx $<"
	@$(CXX) -x c++ --std=c++17 $< -I$(ALGOROOT)/third_party/jngen/includes -I$(ALGOROOT) -o $@

clean:
	@-rm -rf $(ELF) *_mp *_ge

samples: clean
	@rm -rf *.inp
	@echo byte-sample $(INP)
	@byte-sample $(INP)

# Run with sample input.
test: samples $(ELF)
	@echo byte-run $(ELF)
	@byte-run $(ELF) $(DEBUG)

comp: $(CMP) $(GEN) $(ELF)
	@echo byte-test
	@byte-test $(cnts)

# Run with random generated input data.
run: $(GEN) $(ELF)
	@echo byte-test
	@byte-test $(cnts)

gen: $(GEN)
	./$(GEN)

memo:
	ps aux | grep "[.]/$(ELF)$$" | awk '{$$6=int($$6/1024)"M";}{print;}'

.PHONY: all clean run test comp run

print-%:
	@echo $* = $($*)
