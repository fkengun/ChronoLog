#ifndef __SPLIT_CHAIN_
#define __SPLIT_CHAIN_

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <climits>
#include <float.h>
#include <mutex>
#include <atomic>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "memory_allocation.h"
#include "node.h"
#include <bitset>

#define MAX_COLLISIONS 40
#define MAX_TABLE_SIZE 0.8*ntables*table_size*MAX_COLLISIONS

#define LOCK_TABLE(t_no,index) \
uint32_t prev = 0, next = 1; \
do \
{ \
    prev = 0; next = 1; \
}while(!(*tables[t_no])[index].node_lock.compare_exchange_strong(prev,next));

#define UNLOCK_TABLE(t_no,index) \
(*tables[t_no])[index].node_lock.store(0);	


template <class KeyT,
	 class ValueT,
	 class HashFcn=std::hash<KeyT>,
	 class EqualFcn=std::equal_to<KeyT>>
struct t_node
{
     struct node<KeyT,ValueT,HashFcn,EqualFcn> *head;
     std::atomic<uint32_t> node_lock;
};


template <class KeyT,
	  class ValueT,
	  class HashFcn=std::hash<KeyT>,
	  class EqualFcn=std::equal_to<KeyT>>
class split_map
{
	public :
		typedef struct node<KeyT,ValueT,HashFcn,EqualFcn> node_type;
		typedef struct t_node<KeyT,ValueT,HashFcn,EqualFcn> head_type;
	private :
		size_t table_size;
		size_t max_tables;
		std::atomic<uint32_t> num_tables;
		std::vector<std::vector<head_type>*> tables;
		size_t max_load;
		boost::shared_mutex t_mutex;
		node_type *list;
		memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *pl;
		std::atomic<uint32_t> num_bits;
		std::atomic<uint64_t> allocated;
		std::atomic<uint64_t> removed;
	public :
		uint64_t next_power_two(uint64_t n)
		{
		    uint64_t c = 1;
			
		    while( c < n)
		    {
		       c = c*2;
		    }

		    return c;
		}

		uint64_t KeyToIdx(KeyT k)
		{
		    uint64_t hash_value = HashFcn()(k);
		    uint64_t size = num_tables.load()*table_size;
		    return hash_value%size;
		}

		split_map(size_t mtable,size_t tsize,memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *m) : max_tables(mtable), table_size(tsize), pl(m)
	        {
		     assert (max_tables > 0 && table_size > 0);
		     tables.resize(max_tables);
		     for(size_t i=0;i<max_tables;i++)
			     tables[i] = nullptr;
		     list = nullptr;
		     int c = 1;

		     KeyT k = UINT32_MAX; 
		
		     node_type *new_node = pl->memory_pool_pop();
		     std::memcpy(&(new_node->key),&k,sizeof(KeyT));
		     new_node->next = nullptr;
		     list = new_node;

		     node_type *n = list;
		     
		     while(c < table_size+1)
		     {
			  new_node = pl->memory_pool_pop();
			  std::memcpy(&(new_node->key),&k,sizeof(KeyT));
			  new_node->next = nullptr;
			  n->next = new_node;
			  n = n->next;
			  c++;
		     } 
		    
		     tables[0] = new std::vector<head_type> (table_size); 
		     n = list;
		     for(int i=0;i<table_size;i++)
		     {
			     (*tables[0])[i].head = n;
			     n = n->next;
		     }
		     num_tables.store(1);
		     max_load = 0;
		     allocated.store(0); removed.store(0);
		}
		~split_map()
		{
		   std::cout <<" num_tables = "<<num_tables.load()<<" table_size = "<<table_size<<std::endl;
		   for(int i=0;i<num_tables.load();i++)
			   delete tables[i];
		}

