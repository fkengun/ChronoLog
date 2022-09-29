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
#define FOLLY_ATOMICHASHMAP_H_

#include <atomic>
#include <functional>
#include <stdexcept>

#include "AtomicHashArray_shm.h"
#include <folly/CPortability.h>
#include <folly/Likely.h>
#include <folly/ThreadCachedInt.h>
#include <folly/container/Foreach.h>
#include <folly/hash/Hash.h>
#include <iostream>

namespace folly {

struct FOLLY_EXPORT AtomicHashMapFullError : std::runtime_error {
  explicit AtomicHashMapFullError()
      : std::runtime_error("AtomicHashMap is full") {}
};

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class ProbeFcn,
    class KeyConvertFcn>
class AtomicHashMap {
  typedef AtomicHashArray<
      KeyT,
      ValueT,
      HashFcn,
      EqualFcn,
      Allocator,
      ProbeFcn,
      KeyConvertFcn>
      SubMap;

 public:
  typedef KeyT key_type;
  typedef ValueT mapped_type;
  typedef std::pair<const KeyT, ValueT> value_type;
  typedef HashFcn hasher;
  typedef EqualFcn key_equal;
  typedef KeyConvertFcn key_convert;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::ptrdiff_t difference_type;
  typedef std::size_t size_type;
  typedef typename SubMap::Config Config;

  template <class ContT, class IterVal, class SubIt>
  struct ahm_iterator;

  typedef ahm_iterator<
      const AtomicHashMap,
      const value_type,
      typename SubMap::const_iterator>
      const_iterator;
  typedef ahm_iterator<AtomicHashMap, value_type, typename SubMap::iterator>
      iterator;

 public:
  const float kGrowthFrac_; 

  explicit AtomicHashMap(size_t finalSizeEst,Config&);

  AtomicHashMap(const AtomicHashMap&) = delete;
  AtomicHashMap& operator=(const AtomicHashMap&) = delete;

  ~AtomicHashMap() {
    const unsigned int numMaps =
        numMapsAllocated_->load(std::memory_order_relaxed);
    FOR_EACH_RANGE (i, 0, numMaps) {
      SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
      DCHECK(thisMap);
      SubMap::destroy(thisMap);
    }
    Allocator().deallocate((char*)numMapsAllocated_,sizeof(std::atomic<uint32_t>));
    Allocator().deallocate((char*)subMaps_,sizeof(std::atomic<SubMap*>)*kNumSubMaps_);
  }

  key_equal key_eq() const { return key_equal(); }
  hasher hash_function() const { return hasher(); }

  std::pair<iterator, bool> insert(const value_type& r) {
    return emplace(r.first, r.second);
  }
  std::pair<iterator, bool> insert(key_type k, const mapped_type& v) {
    return emplace(k, v);
  }
  std::pair<iterator, bool> insert(value_type&& r) {
    return emplace(r.first, std::move(r.second));
  }
  std::pair<iterator, bool> insert(key_type k, mapped_type&& v) {
    return emplace(k, std::move(v));
  }

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal,
      typename LookupKeyToKeyFcn = key_convert,
      typename... ArgTs>
  std::pair<iterator, bool> emplace(LookupKeyT k, ArgTs&&... vCtorArg);

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  iterator find(LookupKeyT k);

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  const_iterator find(LookupKeyT k) const;

  size_type erase(key_type k);

  void clear();

  size_t size() const;

  bool empty() const { return size() == 0; }

  size_type count(key_type k) const { return find(k) == end() ? 0 : 1; }

  iterator findAt(uint32_t idx) {
    SimpleRetT ret = findAtInternal(idx);
    DCHECK_LT(ret.i, numSubMaps());
    return iterator(
        this,
        ret.i,
        subMaps_[ret.i].load(std::memory_order_relaxed)->makeIter(ret.j));
  }
  const_iterator findAt(uint32_t idx) const {
    return const_cast<AtomicHashMap*>(this)->findAt(idx);
  }

  size_t capacity() const;

  size_t spaceRemaining() const;

