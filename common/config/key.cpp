/*!
 *\file
*/
// Config Class
// Author: Charles Gruenwald III
#include "key.hpp"

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#if (BOOST_VERSION==103500)
# include <boost/spirit/core.hpp>
using namespace boost::spirit;
#else
# include <boost/spirit/include/classic_core.hpp>
using namespace boost::spirit::classic;
#endif

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
        if(parse(value.c_str(),real_p).full)
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
        if(parse(value.c_str(),int_p).full)
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

    bool Key::getBool() const
    {
        if(m_type & TYPE_BOOL_VALID)
            return m_value_b;
        else
            throw std::bad_cast();
    }

    int Key::getInt() const
    {
        if(m_type & TYPE_INT_VALID)
            return m_value_i;
        else
            throw std::bad_cast();
    }

    const std::string Key::getString() const
    {
        return m_value;
    }

    double Key::getFloat() const
    {
        if(m_type & TYPE_FLOAT_VALID)
            return m_value_f;
        else
            throw std::bad_cast();
    }

    void Key::getValue(bool &bool_val) const
    {
        bool_val = getBool();
    }

    void Key::getValue(int &int_val) const
    {
        int_val = getInt();
    }

    void Key::getValue(std::string &string_val) const
    {
        string_val = getString();
    }

    void Key::getValue(double &double_val) const
    {
        double_val = getFloat();
    }

}//end of namespace config

