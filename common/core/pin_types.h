#ifndef PIN_TYPES_H
#define PIN_TYPES_H
// Temporarily stolen from pin...
template<int dummy>
class INDEX 
{
public:
    /*
      INT32 index; must be public - so that both vs8 and icc will treat
      functions that return  INDEX<num> classes (such as BBL) the same  - i.e. will return
      the value in a register without passing an implicit param into the function that is
      a pointer to the location to store the return value to.
      If it is private then vs8 will do the implicit passing and icc will not.
      This incompatability is supposed to be fixed in icc version 11.0
    */
    SInt32 index;
    bool operator==(const INDEX<dummy> right) const { return right.index == index;}\
    bool operator!=(const INDEX<dummy> right) const { return right.index != index;}\
    bool operator<(const INDEX<dummy> right)  const { return index < right.index;}\
    
    // please do not try to introduce a constructor here
    // otherwise prograns will not compile
    // and there are possibly performance penalties as well
    //INDEX(INT32 x=0) : index(x) {}  
    SInt32 q() const {return index;}
    bool is_valid() const { return (index>0);}
    void  q_set(SInt32 y) {index=y;}
    void  invalidate() {index=0;}

};
typedef class INDEX<6> INS;
typedef int REG;

#endif