  void setEntryCountThreadCacheSize(int32_t newSize) {
    const int numMaps = numMapsAllocated_->load(std::memory_order_acquire);
    for (int i = 0; i < numMaps; ++i) {
      SubMap* map = subMaps_[i].load(std::memory_order_relaxed);
      map->setEntryCountThreadCacheSize(newSize);
    }
  }

  int numSubMaps() const {
    return numMapsAllocated_->load(std::memory_order_acquire);
  }

  iterator begin() {
    iterator it(this, 0, subMaps_[0].load(std::memory_order_relaxed)->begin());
    it.checkAdvanceToNextSubmap();
    return it;
  }

  const_iterator begin() const {
    const_iterator it(
        this, 0, subMaps_[0].load(std::memory_order_relaxed)->begin());
    it.checkAdvanceToNextSubmap();
    return it;
  }

  iterator end() { return iterator(); }

  const_iterator end() const { return const_iterator(); }

  inline uint32_t recToIdx(const value_type& r, bool mayInsert = true) {
    SimpleRetT ret =
        mayInsert ? insertInternal(r.first, r.second) : findInternal(r.first);
    return encodeIndex(ret.i, ret.j);
  }

  inline uint32_t recToIdx(value_type&& r, bool mayInsert = true) {
    SimpleRetT ret = mayInsert ? insertInternal(r.first, std::move(r.second))
                               : findInternal(r.first);
    return encodeIndex(ret.i, ret.j);
  }

  inline uint32_t recToIdx(
      key_type k, const mapped_type& v, bool mayInsert = true) {
    SimpleRetT ret = mayInsert ? insertInternal(k, v) : findInternal(k);
    return encodeIndex(ret.i, ret.j);
  }

  inline uint32_t recToIdx(key_type k, mapped_type&& v, bool mayInsert = true) {
    SimpleRetT ret =
        mayInsert ? insertInternal(k, std::move(v)) : findInternal(k);
    return encodeIndex(ret.i, ret.j);
  }

  inline uint32_t keyToIdx(const KeyT k, bool mayInsert = false) {
    return recToIdx(value_type(k), mayInsert);
  }

  inline const value_type& idxToRec(uint32_t idx) const {
    SimpleRetT ret = findAtInternal(idx);
    return subMaps_[ret.i].load(std::memory_order_relaxed)->idxToRec(ret.j);
  }

 private:
  static const uint32_t kNumSubMapBits_ = 4;
  static const uint32_t kSecondaryMapBit_ = 1u << 31; 
  static const uint32_t kSubMapIndexShift_ = 32 - kNumSubMapBits_ - 1;
  static const uint32_t kSubMapIndexMask_ = (1 << kSubMapIndexShift_) - 1;
  static const uint32_t kNumSubMaps_ = 1 << kNumSubMapBits_;
  static const uintptr_t kLockedPtr_ = 0x88ULL << 48; 

  struct SimpleRetT {
    uint32_t i;
    size_t j;
    bool success;
    SimpleRetT(uint32_t ii, size_t jj, bool s) : i(ii), j(jj), success(s) {}
    SimpleRetT() = default;
  };

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal,
      typename LookupKeyToKeyFcn = key_convert,
      typename... ArgTs>
  SimpleRetT insertInternal(LookupKeyT key, ArgTs&&... value);

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  SimpleRetT findInternal(const LookupKeyT k) const;

  SimpleRetT findAtInternal(uint32_t idx) const;

  std::atomic<SubMap*> *subMaps_;
  std::atomic<uint32_t>* numMapsAllocated_;

  inline bool tryLockMap(unsigned int idx) {
    SubMap* val = nullptr;
    return subMaps_[idx].compare_exchange_strong(
        val, (SubMap*)kLockedPtr_, std::memory_order_acquire);
  }

  static inline uint32_t encodeIndex(uint32_t subMap, uint32_t subMapIdx);

}; 

/*template <
    class KeyT,
    class ValueT,
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>,
    class Allocator = std::allocator<char>>
using QuadraticProbingAtomicHashMap = AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    AtomicHashArrayQuadraticProbeFcn>;*/
} 

#include "AtomicHashMap-inl_shm.h"
