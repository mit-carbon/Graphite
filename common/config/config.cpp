/*!
 *\file
*/
// Config Class
// Author: Charles Gruenwald III
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "config.hpp"


namespace config
{

    bool Config::IsLeaf(const std::string & path)
    {
        return !boost::find_first(path, "/");
    }

    //Configuration Management
    const Section & Config::GetSection(const std::string & path)
    {
        return GetSection_unsafe(path);
    }

    Section & Config::GetSection_unsafe(const std::string & path)
    {
        //Handle the base case
        if(IsLeaf(path))
        {
            if(!m_root.HasSection(path))
                m_root.AddSubsection(path);
            return m_root.GetSection_unsafe(path);
        }

        //Split up the path on "/", and loop through each entry of this 
        //split to obtain the actual section
        PathElementList path_elements;
        Config::SplitPathElements(path, path_elements);

        Section * current = &m_root;
        for(PathElementList::iterator path_element = path_elements.begin(); 
                path_element != path_elements.end(); path_element++)
        {
            //Add the section if it doesn't already exist
            if(!current->HasSection(*path_element))
            {
                current->AddSubsection(*path_element);
            }

            //Find the current element name as a sub section of the current section
            current = &(current->GetSection_unsafe(*path_element));
        }
        return *current;
    }

    //Small wrapper which sets the m_path variable appropriatly and calls the virtual LoadConfig()
    void Config::Load(const std::string & path)
    {
        m_path = path;
        LoadConfig();
    }

    void Config::Clear()
    {
        m_root.Clear();
    }

