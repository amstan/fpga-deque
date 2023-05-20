VFLAGS ?= -Wall

# These are inane, something as simple as (var == 25) ? : will warn with this on
VFLAGS += -Wno-WIDTH

VFLAGS += -DDEBUG=1

build/V%__ALL.o build/V%.h: %.sv
	verilator $(VFLAGS) -cc $*.sv --Mdir build/ --build

build/verilated%o: /usr/share/verilator/include/verilated%cpp
	g++ $^ -c -o $@

build/test_%.o: test/%.cpp build/V%.h
	g++ -I /usr/share/verilator/include -I build $< -c -o $@

VERILATOR_DEPS = build/verilated.o build/verilated_threads.o

build/test_%: build/V%__ALL.o build/test_%.o $(VERILATOR_DEPS)
	g++ $? -o build/test_$*

## More direct way of invoking
# build/test_%: %.sv test/%.cpp
# 	verilator $(VFLAGS) -cc $*.sv --Mdir build/ --build test/$*.cpp --exe -o test_$*

.SECONDARY:
# Need .SECONDARY otherwise dependencies for the test,
# including the test will get deleted after executing the following rule:
.PHONY: test_%
test_%: build/test_%
	# Run test
	./$<

.DEFAULT: all
.PHONY: all
all: tests

.PHONY: tests
tests: $(patsubst test/%.cpp,test_%,$(wildcard test/*.cpp))

.PHONY: clean
clean:
	-rm -rf build/*
