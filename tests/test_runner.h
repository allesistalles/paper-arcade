#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int _tests_run = 0, _tests_failed = 0;

#define ASSERT_EQ(a, b) do { \
  _tests_run++; \
  if ((long long)(a) != (long long)(b)) { \
    printf("  FAIL %s:%d  expected %lld got %lld\n", __FILE__, __LINE__, (long long)(b), (long long)(a)); \
    _tests_failed++; \
  } \
} while(0)

#define ASSERT_TRUE(x)  ASSERT_EQ(!!(x), 1)
#define ASSERT_FALSE(x) ASSERT_EQ(!!(x), 0)

#define TEST_SUMMARY() do { \
  printf("%d/%d passed\n", _tests_run - _tests_failed, _tests_run); \
  return _tests_failed > 0 ? 1 : 0; \
} while(0)
