#include "V_ref_map.h"
#include <thread>

typedef vec_ht_map<int32_t,int32_t,std::hash<int32_t>,std::equal_to<int32_t>,std::allocator<char>,std::mutex,LinearProbeFcn> vht_m;


int main(int argc,char **argv)
{

   vht_m *m = new vht_m(512,0.5,0.8);

   for(int i=0;i<513;i++)
   {
	std::pair<int32_t,int32_t> e(i,10*i);
	m->insert(e);

   } 

   for(int i=0;i<1;i++)
   {

	m->find((int32_t)i);
   }

   delete m;
}
