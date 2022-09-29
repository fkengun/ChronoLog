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

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <random>
#include <sys/unistd.h>
#include <cstring>
#include <string.h>

/*#include <folly/Conv.h>
#include <folly/Likely.h>
#include <folly/Random.h>
#include <folly/Traits.h>*/
#include "AtomicUnorderedMapUtils.h"
/*#include <folly/lang/Bits.h>
#include <folly/portability/SysMman.h>
#include <folly/portability/Unistd.h>*/

size_t nextPowTwo(size_t c)
{

  size_t n = 1;

  while(n < c)
  {
        n = 2*n;
  }

  return n;
}


namespace folly {

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    bool SkipKeyValueDeletion =
        (std::is_trivially_destructible<Key>::value &&
         std::is_trivially_destructible<Value>::value),
    template <typename> class Atom = std::atomic,
    typename IndexType = uint32_t,
    typename Allocator = folly::detail::MMapAlloc>

struct AtomicUnorderedInsertMap {
  typedef Key key_type;
  typedef Value mapped_type;
  typedef std::pair<Key, Value> value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef Hash hasher;
  typedef KeyEqual key_equal;
  typedef const value_type& const_reference;

  typedef struct ConstIterator {
    ConstIterator(const AtomicUnorderedInsertMap& owner, IndexType slot)
        : owner_(owner), slot_(slot) {}

    ConstIterator(const ConstIterator&) = default;
    ConstIterator& operator=(const ConstIterator&) = default;

    const value_type& operator*() const {
      return owner_.slots_[slot_].keyValue();
    }

    const value_type* operator->() const {
      return &owner_.slots_[slot_].keyValue();
    }

    const ConstIterator& operator++() {
      while (slot_ > 0) {
        --slot_;
        if (owner_.slots_[slot_].state() == LINKED) {
          break;
        }
      }
      return *this;
    }

    ConstIterator operator++(int) {
      auto prev = *this;
      ++*this;
      return prev;
    }

    bool operator==(const ConstIterator& rhs) const {
      return slot_ == rhs.slot_;
    }
    bool operator!=(const ConstIterator& rhs) const { return !(*this == rhs); }

   private:
    const AtomicUnorderedInsertMap& owner_;
    IndexType slot_;
  } const_iterator;

  friend ConstIterator;

  explicit AtomicUnorderedInsertMap(
      size_t maxSize,
      float maxLoadFactor = 0.8f,
      const Allocator& alloc = Allocator())
      : allocator_(alloc) {
    size_t capacity = size_t(maxSize / std::min(1.0f, maxLoadFactor) + 128);
    size_t avail = size_t{1} << (8 * sizeof(IndexType) - 2);
    if (capacity > avail && maxSize < avail) {
      capacity = avail;
    }
    if (capacity < maxSize || capacity > avail) {
      throw std::invalid_argument(
          "AtomicUnorderedInsertMap capacity must fit in IndexType with 2 bits "
          "left over");
    }

    numSlots_ = capacity;
    slotMask_ = nextPowTwo(capacity * 4) - 1;
    mmapRequested_ = sizeof(Slot) * capacity;
    slots_ = reinterpret_cast<Slot*>(allocator_.allocate(mmapRequested_));
    zeroFillSlots();
    slots_[0].stateUpdate(EMPTY, CONSTRUCTING);
  }

  ~AtomicUnorderedInsertMap() {
    if (!SkipKeyValueDeletion) {
      for (size_t i = 1; i < numSlots_; ++i) {
        slots_[i].~Slot();
      }
    }
    allocator_.deallocate(reinterpret_cast<char*>(slots_), mmapRequested_);
  }

  template <typename Func>
  std::pair<const_iterator, bool> findOrConstruct(const Key& key, Func&& func) {
    auto const slot = keyToSlotIdx(key);
    auto prev = slots_[slot].headAndState_.load(std::memory_order_acquire);

    auto existing = find(key, slot);
    if (existing != 0) {
      return std::make_pair(ConstIterator(*this, existing), false);
    }

    auto idx = allocateNear(slot);
    new (&slots_[idx].keyValue().first) Key(key);
    func(static_cast<void*>(&slots_[idx].keyValue().second));

    while (true) {
      slots_[idx].next_ = prev >> 2;

      auto after = idx << 2;
      if (slot == idx) {
        after += LINKED;
      } else {
        after += (prev & 3);
      }

      if (slots_[slot].headAndState_.compare_exchange_strong(prev, after)) {
        if (idx != slot) {
          slots_[idx].stateUpdate(CONSTRUCTING, LINKED);
        }
        return std::make_pair(ConstIterator(*this, idx), true);
      }

      existing = find(key, slot);
      if (existing != 0) {
        slots_[idx].keyValue().first.~Key();
        slots_[idx].keyValue().second.~Value();
        slots_[idx].stateUpdate(CONSTRUCTING, EMPTY);

        return std::make_pair(ConstIterator(*this, existing), false);
      }
    }
  }

