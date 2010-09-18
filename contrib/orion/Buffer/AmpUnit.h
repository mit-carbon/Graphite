#ifndef __AMPUNIT_H__
#define __AMPUNIT_H__

#include "Type.h"

class TechParameter;

class AmpUnit
{
  public:
    enum AmpModel
    {
      NO_MODEL = 0,
      GENERIC_AMP
    };

  public:
    AmpUnit(
      const string& amp_model_str_,
      const TechParameter* tech_param_ptr_
    );
    ~AmpUnit();

  public:
    double get_e_access() const { return m_e_access; }

  private:
    void init();

  private:
    AmpModel m_amp_model;
    const TechParameter* m_tech_param_ptr;

    double m_e_access;
};

#endif

