#ifndef __BLOCK_TABLE_
#define __BLOCK_TABLE_

#include <vector>
#include <queue>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <atomic>
#include <memory>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "node.h"
#include "memory_allocation.h"

#define MAX_COLLISIONS 10
#define NOT_IN_TABLE UINT32_MAX
#define MAX_TABLE_SIZE 0.8*maxSize*MAX_COLLISIONS
#define MIN_TABLE_SIZE 0.15*maxSize*MAX_COLLISIONS
#define FULL 2
#define EXISTS 1
#define INSERTED 0

#define LOCK_TABLE(pos) \
 uint32_t prev, next; \
            bool b; \
            do  \
            { \
                prev = 0; \
                next = 1; \
            }while(!(b=table[pos].m_mutex.compare_exchange_strong(prev,next)));
	
#define UNLOCK_TABLE(pos) \
 table[pos].m_mutex.store(0);	

template <
	class KeyT,
	class ValueT,
	class HashFcn=std::hash<KeyT>,
	class EqualFcn=std::equal_to<KeyT>>
struct f_node
{
    uint64_t num_nodes;
    boost::atomic<uint32_t> m_mutex;
    //boost::mutex m_mutex;
    boost::mutex t_mutex;
    struct node<KeyT,ValueT,HashFcn,EqualFcn> *head;
};

template <
    class KeyT,
    class ValueT, 
    class HashFcn = std::hash<KeyT>,
    class EqualFcn = std::equal_to<KeyT>>
class BlockTable
{

   public :
	typedef struct node<KeyT,ValueT,HashFcn,EqualFcn> node_type;
	typedef struct f_node<KeyT,ValueT,HashFcn,EqualFcn> fnode_type;
   private :
	fnode_type *table;
	uint32_t maxSize;
	std::atomic<uint64_t> allocated;
	std::atomic<uint64_t> removed;
	memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *pl;
	boost::atomic<boost::int128_type> full_empty;
	int shift;
	size_t KeyToIndex(KeyT k)
	{
	    size_t hashval = HashFcn()(k);
	    size_t p = hashval;
	    hashval = hashval >> shift;
	    size_t pos = hashval % maxSize;
	    return pos;
	}
  public:

	BlockTable(uint32_t n, memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *m,int s) : maxSize(n), pl(m), shift(s)
	{
  	   assert (maxSize > 0);
	   table = (fnode_type *)std::malloc(maxSize*sizeof(fnode_type));
	   for(int i=0;i<maxSize;i++)
	   {
	      table[i].num_nodes = 0;
	      table[i].head = pl->memory_pool_pop();
	      table[i].head->key = UINT_MAX;
	      table[i].head->next = nullptr;
	      table[i].m_mutex.store(0); 
	   }
	   full_empty.store(0);
	   assert(maxSize < UINT32_MAX && maxSize*MAX_COLLISIONS < UINT32_MAX);
	}

  	~BlockTable()
	{
	    std::free(table);
	}

	fnode_type *&get_table()
	{
	    return table;
	}

	uint32_t table_size()
	{
	    return maxSize;
	}
	int insert(KeyT k,ValueT v)
	{
	    size_t pos = KeyToIndex(k);

	    assert (pos >= 0 && pos < maxSize);
	
	    LOCK_TABLE(pos);

	    int ret;
	   
	    node_type *p = table[pos].head;
	    node_type *n = table[pos].head->next;

	    bool found = false;
	    while(n != nullptr)
	    {
		if(EqualFcn()(n->key,k)) found = true;
		if(HashFcn()(n->key)>HashFcn()(k)) 
		{
		   break;
		}
		p = n;
		n = n->next;
	    }

	    ret = (found) ? EXISTS : 0;
	    if(!found)
	    {
		  node_type *new_node=pl->memory_pool_pop();
		  std::memcpy(&new_node->key,&k,sizeof(k));
		  std::memcpy(&new_node->value,&v,sizeof(v));
		  new_node->next = n;
		  p->next = new_node;
		  found = true;
		  ret = INSERTED;
	       
	    }

	   UNLOCK_TABLE(pos);

	   return ret;
	}

