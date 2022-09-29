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

#ifndef FOLLY_ATOMICHASHARRAY_H_
#error "This should only be included by AtomicHashArray.h"
#endif

#include <type_traits>

//#include <folly/detail/AtomicHashUtils.h>
#include "Iterators_shm.h"
#include <cassert>
//#include <folly/lang/Bits.h>
//#include <folly/lang/Exception.h>

namespace folly {

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::
    AtomicHashArray(
        size_t capacity,
        KeyT emptyKey,
        KeyT lockedKey,
        KeyT erasedKey,
        double _maxLoadFactor,
        uint32_t cacheSize)
    : capacity_(capacity),
      maxEntries_(size_t(_maxLoadFactor * capacity_ + 0.5)),
      kEmptyKey_(emptyKey),
      kLockedKey_(lockedKey),
      kErasedKey_(erasedKey),
      kAnchorMask_(nextPowTwo(capacity_) - 1)
      {
      numEntries_ = (std::atomic<uint64_t>*)Allocator().allocate(sizeof(std::atomic<uint64_t>));
      numEntries_->store(0);     
      isFull_ = (std::atomic<int64_t>*)Allocator().allocate(sizeof(std::atomic<int64_t>));
      isFull_->store(0);
      numErases_ = (std::atomic<int64_t>*)Allocator().allocate(sizeof(std::atomic<int64_t>));
      numErases_->store(0); 
      numPendingEntries_ = (std::atomic<uint64_t>*)Allocator().allocate(sizeof(std::atomic<uint64_t>));
      numPendingEntries_->store(0);
      assert (capacity > 0);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
typename AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SimpleRetT
AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::findInternal(const LookupKeyT key_in) {
  checkLegalKeyIfKey<LookupKeyT>(key_in);

  for (size_t idx = keyToAnchorIdx<LookupKeyT, LookupHashFcn>(key_in),
              numProbes = 0;
       ;
       idx = ProbeFcn()(idx, numProbes, capacity_)) {
    const KeyT key = acquireLoadKey(cells_[idx]);
    if ((LookupEqualFcn()(key, key_in))) 
    { 
      return SimpleRetT(idx, true);
    }
    if ((key == kEmptyKey_)) 
    { 
      return SimpleRetT(capacity_, false);
    }
    ++numProbes;
    if ((numProbes >= capacity_)) 
    { 
      return SimpleRetT(capacity_, false);
    }
  }
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
template <
    typename LookupKeyT,
    typename LookupHashFcn,
    typename LookupEqualFcn,
    typename LookupKeyToKeyFcn,
    typename... ArgTs>
typename AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SimpleRetT
AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::insertInternal(LookupKeyT key_in, ArgTs&&... vCtorArgs) {
  const short NO_NEW_INSERTS = 1;
  const short NO_PENDING_INSERTS = 2;
  checkLegalKeyIfKey<LookupKeyT>(key_in);

  size_t idx = keyToAnchorIdx<LookupKeyT, LookupHashFcn>(key_in);
  size_t numProbes = 0;
  for (;;) {
    value_type* cell = &cells_[idx];
    if (relaxedLoadKey(*cell) == kEmptyKey_) 
    {
      numPendingEntries_->fetch_add(1);
      if (isFull_->load(std::memory_order_acquire)) {
        numPendingEntries_->fetch_sub(1);

        detail::atomic_hash_spin_wait([&] {
          return (isFull_->load(std::memory_order_acquire) !=
                  NO_PENDING_INSERTS) &&
              (numPendingEntries_->load() != 0);
        });
        isFull_->store(NO_PENDING_INSERTS, std::memory_order_release);

        if (relaxedLoadKey(*cell) == kEmptyKey_) {
          return SimpleRetT(capacity_, false);
        }
      } else {
        if (tryLockCell(cell)) {
          KeyT key_new;
          try {
            key_new = LookupKeyToKeyFcn()(key_in);
            typedef
                typename std::remove_const<LookupKeyT>::type LookupKeyTNoConst;
            constexpr bool kAlreadyChecked =
                std::is_same<KeyT, LookupKeyTNoConst>::value;
            if (!kAlreadyChecked) {
              checkLegalKeyIfKey(key_new);
            }
            using mapped = typename std::remove_const<mapped_type>::type;
            new (const_cast<mapped*>(&cell->second))
                ValueT(std::forward<ArgTs>(vCtorArgs)...);
            unlockCell(cell, key_new); 
          } catch (...) {
            unlockCell(cell, kEmptyKey_);
            numPendingEntries_->fetch_sub(1);
            throw;
          }
          numPendingEntries_->fetch_sub(1);
          numEntries_->fetch_add(1);
          if (numEntries_->load() >= maxEntries_) {
            isFull_->store(NO_NEW_INSERTS, std::memory_order_relaxed);
          }
          return SimpleRetT(idx, true);
        }
        numPendingEntries_->fetch_sub(1);
      }
    }
    if (kLockedKey_ == acquireLoadKey(*cell)) {
      detail::atomic_hash_spin_wait(
          [&] { return kLockedKey_ == acquireLoadKey(*cell); });
    }

    const KeyT thisKey = acquireLoadKey(*cell);
    if (LookupEqualFcn()(thisKey, key_in)) {
      return SimpleRetT(idx, false);
    } else if (thisKey == kEmptyKey_ || thisKey == kLockedKey_) {
      continue;
    }

    ++numProbes;
    if ((numProbes >= capacity_)) { 
      return SimpleRetT(capacity_, false);
    }

    idx = ProbeFcn()(idx, numProbes, capacity_);
  }
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
size_t AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::erase(KeyT key_in) {
    assert(key_in!=kEmptyKey_);
    assert(key_in!=kLockedKey_);
    assert(key_in!=kErasedKey_);

  for (size_t idx = keyToAnchorIdx(key_in), numProbes = 0;;
       idx = ProbeFcn()(idx, numProbes, capacity_)) {
    value_type* cell = &cells_[idx];
    KeyT currentKey = acquireLoadKey(*cell);
    if (currentKey == kEmptyKey_ || currentKey == kLockedKey_) {
      return 0;
    }
    if (EqualFcn()(currentKey, key_in)) {
      KeyT expect = currentKey;
      if (cellKeyPtr(*cell)->compare_exchange_strong(expect, kErasedKey_)) {
        numErases_->fetch_add(1, std::memory_order_relaxed);

        return 1;
      }
      return 0;
    }

    ++numProbes;
    if ((numProbes >= capacity_)) {  
      return 0;
    }
  }
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
typename AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SmartPtr
AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::create(size_t maxSize, const Config& c) {
  size_t capacity = size_t(maxSize / c.maxLoadFactor);
  size_t sz = sizeof(AtomicHashArray) + sizeof(value_type) * capacity;

  auto const mem = Allocator().allocate(sz);
  try {
    new (mem) AtomicHashArray(
        capacity,
        c.emptyKey,
        c.lockedKey,
        c.erasedKey,
        c.maxLoadFactor,
        c.entryCountThreadCacheSize);
  } catch (...) {
    Allocator().deallocate(mem, sz);
    throw;
  }

  SmartPtr map(static_cast<AtomicHashArray*>((void*)mem));

  for(int i=0;i<map->capacity_;i++){
    cellKeyPtr(map->cells_[i])
        ->store(map->kEmptyKey_, std::memory_order_relaxed);
  }
  return map;
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
void AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::destroy(AtomicHashArray* p) {
  assert(p);

  size_t sz = sizeof(AtomicHashArray) + sizeof(value_type) * p->capacity_;

  for(int i=0;i<p->capacity_;i++){
    if (p->cells_[i].first != p->kEmptyKey_) {
      p->cells_[i].~value_type();
    }
  }
  p->~AtomicHashArray();

  Allocator().deallocate((char*)p, sz);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
void AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::clear() {
  for(int i=0;i<capacity_;i++){
    if (cells_[i].first != kEmptyKey_) {
      cells_[i].~value_type();
      *const_cast<KeyT*>(&cells_[i].first) = kEmptyKey_;
    }
    CHECK(cells_[i].first == kEmptyKey_);
  }
  numEntries_->store(0);
  numPendingEntries_->store(0);
  isFull_->store(0, std::memory_order_relaxed);
  numErases_->store(0, std::memory_order_relaxed);
}


template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
template <class ContT, class IterVal>
struct AtomicHashArray<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::aha_iterator
    : detail::IteratorFacade<
          aha_iterator<ContT, IterVal>,
          IterVal,
          std::forward_iterator_tag> {
  explicit aha_iterator() : aha_(nullptr) {}

  template <class OtherContT, class OtherVal>
  aha_iterator(
      const aha_iterator<OtherContT, OtherVal>& o,
      typename std::enable_if<
          std::is_convertible<OtherVal*, IterVal*>::value>::type* = nullptr)
      : aha_(o.aha_), offset_(o.offset_) {}

  explicit aha_iterator(ContT* array, size_t offset)
      : aha_(array), offset_(offset) {}

  uint32_t getIndex() const { return offset_; }

  void advancePastEmpty() {
    while (offset_ < aha_->capacity_ && !isValid()) {
      ++offset_;
    }
  }

 private:
  friend class AtomicHashArray;
  friend class detail::
      IteratorFacade<aha_iterator, IterVal, std::forward_iterator_tag>;

  void increment() {
    ++offset_;
    advancePastEmpty();
  }

  bool equal(const aha_iterator& o) const {
    return aha_ == o.aha_ && offset_ == o.offset_;
  }

  IterVal& dereference() const { return aha_->cells_[offset_]; }

  bool isValid() const {
    KeyT key = acquireLoadKey(aha_->cells_[offset_]);
    return key != aha_->kEmptyKey_ && key != aha_->kLockedKey_ &&
        key != aha_->kErasedKey_;
  }

 private:
  ContT* aha_;
  size_t offset_;
}; 

} 