    const Key & Config::GetKey(const std::string & path)
    {
        //Handle the base case
        if(IsLeaf(path))
        {
            if(!m_root.HasKey(path))
                throw KeyNotFound();
            else
                return m_root.GetKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::SplitPath(path);
        Section & section = GetSection_unsafe(path_pair.first);

        //Add the key if it doesn't already exist
        if(!section.HasKey(path_pair.second))
        {
            section.AddKey(path_pair.second, "");
        }

        return section.GetKey(path_pair.second);
    }


    const Key & Config::GetKey(const std::string & path, int default_val)
    {
        //Handle the base case
        if(IsLeaf(path))
        {
            if(!m_root.HasKey(path))
                m_root.AddKey(path, default_val);
            return m_root.GetKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::SplitPath(path);
        Section & section = GetSection_unsafe(path_pair.first);

        //Add the key if it doesn't already exist
        if(!section.HasKey(path_pair.second))
        {
            section.AddKey(path_pair.second, default_val);
        }

        return section.GetKey(path_pair.second);
    }

    const Key & Config::GetKey(const std::string & path, double default_val)
    {
        //Handle the base case
        if(IsLeaf(path))
        {
            if(!m_root.HasKey(path))
                m_root.AddKey(path, default_val);
            return m_root.GetKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::SplitPath(path);
        Section & section = GetSection_unsafe(path_pair.first);

        //Add the key if it doesn't already exist
        if(!section.HasKey(path_pair.second))
        {
            section.AddKey(path_pair.second, default_val);
        }

        return section.GetKey(path_pair.second);
    }

    const Key & Config::GetKey(const std::string & path, const std::string &default_val)
    {
        //Handle the base case
        if(IsLeaf(path))
        {
            if(!m_root.HasKey(path))
                m_root.AddKey(path, default_val);
            return m_root.GetKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::SplitPath(path);
        Section & section = GetSection_unsafe(path_pair.first);

        //Add the key if it doesn't already exist
        if(!section.HasKey(path_pair.second))
        {
            section.AddKey(path_pair.second, default_val);
        }

        return section.GetKey(path_pair.second);
    }

    const Section & Config::AddSection(const std::string & path)
    {
        //Disect the path
        PathPair path_pair = Config::SplitPath(path);
        Section &parent = GetSection_unsafe(path_pair.first);
        return parent.AddSubsection(path_pair.second);
    }

    std::pair<std::string,std::string> Config::SplitPath(const std::string & path)
    {
        //Throw away path_elements, just return base and key/section
        std::vector<std::string> path_elements;
        return Config::SplitPathElements(path, path_elements);
    }

    std::pair<std::string,std::string> Config::SplitPathElements(const std::string & path, PathElementList & path_elements)
    {
        //Split up the path on "/", the last entry is the name of the key
        //Everything up to the last "/" is the 'base_path' (which will specify a section)

        boost::split(path_elements, path, boost::is_any_of("/"));

        //Grab the appropriate pieces from the split
        std::string key_name = path_elements[path_elements.size() - 1];
        std::string base_path = "";
        if(path.rfind("/") != std::string::npos)
            base_path = path.substr(0, path.rfind("/"));

        return PathPair(base_path,key_name);
    }

    const Key & Config::AddKey(const std::string & path, const std::string & value)
    {
        //Handle the base case
        if(IsLeaf(path))
            return m_root.AddKey(path, value);

        PathPair path_pair = Config::SplitPath(path);
        Section &parent = GetSection_unsafe(path_pair.first);
        return parent.AddKey(path_pair.second, value);
    }

    const Key & Config::AddKey(const std::string & path, int value)
    {
        //Handle the base case
        if(IsLeaf(path))
            return m_root.AddKey(path, value);

        PathPair path_pair = Config::SplitPath(path);
        Section &parent = GetSection_unsafe(path_pair.first);
        return parent.AddKey(path_pair.second, value);
    }

    const Key & Config::AddKey(const std::string & path, double value)
    {
        //Handle the base case
        if(IsLeaf(path))
            return m_root.AddKey(path, value);

        PathPair path_pair = Config::SplitPath(path);
        Section &parent = GetSection_unsafe(path_pair.first);
        return parent.AddKey(path_pair.second, value);
    }

    //Convert the in-memory representation into a string
    std::string Config::ShowTree(const Section & current, int depth)
    {
        std::string result = "";

        std::string ret = "";
        std::string tabs = "";
        for(int i=0;i<depth;i++)
            tabs = tabs.append("    ");

        //First loop through all the subsections
        SectionList const & subsections = current.GetSubsections();
        for(SectionList::const_iterator i = subsections.begin(); i != subsections.end(); i++)
        {
            Section const & subsection = *(i->second.get());
            result += tabs + "Section: " + i->second->GetName() + "\n";
            //recurse
            result += ShowTree(subsection, depth+1);
        }

        //Now add all the keys of this section
        KeyList const & keys = current.GetKeys();
        for(KeyList::const_iterator i = keys.begin(); i != keys.end();i++)
        {
            result += tabs + "Key: " + i->second->GetName() + " - " + i->second->GetString() + "\n";
        }
        return result;
    }

    void Config::Set(const std::string & path, const std::string & new_value)
    {
        AddKey(path, new_value);
    }

    void Config::Set(const std::string & path, int new_value)
    {
        AddKey(path, new_value);
    }

    void Config::Set(const std::string & path, double new_value)
    {
        AddKey(path, new_value);
    }

    //Below are the getters which also handle default values
    bool Config::GetBool(const std::string & path)
    {
        return GetKey(path).GetBool();
    }

    bool Config::GetBool(const std::string & path, bool default_val)
    {
        return GetKey(path,default_val).GetBool();
    }

    int Config::GetInt(const std::string & path)
    {
        return GetKey(path).GetInt();
    }
    int Config::GetInt(const std::string & path, int default_val)
    {
        return GetKey(path,default_val).GetInt();
    }

    const std::string Config::GetString(const std::string & path)
    {
        return GetKey(path).GetString();
    }
    const std::string Config::GetString(const std::string & path, const std::string & default_val)
    {
        return GetKey(path,default_val).GetString();
    }

    double Config::GetFloat(const std::string & path)
    {
        return GetKey(path).GetFloat();
    }
    double Config::GetFloat(const std::string & path, double default_val)
    {
        return GetKey(path,default_val).GetFloat();
    }

}//end of namespace config
