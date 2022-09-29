#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/containers_fwd.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "libs/interprocess/test/allocator_v1.hpp"
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include "libs/interprocess/test/dummy_test_allocator.hpp"
#include "libs/interprocess/test/movable_int.hpp"
#include "libs/interprocess/test/boost_interprocess_check.hpp"

#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/lockfree/queue.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/deque.hpp>
#include <string.h>
#include <vector>
#include <random>

#include <boost/interprocess/detail/config_begin.hpp>

namespace boost {
namespace interprocess {

//using boost::container::deque;
using boost::lockfree::queue;
using boost::lockfree::detail::freelist_stack;

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

struct vtype
{
   int value;
};

using namespace boost::interprocess;

typedef allocator<int,managed_shared_memory::segment_manager> allocator_t;
typedef std::vector<int,allocator<int,managed_shared_memory::segment_manager>> shmem_vec;

struct thread_arg
{
        int tid;
        int pid;
	int id;
	int num_elements;
	shmem_vec *v;
	std::mutex *m;
};

std::vector<std::mutex> mx;
std::mutex mxt;

void func(thread_arg *t)
{

	for(int i=0;i<10;i++)
	{
	   mx[i].lock();
	   (*(t->v))[i] = t->id*i;
	   mx[i].unlock();
	}
}



int main(int argc,char **argv)
{
   int numprocs = 2;
   int numthreads = 1;
   int my_pid = getpid();
   int pid=0;

   typedef basic_managed_shared_memory
      <char,
      rbtree_best_fit<mutex_family>,
      iset_index
      > my_managed_shared_memory;

   std::string shm_name = "shm_file";
   shared_memory_object::remove(shm_name.c_str());

   managed_shared_memory shmem(open_or_create,shm_name.c_str(),65536);

   std::string mname = "shm_mutex";
   shared_memory_object::remove(mname.c_str());
   named_mutex mt(open_or_create,mname.c_str());

   allocator_t shmem_allocator(shmem.get_segment_manager());

   shmem_vec *v = shmem.construct<shmem_vec> ("shmem_vec") (shmem_allocator);

   auto m1 = shmem.find<shmem_vec>("shmem_vec").first;

   m1->reserve(1000);

   for(int i=0;i<10;i++)
	   m1->push_back(10);

   std::atomic<int> *mutex = (std::atomic<int>*) shmem.allocate(sizeof(std::atomic<int>));

   mutex->store(0,std::memory_order_seq_cst);

   std::atomic<int> *mutexes = (std::atomic<int>*)shmem.allocate(10*sizeof(std::atomic<int>));

   for(int i=0;i<10;i++)
	   mutexes[i].store(0);

   int id = 0;
   for(int i=0;i<numprocs;i++)
   {
	if(getpid()==my_pid)
	{
           id = i;
	   pid = fork();
	}
	
   }

   if(pid==0)
   {
	 std::string name = "shm_file";
	 managed_shared_memory shm(open_only,shm_name.c_str());
	 auto m2 = shm.find<shmem_vec>("shmem_vec").first;
         std::cout <<" id = "<<id<<std::endl;	
	 std::string mname_l = "shm_mutex";	
	 named_mutex nt(open_only,mname_l.c_str()); 
	 int cn = 0; int n = 0;
	 bool flag = false;

	 for(int i=0;i<10;i++)
	 {
             /*scoped_lock<named_mutex> lock(nt,defer_lock);
     	     {	     
	        m2->push_back(id);
	     }*/
	
	     do
	     {
		 flag = false;
		 cn = 0;
		 n = 1;
	     }while(!(flag=mutex->compare_exchange_strong(cn,n,std::memory_order_seq_cst)));
	    
	     if(flag)
	     {
		  m2->push_back(id);
		  mutex->store(0);
	     }

	 }

	 for(int i=0;i<10;i++)
	 {
		do
		{
		  flag = false;
		  cn = 0;
		  n = 1;
		}while(!(flag=mutexes[i].compare_exchange_strong(cn,n,std::memory_order_seq_cst)));

		if(flag)
		{
		   (*m2)[i] += (id+1)*10;
		   mutexes[i].store(0,std::memory_order_seq_cst);
		}

	 }
	 std::cout <<" mutex = "<<mutex->load()<<std::endl;
	 std::cout <<" m2 size = "<<m2->size()<<std::endl;

   }

   if(my_pid == getpid())
   {
	int wstatus;
        pid_t w;

         do
         {
            w = waitpid(pid,&wstatus,__WALL);
         }while(!WIFEXITED(wstatus));

      for(int i=0;i<10;i++)
	      std::cout <<" i = "<<i<<" v = "<<(*m1)[i]<<std::endl;
      shmem.deallocate((void*)mutexes);
      shmem.deallocate((void*)mutex);
      shmem.destroy<shmem_vec> ("shmem_vec");
   }

}

