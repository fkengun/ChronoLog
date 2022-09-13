#include <mpi.h>
#include "Block_chain_memory_dist.h"
#include <thread>
#include <chrono>
#include <random>

template<class KeyT,class ValueT>
struct req
{
   int type;
   KeyT key;
   ValueT value;
};

struct thread_arg
{
   int tid;
   int num_operations;
};

std::default_random_engine rd;
std::uniform_int_distribution<int32_t> dist(0,100000000);

std::vector<boost::lockfree::queue<struct req<int32_t,int32_t>>*> requests;
BlockMap<int32_t,int32_t> *m;

void process_remote_requests(int,int);

void map_operations(thread_arg *t)
{
    for(int i=0;i<t->num_operations;i++)
    {
        int op = random()%3;
        int32_t k = dist(rd);
	if(m->isLocal(k))
	{
          if(op==0)
           int s = m->insert(k,k);
          else if(op==1)
           int pos = m->find(k);
          else if(op==2)
           bool s = m->erase(k);
        }
	else
	{
	   int pid = m->procLocation(k);
	   struct req<int32_t,int32_t> r;
	   r.type = op;
	   r.key = k;
	   r.value = k;
	   requests[pid]->push(r);
	}

    }

}



int main(int argc,char **argv)
{

   int provided;
   MPI_Init_thread(&argc,&argv,MPI_THREAD_FUNNELED,&provided);

   uint64_t total_table_size = 65536;
   int comm_size, rank;
   MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   rd.seed(rank);
   auto die = std::bind(dist,rd);

   memory_pool<int32_t,int32_t> *p = new memory_pool<int32_t,int32_t> (100);    
   m = new BlockMap<int32_t,int32_t> (65536,p,INT_MAX,comm_size,rank);

   requests.resize(comm_size);

   for(int i=0;i<comm_size;i++)
   {
	requests[i] = new boost::lockfree::queue<struct req<int32_t,int32_t>> (128);
   }
 
   int num_iter = 10;
   int num_threads = 1;
   uint64_t num_operations = 10000;
   uint64_t ops = num_operations/num_threads;
   uint64_t rem = num_operations%num_threads;

   std::vector<struct thread_arg> t_args(num_threads);
   std::vector<std::thread> workers(num_threads);

   for(int i=0;i<num_threads;i++)
   {
	  t_args[i].tid = i;
	  if(i < rem) t_args[i].num_operations = ops+1;
	  else t_args[i].num_operations = ops;
   }

   for(int i=0;i<num_iter;i++)
   { 
	for(int j=0;j<num_threads;j++)
	{
	   std::thread t{map_operations,&t_args[j]};
   	   workers[j] = std::move(t);
	}

	for(int j=0;j<num_threads;j++)
	  workers[j].join();	

	process_remote_requests(comm_size,rank);
   }



  for(int i=0;i<comm_size;i++)
	  delete requests[i];

  delete m;
  MPI_Finalize();

}

void process_remote_requests(int num_procs,int myrank)
{
   std::vector<int32_t>* send_buff, *recv_buff;

   std::vector<int> recv_counts(num_procs), send_counts(num_procs);
   std::vector<int> send_displ(num_procs), recv_displ(num_procs);
   std::fill(recv_counts.begin(),recv_counts.end(),0);
   std::fill(send_counts.begin(),send_counts.end(),0);
   std::fill(send_displ.begin(),send_displ.end(),0);
   std::fill(recv_displ.begin(),recv_displ.end(),0);

   send_buff = new std::vector<int32_t> ();
   recv_buff = new std::vector<int32_t> ();

   for(int i=0;i<num_procs;i++)
   {
	struct req<int32_t,int32_t> r;
	int count = 0;
	while(requests[i]->pop(r))
	{
		send_buff->push_back(r.type);
		send_buff->push_back(r.key);
		send_buff->push_back(r.value);
		count += 3;
	}
	send_counts[i] = count;
   }

   MPI_Alltoall(send_counts.data(),1,MPI_INT,recv_counts.data(),1,MPI_INT,MPI_COMM_WORLD);

   int tsize = 0;
   for(int i=0;i<num_procs;i++)
   {
	tsize+=(recv_counts[i]);
   }

   recv_buff->resize(tsize);

   send_displ[0] = 0;
   for(int i=1;i<num_procs;i++)
	   send_displ[i] = send_displ[i-1]+send_counts[i-1];
   recv_displ[0] = 0;
   for(int i=1;i<num_procs;i++)
	   recv_displ[i] = recv_displ[i-1]+recv_counts[i-1];

   MPI_Alltoallv(send_buff->data(),send_counts.data(),send_displ.data(),MPI_INT,recv_buff->data(),recv_counts.data(),recv_displ.data(),MPI_INT,MPI_COMM_WORLD);

   for(int i=0;i<num_procs;i++)
   {
	for(int j=0;j<recv_counts[i];j+=3)
	{
		int op = (*recv_buff)[recv_displ[i]+j];
		int32_t key = (*recv_buff)[recv_displ[i]+j+1];
		int32_t value = (*recv_buff)[recv_displ[i]+j+2];
		if(op==0)
           		int s = m->insert(key,value);
          	else if(op==1)
           		int pos = m->find(key);
          	else if(op==2)
           	bool s = m->erase(key);
	}
   }

   delete send_buff;
   delete recv_buff;
}
