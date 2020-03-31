
#include "polyscope_test.h"

#include "gtest/gtest.h"

#include <iostream>
#include <string>

using std::cout;
using std::endl;

// The global polyscope backend setting for tests
std::string testBackend = "openGL_mock";

TEST(BasicTest, HelloWorldTest) {
  int two = 2;
  int four = two + two;
  EXPECT_EQ(four, 4);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // Process custom test args
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    std::cout << "got arg " << arg << std::endl;

    { // look for a backend setting
      std::string prefix = "backend=";
      auto p = arg.rfind(prefix, 0);
      if (p == 0) {
        std::string val = arg.substr(prefix.size(), std::string::npos);
        testBackend = val;
        continue;
      }
    }

    throw std::runtime_error("unrecognized argument " + arg);
  }

  return RUN_ALL_TESTS();
}
