#include "lockfree_ll.h"
#include <thread>

ll_lf<int32_t,int32_t> *m;

struct thread_arg
{
   int tid;
};

void list_operations_1(struct thread_arg *t)
{
     for(int i=0;i<100;i++)
     {
	bool s = m->insert(i+1,i+1);
	s = m->contains(i+1);
	//if(!s) std::cout <<" t = "<<t->tid<<" not inserted a = "<<i+1<<std::endl;
     }

}

void list_operations_2(struct thread_arg *t)
{
	
    for(int i=0;i<500;i++)
    {
	 bool s = m->insert(500+i+1,500+i+1);
	 if(!s) s = m->contains(500+i+1);
    }
	
    for(int i=0;i<100;i++)
    {
	int r = random()%10000;
	bool s = m->erase(r+1);
    }

}

int main(int argc,char **argv)
{

	memory_pool<int32_t,int32_t> *pl = new memory_pool<int32_t,int32_t>(100);
	boost::atomic<uint32_t> added, removed;
	added.store(0); removed.store(0);
	int32_t maxkey = INT_MAX;
	m = new ll_lf<int32_t,int32_t> (pl,&added,&removed,maxkey);

	int num_threads = 12;
	std::vector<struct thread_arg> t_args(num_threads);
	std::vector<std::thread> workers(num_threads);

	for(int i=0;i<num_threads;i++)
	{
	     t_args[i].tid = i;
	     std::thread t{list_operations_1,&t_args[i]};
	     workers[i] = std::move(t);
	}
	
	for(int i=0;i<num_threads;i++)
		workers[i].join();	

	for(int i=0;i<num_threads;i++)
	{
		t_args[i].tid = i;
		std::thread t{list_operations_2,&t_args[i]};
		workers[i] = std::move(t);
	}

	for(int i=0;i<num_threads;i++)
		workers[i].join();

//	m->show_list();

	delete pl;
	delete m;

}
