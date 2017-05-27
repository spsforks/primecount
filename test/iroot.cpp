///
/// @file   iroot.cpp
/// @brief  Test integer nth root function.
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <imath.hpp>
#include <int128_t.hpp>

#include <stdint.h>
#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;
using namespace primecount;

void check(bool OK)
{
  cout << "   " << (OK ? "OK" : "ERROR") << "\n";
  if (!OK)
    exit(1);
}

int main()
{
  uint64_t n;
  uint64_t res1;
  double res2;

  if (sizeof(long double) > 8)
  {
    for (n = 0; n < 100000; n++)
    {
      res1 = iroot<2>(n);
      res2 = pow((long double) n, 1.0L / 2);
      cout << "iroot<2>(" << n << ") = " << res1;
      check(res1 == (uint64_t) res2);
    }

    for (n = 0; n < 100000; n++)
    {
      res1 = iroot<3>(n);
      res2 = pow((long double) n, 1.0L / 3);
      cout << "iroot<3>(" << n << ") = " << res1;
      check(res1 == (uint64_t) res2);
    }

    for (n = 0; n < 100000; n++)
    {
      res1 = iroot<4>(n);
      res2 = pow((long double) n, 1.0L / 4);
      cout << "iroot<4>(" << n << ") = " << res1;
      check(res1 == (uint64_t) res2);
    }

    for (n = 0; n < 100000; n++)
    {
      res1 = iroot<6>(n);
      res2 = pow((long double) n, 1.0L / 6);
      cout << "iroot<6>(" << n << ") = " << res1;
      check(res1 == (uint64_t) res2);
    }
  }

#ifdef HAVE_INT128_T

  res1 = iroot<2>((int128_t) 18446744073709551615ull);
  cout << "iroot<2>(" << n << ") = " << res1;
  check(res1 == 4294967295ull);

  res1 = iroot<3>((int128_t) 18446744073709551615ull);
  cout << "iroot<3>(" << n << ") = " << res1;
  check(res1 == 2642245);

  res1 = iroot<4>((int128_t) 18446744073709551615ull);
  cout << "iroot<4>(" << n << ") = " << res1;
  check(res1 == 65535);

  res1 = iroot<6>((int128_t) 18446744073709551615ull);
  cout << "iroot<6>(" << n << ") = " << res1;
  check(res1 == 1625);

#endif

  cout << endl;
  cout << "All tests passed successfully!" << endl;

  return 0;
}