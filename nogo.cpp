#include "agent.hpp"
#include "gtp.hpp"

int main(int argc, char **argv) {
  auto &gtp = GTPHelper::getInstance();
  gtp.registerAgent(std::make_unique<RandomAgent>());
  while (gtp.execute()) {
    ;
  }
}