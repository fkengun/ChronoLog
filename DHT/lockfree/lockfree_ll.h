#ifndef __LL_H_
#define __LL_H_

#include <vector>
#include <iostream>
#include <cmath>
#include <cstring>
#include <string.h>
#include <boost/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/config.hpp>
#include <boost/lockfree/queue.hpp>
#include <mutex>
#include <cassert>
#include <climits>
#include "node.h"
#include "memory_allocation.h"

#define CHUNK_LEN 100
#define INVALID 1
#define VALID 2


using namespace boost;

template<class KeyT,
	 class ValueT,
	 class HashFcn=std::hash<KeyT>,
	 class EqualFcn=std::equal_to<KeyT>>
class ll_lf
{
    public :
	    typedef struct node<KeyT,ValueT,HashFcn,EqualFcn> node_type;
    private:
	    boost::atomic<node_type *> head;
	    boost::atomic<node_type *> tail;
	    memory_pool<KeyT,ValueT,HashFcn,EqualFcn> *my_pool;
	    boost::atomic<uint32_t> *added;
	    boost::atomic<uint32_t> *removed;
	    KeyT maxKey;
    public:
	ll_lf(memory_pool<KeyT,ValueT,HashFcn,EqualFcn>*pl,boost::atomic<uint32_t> *a,boost::atomic<uint32_t> *b,KeyT m) : my_pool(pl),added(a),removed(b),maxKey(m)
	{
	   
	   node_type *n=nullptr;
	   node_type *p=nullptr;
	   n = my_pool->memory_pool_pop();
	   p = my_pool->memory_pool_pop();
	   new (&n->key) KeyT(0);
	   new (&n->value) ValueT();
	   new (&p->key) KeyT(INT_MAX);
	   new (&p->value) ValueT();
	   n->set_mark_reference(0,(boost::int64_t)p); 
	   p->set_mark_reference(0,(boost::int64_t)nullptr);
	   head.store(n);
	   tail.store(p);
   	}
	~ll_lf()
	{

	}	

	bool insert(KeyT a,ValueT v)
	{
		node_type *n = head.load();
		bool found = false;
		bool add = false;
	
		node_type *new_node = nullptr;
	
		new_node = my_pool->memory_pool_pop();	
		std::memcpy(&(new_node->key),&a,sizeof(KeyT));
		std::memcpy(&(new_node->value),&v,sizeof(ValueT));
		boost::int128_type new_mref = new_node->mark_ref.load();

		int num_turns = 0;
		while(true)
		{
		    num_turns++;
		    std::pair<node_type*,node_type*> pn = find(a);
                    node_type *p = pn.first; node_type *n = pn.second;

                    boost::int128_type p_mref = p->mark_ref.load();
                    boost::int64_t mark_p = (boost::int64_t)(p_mref >> 64);
                    node_type *p_n = (node_type *)p_mref;
                    boost::int128_type n_mref = n->mark_ref.load();
                    boost::int64_t mark_n = (boost::int64_t)(n_mref >> 64);
                    node_type *n_n = (node_type *)n_mref;
                    boost::int128_type next = 0;
                    next = next << 64;
                    next = next | (boost::int64_t)new_node;
		    new_mref = 0;
		    new_mref = new_mref << 64;
		    new_mref = new_mref | (boost::int64_t) p_n;
		    new_node->mark_ref.store(new_mref);

                    if(mark_p) continue;
                    if(EqualFcn()(n->key,a)) break;

                    bool b = p->mark_ref.compare_exchange_strong(p_mref,next);
                    if(b)
                    {
			added->fetch_add(1);
                    	add = true; 
		        break;	
		    }
                    
		}

		if(!add) my_pool->memory_pool_push(new_node);

		return add;
	}
	
	bool contains(KeyT a)
	{
		bool found = false;

		  node_type *n = head.load();
		  while(n != tail.load())
		  {
		   if(EqualFcn()(n->key,a))
		   {
			   if((boost::int64_t)(n->mark_ref.load() >> 64)==0)
			   {
			     found = true; break;
			   }
		   }
		   n = (node_type*) n->mark_ref.load();
		   //if(n==nullptr) break;
		 }

	 	return found;
	}

