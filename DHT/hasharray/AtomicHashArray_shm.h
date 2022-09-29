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

/**
 *  AtomicHashArray is the building block for AtomicHashMap.  It provides the
 *  core lock-free functionality, but is limited by the fact that it cannot
 *  grow past its initialization size and is a little more awkward (no public
 *  constructor, for example).  If you're confident that you won't run out of
 *  space, don't mind the awkardness, and really need bare-metal performance,
 *  feel free to use AHA directly.
 *
 *  Check out AtomicHashMap.h for more thorough documentation on perf and
 *  general pros and cons relative to other hash maps.
 *
 *  @author Spencer Ahrens <sahrens@fb.com>
 *  @author Jordan DeLong <delong.j@fb.com>
 */

#pragma once
#define FOLLY_ATOMICHASHARRAY_H_

#include <atomic>
#include <memory>

//#include <folly/Utility.h>
#include "Hash_shm.h"
#include <iostream>

namespace folly {

struct AtomicHashArrayLinearProbeFcn {
  inline size_t operator()(
      size_t idx, size_t, size_t capacity) const {
    idx += 1; 

    return (idx < capacity) ? idx : (idx - capacity);
  }
};

struct AtomicHashArrayQuadraticProbeFcn {
  inline size_t operator()(
      size_t idx, size_t numProbes, size_t capacity) const {
    idx += numProbes;

    return (idx < capacity) ? idx : (idx - capacity); 
  }
};

namespace detail {
template <typename NotKeyT, typename KeyT>
inline void checkLegalKeyIfKeyTImpl(
    NotKeyT,
    KeyT,
    KeyT,
    KeyT) {}

template <typename KeyT>
inline void checkLegalKeyIfKeyTImpl(
    KeyT key_in, KeyT emptyKey, KeyT lockedKey, KeyT erasedKey) 
{
	assert(key_in != emptyKey);
	assert(key_in != lockedKey);
	assert(key_in != erasedKey);
}
} 

template <
    class KeyT,
    class ValueT,
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>,
    class Allocator = std::allocator<char>,
    class ProbeFcn = AtomicHashArrayLinearProbeFcn,
    class KeyConvertFcn = Identity>
class AtomicHashMap;

template <
    class KeyT,
    class ValueT,
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>,
    class Allocator = std::allocator<char>,
    class ProbeFcn = AtomicHashArrayLinearProbeFcn,
    class KeyConvertFcn = Identity>
class AtomicHashArray {
  static_assert(
      (std::is_convertible<KeyT, int32_t>::value ||
       std::is_convertible<KeyT, int64_t>::value ||
       std::is_convertible<KeyT, const void*>::value),
      "You are trying to use AtomicHashArray with disallowed key "
      "types.  You must use atomically compare-and-swappable integer "
      "keys, or a different container class.");

 public:
  typedef KeyT key_type;
  typedef ValueT mapped_type;
  typedef HashFcn hasher;
  typedef EqualFcn key_equal;
  typedef KeyConvertFcn key_convert;
  typedef std::pair<const KeyT, ValueT> value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  const size_t capacity_;
  const size_t maxEntries_;
  const KeyT kEmptyKey_;
  const KeyT kLockedKey_;
  const KeyT kErasedKey_;

  template <class ContT, class IterVal>
  struct aha_iterator;

  typedef aha_iterator<const AtomicHashArray, const value_type> const_iterator;
  typedef aha_iterator<AtomicHashArray, value_type> iterator;

  static void destroy(AtomicHashArray*);

 private:
  const size_t kAnchorMask_;

  struct Deleter {
    void operator()(AtomicHashArray* ptr) { AtomicHashArray::destroy(ptr); }
  };

 public:
  typedef std::unique_ptr<AtomicHashArray, Deleter> SmartPtr;

  struct Config {
    KeyT emptyKey;
    KeyT lockedKey;
    KeyT erasedKey;
    double maxLoadFactor;
    double growthFactor;
    uint32_t entryCountThreadCacheSize;
    size_t capacity;

    Config()
        : emptyKey((KeyT)-1),
          lockedKey((KeyT)-2),
          erasedKey((KeyT)-3),
          maxLoadFactor(0.8),
          growthFactor(-1),
          entryCountThreadCacheSize(1000),
          capacity(0) {}
  };

  static SmartPtr create(size_t maxSize, const Config& c = Config());

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  iterator find(LookupKeyT k) {
    return iterator(
        this, findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k).idx);
  }

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  const_iterator find(LookupKeyT k) const {
    return const_cast<AtomicHashArray*>(this)
        ->find<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
  }

  std::pair<iterator, bool> insert(const value_type& r) {
    return emplace(r.first, r.second);
  }
  std::pair<iterator, bool> insert(value_type&& r) {
    return emplace(r.first, std::move(r.second));
  }

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal,
      typename LookupKeyToKeyFcn = key_convert,
      typename... ArgTs>
  std::pair<iterator, bool> emplace(LookupKeyT key_in, ArgTs&&... vCtorArgs) {
    SimpleRetT ret = insertInternal<
        LookupKeyT,
        LookupHashFcn,
        LookupEqualFcn,
        LookupKeyToKeyFcn>(key_in, std::forward<ArgTs>(vCtorArgs)...);
    return std::make_pair(iterator(this, ret.idx), ret.success);
  }