		void recursive_fill(uint64_t table_no,uint64_t index_p,uint32_t level)
		{
		   uint64_t pos = table_no*table_size+index_p;
		   uint32_t plevel = level-1;
		   uint64_t mask = UINT64_MAX;
		   mask = mask >> (64-plevel);
		   uint64_t parent = pos & mask;
		   uint64_t p_table_no = parent/table_size;
		   uint64_t index = parent%table_size;

		   node_type *ret_node = nullptr;
		   int t_no = p_table_no;

		   LOCK_TABLE(t_no,index);

		   if((*tables[p_table_no])[index].head == nullptr)
		   {
		      recursive_fill(p_table_no,index,level-1);
		   }

		   if((*tables[p_table_no])[index].head != nullptr)
		   {
		   node_type *p = (*tables[p_table_no])[index].head;
		   node_type *n = (*tables[p_table_no])[index].head->next;
		   assert (p != nullptr && n!=nullptr);
		   bool found = false;
		   while(n->key != INT_MAX)
		   {
		            uint64_t hash_value = HashFcn()(n->key);
	    		    hash_value = hash_value >> plevel;
			    uint64_t bit_value = hash_value & (uint64_t)1;
		            if(bit_value) 
			    {
				found = true; break;
    			    }
			    p = n;
			    n = n->next;
		   }
		   
		   node_type *new_node = pl->memory_pool_pop();
		   new_node->key = UINT32_MAX;
		   new_node->next = n;
		   p->next = new_node;
		   (*tables[table_no])[index_p].head = new_node;
		   }

		   UNLOCK_TABLE(t_no,index);

		   return;
		}

		bool insert(KeyT k, ValueT v)
		{

		    bool added = false;
		    uint32_t inserted = allocated.load(); uint32_t deleted = removed.load();
		    uint32_t diff = inserted-deleted;
		    uint32_t ntables = num_tables.load();
		    uint32_t maxt = MAX_TABLE_SIZE;
	
	           if(diff >= maxt)
	           {		   
	           	    
		      boost::upgrade_lock<boost::shared_mutex> lk(t_mutex);

		      if(num_tables.load()==ntables)
		      {
			  uint32_t new_tables = 2*ntables;
			  new_tables = new_tables-ntables;
			  if(new_tables <= max_tables-ntables)
			  {
			    for(int j=0;j<new_tables;j++)
			    {
			      tables[ntables+j] = new std::vector<head_type> (table_size);
			      for(int i=0;i<table_size;i++)
			       (*tables[ntables+j])[i].head = nullptr;
			    }
			    num_tables.fetch_add(new_tables);
			  }
			  else 
			  {
				std::cout <<" num_entries = "<<allocated.load()-removed.load()<<std::endl;
				std::cout <<" max_size = "<<0.8*num_tables.load()*table_size*MAX_COLLISIONS<<std::endl;
				std::cout <<" num_tables = "<<num_tables.load()<<" max size exceeded"<<std::endl;
				exit(-1);
			  }
		     }
		     lk.unlock();
		     lk.release();
		   }

		    boost::shared_lock<boost::shared_mutex> lk(t_mutex);

		    ntables = num_tables.load();
		    uint64_t pos = KeyToIdx(k); 
		    uint64_t table_no = pos/table_size; 
		    uint64_t index = pos%table_size;
		    uint64_t t_no = table_no;

		    LOCK_TABLE(t_no,index);

		    node_type *p = (*tables[table_no])[index].head;
			
		    assert (table_no < num_tables.load());
		    if(p==nullptr) 
		    {
			uint32_t table_p = next_power_two(table_no+1); 
			int nbits = log2(table_p*table_size);
			recursive_fill(table_no,index,nbits);
		    }
		    p = (*tables[table_no])[index].head;

		    node_type *n = (*tables[table_no])[index].head->next;

		    bool found = false;
		    added = false;

		    while(!EqualFcn()(n->key,INT_MAX))
		    {
			if(EqualFcn()(n->key,k)) found = true;
			if(HashFcn()(n->key) > HashFcn()(k))
			{
				break;
			}
			p = n;
			n = n->next;
		   }

		   if(!found)
		   {
		       node_type *new_node = pl->memory_pool_pop();
		       std::memcpy(&(new_node->key),&k,sizeof(KeyT));
		       std::memcpy(&(new_node->value),&v,sizeof(ValueT));
		       new_node->next = n;
		       p->next = new_node;
		       allocated.fetch_add(1);
		       added = true;
		   }

		   UNLOCK_TABLE(t_no,index);
		   lk.unlock();
		   lk.release();

		   return added;
		}
		
