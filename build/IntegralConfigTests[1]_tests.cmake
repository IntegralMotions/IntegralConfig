add_test([=[IntegralConfig.HelloFunction]=]  /Users/jos/ws/IntegralConfig/build/IntegralConfigTests [==[--gtest_filter=IntegralConfig.HelloFunction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[IntegralConfig.HelloFunction]=]  PROPERTIES WORKING_DIRECTORY /Users/jos/ws/IntegralConfig/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  IntegralConfigTests_TESTS IntegralConfig.HelloFunction)
