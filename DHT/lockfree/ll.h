#ifndef __LL_H_
#define __LL_H_

#include <vector>
#include <iostream>
#include <cmath>
#include <cstring>
#include <string.h>
#include <atomic>
#include <climits>
#include <mutex>
#include <algorithm>
#include <random>

#define INVALID 1
#define VALID 2

struct node
{
   std::mutex m;
   std::atomic<uint32_t> node_state;
   int value;
   std::atomic<struct node *>next;
};

class ll
{
    private:
	    std::atomic<struct node *> head;
	    std::atomic<uint32_t> num_nodes;
    public:
	ll()
	{
	   struct node *n = new struct node();
	   n->node_state.store(VALID);
	   n->value = INT_MAX;
	   n->next.store(nullptr);
	   head.store(n);
	   num_nodes.store(0);
   	}
	~ll()
	{
	    struct node *n = head.load();

	    while(n != nullptr)
	    {
		struct node *nn = n->next.load();
		delete n;
		n = nn;
	    }
	}	

	bool insert(int a)
	{	
	   struct node *n = head.load();
	   bool found = false;
	   bool add = false;

	   while(n != nullptr)
	   {
		n->m.lock();
		if(n->value == a)
		{	
	            found = true;
		}
		if(!found && n->next.load()==nullptr)
		{
		    struct node *new_node = new struct node();
		    new_node->value = a;
		    n->next.store(new_node);
		    add = true;
		}	
		struct node *nn = n->next.load();
		n->m.unlock();
		n = nn;
		if(found) break;
	   }

	  return add;	
	}

	bool find(int b)
	{

	   struct node *n = head.load();
	   bool found = false;

           while(n != nullptr)
	   {
		n->m.lock();
	        if(n->value==b)
		{
		    found = true;
		}
		struct node *nn = n->next.load();
		n->m.unlock();
		n = nn;
		if(found) break;
	   }

	   return found;
	}

	bool erase(int a)
	{
	   struct node *p = head.load();
	   struct node *n;

	   p->m.lock();

	   while(p != nullptr)
	   {
		n = p->next.load();
		if(n != nullptr)
		{
		   n->m.lock();
		   if(n->value==a)
		   {
		      p->next.store(n->next.load());
		      n->m.unlock();
		      p->m.unlock();
		      break;
		   }
		   else
		   {
		        
			struct node *pn = n;
			p->m.unlock();
			p = pn;
		   }
	        }
		else
		{
		   struct node *pn = n;
		   p->m.unlock();
		   p = pn;
		}
	   }

	   return true;
	}
};

#endif
