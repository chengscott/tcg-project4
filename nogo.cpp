#include "gtp.hpp"

int main(int argc, char **argv) {
  auto &gtp = GTPHelper::getInstance();
  while (gtp.execute()) {
    ;
  }
}