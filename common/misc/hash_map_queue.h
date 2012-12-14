#pragma once

#include <map>
#include <queue>
using std::map;
using std::queue;
using std::make_pair;

template<typename K, typename V>
class HashMapQueue
{
public:
   void enqueue(K key, V value);
   V dequeue(K key);
   V front(K key) const;
   size_t count(K key) const;
   bool empty(K key) const;
   size_t size() const;

private:
   map<K, queue<V> > _hash_map_queue;
};

template <typename K, typename V>
void HashMapQueue<K,V>::enqueue(K key, V value)
{
   // Get the iterator
   typename map<K, queue<V> >::iterator it = _hash_map_queue.find(key);

   if (it == _hash_map_queue.end())
   {
      // Create a new queue whose only element is the value
      queue<V> value_queue;
      value_queue.push(value);
      _hash_map_queue.insert(make_pair(key, value_queue));
   }
   else
   {
      // Push the value
      queue<V>& value_queue = (it->second);
      value_queue.push(value);
   }
}

template <typename K, typename V>
V HashMapQueue<K,V>::dequeue(K key)
{
   // The hash table cannot be empty for that key
   typename map<K, queue<V> >::iterator it = _hash_map_queue.find(key);
   if (it == _hash_map_queue.end())
      return V();

   // Get the value
   queue<V>& value_queue = (it->second);
   V value = value_queue.front();
   value_queue.pop();

   // Remove the queue if empty
   if (value_queue.empty())
      _hash_map_queue.erase(it);

   // Return the value
   return value;
}

template <typename K, typename V>
V HashMapQueue<K,V>::front(K key) const
{
   // The hash table cannot be empty for that key
   typename map<K, queue<V> >::const_iterator it = _hash_map_queue.find(key);
   if (it == _hash_map_queue.end())
      return V();

   // Return the value
   const queue<V>& value_queue = (it->second);
   return value_queue.front();
}

template <typename K, typename V>
size_t HashMapQueue<K,V>::count(K key) const
{
   // Get the iterator
   typename map<K, queue<V> >::const_iterator it = _hash_map_queue.find(key);
  
   if (it == _hash_map_queue.end())
   {
      // If not present, return 0 
      return 0;
   }
   else
   {
      // Return the count from the queue
      const queue<V>& value_queue = (it->second);
      return value_queue.size();
   }
}

template <typename K, typename V>
bool HashMapQueue<K,V>::empty(K key) const
{
   // Get the iterator
   typename map<K, queue<V> >::const_iterator it = _hash_map_queue.find(key);
  
   if (it == _hash_map_queue.end())
   {
      // If not present, return true
      return true;
   }
   else
   {
      // The queue is present, if present it should be non-empty
      return false;
   }
}

template <typename K, typename V>
size_t HashMapQueue<K,V>::size() const
{
   return _hash_map_queue.size();
}
