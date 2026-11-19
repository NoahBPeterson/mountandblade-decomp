// tiny test harness: CHECK(cond) prints PASS / FAIL lines; main returns the failure count
#pragma once
#include <stdio.h>
static int g_fails = 0, g_checks = 0;
#define CHECK(cond) do { ++g_checks; if (cond) printf("PASS  %s\n", #cond); else { ++g_fails; printf("FAIL  %s  (%s:%d)\n", #cond, __FILE__, __LINE__); } } while (0)
#define CHECK_MSG(cond, ...) do { ++g_checks; if (cond) printf("PASS  %s\n", #cond); else { ++g_fails; printf("FAIL  %s  (%s:%d) ", #cond, __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static struct MbpUnbuf { MbpUnbuf() { setvbuf(stdout, 0, _IONBF, 0); } } g_mbp_unbuf;