	int insert_node(node_type *newnode)
	{
	   uint32_t pos = KeyToIndex(newnode->key);

	   LOCK_TABLE(pos);

	   int ret = -1;

	   node_type *p = table[pos].head;
	   node_type *n = table[pos].head->next;

	   bool found = false;

	   while(n != nullptr)
	   {
	      if(EqualFcn()(n->key,newnode->key))
	      {
		 found = true; 
	      }
	      if(HashFcn()(n->key) > HashFcn()(newnode->key)) break;
	      p = n;
	      n = n->next;
	   }

	   ret = (found) ? EXISTS : 0;

	   if(!found)
	   {
	      newnode->next = n;
	      p->next = newnode;
	      ret = INSERTED;
	   }

	   UNLOCK_TABLE(pos);

	   return ret;

	}

	uint32_t find(KeyT k)
	{
	    uint32_t pos = KeyToIndex(k);

	    LOCK_TABLE(pos);

	    node_type *n = table[pos].head;
	    bool found = false;
	    while(n != nullptr)
	    {
		if(EqualFcn()(n->key,k))
		{
		   found = true;
		}
		if(HashFcn()(n->key) > HashFcn()(k)) break;
		n = n->next;
	    }

	    UNLOCK_TABLE(pos);

	    return (found ? pos : NOT_IN_TABLE);
	}

	bool update(KeyT k,ValueT v)
	{
	   uint32_t pos = KeyToIndex(k);

	   LOCK_TABLE(pos);

	   node_type *n = table[pos].head;

	   bool found = false;
	   while(n != nullptr)
	   {
		if(EqualFcn()(n->key,k))
		{
		   found = true;
		   std::memcpy(&(n->value),&v,sizeof(ValueT));
		}
		if(HashFcn()(n->key) > HashFcn()(k)) break;
		n = n->next;
	   }

	   UNLOCK_TABLE(pos);

	   return found;
	}

	bool erase(KeyT k)
	{
	   uint32_t pos = KeyToIndex(k);


	   LOCK_TABLE(pos);

	   node_type *p = table[pos].head;
	   node_type *n = table[pos].head->next;

	   bool found = false;
		
	   while(n != nullptr)
	   {
		if(EqualFcn()(n->key,k)) break;

		if(HashFcn()(n->key) > HashFcn()(k)) break;
		p = n;
		n = n->next;
	  }
         
	  if(n != nullptr)
	  if(EqualFcn()(n->key,k))
	  {
		found = true;
		p->next = n->next;
		pl->memory_pool_push(n);
		table[pos].num_nodes--;
		removed.fetch_add(1);
	  }
	   

	  UNLOCK_TABLE(pos);

	   return found;
	}

	void expand_shrink_table(uint32_t newsize)
	{
	   if(newsize==maxSize) return;

	   uint32_t prev_size = maxSize;

	   fnode_type *newtable = (fnode_type*)std::malloc(newsize*sizeof(fnode_type));
	   assert(newtable != nullptr);
	   for(int i=0;i<newsize;i++)
	   {
		newtable[i].head = pl->memory_pool_pop();
		newtable[i].num_nodes = 0;
		newtable[i].head->next = nullptr;
		newtable[i].m_mutex.store(0);
	   }
	  
	   maxSize = newsize;
	   fnode_type *prev_table = table;
	   table = newtable;

	   for(int i=0;i<prev_size;i++)
	   {

		node_type *n = prev_table[i].head->next;
		while(n != nullptr)
		{
		   node_type *p = n->next;
		   insert_node(n);
		   n = p;
		}
		pl->memory_pool_push(prev_table[i].head);
	   }
	   std::free(prev_table);

	}

};

#endif
