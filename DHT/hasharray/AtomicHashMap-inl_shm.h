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

#ifndef FOLLY_ATOMICHASHMAP_H_
#error "This should only be included by AtomicHashMap.h"
#endif

#include <folly/detail/AtomicHashUtils.h>
#include <folly/detail/Iterators.h>

#include <type_traits>
#include <iostream>

namespace folly {

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::AtomicHashMap(size_t finalSizeEst,Config& config)
    :kGrowthFrac_(config.growthFactor < 0 ? 1.0f - config.maxLoadFactor
                                  : config.growthFactor)
    {
	  
      numMapsAllocated_ = (std::atomic<uint32_t>*)Allocator().allocate(sizeof(std::atomic<uint32_t>)); 
      subMaps_ = (std::atomic<SubMap*>*)Allocator().allocate(sizeof(std::atomic<SubMap*>)*kNumSubMaps_); 
      subMaps_[0].store(SubMap::create(finalSizeEst, config).release(),std::memory_order_relaxed);
      auto subMapCount = kNumSubMaps_;
      FOR_EACH_RANGE (i, 1, subMapCount) 
      {
        subMaps_[i].store(nullptr, std::memory_order_relaxed);
      }
      numMapsAllocated_->store(1, std::memory_order_relaxed);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <
    typename LookupKeyT,
    typename LookupHashFcn,
    typename LookupEqualFcn,
    typename LookupKeyToKeyFcn,
    typename... ArgTs>
std::pair<
    typename AtomicHashMap<
        KeyT,
        ValueT,
        HashFcn,
        EqualFcn,
        Allocator,
        ProbeFcn,
        KeyConvertFcn>::iterator,
    bool>
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::emplace(LookupKeyT k, ArgTs&&... vCtorArgs) {
  SimpleRetT ret = insertInternal<
      LookupKeyT,
      LookupHashFcn,
      LookupEqualFcn,
      LookupKeyToKeyFcn>(k, std::forward<ArgTs>(vCtorArgs)...);
  SubMap* subMap = subMaps_[ret.i].load(std::memory_order_relaxed);
  return std::make_pair(
      iterator(this, ret.i, subMap->makeIter(ret.j)), ret.success);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <
    typename LookupKeyT,
    typename LookupHashFcn,
    typename LookupEqualFcn,
    typename LookupKeyToKeyFcn,
    typename... ArgTs>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SimpleRetT
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::insertInternal(LookupKeyT key, ArgTs&&... vCtorArgs) {
beginInsertInternal:
  auto nextMapIdx = 
      numMapsAllocated_->load(std::memory_order_acquire);
  typename SubMap::SimpleRetT ret;
  FOR_EACH_RANGE (i, 0, nextMapIdx) {
    SubMap* subMap = subMaps_[i].load(std::memory_order_relaxed);
    ret = subMap->template insertInternal<
        LookupKeyT,
        LookupHashFcn,
        LookupEqualFcn,
        LookupKeyToKeyFcn>(key, std::forward<ArgTs>(vCtorArgs)...);
    if (ret.idx == subMap->capacity_) {
      continue; 
    }
    return SimpleRetT(i, ret.idx, ret.success);
  }

  SubMap* primarySubMap = subMaps_[0].load(std::memory_order_relaxed);
  if (nextMapIdx >= kNumSubMaps_ ||
      primarySubMap->capacity_ * kGrowthFrac_ < 1.0) {
    throw AtomicHashMapFullError();
  }

  if (tryLockMap(nextMapIdx)) {
    size_t numCellsAllocated =
        (size_t)(primarySubMap->capacity_ * std::pow(1.0 + kGrowthFrac_, nextMapIdx - 1));
    size_t newSize = size_t(numCellsAllocated * kGrowthFrac_);
    DCHECK(
        subMaps_[nextMapIdx].load(std::memory_order_relaxed) ==
        (SubMap*)kLockedPtr_);
    Config config;
    config.emptyKey = primarySubMap->kEmptyKey_;
    config.lockedKey = primarySubMap->kLockedKey_;
    config.erasedKey = primarySubMap->kErasedKey_;
    config.maxLoadFactor = primarySubMap->maxLoadFactor();
    config.entryCountThreadCacheSize =
        primarySubMap->getEntryCountThreadCacheSize();
    subMaps_[nextMapIdx].store(
        SubMap::create(newSize, config).release(), std::memory_order_relaxed);

    numMapsAllocated_->fetch_add(1, std::memory_order_release);
    DCHECK_EQ(
        nextMapIdx + 1, numMapsAllocated_->load(std::memory_order_relaxed));
  } else {
    detail::atomic_hash_spin_wait([&] {
      return nextMapIdx >= numMapsAllocated_->load(std::memory_order_acquire);
    });
  }

  SubMap* loadedMap = subMaps_[nextMapIdx].load(std::memory_order_relaxed);
  DCHECK(loadedMap && loadedMap != (SubMap*)kLockedPtr_);
  ret = loadedMap->insertInternal(key, std::forward<ArgTs>(vCtorArgs)...);
  if (ret.idx != loadedMap->capacity_) {
    return SimpleRetT(nextMapIdx, ret.idx, ret.success);
  }
  goto beginInsertInternal;
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::iterator
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::find(LookupKeyT k) {
  SimpleRetT ret = findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
  if (!ret.success) {
    return end();
  }
  SubMap* subMap = subMaps_[ret.i].load(std::memory_order_relaxed);
  return iterator(this, ret.i, subMap->makeIter(ret.j));
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::const_iterator
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::find(LookupKeyT k) const {
  return const_cast<AtomicHashMap*>(this)
      ->find<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SimpleRetT
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::findInternal(const LookupKeyT k) const {
  SubMap* const primaryMap = subMaps_[0].load(std::memory_order_relaxed);
  typename SubMap::SimpleRetT ret =
      primaryMap
          ->template findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
  if (LIKELY(ret.idx != primaryMap->capacity_)) {
    return SimpleRetT(0, ret.idx, ret.success);
  }
  const unsigned int numMaps =
      numMapsAllocated_->load(std::memory_order_acquire);
  FOR_EACH_RANGE (i, 1, numMaps) {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    ret =
        thisMap
            ->template findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(
                k);
    if (LIKELY(ret.idx != thisMap->capacity_)) {
      return SimpleRetT(i, ret.idx, ret.success);
    }
  }
  return SimpleRetT(numMaps, 0, false);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::SimpleRetT
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::findAtInternal(uint32_t idx) const {
  uint32_t subMapIdx, subMapOffset;
  if (idx & kSecondaryMapBit_) {
    idx &= ~kSecondaryMapBit_; 
    subMapIdx = idx >> kSubMapIndexShift_;
    DCHECK_LT(subMapIdx, numMapsAllocated_->load(std::memory_order_relaxed));
    subMapOffset = idx & kSubMapIndexMask_;
  } else {
    subMapIdx = 0;
    subMapOffset = idx;
  }
  return SimpleRetT(subMapIdx, subMapOffset, true);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
typename AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::size_type
AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::erase(const KeyT k) {
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  FOR_EACH_RANGE (i, 0, numMaps) {
    if (subMaps_[i].load(std::memory_order_relaxed)->erase(k)) {
      return 1;
    }
  }
  return 0;
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
size_t AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::capacity() const {
  size_t totalCap(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  FOR_EACH_RANGE (i, 0, numMaps) {
    totalCap += subMaps_[i].load(std::memory_order_relaxed)->capacity_;
  }
  return totalCap;
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
size_t AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::spaceRemaining() const {
  size_t spaceRem(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  FOR_EACH_RANGE (i, 0, numMaps) {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    spaceRem +=
        std::max(0, thisMap->maxEntries_ - &thisMap->numEntries_.readFull());
  }
  return spaceRem;
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
void AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::clear() {
  subMaps_[0].load(std::memory_order_relaxed)->clear();
  int const numMaps = numMapsAllocated_->load(std::memory_order_relaxed);
  FOR_EACH_RANGE (i, 1, numMaps) {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    DCHECK(thisMap);
    SubMap::destroy(thisMap);
    subMaps_[i].store(nullptr, std::memory_order_relaxed);
  }
  numMapsAllocated_->store(1, std::memory_order_relaxed);
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
size_t AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::size() const {
  size_t totalSize(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  FOR_EACH_RANGE (i, 0, numMaps) {
    totalSize += subMaps_[i].load(std::memory_order_relaxed)->size();
  }
  return totalSize;
}

template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
inline uint32_t AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::encodeIndex(uint32_t subMap, uint32_t offset) {
  DCHECK_EQ(offset & kSecondaryMapBit_, 0); 
  if (subMap == 0) {
    return offset;
  }
  DCHECK_EQ(subMap >> kNumSubMapBits_, 0);
  DCHECK_EQ(offset & (~kSubMapIndexMask_ | kSecondaryMapBit_), 0);

  return offset | (subMap << kSubMapIndexShift_) | kSecondaryMapBit_;
}


template <
    typename KeyT,
    typename ValueT,
    typename HashFcn,
    typename EqualFcn,
    typename Allocator,
    typename ProbeFcn,
    typename KeyConvertFcn>
template <class ContT, class IterVal, class SubIt>
struct AtomicHashMap<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    ProbeFcn,
    KeyConvertFcn>::ahm_iterator
    : detail::IteratorFacade<
          ahm_iterator<ContT, IterVal, SubIt>,
          IterVal,
          std::forward_iterator_tag> {
  explicit ahm_iterator() : ahm_(nullptr) {}

  template <class OtherContT, class OtherVal, class OtherSubIt>
  ahm_iterator(
      const ahm_iterator<OtherContT, OtherVal, OtherSubIt>& o,
      typename std::enable_if<
          std::is_convertible<OtherSubIt, SubIt>::value>::type* = nullptr)
      : ahm_(o.ahm_), subMap_(o.subMap_), subIt_(o.subIt_) {}

  uint32_t getIndex() const {
    CHECK(!isEnd());
    return ahm_->encodeIndex(subMap_, subIt_.getIndex());
  }

 private:
  friend class AtomicHashMap;
  explicit ahm_iterator(ContT* ahm, uint32_t subMap, const SubIt& subIt)
      : ahm_(ahm), subMap_(subMap), subIt_(subIt) {}

  friend class detail::
      IteratorFacade<ahm_iterator, IterVal, std::forward_iterator_tag>;

  void increment() {
    CHECK(!isEnd());
    ++subIt_;
    checkAdvanceToNextSubmap();
  }

  bool equal(const ahm_iterator& other) const {
    if (ahm_ != other.ahm_) {
      return false;
    }

    if (isEnd() || other.isEnd()) {
      return isEnd() == other.isEnd();
    }

    return subMap_ == other.subMap_ && subIt_ == other.subIt_;
  }

  IterVal& dereference() const { return *subIt_; }

  bool isEnd() const { return ahm_ == nullptr; }

  void checkAdvanceToNextSubmap() {
    if (isEnd()) {
      return;
    }

    SubMap* thisMap = ahm_->subMaps_[subMap_].load(std::memory_order_relaxed);
    while (subIt_ == thisMap->end()) {
      if (subMap_ + 1 <
          ahm_->numMapsAllocated_->load(std::memory_order_acquire)) {
        ++subMap_;
        thisMap = ahm_->subMaps_[subMap_].load(std::memory_order_relaxed);
        subIt_ = thisMap->begin();
      } else {
        ahm_ = nullptr;
        return;
      }
    }
  }

 private:
  ContT* ahm_;
  uint32_t subMap_;
  SubIt subIt_;
}; 

} 