  template <class K, class V>
  std::pair<const_iterator, bool> emplace(const K& key, V&& value) {
    return findOrConstruct(
        key, [&](void* raw) { new (raw) Value(std::forward<V>(value)); });
  }

  const_iterator find(const Key& key) const {
    return ConstIterator(*this, find(key, keyToSlotIdx(key)));
  }

  const_iterator cbegin() const {
    IndexType slot = numSlots_ - 1;
    while (slot > 0 && slots_[slot].state() != LINKED) {
      --slot;
    }
    return ConstIterator(*this, slot);
  }

  const_iterator cend() const { return ConstIterator(*this, 0); }

 private:
  enum : IndexType {
    kMaxAllocationTries = 1000, 
  };

  enum BucketState : IndexType {
    EMPTY = 0,
    CONSTRUCTING = 1,
    LINKED = 2,
  };

  struct Slot {
    Atom<IndexType> headAndState_;

    IndexType next_;

    value_type raw_;

    ~Slot() {
      auto s = state();
      assert(s == EMPTY || s == LINKED);
      if (s == LINKED) {
        keyValue().first.~Key();
        keyValue().second.~Value();
      }
    }

    BucketState state() const {
      return BucketState(headAndState_.load(std::memory_order_acquire) & 3);
    }

    void stateUpdate(BucketState before, BucketState after) {
      assert(state() == before);
      headAndState_ += (after - before);
    }

    value_type& keyValue() {
      assert(state() != EMPTY);
      return *static_cast<value_type*>(static_cast<void*>(&raw_));
    }

    const value_type& keyValue() const {
      assert(state() != EMPTY);
      return *static_cast<const value_type*>(static_cast<const void*>(&raw_));
    }
  };

  size_t mmapRequested_;
  size_t numSlots_;

  size_t slotMask_;

  Allocator allocator_;
  Slot* slots_;

  IndexType keyToSlotIdx(const Key& key) const {
    size_t h = hasher()(key);
    h &= slotMask_;
    while (h >= numSlots_) {
      h -= numSlots_;
    }
    return h;
  }

  IndexType find(const Key& key, IndexType slot) const {
    KeyEqual ke = {};
    auto hs = slots_[slot].headAndState_.load(std::memory_order_acquire);
    for (slot = hs >> 2; slot != 0; slot = slots_[slot].next_) {
      if (ke(key, slots_[slot].keyValue().first)) {
        return slot;
      }
    }
    return 0;
  }

  IndexType allocateNear(IndexType start) {
    for (IndexType tries = 0; tries < kMaxAllocationTries; ++tries) {
      auto slot = allocationAttempt(start, tries);
      auto prev = slots_[slot].headAndState_.load(std::memory_order_acquire);
      if ((prev & 3) == EMPTY &&
          slots_[slot].headAndState_.compare_exchange_strong(
              prev, prev + CONSTRUCTING - EMPTY)) {
        return slot;
      }
    }
    throw std::bad_alloc();
  }

  IndexType allocationAttempt(IndexType start, IndexType tries) const {
    if (tries < 8 && start + tries < numSlots_) {
      return IndexType(start + tries);
    } else {
      IndexType rv;
      if (sizeof(IndexType) <= 4) {
        rv = IndexType(random()%numSlots_);
      } else {
        rv = IndexType(random()%numSlots_);
      }
      assert(rv < numSlots_);
      return rv;
    }
  }

  void zeroFillSlots() {
    using folly::detail::GivesZeroFilledMemory;
    if (!GivesZeroFilledMemory<Allocator>::value) {
	    std::memset(static_cast<void*>(slots_), 0, mmapRequested_);
    }
  }
};

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    bool SkipKeyValueDeletion =
        (std::is_trivially_destructible<Key>::value &&
         std::is_trivially_destructible<Value>::value),
    template <typename> class Atom = std::atomic,
    typename Allocator = folly::detail::MMapAlloc>
using AtomicUnorderedInsertMap64 = AtomicUnorderedInsertMap<
    Key,
    Value,
    Hash,
    KeyEqual,
    SkipKeyValueDeletion,
    Atom,
    uint64_t,
    Allocator>;

template <typename T, template <typename> class Atom = std::atomic>
struct MutableAtom {
  mutable Atom<T> data;

  explicit MutableAtom(const T& init) : data(init) {}
};

template <typename T>
struct MutableData {
  mutable T data;
  explicit MutableData(const T& init) : data(init) {}
};

} 