  size_t erase(KeyT k);

  void clear();

  size_t size() const {
    return numEntries_->load() - numErases_->load(std::memory_order_relaxed);
  }

  bool empty() const { return size() == 0; }

  iterator begin() {
    iterator it(this, 0);
    it.advancePastEmpty();
    return it;
  }
  const_iterator begin() const {
    const_iterator it(this, 0);
    it.advancePastEmpty();
    return it;
  }

  iterator end() { return iterator(this, capacity_); }
  const_iterator end() const { return const_iterator(this, capacity_); }

  iterator findAt(uint32_t idx) {
    return iterator(this, idx);
  }
  const_iterator findAt(uint32_t idx) const {
    return const_cast<AtomicHashArray*>(this)->findAt(idx);
  }

  iterator makeIter(size_t idx) { return iterator(this, idx); }
  const_iterator makeIter(size_t idx) const {
    return const_iterator(this, idx);
  }

  double maxLoadFactor() const { return ((double)maxEntries_) / capacity_; }

  void setEntryCountThreadCacheSize(uint32_t newSize) {
  }

  uint32_t getEntryCountThreadCacheSize() const {
    return numEntries_->load();
  }

 private:
  friend class AtomicHashMap<
      KeyT,
      ValueT,
      HashFcn,
      EqualFcn,
      Allocator,
      ProbeFcn>;

  struct SimpleRetT {
    size_t idx;
    bool success;
    SimpleRetT(size_t i, bool s) : idx(i), success(s) {}
    SimpleRetT() = default;
  };

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal,
      typename LookupKeyToKeyFcn = Identity,
      typename... ArgTs>
  SimpleRetT insertInternal(LookupKeyT key, ArgTs&&... vCtorArgs);

  template <
      typename LookupKeyT = key_type,
      typename LookupHashFcn = hasher,
      typename LookupEqualFcn = key_equal>
  SimpleRetT findInternal(const LookupKeyT key);

  template <typename MaybeKeyT>
  void checkLegalKeyIfKey(MaybeKeyT key) {
    detail::checkLegalKeyIfKeyTImpl(key, kEmptyKey_, kLockedKey_, kErasedKey_);
  }

  static std::atomic<KeyT>* cellKeyPtr(const value_type& r) {
    static_assert(
        sizeof(std::atomic<KeyT>) == sizeof(KeyT),
        "std::atomic is implemented in an unexpected way for AHM");
    return const_cast<std::atomic<KeyT>*>(
        reinterpret_cast<std::atomic<KeyT> const*>(&r.first));
  }

  static KeyT relaxedLoadKey(const value_type& r) {
    return cellKeyPtr(r)->load(std::memory_order_relaxed);
  }

  static KeyT acquireLoadKey(const value_type& r) {
    return cellKeyPtr(r)->load(std::memory_order_acquire);
  }

  std::atomic<uint64_t> *numEntries_; 
  std::atomic<uint64_t> *numPendingEntries_; 
  std::atomic<int64_t>* isFull_; 
  std::atomic<int64_t>* numErases_; 

  value_type cells_[0]; 

  AtomicHashArray(
      size_t capacity,
      KeyT emptyKey,
      KeyT lockedKey,
      KeyT erasedKey,
      double maxLoadFactor,
      uint32_t cacheSize);

  AtomicHashArray(const AtomicHashArray&) = delete;
  AtomicHashArray& operator=(const AtomicHashArray&) = delete;

  ~AtomicHashArray()
  {
     Allocator().deallocate((char*)numEntries_,sizeof(std::atomic<uint64_t>));
     Allocator().deallocate((char*)numPendingEntries_,sizeof(std::atomic<uint64_t>));
     Allocator().deallocate((char*)isFull_,sizeof(std::atomic<int64_t>));
     Allocator().deallocate((char*)numErases_,sizeof(std::atomic<int64_t>));
  }

  inline void unlockCell(value_type* const cell, KeyT newKey) {
    cellKeyPtr(*cell)->store(newKey, std::memory_order_release);
  }

  inline bool tryLockCell(value_type* const cell) {
    KeyT expect = kEmptyKey_;
    return cellKeyPtr(*cell)->compare_exchange_strong(
        expect, kLockedKey_, std::memory_order_acq_rel);
  }

  template <class LookupKeyT = key_type, class LookupHashFcn = hasher>
  inline size_t keyToAnchorIdx(const LookupKeyT k) const {
    const size_t hashVal = LookupHashFcn()(k);
    const size_t probe = hashVal & kAnchorMask_;
    return (probe < capacity_) ? probe : hashVal % capacity_;  
  }

}; 

} 

#include "AtomicHashArray-inl_shm.h"
