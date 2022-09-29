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

#include "V_ref_map.h"
#include <cmath>

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::vec_ht_map(size_t finalSizeEst,double growthFactor,double maxLoadFactor)
    :kGrowthFrac_(growthFactor < 0 ? 1.0f - maxLoadFactor
                                  : growthFactor)
    {
	  
      numMapsAllocated_ = (std::atomic<uint32_t>*)Allocator().allocate(sizeof(std::atomic<uint32_t>)); 
      subMaps_ = (std::atomic<SubMap*>*)Allocator().allocate(sizeof(std::atomic<SubMap*>)*kNumSubMaps_);
      map_mutexes_ = (Locktype*)Allocator().allocate(sizeof(Locktype)*kNumSubMaps_);

      vec_ht<KeyT,ValueT,HashFcn,EqualFcn,Allocator,Locktype,ProbeFcn> *v = new vec_ht<KeyT,ValueT,HashFcn,EqualFcn,Allocator,Locktype,ProbeFcn> (512,-1,maxLoadFactor); 

      for(int i=0;i<kNumSubMaps_;i++)
	subMaps_[i].store(nullptr);
      subMaps_[0].store(v,std::memory_order_relaxed);
      auto subMapCount = kNumSubMaps_;
      for(int i=1;i<subMapCount;i++) 
      {
        subMaps_[i].store(nullptr, std::memory_order_relaxed);
      }
      numMapsAllocated_->store(1, std::memory_order_relaxed);
}

