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

#ifndef __V_REF_MAP_H_
#define __V_REF_MAP_H_

#include <functional>
#include <stdexcept>
#include "V_ref.h"
#include <iostream>

template <
    class KeyT,
    class ValueT,
    class HashFcn,
    class EqualFcn,
    class Allocator,
    class Locktype,
    class ProbeFcn>
class vec_ht_map {
  typedef vec_ht<
      KeyT,
      ValueT,
      HashFcn,
      EqualFcn,
      Allocator,
      Locktype,
      ProbeFcn>
      SubMap;

 public:

  typedef std::pair<const KeyT, ValueT> value_type;
  float kGrowthFrac_;
  std::atomic<SubMap*> *subMaps_;
  std::atomic<uint32_t>* numMapsAllocated_;
  Locktype *map_mutexes_;

   vec_ht_map(size_t,double,double);
  ~vec_ht_map(); 

  std::pair<std::pair<uint32_t,size_t>, bool> insert(const value_type& r) 
  {
    return emplace(r.first, r.second);
  }
  
  template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn>
  std::pair<uint32_t,size_t> find(LookupKeyT k);

  size_t size() const;
  
  size_t erase(KeyT k);

  void clear();

  size_t capacity() const;

  size_t spaceRemaining() const;

 private:
  static const uint32_t kNumSubMapBits_ = 4;
  static const uint32_t kSecondaryMapBit_ = 1u << 31;
  static const uint32_t kSubMapIndexShift_ = 32 - kNumSubMapBits_ - 1;
  static const uint32_t kSubMapIndexMask_ = (1 << kSubMapIndexShift_) - 1;
  static const uint32_t kNumSubMaps_ = 1 << kNumSubMapBits_;

   struct SimpleRetT {
    uint32_t i;
    size_t j;
    bool success;
    SimpleRetT(uint32_t ii, size_t jj, bool s) : i(ii), j(jj), success(s) {}
    SimpleRetT() = default;
  };

   template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn,
      typename... ArgTs>
  SimpleRetT insertInternal(LookupKeyT key, ArgTs&&... value);

     template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn>
  SimpleRetT findInternal(const LookupKeyT k) const;

     template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn,
      typename... ArgTs>
  std::pair<std::pair<uint32_t,size_t>, bool> emplace(LookupKeyT k, ArgTs&&... vCtorArg);


  int numSubMaps() const 
  {
    return numMapsAllocated_->load(std::memory_order_acquire);
  }

}; 

#include "V_ref_map_impl.h"

#endif
