#ifndef __HASHMAP_CHAIN_ML_H_
#define __HASHMAP_CHAIN_ML_H_

#include "Block_chain_memory.h"

template <class KeyT,
	  class ValueT,
	  class HashFcn=std::hash<KeyT>,
	  class EqualFcn=std::equal_to<KeyT>>
class HashMap_ml
{
   public : 
	  typedef BlockMap<KeyT,ValueT,HashFcn,EqualFcn> block_type;

   private :










};

#endif
