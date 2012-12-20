#pragma once

#include <map>
#include <list>
using std::map;
using std::list;
using std::make_pair;

template<typename K, typename V>
class HashMapList
{
public:
   void enqueue(K key, V value);
   V dequeue(K key);
   V front(K key) const;
   size_t count(K key) const;
   bool empty(K key) const;
   size_t size() const;
  
   typedef typename map<K,list<V> >::iterator iterator;
   iterator begin()  { return _hash_map_list.begin(); }
   iterator end()    { return _hash_map_list.end(); }

private:
   map<K,list<V> > _hash_map_list;
};

template <typename K, typename V>
void HashMapList<K,V>::enqueue(K key, V value)
{
   // Get the iterator
   typename map<K, list<V> >::iterator it = _hash_map_list.find(key);

   if (it == _hash_map_list.end())
   {
      // Create a new list whose only element is the value
      list<V> value_list;
      value_list.push_back(value);
      _hash_map_list.insert(make_pair(key, value_list));
   }
   else
   {
      // Push the value
      list<V>& value_list = (it->second);
      value_list.push_back(value);
   }
}

template <typename K, typename V>
V HashMapList<K,V>::dequeue(K key)
{
   // The hash table cannot be empty for that key
   typename map<K, list<V> >::iterator it = _hash_map_list.find(key);
   if (it == _hash_map_list.end())
      return V();

   // Get the value
   list<V>& value_list = (it->second);
   V value = value_list.front();
   value_list.pop_front();

   // Remove the list if empty
   if (value_list.empty())
      _hash_map_list.erase(it);

   // Return the value
   return value;
}

template <typename K, typename V>
V HashMapList<K,V>::front(K key) const
{
   // The hash table cannot be empty for that key
   typename map<K, list<V> >::const_iterator it = _hash_map_list.find(key);
   if (it == _hash_map_list.end())
      return V();

   // Return the value
   const list<V>& value_list = (it->second);
   return value_list.front();
}

template <typename K, typename V>
size_t HashMapList<K,V>::count(K key) const
{
   // Get the iterator
   typename map<K, list<V> >::const_iterator it = _hash_map_list.find(key);
  
   if (it == _hash_map_list.end())
   {
      // If not present, return 0 
      return 0;
   }
   else
   {
      // Return the count from the list
      const list<V>& value_list = (it->second);
      return value_list.size();
   }
}

template <typename K, typename V>
bool HashMapList<K,V>::empty(K key) const
{
   // Get the iterator
   typename map<K, list<V> >::const_iterator it = _hash_map_list.find(key);
  
   if (it == _hash_map_list.end())
   {
      // If not present, return true
      return true;
   }
   else
   {
      // The list is present, if present it should be non-empty
      return false;
   }
}

template <typename K, typename V>
size_t HashMapList<K,V>::size() const
{
   return _hash_map_list.size();
}
