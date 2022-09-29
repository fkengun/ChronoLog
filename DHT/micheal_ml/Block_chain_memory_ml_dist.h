#ifndef __BLOCK_CHAIN_ML_
#define __BLOCK_CHAIN_ML_

#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <mutex>
#include <cassert>
#include <atomic>
#include <memory>
#include "Block_table.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#define MAX_COLLISIONS 10
#define NOT_IN_TABLE UINT32_MAX
#define MAX_TABLE_SIZE 0.8*maxSize*MAX_COLLISIONS
#define MIN_TABLE_SIZE 0.15*maxSize*MAX_COLLISIONS
#define FULL 2
#define EXISTS 1
#define INSERTED 0

template <
	class KeyT,
	class ValueT,
	class HashFcn=std::hash<KeyT>,
	class EqualFcn=std::equal_to<KeyT>>
struct fnode_ml
{
    boost::shared_mutex mutex_t;
    BlockTable<KeyT,ValueT,HashFcn,EqualFcn> *table;
};

template <
    class KeyT,
    class ValueT, 
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>>
class BlockMap_ml
{

   public :
	typedef struct node<KeyT,ValueT,HashFcn,EqualFcn> node_type;
	typedef struct fnode_ml<KeyT,ValueT,HashFcn,EqualFcn> fnode_type;
	typedef BlockTable<KeyT,ValueT,HashFcn,EqualFcn> blocktable_type;
	typedef struct f_node<KeyT,ValueT,HashFcn,EqualFcn> fnode;

   private :
	fnode_type *table;
	uint64_t totalSize;
	uint32_t maxSize;
	std::atomic<uint32_t> *allocated;
	std::atomic<uint32_t> *removed;
	memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *pl;
	int sub_table_size;
	uint32_t num_procs;
	uint32_t myrank;

	uint32_t next_power_two(uint32_t n)
        {
            uint32_t c = 1;

            while(c < n)
            {
               c = c*2;
            }

            return c;
        }

	uint64_t KeyToIndex(KeyT k)
	{
	    uint64_t hashval = HashFcn()(k);
	    uint32_t shift = log2(num_procs);
	    hashval = hashval >> shift;
	    return hashval%maxSize;
	}
  public:
	uint64_t procLocation(KeyT k)
	{
	   uint64_t hashval = HashFcn()(k);
	   uint64_t mask = UINT64_MAX;
	   uint32_t shift = log2(num_procs);
	   mask = mask >> (64-shift);
	   return hashval & mask;
	}

        bool isLocal(KeyT k)
	{
	   uint64_t hashval = HashFcn()(k);
	   uint64_t mask = UINT64_MAX;
	   uint32_t shift = log2(num_procs);
	   mask = mask >> (64-shift);
	   uint64_t proc_id = hashval & mask;
	   if(proc_id==myrank) return true;
	   else return false;
	}

	BlockMap_ml(uint32_t n,memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *m,uint32_t s,uint32_t np, uint32_t rank) : totalSize(n), pl(m),sub_table_size(s),num_procs(np), myrank(rank)
	{
  	   assert (totalSize > 0);
	   totalSize = next_power_two(totalSize);
	   assert (next_power_two(num_procs)==num_procs);
	   maxSize = totalSize/num_procs;

	   maxSize = next_power_two(maxSize);

	   allocated = (std::atomic<uint32_t>*)std::malloc(maxSize*sizeof(std::atomic<uint32_t>));
	   removed = (std::atomic<uint32_t>*)std::malloc(maxSize*sizeof(std::atomic<uint32_t>));
	   sub_table_size = next_power_two(sub_table_size);
	   table = (fnode_type *)std::malloc(maxSize*sizeof(fnode_type));
	   for(int i=0;i<maxSize;i++)
	   {
	      allocated[i].store(0);
	      removed[i].store(0);
	      table[i].table = new blocktable_type(1,pl,log2(maxSize)+log2(num_procs));
	   }
	   assert(log2(maxSize)+log2(sub_table_size) <= 32);
	}

  	~BlockMap_ml()
	{
	    for(int i=0;i<maxSize;i++)
	    {
	       if(table[i].table != nullptr) delete table[i].table;
	    }
	    std::free(table);
	    std::free((void*)allocated); std::free((void*)removed);
	}

	int insert(KeyT k,ValueT v)
	{
	    uint32_t hash_value = HashFcn()(k);
	    uint32_t pos = KeyToIndex(k);

	    int r = -1;

	    bool found = false;

	    boost::shared_lock<boost::shared_mutex> lk1(table[pos].mutex_t);
	    {	    
	       r = table[pos].table->insert(k,v);
	    }
	    lk1.unlock();
	    lk1.release();

	    if(r==INSERTED) allocated[pos].fetch_add(1);

	    uint32_t a = allocated[pos].load(); uint32_t b = removed[pos].load();
	    if(a-b > 1) 
	    {
	       boost::upgrade_lock<boost::shared_mutex> lk2(table[pos].mutex_t);
	       {
	         table[pos].table->expand_shrink_table(sub_table_size);
	       }
	       lk2.unlock();
	       lk2.release();
	    }

	    return r;
	}
		
	uint32_t find(KeyT k)
	{
	    uint32_t pos = KeyToIndex(k);

	    uint32_t r = NOT_IN_TABLE;

            boost::shared_lock<boost::shared_mutex>  lk(table[pos].mutex_t);
	    {
	      r = table[pos].table->find(k);
	    }
	    lk.unlock();
	    lk.release();

	    return (r != NOT_IN_TABLE) ? pos : NOT_IN_TABLE;
	}

	bool update(KeyT k,ValueT v)
	{
	   uint32_t pos = KeyToIndex(k);

	   bool found = false;
	   boost::shared_lock<boost::shared_mutex> lk(table[pos].mutex_t);
	   {
	     found = table[pos].table->update(k,v);
	   }
	   lk.unlock();
	   lk.release();

	   return found;
	}

	bool erase(KeyT k)
	{
	   uint32_t pos = KeyToIndex(k);

	   bool found = false;
	 
	   boost::shared_lock<boost::shared_mutex> lk1(table[pos].mutex_t); 
	   found = table[pos].table->erase(k);
	   lk1.unlock();
	   lk1.release();
	   
	   if(found) removed[pos].fetch_add(1);
	   uint32_t a = allocated[pos].load(); uint32_t b = removed[pos].load();
	   if(a-b < 2) 
	   {
	     boost::upgrade_lock<boost::shared_mutex> lk2(table[pos].mutex_t);
	     table[pos].table->expand_shrink_table(1);
	     lk2.unlock();
	     lk2.release();
	   }

	   return found;
	}

	uint64_t allocated_sum()
	{
	   uint64_t sum = 0;
	   for(uint64_t i=0;i<maxSize;i++)
		 sum += allocated[i].load();
	   return sum;
	}
	
	uint64_t removed_sum()
	{
	   uint64_t sum = 0;
	   for(uint64_t i=0;i<maxSize;i++)
		   sum += removed[i].load();
	   return sum;
	}
};

#endif
