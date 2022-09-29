/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <thread>

//#include <folly/portability/SysTypes.h>

template <class D, class V, class Tag>
class IteratorFacade {
 public:
  using value_type = V;
  using reference = value_type&;
  using pointer = value_type*;
  using difference_type = ssize_t;
  using iterator_category = Tag;

  friend bool operator==(D const& lhs, D const& rhs) { return equal(lhs, rhs); }

  friend bool operator!=(D const& lhs, D const& rhs) { return !(lhs == rhs); }

  V& operator*() const { return asDerivedConst().dereference(); }

  V* operator->() const { return std::addressof(operator*()); }

  D& operator++() {
    asDerived().increment();
    return asDerived();
  }

  D operator++(int) {
    auto ret = asDerived(); 
    asDerived().increment();
    return ret;
  }

  D& operator--() {
    asDerived().decrement();
    return asDerived();
  }

  D operator--(int) {
    auto ret = asDerived();
    asDerived().decrement();
    return ret;
  }

 private:
  D& asDerived() { return static_cast<D&>(*this); }

  D const& asDerivedConst() const { return static_cast<D const&>(*this); }

  static bool equal(D const& lhs, D const& rhs) { return lhs.equal(rhs); }
};

inline void asm_volatile_memory() {
  asm volatile("" : : : "memory");
}

inline void asm_volatile_pause() {
  asm volatile("pause");
}


template <typename Cond>
void atomic_hash_spin_wait(Cond condition) {
  constexpr size_t kPauseLimit = 10000;
  for (size_t i = 0; condition(); ++i) {
    if (i < kPauseLimit) {
      asm_volatile_pause();
    } else {
      std::this_thread::yield();
    }
  }
}

size_t nextPowTwo(size_t c)
{

  size_t n = 1;

  while(n < c)
  {
	n = 2*n;
	
  }

 return n;
}
