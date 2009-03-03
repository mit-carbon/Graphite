/*!
 *\file
*/
// Config Class
// Author: Charles Gruenwald III
#include "section.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace config
{

    //! Section() Constructor for a root section
    Section::Section(const std::string & name, bool case_sensitive)
        :
            m_name(name),
            m_subSections(),
            m_keys(),
            m_parent(*this),
            m_isroot(true),
            m_case_sensitive(case_sensitive)
    {
    }

    //! Section() Constructor for a subsection
    Section::Section(Section const & parent, const std::string & name, bool case_sensitive)
        :
            m_name(name),
            m_subSections(),
            m_keys(),
            m_parent(parent),
            m_isroot(false),
            m_case_sensitive(case_sensitive)
    {
    }

    //! ~Section() Destructor
    Section::~Section()
    {
        m_subSections.clear();
        m_keys.clear();
    }

    //Create a new section and add it to the subsection map
    Section & Section::AddSubsection(const std::string & name_)
    {
        std::string iname(name_);
        if(!m_case_sensitive)
            boost::to_lower(iname);

        m_subSections.insert(std::make_pair(iname, new Section(*this, name_, m_case_sensitive)));
        return *(m_subSections[iname].get());
    }

    //Add a new key/value pair to this section
    const Key & Section::AddKey(const std::string & name_, const std::string & value)
    {
        //Make the key all lower-case if not case sensitive
        std::string iname(name_);
        if(!m_case_sensitive)
            boost::to_lower(iname);

        //Remove existing key if found
        KeyList::const_iterator found = m_keys.find(iname);
        if(found != m_keys.end())
            m_keys.erase(iname);

        m_keys.insert(std::make_pair(iname, new Key(this->GetFullPath(),name_,value)));
        return *(m_keys[iname].get());
    }

    const Key & Section::AddKey(const std::string & name_, const int value)
    {
        //Make the key all lower-case if not case sensitive
        std::string iname(name_);
        if(!m_case_sensitive)
            boost::to_lower(iname);

        //Remove existing key if found
        KeyList::const_iterator found = m_keys.find(iname);
        if(found != m_keys.end())
            m_keys.erase(iname);

        m_keys.insert(std::make_pair(iname, new Key(this->GetFullPath(),name_,value)));
        return *(m_keys[iname].get());
    }

    const Key & Section::AddKey(const std::string & name_, const double value)
    {
        //Make the key all lower-case if not case sensitive
        std::string iname(name_);
        if(!m_case_sensitive)
            boost::to_lower(iname);

        //Remove existing key if found
        KeyList::const_iterator found = m_keys.find(iname);
        if(found != m_keys.end())
            m_keys.erase(iname);

        m_keys.insert(std::make_pair(iname, new Key(this->GetFullPath(),name_,value)));
        return *(m_keys[iname].get());
    }

    //Get a subkey of the given name
    const Key & Section::GetKey(const std::string & name_)
    {
        std::string iname(name_);
        if(!m_case_sensitive)
            boost::to_lower(iname);
        return *(m_keys[iname].get());
    }

    //Get a subsection of the given name
    const Section & Section::GetSection(const std::string & name)
    {
        return GetSection_unsafe(name);
    }

    //Unsafe version of the GetSection function, this should only be used internally
    Section & Section::GetSection_unsafe(const std::string & name)
    {
        std::string iname(name);
        if(!m_case_sensitive)
            boost::to_lower(iname);
        return *(m_subSections[iname].get());
    }

    bool Section::HasSection(const std::string & name) const
    {
        std::string iname(name);
        if(!m_case_sensitive)
            boost::to_lower(iname);
        SectionList::const_iterator found = m_subSections.find(iname);
        return (found != m_subSections.end());
    }

    bool Section::HasKey(const std::string & name) const
    {
        std::string iname(name);
        if(!m_case_sensitive)
            boost::to_lower(iname);
        KeyList::const_iterator found = m_keys.find(iname);
        return (found != m_keys.end());
    }

    const std::string Section::GetFullPath() const
    {
        std::string path = "";
        if(IsRoot())
            return path;

        if(GetParent().IsRoot())
            return GetName();

        //Create the path from the parent section and this section's name
        path = std::string(GetParent().GetFullPath()) + "/" + GetName();
        return path;
    }

}//end of namespace config


