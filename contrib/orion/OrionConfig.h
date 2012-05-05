#ifndef __ORIONCONFIG_H__
#define __ORIONCONFIG_H__

#include <iostream>
#include <sstream>
#include <map>
#include <cassert>

#include "Type.h"

using namespace std;

class TechParameter;

class OrionConfig
{
  public:
    OrionConfig(const string& cfg_fn_, uint32_t tech_node_);
    OrionConfig(const OrionConfig& orion_cfg_);
    ~OrionConfig();

  public:
    static void allocate(string& orion_cfg_file_, uint32_t tech_node_);
    static void release();
    static OrionConfig* getSingleton();

    void set_num_in_port(uint32_t num_in_port_);
    void set_num_out_port(uint32_t num_out_port_);
    void set_num_vclass(uint32_t num_vclass_);
    void set_num_vchannel(uint32_t num_vchannel_);
    void set_in_buf_num_set(uint32_t in_buf_num_set_);
    void set_flit_width(uint32_t flit_width_);
    void set_frequency(float frequency_);

    void read_file(const string& filename_);
    void print_config(ostream& out_);

  public:
    template<class T>
    T get(const string& key_) const;
    const TechParameter* get_tech_param_ptr() const { return m_tech_param_ptr; }
    uint32_t get_num_in_port() const { return m_num_in_port; }
    uint32_t get_num_out_port() const { return m_num_out_port; }
    uint32_t get_num_vclass() const { return m_num_vclass; }
    uint32_t get_num_vchannel() const { return m_num_vchannel; }
    uint32_t get_in_buf_num_set() const { return m_in_buf_num_set; }
    uint32_t get_flit_width() const { return m_flit_width; }

  private:
    map<string, string> m_params_map;

    TechParameter* m_tech_param_ptr;
    uint32_t m_num_in_port;
    uint32_t m_num_out_port;
    uint32_t m_num_vclass;
    uint32_t m_num_vchannel;
    uint32_t m_in_buf_num_set;
    uint32_t m_flit_width;

  protected:
    struct key_not_found
    {
      string m_key;
      key_not_found(const string& key_ = string()) : m_key(key_)
      {}
    };
    template<class T>
    static T string_as_T(const string& str_);
    template<class T>
    static string T_as_string(const T& t_);

  private:
    static OrionConfig* m_singleton;
    static string ms_param_name[];
};

template<class T>
T OrionConfig::get(const string& key_) const
{
  map<string, string>::const_iterator it;

  it = m_params_map.find(key_);
  if (it == m_params_map.end()) 
  {
    fprintf(stderr, "ERROR: Orion: %s NOT FOUND!\n", key_.c_str());
    throw key_not_found(key_);
  }
  return string_as_T<T>(it->second);
}

template<class T>
T OrionConfig::string_as_T(const string& str_)
{
  T ret;
  std::istringstream ist(str_);
  ist >> ret;
  return ret;
}

template<>
inline string OrionConfig::string_as_T<string>(const string& str_)
{
  return str_;
}

template<>
inline bool OrionConfig::string_as_T<bool>(const string& str_)
{
  bool ret;
  if (str_ == string("TRUE"))
  {
    ret = true;
  }
  else if (str_ == string("FALSE"))
  {
    ret = false;
  }
  else
  {
    cerr << "Invalid bool value: '" << str_ << "'. Treated as FALSE." << endl;
    ret = false;
  }
  return ret;
}

template<class T>
string OrionConfig::T_as_string(const T& t)
{
  std::ostringstream ost;
  ost << t;
  return ost.str();
}

#endif

