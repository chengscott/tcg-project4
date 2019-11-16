CXX ?= g++
CXXFLAGS += -std=c++17 -O3 -Wall

all: nogo

nogo: nogo.cpp gtp.hpp board.hpp dset.hpp agent.hpp
	${CXX} ${CXXFLAGS} $< -o $@

format:
	clang-format -i *.cpp *.hpp

clean:
	rm -f nogo gtp board dset agent
