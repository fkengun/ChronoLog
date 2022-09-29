#ifndef __HashMap_Chain_H_
#define __HashMap_Chain_H_

#include "Block_chain_lockfree.h"
#include <mutex>

template <
	class KeyT,
	class ValueT,
	class HashFcn = std::hash<KeyT>,
	class EqualFcn = std::equal_to<KeyT>>
class HashMap_lockfree
{
   private :
	   uint32_t maxBlocks;
	   std::atomic<uint32_t> activeBlocks;
	   uint32_t blockSize;
	   std::vector<BlockMap_lockfree<KeyT,ValueT,HashFcn,EqualFcn>*> blocks;
	   std::vector<std::mutex> block_mutexes;
	   memory_pool<KeyT,ValueT> *pl;
	   KeyT maxKey;

  public:
	HashMap_lockfree(uint32_t n,uint32_t m,memory_pool<KeyT,ValueT>*p,KeyT t) : maxBlocks(n), blockSize(m),pl(p),maxKey(t)
	{
	    assert (maxBlocks > 0 && blockSize > 0);
	    blocks.resize(maxBlocks);
	    for(uint32_t i=0;i<maxBlocks;i++)
		    blocks[i] = nullptr;
	    std::vector<std::mutex> temp(maxBlocks);
	    std::swap(block_mutexes,temp);
	    activeBlocks.store(1);
	    blocks[0] = new BlockMap_lockfree<KeyT,ValueT,HashFcn,EqualFcn> (blockSize,pl,maxKey);
	}
	~HashMap_lockfree()
	{
	   for(uint32_t i=0;i<activeBlocks.load();i++)
		if(blocks[i] != nullptr) delete blocks[i];
	
	}
	bool insert(KeyT k,ValueT v)
	{
	   uint32_t num_active = activeBlocks.load();
	   uint32_t table_no;
           bool found = false;
	   uint32_t num_active_ = num_active;

	   for(uint32_t i=0;i<num_active;i++)
	   {
		uint32_t p = blocks[i]->find(k);
		found = (p != NOT_IN_TABLE) ? true : false;
		if(found) break;
	   }

	   if(!found)
	   {
	       do
	       {
	     	  found = false;
	          num_active = activeBlocks.load();
	          uint32_t block_m = num_active-1;
	          if(blocks[block_m]->block_full() && block_m < maxBlocks-1)
	          {
		    block_mutexes[block_m+1].lock();
		    if(activeBlocks.load()==num_active)
		    {
		      blocks[block_m+1] = new BlockMap_lockfree<KeyT,ValueT,HashFcn,EqualFcn> (blockSize,pl,maxKey);
		      activeBlocks.fetch_add(1);
		    }
		    block_mutexes[block_m+1].unlock();
	          }
		  else if(blocks[block_m]->block_full())
		  {
			  std::cout <<" error : max blocks reached"<<std::endl;
			  break;
		  }

	          for(uint32_t i=num_active_;i<num_active;i++)
	          {
		    uint32_t p = blocks[i]->find(k);
		    found = (p != NOT_IN_TABLE) ? true : false;
		    if(found) break;
	          }
	          num_active_ = num_active;
	        }while(num_active!=activeBlocks.load());
	   }
	   return (found ? false : blocks[num_active-1]->insert(k,v));

	}


	std::pair<uint32_t,uint32_t> find(KeyT k)
	{
	   uint32_t num_active = activeBlocks.load();
	   uint32_t v = NOT_IN_TABLE;
	   uint32_t num_active_ = num_active;
	   uint32_t t = 0;

	   for(uint32_t i=0;i<num_active;i++)
	   {
		  v = blocks[i]->find(k);
		  t = i;
		  if(v != NOT_IN_TABLE) break;
	   }

	   if(v==NOT_IN_TABLE)
	   {
	     do
	     {
	        num_active = activeBlocks.load();
	        for(uint32_t i=num_active_;i<num_active;i++)
	        {	   
		    v = blocks[i]->find(k);
		    t = i;
		    if(v != NOT_IN_TABLE) break;
                }
		num_active_ = num_active;
	      }while(v == NOT_IN_TABLE && num_active != activeBlocks.load());
	   }
	  
	   std::pair<uint32_t,uint32_t> p(t,v);
	   return p;
	}
	bool erase(KeyT k)
	{
	   uint32_t num_active = activeBlocks.load();
	   uint32_t table_no;
	   bool found = false;
	   uint32_t num_active_ = num_active;

	   for(uint32_t i=0;i<num_active;i++)
	   {
		found = blocks[i]->erase(k);
		if(found) break;
	   }

	   if(!found)
	   {
	       do
	       {
	          found = false;
		  num_active = activeBlocks.load();
		
		   for(uint32_t i=num_active_;i<num_active;i++)
		   {
		    found = blocks[i]->erase(k);
		    if(found) break;
		  }
		  num_active_ = num_active;

	       }while(!found && num_active!=activeBlocks.load());
	   }

	   return found;
	}

};

#endif
