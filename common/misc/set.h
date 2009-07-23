#ifndef __SET_H__
#define __SET_H__

#include <set>

template <class T>
class Set
{
   private:
      std::set<T>** m_set_list;
      UInt32 m_num_buckets;
      UInt32 (*m_hash_fn)(T, UInt32, UInt32);
      UInt32 m_hash_fn_param;

   public:
      Set(UInt32 num_buckets, UInt32 (*hash_fn)(T, UInt32, UInt32), UInt32 hash_fn_param) :
         m_num_buckets(num_buckets),
         m_hash_fn(hash_fn),
         m_hash_fn_param(hash_fn_param)
      {
         m_set_list = new std::set<T>*[m_num_buckets];
         for (UInt32 i = 0; i < m_num_buckets; i++)
         {
            m_set_list[i] = new std::set<T>;
         }
      }

      ~Set() {}

      void insert(T elem)
      {
         UInt32 bucket = m_hash_fn(elem, m_hash_fn_param, m_num_buckets);
         m_set_list[bucket]->insert(elem);
         LOG_ASSERT_WARNING(m_set_list[bucket]->size() <= 100,
               "m_set_list[%u] : (%u > 100)", bucket, m_set_list[bucket]->size())
      }
      UInt32 count(T elem)
      {
         UInt32 bucket = m_hash_fn(elem, m_hash_fn_param, m_num_buckets);
         return m_set_list[bucket]->count(elem);
      }
      void erase(T elem)
      {
         UInt32 bucket = m_hash_fn(elem, m_hash_fn_param, m_num_buckets);
         m_set_list[bucket]->erase(elem);
      }

      
};


#endif /* __SET_H__ */
