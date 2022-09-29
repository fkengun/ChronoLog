
#include "V_ref.h"

#include <type_traits>
#include <cassert>
#include <cstring>



template<class KeyT,
	class ValueT,
	class HashFcn,
	class EqualFcn,
	class Allocator,
	class Locktype,
	class ProbeFcn>
        vec_ht<KeyT,ValueT,HashFcn,EqualFcn,Allocator,Locktype,ProbeFcn>
	::vec_ht(size_t capacity,KeyT emptyKey,double maxLoadFactor)
	: capacity_(capacity),
      	  maxEntries_(size_t(maxLoadFactor * capacity_ + 0.5)),
      	  kEmptyKey_(emptyKey),
	  kAnchorMask_(nextPowTwo(capacity_) - 1)
       {
	assert(maxLoadFactor <= 1.0);
	assert(maxLoadFactor > 0.0);
        assert (capacity > 0);
	
	 cells_ = (value_type*) Allocator().allocate(sizeof(value_type)*capacity);

	 mutexes_ = (Locktype*) Allocator().allocate(sizeof(Locktype)*capacity);

	 for(int i=0;i<capacity;i++)
	 {
	    cells_[i].first = kEmptyKey_;
	 }
	 numEntries_.store(0,std::memory_order_relaxed);
       }

template<class KeyT,
	 class ValueT,
	 class HashFcn,
	 class EqualFcn,
	 class Allocator,
	 class Locktype,
	 class ProbeFcn>
	 vec_ht<KeyT,ValueT,HashFcn,EqualFcn,Allocator,Locktype,ProbeFcn>
	 ::~vec_ht()
{
	Allocator().deallocate((char*)cells_,sizeof(value_type)*capacity_);
	Allocator().deallocate((char*)mutexes_,sizeof(Locktype)*capacity_);
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
typename vec_ht<KeyT,
	 ValueT,
	 HashFcn,
	 EqualFcn,
	 Allocator,
	 Locktype,
	 ProbeFcn>::
SimpleRetT
vec_ht<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::findInternal(const LookupKeyT key_in) 
   {
     checkLegalKeyIfKey<LookupKeyT>(key_in);

  for (size_t idx = keyToAnchorIdx<LookupKeyT, LookupHashFcn>(key_in),
              numProbes = 0;;idx = ProbeFcn()(idx, numProbes, capacity_))
  {
	  mutexes_[idx].lock();
	  const KeyT key = cells_[idx].first; 
	  mutexes_[idx].unlock();
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
    class Locktype,
    class ProbeFcn>
void vec_ht<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::clear()
{
  for(int i=0;i<capacity_;i++)
  {
     cells_[i].first = kEmptyKey_;
  }
  numEntries_.store(0,std::memory_order_relaxed);
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
typename vec_ht<KeyT,
	 ValueT,
	 HashFcn,
	 EqualFcn,
	 Allocator,
	 Locktype,
	 ProbeFcn>::
SimpleRetT
vec_ht<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::insertInternal(LookupKeyT key_in, ArgTs&&... vCtorArgs) {
  const short NO_NEW_INSERTS = 1;
  const short NO_PENDING_INSERTS = 2;
  checkLegalKeyIfKey<LookupKeyT>(key_in);

  size_t idx = keyToAnchorIdx<LookupKeyT, LookupHashFcn>(key_in);
  size_t numProbes = 0;
  for (;;) 
  {
    assert(idx < capacity_);
    mutexes_[idx].lock();
    if(cells_[idx].first == kEmptyKey_)
    {
            KeyT key_new = key_in;
            checkLegalKeyIfKey(key_new);
	    cells_[idx].first = key_new;
	    ValueT v = ValueT(std::forward<ArgTs>(vCtorArgs)...);
            using mapped = typename std::remove_const<ValueT>::type;
	    new (const_cast<mapped*>(&(cells_[idx].second))) ValueT(std::forward<ArgTs>(vCtorArgs)...);
	    //std::memcpy(&(cells_[idx].second),&v,sizeof(ValueT));
            assert(cells_[idx].first != kEmptyKey_);
	    numEntries_.fetch_add(1,std::memory_order_relaxed);
	    mutexes_[idx].unlock(); 
	    return SimpleRetT(idx, true);
    }

    mutexes_[idx].unlock();
    ++numProbes;
    if ((numProbes >= capacity_))
    {
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
    class Locktype,
    class ProbeFcn>
size_t vec_ht<
    KeyT,
    ValueT,
    HashFcn,
    EqualFcn,
    Allocator,
    Locktype,
    ProbeFcn>::erase(KeyT key_in)
{
   assert (key_in != kEmptyKey_);
   for (size_t idx = keyToAnchorIdx(key_in), numProbes = 0;;
       idx = ProbeFcn()(idx, numProbes, capacity_))
  {
    assert(idx < capacity_);
    mutexes_[idx].lock();
    KeyT currentKey = cells_[idx].first;
    if (currentKey == kEmptyKey_) 
    {
      mutexes_[idx].unlock();
      return 0;
    }
    if (EqualFcn()(currentKey, key_in))
    {
      cells_[idx].first = kEmptyKey_;
      mutexes_[idx].unlock();
      return 1;
    }
    
    mutexes_[idx].unlock();
    ++numProbes;
    if ((numProbes >= capacity_)) 
    {
      return 0;
    }
  }

}

