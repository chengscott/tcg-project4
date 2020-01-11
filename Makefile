CXX ?= g++
CXXFLAGS += -std=c++17 -O3 -march=native #-flto
CXXFLAGS += -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wpedantic
CXXFLAGS += -Wunused -Wsign-conversion -Wdouble-promotion
BIN = nogo
SRCS = nogo.cpp
DEPS = gtp.hpp board.hpp agent.hpp random.hpp
OBJS = $(SRCS:.cpp=)
OBJS += $(DEPS:.hpp=)
CHECKS = -checks=bugprone-*,clang-analyzer-*,modernize-*,performance-*,readability-*

.PHONY: $(BIN) format check clean
all: $(BIN)

$(BIN): $(SRCS) $(DEPS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

format:
	clang-format -i $(SRCS) $(DEPS)

check:
	clang-tidy $(SRCS) $(DEPS) $(CHECKS) -- $(CXXFLAGS)

clean:
	rm -f $(OBJS)
