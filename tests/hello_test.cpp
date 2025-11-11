#include "integral_config.h"
#include <gtest/gtest.h>

TEST(IntegralConfig, HelloFunction) {
  testing::internal::CaptureStdout();
  IntegralConfig::hello();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Hello from IntegralConfig!"), std::string::npos);
}
