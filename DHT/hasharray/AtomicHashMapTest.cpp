/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "AtomicHashMap_shm.h"
#include <cstddef>
#include <atomic>
#include <memory>
#include <thread>
#include <map>
#include <glog/logging.h>
#include <boost/interprocess/allocators/adaptive_pool.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <folly/hash/Hash.h>
#include <folly/Benchmark.h>
#include <folly/Conv.h>
#include <folly/portability/Atomic.h>
#include <folly/portability/GTest.h>
#include <folly/portability/SysTime.h>
#include <folly/portability/SysMman.h>
#include <sys/wait.h>

using folly::AtomicHashArray;
using folly::AtomicHashMap;
using folly::StringPiece;
using std::string;
using std::vector;

using namespace boost::interprocess;
using namespace folly;

template <class T>
class MmapAllocator {
 public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;

  typedef ptrdiff_t difference_type;
  typedef size_t size_type;

  T* address(T& x) const { return std::addressof(x); }

  const T* address(const T& x) const { return std::addressof(x); }

  size_t max_size() const { return std::numeric_limits<size_t>::max(); }

  template <class U>
  struct rebind {
    typedef MmapAllocator<U> other;
  };

  bool operator!=(const MmapAllocator<T>& other) const {
    return !(*this == other);
  }

  bool operator==(const MmapAllocator<T>& /* other */) const { return true; }

  template <class... Args>
  void construct(T* p, Args&&... args) {
    new (p) T(std::forward<Args>(args)...);
  }

  void destroy(T* p) { p->~T(); }
  T* allocate(size_t n) {
    void* p = mmap(
        nullptr,
        n * sizeof(T),
        PROT_READ | PROT_WRITE,
        MAP_SHARED|MAP_SYNC|MAP_ANONYMOUS,
        -1,
        0);
    if (p == MAP_FAILED) {
      throw std::bad_alloc();
    }
    return (T*)p;
  }

  void deallocate(T* p, size_t n) { munmap(p, n * sizeof(T)); }
};

static bool legalKey(const char* a);

struct EqTraits {
  bool operator()(const char* a, const char* b) {
    return legalKey(a) && (strcmp(a, b) == 0);
  }
  bool operator()(const char* a, const char& b) {
    return legalKey(a) && (a[0] != '\0') && (a[0] == b);
  }
  bool operator()(const char* a, const StringPiece b) {
    return legalKey(a) && (strlen(a) == b.size()) &&
        (strcmp(a, b.begin()) == 0);
  }
};

struct HashTraits {
  size_t operator()(const char* a) {
    size_t result = 0;
    while (a[0] != 0) {
      result += static_cast<size_t>(*(a++));
    }
    return result;
  }
  size_t operator()(const char& a) { return static_cast<size_t>(a); }
  size_t operator()(const StringPiece a) {
    size_t result = 0;
    for (const auto& ch : a) {
      result += static_cast<size_t>(ch);
    }
    return result;
  }
};


typedef AtomicHashMap<const char*, int64_t, HashTraits, EqTraits> AHMCstrInt;
AHMCstrInt::Config cstrIntCfg;

static bool legalKey(const char* a) {
  return a != cstrIntCfg.emptyKey && a != cstrIntCfg.lockedKey &&
      a != cstrIntCfg.erasedKey;
}

int main(int argc, char** argv) 
{
  std::string name = "shm_file";
  shared_memory_object::remove(name.c_str());
  typedef  AtomicHashMap<const char *, uint64_t,HashTraits,EqTraits,MmapAllocator<char>> HMap;
  typedef AtomicHashArray<const char *,uint64_t,HashTraits,EqTraits,MmapAllocator<char>> array_type;
  array_type::Config config;

  HMap *hm = new HMap(1024,config);

  auto r = hm->insert("a",1);

  std::cout <<" size = "<<hm->size()<<" f = "<<r.first->first<<" s = "<<r.first->second<<std::endl;
  //AHMapT *sm = new AHMapT(1024,config);
  // SMap *sm = new SMap(1024, config); 
  //
  //QuadraticProbingAtomicHashMap<uint64_t,uint64_t,std::allocator<char>> qp;

  int numprocs = 2;

  int p_id = getpid();

  int pid,id;

  for(int i=0;i<numprocs;i++)
  {
	  if(getpid()==p_id)
	  {
		  id = i;
		  pid = fork();
	  }
  }


  if(pid==0)
  {
	if(id==0)
	{
	  auto r = hm->insert("b",2);
	  auto t = hm->find("b");
	  std::cout <<" id = "<<id<<" v = "<<t->second<<std::endl;
	}
	else 
	{
	   auto r = hm->insert("c",3);
	   auto t = hm->find("c");
	   std::cout <<" id = "<<id<<" v = "<<t->second<<std::endl;
	}
  }
  else
  {
      int wstatus;
      pid_t w;

      do
      {
        w = waitpid(pid,&wstatus,__WALL);
      }while(!WIFEXITED(wstatus));

      auto a = hm->find("a");
      std::cout <<" v = "<<a->second<<std::endl;
      a = hm->find("b");
      std::cout <<" v = "<<a->second<<std::endl;
      a = hm->find("c");
      std::cout <<" v = "<<a->second<<std::endl;
  }



  return 0;
}

/*
loading global AHA with 8 threads...
  took 487 ms (40 ns/insert).
loading global AHM with 8 threads...
  took 478 ms (39 ns/insert).
loading global QPAHM with 8 threads...
  took 478 ms (39 ns/insert).

Running AHM benchmarks on machine with 24 logical cores.
  num elements per map: 12000000
  num threads for mt tests: 24
  AHM load factor: 0.75

============================================================================
folly/test/AtomicHashMapTest.cpp                relative  time/iter  iters/s
============================================================================
st_aha_find                                                 92.63ns   10.80M
st_ahm_find                                                107.78ns    9.28M
st_qpahm_find                                               90.69ns   11.03M
----------------------------------------------------------------------------
mt_ahm_miss                                                  2.09ns  477.36M
mt_qpahm_miss                                                1.37ns  728.82M
st_ahm_miss                                                241.07ns    4.15M
st_qpahm_miss                                              223.17ns    4.48M
mt_ahm_find_insert_mix                                       8.05ns  124.24M
mt_qpahm_find_insert_mix                                     9.10ns  109.85M
mt_aha_find                                                  6.82ns  146.68M
mt_ahm_find                                                  7.95ns  125.77M
mt_qpahm_find                                                6.81ns  146.83M
st_baseline_modulus_and_random                               6.02ns  166.03M
mt_ahm_insert                                               14.29ns   69.97M
mt_qpahm_insert                                             11.68ns   85.61M
st_ahm_insert                                              125.39ns    7.98M
st_qpahm_insert                                            128.76ns    7.77M
============================================================================
*/
