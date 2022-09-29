#ifndef _V_REF_H_
#define _V_REF_H_

#include <mutex>
#include <vector>
#include <climits>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <memory>
#include <atomic>

struct LinearProbeFcn {
  inline size_t operator()(
      size_t idx, size_t , size_t capacity) const {
    idx += 1;
    return (idx < capacity) ? idx : (idx - capacity);
  }
};

size_t nextPowTwo(size_t c)
{

  size_t n = 1;

  while(n < c)
  {
        n = 2*n;
  }

  return n;
}

template <typename KeyT>
inline void checkLegalKeyIfKeyTImpl(
    KeyT key_in, KeyT emptyKey) {
    assert(key_in != emptyKey);
}

template<class KeyT,
	class ValueT,
	class HashFcn = std::hash<KeyT>,
	class EqualFcn = std::equal_to<KeyT>,
	class Allocator = std::allocator<char>,
	class Locktype = std::mutex,
	class ProbeFcn = LinearProbeFcn>
	class vec_ht_map;

template<class KeyT,
	class ValueT,
	class HashFcn = std::hash<KeyT>,
        class EqualFcn = std::equal_to<KeyT>,
	class Allocator = std::allocator<char>,
	class Locktype = std::mutex,
        class ProbeFcn = LinearProbeFcn>	
class vec_ht
{

 public:
   typedef std::pair<KeyT,ValueT> value_type;
   size_t capacity_;
   size_t maxEntries_;
   KeyT kEmptyKey_;
   size_t kAnchorMask_;
   value_type *cells_;
   Locktype *mutexes_;
   std::atomic<uint64_t> numEntries_;

   vec_ht(size_t capacity,KeyT emptyKey ,double maxLoadFactor);
   ~vec_ht();

  void clear();

  size_t erase(KeyT k);

  std::pair<std::pair<KeyT,ValueT>, bool> insert(const value_type& r) 
  {
    return emplace(r.first, std::move(r.second));
  }

     template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn>
  std::pair<KeyT,ValueT> find(LookupKeyT k)
  {
    return std::make_pair(k,findInternal<LookupKeyT, LookupHashFcn, LookupEqualFcn>(k).idx);
  }

  double maxLoadFactor() const
  {
    return ((double)maxEntries_) / capacity_;
  }

 private:

  friend class vec_ht_map
	 <KeyT,
	 ValueT,
	 HashFcn,
	 EqualFcn,
	 Allocator,
	 Locktype,
	 ProbeFcn>;

  struct SimpleRetT
  {
    size_t idx;
    ValueT v;
    bool success;
    SimpleRetT(size_t i, bool s) : idx(i), success(s) {}
    SimpleRetT() = default;
  };

  template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn,
      typename... ArgTs>
  std::pair<std::pair<KeyT,ValueT>, bool> emplace(LookupKeyT key_in, ArgTs&&... vCtorArgs)
  {
    SimpleRetT ret = insertInternal<
        LookupKeyT,
        LookupHashFcn,
        LookupEqualFcn>(key_in, std::forward<ArgTs>(vCtorArgs)...);
    return std::make_pair(cells_[ret.idx], ret.success);
  }

   template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn,
      typename... ArgTs>
  SimpleRetT insertInternal(LookupKeyT key, ArgTs&&... vCtorArgs);

  template <typename MaybeKeyT>
  void checkLegalKeyIfKey(MaybeKeyT key)
  {
    checkLegalKeyIfKeyTImpl(key, kEmptyKey_);
  }

  template <class LookupKeyT = KeyT, class LookupHashFcn = HashFcn>
  inline size_t keyToAnchorIdx(const LookupKeyT k) const
  {
    const size_t hashVal = LookupHashFcn()(k);
    const size_t probe = hashVal & kAnchorMask_;
    return (probe < capacity_) ? probe : hashVal % capacity_;
  }

   template <
      typename LookupKeyT = KeyT,
      typename LookupHashFcn = HashFcn,
      typename LookupEqualFcn = EqualFcn>
  SimpleRetT findInternal(const LookupKeyT key);


};

#include "V_ref_impl.h"

#endif


   








