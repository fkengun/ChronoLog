#ifndef __BLOCK_CHAIN_LOCKFREE_
#define __BLOCK_CHAIN_LOCKFREE_

#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <atomic>

#define MAX_COLLISIONS 10
#define NOT_IN_TABLE UINT32_MAX
#define FULL 1
#define EMPTY 2
#define SENTINEL 3
#define MAX_LIST_SIZE 100

template<
	class KeyT,
	class ValueT
	>
struct node
{
   std::atomic<int32_t> node_state;
   KeyT key;
   ValueT value;
   std::atomic<struct node*>next;
};

template <
	class KeyT,
	class ValueT>
struct f_node
{
    std::atomic<uint32_t> num_nodes;
    std::atomic<uint32_t> read_write;
    std::atomic<struct node<KeyT,ValueT> *> head;
};

template <
    class KeyT,
    class ValueT, 
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>>
class BlockMap_lockfree
{

   private :
	struct f_node<KeyT,ValueT> *table;
	uint32_t maxSize;
        std::atomic<uint32_t> allocated;

	uint32_t KeyToIndex(KeyT k)
	{
	    size_t hashval = HashFcn()(k);
	    return hashval % maxSize;
	}
  public:
	BlockMap_lockfree(uint32_t n) : maxSize(n)
	{
  	   assert (maxSize > 0);
	   table = (struct f_node<KeyT,ValueT> *)std::malloc(maxSize*sizeof(struct f_node<KeyT,ValueT>));
	   for(int i=0;i<maxSize;i++)
	   {
	      table[i].num_nodes.store(0);
	      struct node<KeyT,ValueT> *n = (struct node<KeyT,ValueT> *)std::malloc(sizeof(struct node<KeyT,ValueT>));
	      n->next.store(nullptr);
	      n->node_state.store(SENTINEL);
	      table[i].head.store(n);
	   }
	   allocated.store(0);
	   assert(maxSize < UINT32_MAX && maxSize*MAX_COLLISIONS < UINT32_MAX);
	}

  	~BlockMap_lockfree()
	{
	   std::free(table);
	}

	bool insert(KeyT k,ValueT v)
	{
	    uint32_t pos = KeyToIndex(k);

	    bool found = false;
	    struct node<KeyT,ValueT> *new_node = (struct node<KeyT,ValueT> *)std::malloc(sizeof(struct node<KeyT,ValueT>));
	    new_node->next.store(nullptr);
	    new (&new_node->key) KeyT(k);
	    new (&new_node->value) ValueT(v);
	    new_node->node_state.store(FULL);

    	    struct node<KeyT,ValueT> *n = table[pos].head.load();
            struct node<KeyT,ValueT> *prev, *next;

	    bool b = false;

	    while(n != nullptr)
	    {
	 	if(n->node_state.load() == FULL && EqualFcn()(n->key,k))
	        {			
	    	  found = true;
		  break;
		}
		else if(n->next.load()==nullptr)
		{
		    
		    do
		    {
			 b = false;
			 prev = nullptr;
		         next = new_node;
			 if(n->next.load() != prev) break;
		    }while(!(b = n->next.compare_exchange_strong(prev,next)));	    

		}	
		n = n->next.load();
	    }

	    if(found && !b) std::free(new_node);

	    bool ret = (found && !b) ? false : true;
	    
	    return ret;
	}
	 
	
	uint32_t find(KeyT k)
	{
	    uint32_t pos = KeyToIndex(k);

	    bool found = false;
	    struct node<KeyT,ValueT> *n = table[pos].head.load();

	    while(n != nullptr)
	    {
		if(EqualFcn()(n->key,k) && n->node_state.load()==FULL)
		{
			found = true; break;
		}
		n = n->next.load();
	    }


	    return (found ? pos : NOT_IN_TABLE);
	}
        	
	bool erase(KeyT k)
	{
	   uint32_t pos = KeyToIndex(k);
	   bool found = false;
	
	   struct node<KeyT,ValueT> *n = table[pos].head.load();

	   while(n != nullptr)
	   {
		if(EqualFcn()(n->key,k) && n->node_state.load()==FULL)
		{
		   uint32_t prev = n->node_state.load();
		   uint32_t next = EMPTY;
		   if(prev==FULL)
		   {
		      bool b = n->node_state.compare_exchange_strong(prev,next);
		      if(b) found = true;
		   }
		   if(found) break;
		}
		n = n->next.load();
	   }

	   return found;
	}

	uint32_t get_block_size()
	{
	   return allocated.load();
	}

	void remove_nodes(int s,int e)
	{
		for(int i=s;i<e;i++)
		{
		   if(table[i].num_nodes.load() > MAX_LIST_SIZE)
		   {
			struct node<KeyT,ValueT> *n = table[i].head.load()->next.load();
			struct node<KeyT,ValueT> *p = table[i].head.load();

			while(n != nullptr)
			{
			    if(n->node_state.load()==EMPTY)
			    {	
				p->next.store(n->next.load());
				delete n;
				n = p->next.load();
			    }
			    else
			    {
				p = n;
				n = n->next.load();
			    }
			}
		   }

		}	
	}

        bool block_full()
	{
	   if(allocated.load() >= 0.8*maxSize*MAX_COLLISIONS) return true;
	   else return false;

	}	
};

#endif
