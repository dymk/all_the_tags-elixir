#include <hayai.hpp>
#include "gtest/gtest.h"

#include "context.h"

int main(int argc, char** argv) {
  // run google tests!
  ::testing::InitGoogleTest(&argc, argv);
  if(RUN_ALL_TESTS() != 0) { return -1; }

  hayai::ConsoleOutputter consoleOutputter;
  hayai::Benchmarker::AddOutputter(consoleOutputter);
  hayai::Benchmarker::RunAllTests();

  return 0;
}
