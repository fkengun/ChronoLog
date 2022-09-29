#ifndef __HashMap_Chain_H_
#define __HashMap_Chain_H_

#include "Block_chain_memory.h"


template <
	class KeyT,
	class ValueT,
	class HashFcn = std::hash<KeyT>,
	class EqualFcn = std::equal_to<KeyT>>
class HashMap
{
   public :
	   typedef BlockMap<KeyT,ValueT,HashFcn,EqualFcn> block_map;
   private :
	   uint32_t maxBlocks;
	   std::atomic<uint32_t> activeBlocks;
	   uint32_t blockSize;
	   std::vector<block_map*> blocks;
	   std::vector<std::mutex> block_mutexes;
	   memory_pool<KeyT,ValueT> *pl;
	   std::atomic<uint32_t> activeBlock;

  public:
	HashMap(uint32_t n,uint32_t m,memory_pool<KeyT,ValueT> *p) : maxBlocks(n), blockSize(m), pl(p)
	{
	    assert (maxBlocks > 0 && blockSize > 0);
	    blocks.resize(maxBlocks);
	    for(uint32_t i=0;i<maxBlocks;i++)
		    blocks[i] = nullptr;
	    std::vector<std::mutex> temp(maxBlocks);
	    std::swap(block_mutexes,temp);
	    activeBlocks.store(1);
	    blocks[0] = new block_map (blockSize,pl);
	    activeBlock.store(0);
	}
	~HashMap()
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

	   for(int i=0;i<num_active;i++)
	   {
		uint32_t p = blocks[i]->find(k);
		found = (p != NOT_IN_TABLE) ? true : false;
		if(found) break;
	   }

	   if(!found)
	   while(true)
	   {
		num_active = activeBlocks.load();
		for(int i=num_active_;i<num_active;i++)
		{
		   uint32_t p = blocks[i]->find(k);
		   found = (p != NOT_IN_TABLE) ? true : false;
		   if(found) break;
		}
		if(found) break;
	
		uint32_t block_id = activeBlock.load();	
		int r = blocks[block_id]->insert(k,v);

		if(r==EXISTS || r==INSERTED)
		{
		   found = (r==EXISTS);
		   break;
		} 

		if(r == FULL && num_active <= maxBlocks-1)
		{
		   bool found_empty = false;
		   for(int i=0;i<num_active;i++)
		   {
		      if(blocks[i]->block_empty())
		      {
			uint32_t next = (uint32_t)i;
			bool b = activeBlock.compare_exchange_strong(block_id,next);
			if(b) 
			{
			  found_empty = true;
			  break;
			}
		      }
		   }

		   if(!found_empty)
		   {
		      block_mutexes[num_active].lock();
		      if(num_active==activeBlocks.load())
		      {
	   	        blocks[num_active] = new block_map (blockSize,pl);
		        uint32_t prev = block_id;
		        bool b = activeBlock.compare_exchange_strong(prev,num_active);
			if(b)
			{
		          activeBlocks.fetch_add(1);
			}
			else delete blocks[num_active];
		     }
		     block_mutexes[num_active].unlock();
		  }
		}
		else if(num_active==maxBlocks)
		{
		   std::cout <<" Max memory exceeded"<<std::endl;
		   found = false;
		   break;
		}
	   }
	
   	    return (found) ? false : true;	   
	}


	std::pair<uint32_t,uint32_t> find(KeyT k)
	{
	   uint32_t num_active = activeBlocks.load();
	   uint32_t v = NOT_IN_TABLE;
	   uint32_t num_active_ = num_active;
	   uint32_t t = 0;
	   uint32_t block_id = activeBlock.load();

	   for(uint32_t i=0;i<num_active;i++)
	   {
		  v = blocks[i]->find(k);
		  t = i;
		  if(v != NOT_IN_TABLE) break;
	   }

	   if(v==NOT_IN_TABLE && num_active != activeBlocks.load())
	   {
		num_active = activeBlocks.load();
		for(int i=num_active_;i<num_active;i++)
		{
			v = blocks[i]->find(k);
			t = i;
			if(v != NOT_IN_TABLE) break;
		}
	   }
	
	   uint32_t block_id_n = activeBlock.load();
	   if(v==NOT_IN_TABLE && block_id != block_id_n) 
	   {
		v = blocks[block_id_n]->find(k);
		t = block_id_n;
	   }
	   std::pair<uint32_t,uint32_t> p(t,v);
	   return p;
	}

	bool erase(KeyT k)
	{
	   bool found = false;
	   uint32_t block_id = activeBlock.load();
	   uint32_t num_active = activeBlocks.load();
	   uint32_t num_active_ = num_active;

	   for(uint32_t i=0;i<num_active;i++)
	   {
		found = blocks[i]->erase(k);
		if(found) break;
	   }

	   if(!found && num_active != activeBlocks.load())
	   {
	       num_active = activeBlocks.load();
	       for(int i=num_active_;i<num_active;i++)
	       {
		   found = blocks[i]->erase(k);
		   if(found) break;
	       }
	   }

	   uint32_t block_id_n = activeBlock.load();
	   if(!found && block_id != block_id_n)
		   found = blocks[block_id_n]->erase(k);
	   return found;
	}

};

#endif
