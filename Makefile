CXX ?= g++
CXXFLAGS += -std=c++17 -O3 -Wall

all: nogo

nogo: nogo.cpp gtp.hpp
	${CXX} ${CXXFLAGS} $< -o $@