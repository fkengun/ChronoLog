#ifndef __NODE_H_
#define __NODE_H_

#include <boost/cstdint.hpp>
#include <boost/config.hpp>
#include <boost/atomic.hpp>

template<
        class KeyT,
        class ValueT,
	class HashFcn=std::hash<KeyT>,
	class EqualFcn=std::equal_to<KeyT>
        >
struct node
{
   KeyT key;
   ValueT value;
   struct node*next;
};




#endif