		bool update(KeyT k, ValueT v)
		{
		    boost::shared_lock<boost::shared_mutex> lk(t_mutex);

		    bool found = false;
		    uint64_t pos = KeyToIdx(k);
		    uint64_t table_no = pos/table_size;
		    uint64_t index = pos%table_size;
		    uint64_t t_no = table_no;

		    LOCK_TABLE(t_no,index);

		    if((*tables[table_no])[index].head == nullptr)
		    {
			    uint32_t p = next_power_two(table_no+1);
			    uint32_t np = log2(p*table_size); 
			    recursive_fill(table_no,index,np);
		    }

		    node_type *n = (*tables[table_no])[index].head->next;

		    while(!EqualFcn()(n->key,INT_MAX))
		    {
			    if(EqualFcn()(n->key,k))
			    {
				  found = true;
				  std::memcpy(&(n->value),&v,sizeof(ValueT));
			    }
			    if(HashFcn()(n->key) > HashFcn()(k)) break;
			    n = n->next;
		    }

		    UNLOCK_TABLE(t_no,index);
		    lk.unlock();
		    lk.release();

		    return found;

		}

		bool find(KeyT k)
		{
		   boost::shared_lock<boost::shared_mutex> lk(t_mutex);

		   bool found = false;
		   uint64_t pos = KeyToIdx(k);
		   uint64_t table_no = pos/table_size;
		   uint64_t index = pos%table_size;
	           uint64_t t_no = table_no;

		   LOCK_TABLE(t_no,index);

		   if((*tables[table_no])[index].head==nullptr)
		   {
			uint32_t p = next_power_two(table_no+1);
			uint32_t np = log2(p*table_size); 
			recursive_fill(table_no,index,np);
		   }

		   node_type *n = (*tables[table_no])[index].head->next;

		   while(!EqualFcn()(n->key,INT_MAX))
		   {
		       if(EqualFcn()(n->key,k))
		       {
			    found = true; 
		       }
		       if(HashFcn()(n->key) > HashFcn()(k)) break;

		       n = n->next;
		   }
		   UNLOCK_TABLE(t_no,index);
		   lk.unlock();
		   lk.release();

		   return found;
		}

		bool erase(KeyT k)
		{
		   boost::shared_lock<boost::shared_mutex> lk(t_mutex);

		   bool found = false;

		   uint64_t pos = KeyToIdx(k);
		   uint64_t table_no = pos/table_size;
		   uint64_t index = pos%table_size;
	 	   uint64_t t_no = table_no;

		    LOCK_TABLE(t_no,index);
	   		   
		    if((*tables[table_no])[index].head==nullptr)
		    {
			  uint32_t table_n = next_power_two(table_no+1);
			  uint32_t np = log2(table_n*table_size);
			  recursive_fill(table_no,index,np);
		    }
		    node_type *p = (*tables[table_no])[index].head;
		    node_type *n = (*tables[table_no])[index].head->next;

		    while(!EqualFcn()(n->key,INT_MAX))
		    {
			if(EqualFcn()(n->key,k))
			{
			    found = true; break;
			}
			if(HashFcn()(n->key) > HashFcn()(k)) break;
			p = n;
			n = n->next;
		    }

		    if(found)
		    {
			p->next = n->next;
			pl->memory_pool_push(n);
			removed.fetch_add(1);
		    }

		    UNLOCK_TABLE(t_no,index);
		    lk.unlock();
		    lk.release();
		   return true;
		}

		uint32_t allocated_entries()
		{
		   return allocated.load();
		}
		uint32_t removed_entries()
		{
		    return removed.load();
		}
};




#endif
