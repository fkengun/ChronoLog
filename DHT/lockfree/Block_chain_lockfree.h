#ifndef __BLOCK_CHAIN_LOCKFREE_
#define __BLOCK_CHAIN_LOCKFREE_

#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <atomic>

#include "lockfree_ll.h"

#define MAX_COLLISIONS 10
#define NOT_IN_TABLE UINT32_MAX
#define FULL 1
#define EMPTY 2
#define SENTINEL 3
#define MAX_LIST_SIZE 100

template <
	class KeyT,
	class ValueT,
	class HashFcn = std::hash<KeyT>,
	class EqualFcn = std::equal_to<KeyT>>
struct f_node
{
    ll_lf<KeyT,ValueT,HashFcn,EqualFcn> *list;
    std::atomic<uint32_t> num_nodes; 
};

template <
    class KeyT,
    class ValueT, 
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>>
class BlockMap_lockfree
{

   public :
	   typedef struct f_node<KeyT,ValueT,HashFcn,EqualFcn> fnode_type;
   private :
	fnode_type *table;
	uint64_t maxSize;
        boost::atomic<uint32_t> allocated;
	boost::atomic<uint32_t> removed;
        memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *pl;
	KeyT maxKey;

	uint64_t KeyToIndex(KeyT k)
	{
	    size_t hashval = HashFcn()(k);
	    return hashval % maxSize;
	}
  public:
	BlockMap_lockfree(uint64_t n,memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *p,KeyT m) : maxSize(n),pl(p),maxKey(m)
	{
  	   assert (maxSize > 0);
	   table = (fnode_type *)std::malloc(maxSize*sizeof(fnode_type));
	   for(size_t i=0;i<maxSize;i++)
	   {
	      table[i].list = new ll_lf<KeyT,ValueT,HashFcn,EqualFcn> (pl,&allocated,&removed,maxKey);
	      table[i].num_nodes.store(0);
	   }
	   allocated.store(0);
	   removed.store(0);
	   assert(maxSize < UINT64_MAX && maxSize*MAX_COLLISIONS < UINT64_MAX);
	}

  	~BlockMap_lockfree()
	{
	   for(size_t i=0;i<maxSize;i++)
	   {
		 delete table[i].list;
	   }
	   std::free(table);
	}

	bool insert(KeyT k,ValueT v)
	{
	    uint64_t pos = KeyToIndex(k);
	    bool ret = table[pos].list->insert(k,v); 
	    return ret;
	}
	 
	
	uint32_t find(KeyT k)
	{
	    uint64_t pos = KeyToIndex(k);

	    bool found = table[pos].list->contains(k);

	    return (found ? pos : NOT_IN_TABLE);
	}
        	
	bool erase(KeyT k)
	{
	   uint64_t pos = KeyToIndex(k);
	   bool found = table[pos].list->erase(k);
	   return found;
	}

	uint32_t allocated_nodes()
	{
		return allocated.load();
	}

	uint32_t removed_nodes()
	{
		return removed.load();
	}

	int num_entries()
	{
		int numentries = 0;
		for(int i=0;i<maxSize;i++)
			numentries += table[i].list->list_size();
		return numentries;
	}

        bool block_full()
	{
	   if(allocated.load()-removed.load() >= 0.8*maxSize*MAX_COLLISIONS) return true;
	   else return false;

	}	
};

#endif
