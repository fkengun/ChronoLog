#include "Block_chain_memory.h"
#include "abt.h"
#include <chrono>
#include <mpi.h>

BlockMap<int32_t,int32_t> *m;

//BlockMap<std::string,uint32_t,HashTraits,EqualTraits> *m;

struct thread_arg
{
   int tid;
   int num_operations;
};

static void map_operations(void *s)
{
    struct thread_arg *t = (struct thread_arg*)s;

    for(int i=0;i<t->num_operations;i++)
    {
	int op = random()%3;
	int32_t k = random()%100000000;
	if(op==0)
	   int s = m->insert(k,k);
	else if(op==1)
	   int pos = m->find(k);
	else if(op==2) 
	{
	   bool s = m->erase(k);
	}

    }	    

}

int main(int argc,char **argv)
{

    memory_pool<int32_t,int32_t> *p = new memory_pool<int32_t,int32_t> (100);    
    m = new BlockMap<int32_t,int32_t> (65536,p,INT_MAX);

    int num_operations = 10000000;

    int numstreams = 8, num_threads = 12;

     std::vector<ABT_xstream> *xstreams = new std::vector<ABT_xstream> (numstreams);
     std::vector<ABT_pool> *pools = new std::vector<ABT_pool> (numstreams);
     ABT_thread *threads = (ABT_thread*)std::malloc(num_threads*sizeof(ABT_thread));
     struct thread_arg *t_args = (struct thread_arg*)std::malloc(num_threads*sizeof(struct thread_arg));

     int num_ops = num_operations/num_threads;
     int rem = num_operations%num_threads;

     auto t1 = std::chrono::high_resolution_clock::now();

     ABT_init(argc,argv);

     ABT_xstream_self(&(*xstreams)[0]);

     for (int i = 1; i < numstreams; i++)
     {
        ABT_xstream_create(ABT_SCHED_NULL, &(*xstreams)[i]);
     }

     for (int i = 0; i < numstreams; i++)
     {
       ABT_xstream_get_main_pools((*xstreams)[i], 1, &(*pools)[i]);
     }

     for(int i=0;i<num_threads;i++)
     {
	  t_args[i].tid = i;
	  if(i < rem) t_args[i].num_operations = num_ops+1;
	  else t_args[i].num_operations = num_ops;
     }

     for(int i=0;i<num_threads;i++)
     {
        int pool_id = i%numstreams;
	ABT_thread_create((*pools)[pool_id],map_operations,&t_args[i],ABT_THREAD_ATTR_NULL, &threads[i]);
     }

     for(int i=0;i<num_threads;i++)
     {
	ABT_thread_free(&threads[i]);
     }

     for (int i = 1; i < numstreams; i++)
     {
        ABT_xstream_join((*xstreams)[i]);
        ABT_xstream_free(&((*xstreams)[i]));
     }

     ABT_finalize();

    auto t2 = std::chrono::high_resolution_clock::now();
    double time_taken = std::chrono::duration<double> (t2-t1).count();

    std::cout <<" time taken = "<<time_taken<<std::endl;
    std::cout <<" allocated = "<<m->allocated_nodes()<<" removed = "<<m->removed_nodes()<<std::endl;
    std::cout <<" table entries = "<<m->count_block_entries()<<" diff = "<<m->allocated_nodes()-m->removed_nodes()<<std::endl;

    delete xstreams;
    delete pools;
    std::free(threads);
    std::free(t_args);
    delete m;
    delete p;

}