template<
class KeyT,
class ValueT,
class HashFcn,
class EqualFcn,
class Allocator,
class Locktype,      
class ProbeFcn>
vec_ht_map<KeyT,
	ValueT,
	HashFcn,
	EqualFcn,
	Allocator,
	Locktype,
	ProbeFcn>::~vec_ht_map()
{
    Allocator().deallocate((char*)numMapsAllocated_,sizeof(std::atomic<uint32_t>));
    Allocator().deallocate((char*)subMaps_,sizeof(std::atomic<SubMap*>)*kNumSubMaps_);
    Allocator().deallocate((char*)map_mutexes_,sizeof(Locktype)*kNumSubMaps_);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
template <
    typename LookupKeyT,
    typename LookupHashFcn,
    typename LookupEqualFcn,
    typename... ArgTs>
std::pair<
    std::pair<uint32_t,size_t>,
    bool>
vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::emplace(LookupKeyT k, ArgTs&&... vCtorArgs) {
  SimpleRetT ret = insertInternal<
      LookupKeyT,
      LookupHashFcn,
      LookupEqualFcn>(k, std::forward<ArgTs>(vCtorArgs)...);
  SubMap* subMap = subMaps_[ret.i].load(std::memory_order_relaxed);
  return std::make_pair(std::pair<uint32_t,size_t>(ret.i,ret.j),ret.success);
}


template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
template <
    typename LookupKeyT,
    typename LookupHashFcn,
    typename LookupEqualFcn,
    typename... ArgTs>
typename vec_ht_map
	 <KeyT,
	 ValueT,
	 HashFcn,
	 EqualFcn,
	 Allocator,
	 Locktype,
	 ProbeFcn>::SimpleRetT
    vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::insertInternal(LookupKeyT key, ArgTs&&... vCtorArgs) 
{

  typename SubMap::SimpleRetT ret;
	
  while(1)
  {
    
  auto nextMapIdx = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<nextMapIdx;i++)
  {
    SubMap* subMap = subMaps_[i].load(std::memory_order_relaxed);
    ret = subMap->template insertInternal<
        LookupKeyT,
        LookupHashFcn,
        LookupEqualFcn>(key, std::forward<ArgTs>(vCtorArgs)...);
    if (ret.idx == subMap->capacity_) 
    {
      continue; 
    }
    return SimpleRetT(i, ret.idx, ret.success);
  }
  
  SubMap* primarySubMap = subMaps_[0].load(std::memory_order_relaxed);
  if (nextMapIdx >= kNumSubMaps_ || primarySubMap->capacity_ * kGrowthFrac_ < 1.0) 
  {
	  std::cout <<" Out of memory"<<std::endl;
	  return SimpleRetT(nextMapIdx,-1,false);

  }
 
  map_mutexes_[nextMapIdx].lock();
  size_t numCellsAllocated = (size_t)(primarySubMap->capacity_ * std::pow(1.0 + kGrowthFrac_, nextMapIdx - 1));
  size_t newSize = size_t(numCellsAllocated * kGrowthFrac_);
  if(numMapsAllocated_->load()==nextMapIdx)
  {
    SubMap* s = new vec_ht<KeyT,ValueT,HashFcn,EqualFcn,Allocator,Locktype,ProbeFcn> (512,-1,0.8); 
    subMaps_[nextMapIdx].store(s);
    numMapsAllocated_->fetch_add(1, std::memory_order_release);
  }
  map_mutexes_[nextMapIdx].unlock();

  SubMap* loadedMap = subMaps_[nextMapIdx].load(std::memory_order_relaxed);
  assert(loadedMap != nullptr);
  ret = loadedMap->insertInternal(key, std::forward<ArgTs>(vCtorArgs)...);
  if (ret.idx != loadedMap->capacity_) 
  {
    return SimpleRetT(nextMapIdx, ret.idx, ret.success);
  }
 }
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
std::pair<uint32_t,size_t>
vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::find(LookupKeyT k) 
{
  SimpleRetT ret = findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
  if (!ret.success) 
  {
    return std::pair<uint32_t,size_t>(-1,-1);
  }
  SubMap* subMap = subMaps_[ret.i].load(std::memory_order_relaxed);
  return std::pair<uint32_t,size_t>(ret.i,ret.j);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
template <class LookupKeyT, class LookupHashFcn, class LookupEqualFcn>
typename vec_ht_map<KeyT,
	  ValueT,
	  HashFcn,
	  EqualFcn,
	  Allocator,
	  Locktype,
	  ProbeFcn>::SimpleRetT
vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::findInternal(const LookupKeyT k) const 
{
  const unsigned int numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<numMaps;i++)
  {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    typename SubMap::SimpleRetT ret =thisMap->template findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k);
    if (ret.idx != thisMap->capacity_) 
    {
      return SimpleRetT(i, ret.idx, ret.success);
    }
  }
  return SimpleRetT(numMaps, 0, false);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
size_t
vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::erase(const KeyT k) 
{
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<numMaps;i++) 
  {
    if (subMaps_[i].load(std::memory_order_relaxed)->erase(k)) {
      return 1;
    }
  }
  return 0;
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
size_t vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::capacity() const {
  size_t totalCap(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<numMaps;i++) 
  {
    totalCap += subMaps_[i].load(std::memory_order_relaxed)->capacity_;
  }
  return totalCap;
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
size_t vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::spaceRemaining() const 
{

  size_t spaceRem(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<numMaps;i++)
  {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    spaceRem +=std::max(0, 0);
  }
  return spaceRem;
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
void vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::clear() 
{
  subMaps_[0].load(std::memory_order_relaxed)->clear();
  int const numMaps = numMapsAllocated_->load(std::memory_order_relaxed);
  for(int i=1;i<numMaps;i++)
  {
    SubMap* thisMap = subMaps_[i].load(std::memory_order_relaxed);
    delete thisMap;
    subMaps_[i].store(nullptr, std::memory_order_relaxed);
  }
  numMapsAllocated_->store(1, std::memory_order_relaxed);
}

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
size_t vec_ht_map<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::size() const 
{
  size_t totalSize(0);
  int const numMaps = numMapsAllocated_->load(std::memory_order_acquire);
  for(int i=0;i<numMaps;i++) 
  {
    totalSize += subMaps_[i].load(std::memory_order_relaxed)->size();
  }
  return totalSize;
}
