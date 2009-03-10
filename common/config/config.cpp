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

    bool Config::isLeaf(const std::string & path)
    {
        return !boost::find_first(path, "/");
    }

    //Configuration Management
    const Section & Config::getSection(const std::string & path)
    {
        return getSection_unsafe(path);
    }

    Section & Config::getSection_unsafe(const std::string & path)
    {
        //Handle the base case
        if(isLeaf(path))
        {
            if(!m_root.hasSection(path))
                m_root.addSubsection(path);
            return m_root.getSection_unsafe(path);
        }

        //split up the path on "/", and loop through each entry of this 
        //split to obtain the actual section
        PathElementList path_elements;
        Config::splitPathElements(path, path_elements);

        Section * current = &m_root;
        for(PathElementList::iterator path_element = path_elements.begin(); 
                path_element != path_elements.end(); path_element++)
        {
            //add the section if it doesn't already exist
            if(!current->hasSection(*path_element))
            {
                current->addSubsection(*path_element);
            }

            //Find the current element name as a sub section of the current section
            current = &(current->getSection_unsafe(*path_element));
        }
        return *current;
    }

    //Small wrapper which sets the m_path variable appropriatly and calls the virtual loadConfig()
    void Config::load(const std::string & path)
    {
        m_path = path;
        loadConfig();
    }

    void Config::clear()
    {
        m_root.clear();
    }

    const Key & Config::getKey(const std::string & path)
    {
        //Handle the base case
        if(isLeaf(path))
        {
            if(!m_root.hasKey(path))
                throw KeyNotFound();
            else
                return m_root.getKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::splitPath(path);
        Section & section = getSection_unsafe(path_pair.first);

        //add the key if it doesn't already exist
        if(!section.hasKey(path_pair.second))
        {
            section.addKey(path_pair.second, "");
        }

        return section.getKey(path_pair.second);
    }


    const Key & Config::getKey(const std::string & path, int default_val)
    {
        //Handle the base case
        if(isLeaf(path))
        {
            if(!m_root.hasKey(path))
                m_root.addKey(path, default_val);
            return m_root.getKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::splitPath(path);
        Section & section = getSection_unsafe(path_pair.first);

        //add the key if it doesn't already exist
        if(!section.hasKey(path_pair.second))
        {
            section.addKey(path_pair.second, default_val);
        }

        return section.getKey(path_pair.second);
    }

    const Key & Config::getKey(const std::string & path, double default_val)
    {
        //Handle the base case
        if(isLeaf(path))
        {
            if(!m_root.hasKey(path))
                m_root.addKey(path, default_val);
            return m_root.getKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::splitPath(path);
        Section & section = getSection_unsafe(path_pair.first);

        //add the key if it doesn't already exist
        if(!section.hasKey(path_pair.second))
        {
            section.addKey(path_pair.second, default_val);
        }

        return section.getKey(path_pair.second);
    }

    const Key & Config::getKey(const std::string & path, const std::string &default_val)
    {
        //Handle the base case
        if(isLeaf(path))
        {
            if(!m_root.hasKey(path))
                m_root.addKey(path, default_val);
            return m_root.getKey(path);
        }

        //Disect the path
        PathPair path_pair = Config::splitPath(path);
        Section & section = getSection_unsafe(path_pair.first);

        //add the key if it doesn't already exist
        if(!section.hasKey(path_pair.second))
        {
            section.addKey(path_pair.second, default_val);
        }

        return section.getKey(path_pair.second);
    }

    const Section & Config::addSection(const std::string & path)
    {
        //Disect the path
        PathPair path_pair = Config::splitPath(path);
        Section &parent = getSection_unsafe(path_pair.first);
        return parent.addSubsection(path_pair.second);
    }

    std::pair<std::string,std::string> Config::splitPath(const std::string & path)
    {
        //Throw away path_elements, just return base and key/section
        std::vector<std::string> path_elements;
        return Config::splitPathElements(path, path_elements);
    }

    std::pair<std::string,std::string> Config::splitPathElements(const std::string & path, PathElementList & path_elements)
    {
        //split up the path on "/", the last entry is the name of the key
        //Everything up to the last "/" is the 'base_path' (which will specify a section)

        boost::split(path_elements, path, boost::is_any_of("/"));

        //Grab the appropriate pieces from the split
        std::string key_name = path_elements[path_elements.size() - 1];
        std::string base_path = "";
        if(path.rfind("/") != std::string::npos)
            base_path = path.substr(0, path.rfind("/"));

        return PathPair(base_path,key_name);
    }

    const Key & Config::addKey(const std::string & path, const std::string & value)
    {
        //Handle the base case
        if(isLeaf(path))
            return m_root.addKey(path, value);

        PathPair path_pair = Config::splitPath(path);
        Section &parent = getSection_unsafe(path_pair.first);
        return parent.addKey(path_pair.second, value);
    }

    const Key & Config::addKey(const std::string & path, int value)
    {
        //Handle the base case
        if(isLeaf(path))
            return m_root.addKey(path, value);

        PathPair path_pair = Config::splitPath(path);
        Section &parent = getSection_unsafe(path_pair.first);
        return parent.addKey(path_pair.second, value);
    }

    const Key & Config::addKey(const std::string & path, double value)
    {
        //Handle the base case
        if(isLeaf(path))
            return m_root.addKey(path, value);

        PathPair path_pair = Config::splitPath(path);
        Section &parent = getSection_unsafe(path_pair.first);
        return parent.addKey(path_pair.second, value);
    }

    //Convert the in-memory representation into a string
    std::string Config::showTree(const Section & current, int depth)
    {
        std::string result = "";

        std::string ret = "";
        std::string tabs = "";
        for(int i=0;i<depth;i++)
            tabs = tabs.append("    ");

        //First loop through all the subsections
        SectionList const & subsections = current.getSubsections();
        for(SectionList::const_iterator i = subsections.begin(); i != subsections.end(); i++)
        {
            Section const & subsection = *(i->second.get());
            result += tabs + "Section: " + i->second->getName() + "\n";
            //recurse
            result += showTree(subsection, depth+1);
        }

        //Now add all the keys of this section
        KeyList const & keys = current.getKeys();
        for(KeyList::const_iterator i = keys.begin(); i != keys.end();i++)
        {
            result += tabs + "Key: " + i->second->getName() + " - " + i->second->getString() + "\n";
        }
        return result;
    }

    void Config::set(const std::string & path, const std::string & new_value)
    {
        addKey(path, new_value);
    }

    void Config::set(const std::string & path, int new_value)
    {
        addKey(path, new_value);
    }

    void Config::set(const std::string & path, double new_value)
    {
        addKey(path, new_value);
    }

    //Below are the getters which also handle default values
    bool Config::getBool(const std::string & path)
    {
        return getKey(path).getBool();
    }

    bool Config::getBool(const std::string & path, bool default_val)
    {
        return getKey(path,default_val).getBool();
    }

    int Config::getInt(const std::string & path)
    {
        return getKey(path).getInt();
    }
    int Config::getInt(const std::string & path, int default_val)
    {
        return getKey(path,default_val).getInt();
    }

    const std::string Config::getString(const std::string & path)
    {
        return getKey(path).getString();
    }
    const std::string Config::getString(const std::string & path, const std::string & default_val)
    {
        return getKey(path,default_val).getString();
    }

    double Config::getFloat(const std::string & path)
    {
        return getKey(path).getFloat();
    }
    double Config::getFloat(const std::string & path, double default_val)
    {
        return getKey(path,default_val).getFloat();
    }

}//end of namespace config
