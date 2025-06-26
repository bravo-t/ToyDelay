#include <cstdio>
#include "DelayCalculator.h"

int main(int argc, char** argv) 
{
  if (argc < 2) {
    printf("Input file missing, please provide a circuit netlist\n");
    return 1;
  }

  NA::DelayCalculator::run(argv[1]);

  return 0;
}
