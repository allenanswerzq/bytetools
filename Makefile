CXX = g++
SHELL = /bin/bash -o pipefail
ALGOROOT = ${ALGO}

CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -Wshadow -Wformat=2
CXXFLAGS += -Wfloat-equal -Wcast-qual -Wcast-align -fvisibility=hidden # -Wconversion

#-------------------------------------------------------------------------------
# By default sets to debug mode.
DEBUG ?= 1
RLOG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -O2
	DBGFLAGS += -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -g -fmax-errors=2
	DBGFLAGS += -DLOCAL
	# DBGFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
	# Since this flag will cause a AddressSantizer error on my debug
	# function `trace`, so here I just simply comment out this one.
	# -fstack-protector
else
	CXXFLAGS += -O2
endif

#-------------------------------------------------------------------------------
# For local debug purpose
CXXINCS = -I$(ALGOROOT) -I$(ALGOROOT)/third_party/jngen/includes
# CXXLIBS += -lgvc -lcgraph -lcdt

# byte-test config
CNT ?= 4
ELF := $(notdir $(CURDIR))
CMP := $(ELF)_mp
GEN := $(ELF)_ge
INP := $(ELF).ii

#-------------------------------------------------------------------------------
all: curdir test

help:
	@echo -e "Usage:"
	@echo -e "\t make DEBUG=0 RLOG=0"
	@echo -e "\t bmk  DEBUG=0 RLOG=0"
	@echo -e "\t byte-test CNT=4 LOG=0"
	@echo -e "\t bsc                     generate compare file"
	@echo -e "\t bsg                     generate random tests"
	@echo -e "\t make test;              run with sample test cases"
	@echo -e "\t make compile;           only compile"
	@echo -e "\t make compare;           DEBUG=0 LOG=1 CNT=4; do compare"
	@echo -e "\t make random;            run with random generated inputs"

#-------------------------------------------------------------------------------
# ref: https://stackoverflow.com/questions/10024279/how-to-use-shell-commands-in-makefile
accpected_time:
	@ctime="$(shell date "+%Y-%m-%d %H:%M:%S")"; \
	perl -pi -e "s|.*accepted.*| \* accepted  : $$ctime|g" $(ELF).cc

#-------------------------------------------------------------------------------
curdir: accpected_time
	@echo $(CURDIR)

#-------------------------------------------------------------------------------
ifeq ($(DEBUG), 1)
% : %.cc
	@echo "[debug -O0] cxx $<"
	@$(CXX) $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@ $(CXXLIBS) $(CXXINCS)
else
% : %.cl
	@echo "[release -O2] cxx $<"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $< $(LDFLAGS) -o $@ $(CXXLIBS) $(CXXINCS)
endif

#-------------------------------------------------------------------------------
%.cl : %.cc
	@echo "byte-post"
	@byte-post $(ELF)
	@pbcopy < $(ELF).cl && pbpaste 2>&1 >/dev/null

#-------------------------------------------------------------------------------
ifneq (,$(wildcard $(ELF).mp))
# if there exists a compare file
%_mp : %(ELF).mp
	@echo "[debug -O2] cxx $(ELF).mp"
	@$(CXX) -x c++ $(CXXFLAGS) $(DBGFLAGS) $(ELF).mp $(LDFLAGS) -o $@
else
# https://www.gnu.org/software/make/manual/html_node/Empty-Recipes.html
%_mp : ;
endif

#-------------------------------------------------------------------------------
%_ge : %.ge
	@echo "cxx $<"
	@$(CXX) -x c++ --std=c++17 -DLOCAL $< $(CXXINCS) -o $@ $(CXXLIBS)

#-------------------------------------------------------------------------------
clean:
	@-rm -rf $(ELF) *_mp *_ge

#-------------------------------------------------------------------------------
deepclean: clean
	@-rm -rf *.gg *.ga *.gb *_err_* *.gi

#-------------------------------------------------------------------------------
samples: clean
	@rm -rf *.in
	@echo byte-sample $(INP)
	@byte-sample $(INP)

#-------------------------------------------------------------------------------
compile: curdir $(ELF)

#-------------------------------------------------------------------------------
# Run with sample input.
test: samples $(ELF)
	@echo byte-run $(ELF)
	@byte-run $(ELF) $(DEBUG) $(RLOG)

#-------------------------------------------------------------------------------
compare: deepclean $(ELF) $(GEN) $(CMP)
	@echo byte-test
	@byte-test $(CNT) $(LOG)

#-------------------------------------------------------------------------------
# Run with random generated input data.
random: $(GEN) $(ELF)
	@echo byte-test
	@byte-test $(CNT) $(LOG)

#-------------------------------------------------------------------------------
post: $(ELF)
	@byte-post $(ELF)
	@pbcopy < "$(ELF).cl" && pbpaste 2>&1 >/dev/null

#-------------------------------------------------------------------------------
gen: $(GEN)
	./$(GEN)

#-------------------------------------------------------------------------------
memory: $(ELF)
	@echo byte-memory
	@byte-memory $(ELF)

#-------------------------------------------------------------------------------
.PHONY: all clean run test comp run

#-------------------------------------------------------------------------------
print-%:
	@echo $* = $($*)
