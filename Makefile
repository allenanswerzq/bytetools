CXX = g++
SHELL = /bin/bash -o pipefail
ALGOROOT = ${ALGO}

CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -O2 -Wshadow -Wformat=2
CXXFLAGS += -Wfloat-equal -Wcast-qual -Wcast-align -fvisibility=hidden # -Wconversion

# By default sets to debug mode.
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	DBGFLAGS += -fsanitize=address -fsanitize=undefined
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

ELF := $(notdir $(CURDIR))
CMP := $(ELF)_mp
GEN := $(ELF)_ge
INP := $(ELF).in

all: test

% : %.cc Makefile
	@echo "cxx $<"
	@$(CXX) $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@

%_mp : %.mp Makefile
	@echo "cxx $<"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@

%_ge : %.ge Makefile
	@echo "cxx $<"
	@$(CXX) -x c++ $< -I$(ALGOROOT)/third_party/jngen/includes -o $@

clean:
	@echo $(CURDIR)
	@-rm -rf *.inp *_mp *_ge

samples: clean
	@echo byte-sample $(INP)
	@byte-sample $(INP)

# run: .inp --> .out
# dif: .out --> .rel
test: samples $(ELF)
	@echo byte-run $(ELF)
	@byte-run $(ELF) $(DEBUG)

# run: .inp --> .cmp
# dif: .out --> .cmp
comp: samples $(CMP)
	@echo byte-cmp $(CMP)
	@byte-cmp $(CMP)

gen: $(GEN)
	@rm -rf *.rel *.pdf *.gv
	./$(GEN) | tee $(INP)

memo:
	ps aux | grep "[.]/$(ELF)$$" | awk '{$$6=int($$6/1024)"M";}{print;}'

.PHONY: all clean run test comp

print-%:
	@echo $* = $($*)
