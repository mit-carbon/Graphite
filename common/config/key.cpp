/*!
 *\file
*/
// Config Class
// Author: Charles Gruenwald III
#include "key.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/classic_core.hpp>


using namespace boost::spirit::classic;

namespace config
{
    Key::Key(const std::string & parentPath_, const std::string & name_, const std::string & value_)
        :
            m_name(name_),
            m_value(value_),
            m_parentPath(parentPath_),
            m_type(DetermineType(m_value))
    {
    }

    Key::Key(const std::string & parentPath_, const std::string & name_, int value_)
        :
            m_name(name_),
            m_value(boost::lexical_cast<std::string>(value_)),
            m_parentPath(parentPath_),
            m_type(DetermineType(m_value))
    {
    }

    Key::Key(const std::string & parentPath_, const std::string & name_, double value_)
        :
            m_name(name_),
            m_value(boost::lexical_cast<std::string>(value_)),
            m_parentPath(parentPath_),
            m_type(DetermineType(m_value))
    {
    }

    //Determine the type of a given key by trying to cast it to different types
    //and catching the error.
    const unsigned short Key::DetermineType(std::string value)
    {
        //strings are always valid
        unsigned short valid = TYPE_STRING_VALID;

        // Try for floats
        if(boost::spirit::classic::parse(value.c_str(),real_p).full)
        {
            try
            {
                m_value_f = boost::lexical_cast<double>(value);
                valid |= TYPE_FLOAT_VALID;
            }
            catch(const boost::bad_lexical_cast &)
            {
            }
        }

        // and ints
        if(boost::spirit::classic::parse(value.c_str(),int_p).full)
        {
            try
            {
                m_value_i = boost::lexical_cast<int>(value);
                valid |= TYPE_INT_VALID;
            }
            catch(const boost::bad_lexical_cast &)
            {
            }
        }

        // and bools
        std::string icase_value = m_value;
        boost::to_lower(icase_value);
        if(icase_value == "true" || icase_value == "yes" || icase_value == "1")
        {
            valid |= TYPE_BOOL_VALID;
            m_value_b = true;
        }
        else if(icase_value == "false" || icase_value == "no" || icase_value == "0")
        {
            valid |= TYPE_BOOL_VALID;
            m_value_b = false;
        }
        return valid;
    }

    bool Key::GetBool() const
    {
        if(m_type & TYPE_BOOL_VALID)
            return m_value_b;
        else
            throw std::bad_cast();
    }

    int Key::GetInt() const
    {
        if(m_type & TYPE_INT_VALID)
            return m_value_i;
        else
            throw std::bad_cast();
    }

    const std::string Key::GetString() const
    {
        return m_value;
    }

    double Key::GetFloat() const
    {
        if(m_type & TYPE_FLOAT_VALID)
            return m_value_f;
        else
            throw std::bad_cast();
    }

    void Key::GetValue(bool &bool_val) const
    {
        bool_val = GetBool();
    }

    void Key::GetValue(int &int_val) const
    {
        int_val = GetInt();
    }

    void Key::GetValue(std::string &string_val) const
    {
        string_val = GetString();
    }

    void Key::GetValue(double &double_val) const
    {
        double_val = GetFloat();
    }

}//end of namespace config

