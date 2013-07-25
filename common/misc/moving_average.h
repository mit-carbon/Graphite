#ifndef __MOVING_AVERAGE_H__
#define __MOVING_AVERAGE_H__

#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

#include "fixed_types.h"
#include "modulo_num.h"

using namespace std;

template <class T>
class MovingAverage
{
   public:
      enum AvgType_t 
      {
         ARITHMETIC_MEAN = 0,
         GEOMETRIC_MEAN,
         MEDIAN,
         NUM_AVG_TYPES
      };

      MovingAverage(UInt32 max_window_size);
      virtual ~MovingAverage();

      static MovingAverage<T>* createAvgType(string avg_type, UInt32 max_window_size);

      virtual T compute(T next_num) { return (T) 0; }

   protected:
      vector<T> _num_list;
      UInt32 _max_window_size;
      ModuloNum _curr_window_front;
      ModuloNum _curr_window_back;

      void addToWindow(T next_num);
      
      void printElements()
      {
         printf("Elements: ");
         ModuloNum curr_num(_curr_window_front);
         for ( ; curr_num != _curr_window_back; curr_num = curr_num + 1)
            printf("%llu ", _num_list[curr_num._value]);
         printf("\n");
      }
};

template<class T>
MovingAverage<T>::MovingAverage(UInt32 max_window_size)
   : _max_window_size(max_window_size)
{
   _num_list.resize(_max_window_size + 1);
   _curr_window_front = ModuloNum(_max_window_size + 1);
   _curr_window_back = ModuloNum(_max_window_size + 1);
}

template<class T>
MovingAverage<T>::~MovingAverage()
{ }

template<class T> 
void MovingAverage<T>::addToWindow(T next_num)
{
   _num_list[_curr_window_back._value] = next_num;
   _curr_window_back = _curr_window_back + 1;
   if (_curr_window_back == _curr_window_front)
   {
      _curr_window_front = _curr_window_front + 1;
   }
}

template <class T>
class MovingArithmeticMean : public MovingAverage<T>
{
   private:
      double _arithmetic_mean;

   public:
      MovingArithmeticMean(UInt32 max_window_size):
         MovingAverage<T>(max_window_size),
         _arithmetic_mean(0.0)
      { }

      T compute(T next_num)
      {
         UInt32 curr_window_size = (this->_curr_window_back - this->_curr_window_front)._value;
         if (curr_window_size == this->_max_window_size)
         {
            T old_num = this->_num_list[this->_curr_window_front._value];
            _arithmetic_mean += ( ((double) next_num / curr_window_size) - \
                                  ((double) old_num / curr_window_size) );
         }
         else // Still adding elements to fill the window
         {
            _arithmetic_mean = (_arithmetic_mean * curr_window_size + next_num) / (curr_window_size + 1);
         }
         
         addToWindow(next_num);

         return (T) _arithmetic_mean;
      }
};

template <class T>
class MovingGeometricMean : public MovingAverage<T>
{
   private:
      double _geometric_mean;

   public:
      MovingGeometricMean(UInt32 max_window_size):
         MovingAverage<T>(max_window_size),
         _geometric_mean(1.0)
      { }

      T compute(T next_num)
      {
         UInt32 curr_window_size = (this->_curr_window_back - this->_curr_window_front)._value;
         if (curr_window_size == this->_max_window_size)
         {
            T old_num = this->_num_list[this->_curr_window_front._value];
            _geometric_mean *= ( pow((double) next_num, (1.0 / curr_window_size)) / pow((double) old_num, (1.0 / curr_window_size)) );
         }
         else // Still adding elements to fill the window
         {
            _geometric_mean = pow( pow(_geometric_mean, curr_window_size) * next_num, (1.0 / (curr_window_size+1)) );
         }

         addToWindow(next_num);
         
         return (T) _geometric_mean; 
      }
};

template <class T>
class MovingMedian : public MovingAverage<T>
{
   public:
      MovingMedian(UInt32 max_window_size):
         MovingAverage<T>(max_window_size)
      { }

      T compute(T next_num)
      {
         // FIXME: Might want to sort this before returning any value
         addToWindow(next_num);
        
         UInt32 curr_window_size = (this->_curr_window_back - this->_curr_window_front)._value;
         UInt32 median_index = (this->_curr_window_front + (curr_window_size / 2))._value;
         return this->_num_list[median_index];
      }
};

template <class T>
MovingAverage<T>* MovingAverage<T>::createAvgType(string avg_type, UInt32 max_window_size)
{
   if (avg_type == "arithmetic_mean")
      return new MovingArithmeticMean<T>(max_window_size);
   else if (avg_type == "geometric_mean")
      return new MovingGeometricMean<T>(max_window_size);
   else if (avg_type == "median")
      return new MovingMedian<T>(max_window_size);
   else
   {
      printf("ERROR: Unsupported Average Type: %s\n", avg_type.c_str());
      return NULL;
   }
}

#endif /* __MOVING_AVERAGE_H__ */
