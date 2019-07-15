#include <iostream>
#include <string>

#include "gtest/gtest.h"

using std::cout;
using std::endl;

TEST(BasicTest, HelloWorldTest) {
  int two = 2;
  int four = two + two;
  EXPECT_EQ(four, 4);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
