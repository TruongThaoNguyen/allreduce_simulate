/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h" // HAVE_FUTEX_H
#include "xbt/parmap.hpp"
#include <simgrid/msg.h>
#include <xbt.h>

#include <cstdlib>
#include <numeric> // std::iota
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(parmap_bench, "Bench for parmap");

#define MODES_DEFAULT 0x7
#define TIMEOUT 10.0
#define ARRAY_SIZE 10007
#define FIBO_MAX 25

void (*fun_to_apply)(unsigned*);

static std::string parmap_mode_name(e_xbt_parmap_mode_t mode)
{
  std::string name;
  switch (mode) {
    case XBT_PARMAP_POSIX:
      name = "POSIX";
      break;
    case XBT_PARMAP_FUTEX:
      name = "FUTEX";
      break;
    case XBT_PARMAP_BUSY_WAIT:
      name = "BUSY_WAIT";
      break;
    case XBT_PARMAP_DEFAULT:
      name = "DEFAULT";
      break;
    default:
      name = "UNKNOWN(" + std::to_string(mode) + ")";
      break;
  }
  return name;
}

static unsigned fibonacci(unsigned n)
{
  if (n < 2)
    return n;
  else
    return fibonacci(n - 1) + fibonacci(n - 2);
}

static void fun_small_comp(unsigned* arg)
{
  *arg = 2 * *arg + 1;
}

static void fun_big_comp(unsigned* arg)
{
  *arg = fibonacci(*arg % FIBO_MAX);
}

static void bench_parmap(int nthreads, e_xbt_parmap_mode_t mode, bool full_bench)
{
  XBT_INFO("** mode = %s", parmap_mode_name(mode).c_str());

  if (mode == XBT_PARMAP_FUTEX && not HAVE_FUTEX_H) {
    XBT_INFO("   not available");
    return;
  }

  std::vector<unsigned> a(ARRAY_SIZE);
  std::vector<unsigned*> data(ARRAY_SIZE);
  std::iota(begin(a), end(a), 0);
  std::iota(begin(data), end(data), &a[0]);

  auto* parmap      = new simgrid::xbt::Parmap<unsigned*>(nthreads, mode);
  int i             = 0;
  double start_time = xbt_os_time();
  double elapsed_time;
  do {
    if (full_bench) {
      delete parmap;
      parmap = new simgrid::xbt::Parmap<unsigned*>(nthreads, mode);
    }
    parmap->apply(fun_to_apply, data);
    elapsed_time = xbt_os_time() - start_time;
    i++;
  } while (elapsed_time < TIMEOUT);
  delete parmap;

  XBT_INFO("   ran %d times in %g seconds (%g/s)", i, elapsed_time, i / elapsed_time);
}

static void bench_all_modes(int nthreads, unsigned modes, bool full_bench)
{
  std::vector<e_xbt_parmap_mode_t> all_modes = {XBT_PARMAP_POSIX, XBT_PARMAP_FUTEX, XBT_PARMAP_BUSY_WAIT,
                                                XBT_PARMAP_DEFAULT};

  for (unsigned i = 0; i < all_modes.size(); i++) {
    if (1U << i & modes)
      bench_parmap(nthreads, all_modes[i], full_bench);
  }
}

int main(int argc, char* argv[])
{
  int nthreads;
  unsigned modes = MODES_DEFAULT;

  xbt_log_control_set("parmap_bench.fmt:[%c/%p]%e%m%n");
  MSG_init(&argc, argv);

  if (argc != 2 && argc != 3) {
    XBT_INFO("Usage: %s nthreads [modes]", argv[0]);
    XBT_INFO("    nthreads - number of working threads");
    XBT_INFO("    modes    - bitmask of modes to test");
    return EXIT_FAILURE;
  }
  nthreads = atoi(argv[1]);
  if (nthreads < 1) {
    XBT_ERROR("Invalid thread count: %d", nthreads);
    return EXIT_FAILURE;
  }
  if (argc == 3)
    modes = strtol(argv[2], NULL, 0);

  XBT_INFO("Parmap benchmark with %d workers (modes = %#x)...", nthreads, modes);
  XBT_INFO("%s", "");

  SIMIX_context_set_nthreads(nthreads);
  fun_to_apply = &fun_small_comp;

  XBT_INFO("Benchmark for parmap create+apply+destroy (small comp):");
  bench_all_modes(nthreads, modes, true);
  XBT_INFO("%s", "");

  XBT_INFO("Benchmark for parmap apply only (small comp):");
  bench_all_modes(nthreads, modes, false);
  XBT_INFO("%s", "");

  fun_to_apply = &fun_big_comp;

  XBT_INFO("Benchmark for parmap create+apply+destroy (big comp):");
  bench_all_modes(nthreads, modes, true);
  XBT_INFO("%s", "");

  XBT_INFO("Benchmark for parmap apply only (big comp):");
  bench_all_modes(nthreads, modes, false);
  XBT_INFO("%s", "");

  return EXIT_SUCCESS;
}