	std::pair<node_type*,node_type*> find(KeyT a)
	{
	    node_type* p, *n;

	    std::pair<node_type*,node_type*> pn(head.load(),tail.load());
	    boost::int128_type free_node = 1;
	    free_node = free_node << 64;
	    free_node = free_node | (boost::int64_t)nullptr;

	    int num_turns=0;
	    while(true)
	    {
	       num_turns++;       
	       p = head.load();
	       n = (node_type*)head.load()->mark_ref.load();	

	       bool found = false;
	       while(p != tail.load())
	       {
		   boost::int128_type p_mref = p->mark_ref.load();
		   boost::int64_t mark_p = (boost::int64_t)(p_mref >> 64); 
		   node_type* n = (node_type*)p_mref;
		
		   if(mark_p) break;

		   if(n != nullptr)
		   {
		     boost::int128_type n_mref = n->mark_ref.load();
		     boost::int64_t mark_n = (boost::int64_t)(n_mref >> 64);
		     node_type *nn = (node_type*)n_mref;
		
		     while(mark_n)
		     {
			boost::int128_type next = (boost::int128_type)mark_p;
			next = next << 64;
			next = next | (boost::int64_t)nn;
			bool b = p->mark_ref.compare_exchange_strong(p_mref,next);
			if(!b) break;
			else 
			{
			    removed->fetch_add(1);
			    //n->mark_ref.store(free_node);
			    my_pool->memory_pool_push(n);
			}
			p_mref = p->mark_ref.load();
			mark_p = (boost::int64_t)(p_mref >> 64);
			n = (node_type*)p_mref;
			if(n == tail.load()||mark_p) break;
		        n_mref = n->mark_ref.load();
			mark_n = (boost::int64_t)(n_mref >> 64);
			nn = (node_type*)n_mref;
			if(nn = nullptr) break;
		     }

		     if(mark_p || mark_n)
		     {
			 break;
		     }
		     else if(EqualFcn()(n->key,a) || (HashFcn()(n->key) > HashFcn()(a)))
		     {
			  found = true;
			  pn.first = p; pn.second = n;
			  break;
		     }
		     else 
		     {
			p = n;

		     }
		     
		   }
		   else
		   {
		      found = true; break;
		   }

	       }
	       if(found) break;
	   	  	   
	    }

	    return pn;
	}
	bool erase(KeyT a)
	{
		
		bool found = false;
		boost::int128_type free_node = 1;
		free_node = free_node << 64;
		free_node = free_node | (boost::int64_t)nullptr;

		while(true)
		{
		    std::pair<node_type*,node_type*> pn = find(a);
		    node_type *p = pn.first; node_type *n = pn.second;
		    if(n==tail.load()) return false;

		     boost::int128_type p_mref = p->mark_ref.load();
                     boost::int64_t mark_p = (boost::int64_t)(p_mref >> 64);
                     node_type *p_n = (node_type *)p_mref;
                     boost::int128_type n_mref = n->mark_ref.load();
                     boost::int64_t mark_n = (boost::int64_t)(n_mref >> 64);
                     node_type *n_n = (node_type *)n_mref;
		     boost::int128_type next = 1;
		     next = next << 64;
		     next = next | (boost::int64_t)n_n;
		
		     if(mark_p || n_n==nullptr) continue;
		     if(!EqualFcn()(n->key,a)) return false;

		     if(mark_n==0)
		     {
		       bool b = n->mark_ref.compare_exchange_strong(n_mref,next);
		       if(b)
		       {		    
		        boost::int128_type nmref = n->mark_ref.load();
			boost::int64_t nmark = (boost::int64_t)(nmref >> 64);
			next = mark_p;
			next = (next << 64);
			next = next | (boost::int64_t)n_n;

			b = p->mark_ref.compare_exchange_strong(p_mref,next);
			if(b) 
			{
				removed->fetch_add(1);
				//n->mark_ref.store(free_node); 
				my_pool->memory_pool_push(n);
			}
			return true;
		      }
		    }

		}

		return false;
	}

	void show_list()
	{
		node_type *n = head.load();

		while(n != tail.load())
		{
		   std::cout <<" key = "<<n->key<<std::endl;
		   n = (node_type*)n->mark_ref.load();
		}
	}

	int list_size()
	{
		node_type *n = head.load();

		int count = 0;
		while(n != tail.load())
		{
		    n = (node_type*) n->mark_ref.load();
		    count++;

		}
		return count;

	}	

};

#endif
