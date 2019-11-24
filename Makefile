CXX ?= g++
CXXFLAGS += -std=c++17 -Wall -O3 #-flto
BIN = nogo
SRCS = nogo.cpp
DEPS = gtp.hpp board.hpp dset.hpp agent.hpp
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
