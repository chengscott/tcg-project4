#include "board.hpp"
#include "gtp.hpp"

int main(int argc, char **argv) {
  board b;
  auto &gtp = GTPHelper::getInstance();
  while (gtp.execute()) {
    ;
  }
}