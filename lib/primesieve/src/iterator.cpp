///
/// @file  iterator.cpp
///
/// Copyright (C) 2022 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve/iterator.hpp>
#include <primesieve/IteratorHelper.hpp>
#include <primesieve/PrimeGenerator.hpp>
#include <primesieve/macros.hpp>
#include <primesieve/pod_vector.hpp>

#include <stdint.h>
#include <limits>

namespace {

void freeAllMemory(primesieve::iterator* it)
{
  if (it->memory_)
  {
    using primesieve::IteratorMemory;
    delete (IteratorMemory*) it->memory_;
    it->memory_ = nullptr;
  }
}

} // namespace

namespace primesieve {

iterator::iterator() noexcept :
  i_(0),
  size_(0),
  start_(0),
  stop_hint_(std::numeric_limits<uint64_t>::max()),
  primes_(nullptr),
  memory_(nullptr)
{ }

iterator::iterator(uint64_t start,
                   uint64_t stop_hint) noexcept :
  i_(0),
  size_(0),
  start_(start),
  stop_hint_(stop_hint),
  primes_(nullptr),
  memory_(nullptr)
{ }

/// Move constructor
iterator::iterator(iterator&& other) noexcept :
  i_(other.i_),
  size_(other.size_),
  start_(other.start_),
  stop_hint_(other.stop_hint_),
  primes_(other.primes_),
  memory_(other.memory_)
{
  other.i_ = 0;
  other.size_ = 0;
  other.start_ = 0;
  other.stop_hint_ = std::numeric_limits<uint64_t>::max();
  other.primes_ = nullptr;
  other.memory_ = nullptr;
}

/// Move assignment operator
iterator& iterator::operator=(iterator&& other) noexcept
{
  if (this != &other)
  {
    freeAllMemory(this);

    i_ = other.i_;
    size_ = other.size_;
    start_ = other.start_;
    stop_hint_ = other.stop_hint_;
    primes_ = other.primes_;
    memory_ = other.memory_;

    other.i_ = 0;
    other.size_ = 0;
    other.start_ = 0;
    other.stop_hint_ = std::numeric_limits<uint64_t>::max();
    other.primes_ = nullptr;
    other.memory_ = nullptr;
  }

  return *this;
}

void iterator::jump_to(uint64_t start,
                       uint64_t stop_hint) noexcept
{
  i_ = 0;
  size_ = 0;
  start_ = start;
  stop_hint_ = stop_hint;
  primes_ = nullptr;

  // Frees most memory, but keeps some smaller data
  // structures (e.g. the PreSieve object) that are
  // useful if the primesieve::iterator is reused.
  // The remaining memory uses at most 200 kilobytes.
  if (memory_)
  {
    auto* memory = (IteratorMemory*) memory_;
    memory->stop = start;
    memory->dist = 0;
    memory->include_start_number = true;
    memory->deletePrimeGenerator();
    memory->deletePrimes();
  }
}

void iterator::clear() noexcept
{
  jump_to(0);
}

iterator::~iterator()
{
  freeAllMemory(this);
}

void iterator::generate_next_primes()
{
  if (!memory_)
    memory_ = new IteratorMemory(start_);

  auto& memory = *(IteratorMemory*) memory_;
  auto& primes = memory.primes;

  while (true)
  {
    if (!memory.primeGenerator)
    {
      IteratorHelper::updateNext(start_, stop_hint_, memory);
      memory.primeGenerator = new PrimeGenerator(start_, memory.stop, memory.preSieve);
    }

    memory.primeGenerator->fillNextPrimes(primes, &size_);

    // There are 2 different cases here:
    // 1) The primes array is empty because the next prime > stop.
    //    In this case we reset the primeGenerator object, increase
    //    the start & stop numbers and sieve the next segment.
    // 2) The primes array is not empty, in this case we return
    //    it to the user. The primes array either contains a few
    //    primes (<= 1024) or an error code (UINT64_MAX). The error
    //    code only occurs if the next prime > 2^64.
    if (size_ == 0)
      memory.deletePrimeGenerator();
    else
      break;
  }

  i_ = 0;
  primes_ = &primes[0];
}

void iterator::generate_prev_primes()
{
  if (!memory_)
    memory_ = new IteratorMemory(start_);

  auto& memory = *(IteratorMemory*) memory_;
  auto& primes = memory.primes;

  // Special case if generate_next_primes() has
  // been used before generate_prev_primes().
  if_unlikely(memory.primeGenerator)
  {
    start_ = primes.front();
    memory.deletePrimeGenerator();
  }

  // When sieving backwards the sieving distance is subdivided
  // into smaller chunks. If we can prove that the total
  // sieving distance is large we enable pre-sieving.
  if (memory.dist == 0 &&
      stop_hint_ < start_)
    memory.preSieve.init(stop_hint_, start_);

  do
  {
    IteratorHelper::updatePrev(start_, stop_hint_, memory);
    PrimeGenerator primeGenerator(start_, memory.stop, memory.preSieve);
    primeGenerator.fillPrevPrimes(primes, &size_);
  }
  while (!size_);

  i_ = size_;
  primes_ = &primes[0];
}

} // namespace
