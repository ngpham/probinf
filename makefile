SrcDir := src
ObjDir := obj

CppDebugAllwarns = -g -Wall -Wextra -pedantic -std=c++17 -fopenmp
CppOptimized = -O3 -std=c++17 -fopenmp
CppArgs := $(CppOptimized)

cc_files = $(wildcard src/*.cc src/**/*.cc)
headers = $(wildcard src/*.h src/**/*.h)

Objects := $(addprefix obj/, $(notdir $(patsubst %.cc, %.o, $(cc_files))))
Prog := infl

$(ObjDir)/%.o: $(SrcDir)/%.cc $(headers)
	g++ $(CppArgs) -c $< -o $@

$(Prog): $(Objects)
	g++ $(CppArgs) -o $@ $^

.PHONY: clean test testvg fetchdata

all: $(Prog)

testvg: $(Prog)
	valgrind ./infl \
	-f data/mini1.txt -k 2 -a 0.1 -rs 50 -rsin 1 -rstest 2 -p 0.7 -alg maxprobbicritinfl -numsamp 8 \
	-numsamptest 4

test: $(Prog)
	chmod a+x ./runtest.sh
	./runtest.sh

fetchdata:
	chmod a+x ./fetchdata.sh
	./fetchdata.sh

print-% : ; @echo $* = $($*)

clean:
	rm -rf obj/*.*
	rm $(Prog)
