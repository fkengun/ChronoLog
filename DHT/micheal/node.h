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
   boost::atomic<boost::int128_type> mark_ref;

   boost::int64_t get_reference()
   {
        return (boost::int64_t)mark_ref.load();
   }
   boost::int64_t get_mark()
   {
        boost::int128_type v = mark_ref.load();
        return (boost::int64_t)(v >> 64);
   }
   void set_mark_reference(boost::int64_t m, boost::int64_t r)
   {
        boost::int128_type v = (boost::int128_type)m;
        v = v << 64;
        v = v | r;
        mark_ref.store(v);
   }
   boost::int128_type get_mark_reference()
   {
           return mark_ref.load();
   }

};




#endif
