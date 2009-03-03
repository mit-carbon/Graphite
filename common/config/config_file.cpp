/*!
 * \file config_file.cpp
 *  The file interface to the config class.
*/
// Config Class
// Author: Charles Gruenwald III
#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/algorithm/string.hpp>

#include "config_file.hpp"
#include "config_exceptions.hpp"

using namespace boost::spirit::classic;

namespace config
{

    ConfigFile::ConfigFile(bool case_sensitive)
        :
            Config(case_sensitive)
    {
    }

    ConfigFile::ConfigFile(const Section & root, bool case_sensitive)
        :
            Config(root, case_sensitive)
    {
    }

    //Load a given filename into a string, taken from boost regexp replace example
    void ConfigFile::LoadFileToString(std::string& s, const std::string& filename)
    {
        std::ifstream file_in(filename.c_str());

        s.erase();
        s.reserve(file_in.rdbuf()->in_avail());
        char c;
        while(file_in.get(c))
        {
            if(s.capacity() == s.size())
                s.reserve(s.capacity() * 3);
            s.append(1, c);
        }
        file_in.close();
    }

    void ConfigFile::LoadConfig()
    {
        std::string filename = m_path;

        //Make sure the file exists
        if(!boost::filesystem::exists(filename))
            throw FileNotFound();

        //Read the data in from the file
        std::string data;
        LoadFileToString(data, filename);
        LoadConfigFromString(data);

    }

    void ConfigFile::LoadConfigFromString(const std::string & cfg)
    {
        Parse(cfg, m_root);
    }


    //Build the in-memory representation by line-by-line matching boost::regex expressions
    void ConfigFile::Parse(const std::string &source, Section & current)
    {
        //rules
        config_parser parser;
        parse_info_t info = ast_parse<config_parser::factory_t>(source.c_str(), source.c_str() + source.size(),parser,space_p | comment_p("#")); 

        if(info.full)
        {
            // Uncomment the following lines for more verbose output
            // ShowParseTree(info.trees.begin());
            EvalTree(m_root, info.trees.begin());
        }
        else
        {
            std::string error_str(info.stop);
            throw ParserError(error_str);
        }
    }

    void ConfigFile::ShowParseTree(tree_iter_t const& node, int depth)
    {
        std::string tabs = "";
        for(int i=0;i<depth;i++)
            tabs = tabs.append("    ");

        RuleID rule = node->value.value().e;

        // Get the string value from the iter
        std::string value(node->value.begin(), node->value.end() - node->value.begin());
        if(rule == sectionID)
            value = "s: ";

        if(rule == configID)
            value = "c: " + value;

        else if(rule == keyID)
            value = "k: " + value;

        else if(rule == keyValueID)
            value = "kv: " + value;

        else if(rule == keyNameID)
            value = "kn: " + value;

        else if(rule == sectionNameID)
            value = "sn: " + value;

        // if(rule == sectionID || rule == keyID || rule == keyValueID || rule == keyNameID || rule == sectionNameID)
        // if(rule > 0)
            std::cout << tabs << (int)(rule) << ".[" << value << "]" << std::endl;

        for(tree_iter_t chi = node->children.begin(); chi != node->children.end(); chi++)
        {
            ShowParseTree(chi, depth + 1);
        }
    }

    void ConfigFile::UnEscapeText(const std::string & source, std::string & dest)
    {
        bool backslash = false;
        for (unsigned int i = 0; i < source.size(); i++) {
            char c = source.c_str()[i];
            if (backslash)
            {
                switch (c)
                {
                    case '\\': dest += "\\"; break;
                    case 'b':  dest += "\b"; break;
                    case 'r':  dest += "\r"; break;
                    case 'f':  dest += "\f"; break;
                    case 't':  dest += "\t"; break;
                    case 'n':  dest += "\n"; break;
                    case '"':  dest += "\""; break;
                    case '\'': dest += "'";  break;
                    default:   dest += c;
                }
                backslash = false;
            }
            else
            {
                if (c == '\\')
                    backslash = true;
                else
                    dest += c;
            }
        }
    }

    std::string ConfigFile::GetNodeValue(tree_iter_t const& node)
    {
        if(node->value.begin() == node->value.end())
            return "";
        return std::string(node->value.begin(), node->value.end());
    }

    RuleID ConfigFile::GetNodeID(tree_iter_t const& node)
    {
        return node->value.value().e;
    }


    void ConfigFile::EscapeText(const std::string & source, std::string & dest)
    {
        for (unsigned int i = 0; i < source.size(); i++) {
            char c = source.c_str()[i];
                switch (c)
                {
                    case '\\': dest += "\\\\"; break;
                    case '\"': dest += "\\\""; break;
                    default:   dest += c;
                }
        }
    }

    // This function recursively decends the tree built by the parser and populates the config
    // file with the appropriate entries
    void ConfigFile::EvalTree(Section & current, tree_iter_t const& node, int depth)
    {
        std::string tabs = "";
        for(int i=0;i<depth;i++)
            tabs = tabs.append("    ");

        const RuleID rule (GetNodeID(node));
        const std::string value(GetNodeValue(node));

        if(rule == sectionNameID)
        {
            // Since this is a  flat file representation, whenever we get a section we only add it 
            // to the root node, thus we break out if we are currently nested.
            if(!current.IsRoot())
                return;

            //HACK: Strip off the '[' and ']', guaranteed to be matched. 
            //This should be removed when we upgrade to Boost 1.35, as per config_file_grammar.hpp
            //note we should just be able to say section_name = value
            std::string section_name;
            section_name = value.substr(1, value.size() - 2);

            // std::cout << "Found section: [" << section_name << "]" << std::endl;

            // Create the section
            Section &child = Config::GetSection_unsafe(section_name);

            // Add each of the children nodes to this section
            for(tree_iter_t chi_node = node->children.begin(); chi_node != node->children.end(); chi_node++)
            {
                EvalTree(child, chi_node, depth + 1);
            }
        }
        else if(rule == keyID)
        {
            //assert(node->children.size() == 1 || node->children.size() == 2);

            std::string key_name = "";
            std::string key_value = "";

            // Nameless node
            if(node->children.size() >= 2)
            {
                UnEscapeText(GetNodeValue(node->children.begin()), key_name);
                UnEscapeText(GetNodeValue(node->children.begin() + 1), key_value);
            }
            else
            {
                if(node->children.size() == 0)
                {
                    std::ostringstream str;
                    str << "Internal parser error in section '" << current.GetName() << "': "
                        << "the node's children is empty.";
                    throw ParserError(str.str());
                }

                switch(GetNodeID(node->children.begin()))
                {
                    case keyNameID:
                        UnEscapeText(GetNodeValue(node->children.begin()), key_name);
                        break;
                    case keyValueID:
                        UnEscapeText(GetNodeValue(node->children.begin()), key_value);
                        break;
                    default:
                        throw ParserError("Internal parser error while walking parse tree and encountring key entry.");
                }
            }

            // Add the node to the tree
            current.AddKey(key_name, key_value);
        }
        else
        {
            for(tree_iter_t chi = node->children.begin(); chi != node->children.end(); chi++)
            {
                EvalTree(current, chi, depth + 1);
            }
        }

    }

    void ConfigFile::SaveAs(const std::string &path)
    {
        const std::string tmp_path(path + ".tmp");

        std::ofstream output_file;
        output_file.exceptions ( ofstream::eofbit | ofstream::failbit | ofstream::badbit );

        //Atomic write with error handling
        try
        {
            output_file.open(tmp_path.c_str());
            SaveTreeAs(output_file, m_root);
            output_file.close();
        }
        catch(const ofstream::failure & e)
        {
            throw SaveError(e.what());
        }

        try
        {
            boost::filesystem::remove(path);
            //please don't break here!
            boost::filesystem::rename(tmp_path, path);
        }
        catch(const boost::filesystem::filesystem_error & e)
        {
            std::string error_str("Moving " + tmp_path + " to " + path + ".");
            throw SaveError(error_str);
        }
    }

    //Dump the tree in a format that is readable by our parser
    void ConfigFile::SaveTreeAs(std::ofstream &out, const Section &current)
    {

        if(!current.IsRoot())
        {
            out << "[" << current.GetFullPath() << "]" << std::endl;
        }

        KeyList const & keys = current.GetKeys();
        for(KeyList::const_iterator i = keys.begin(); i != keys.end();i++)
        {

            const std::string & name = i->second->GetName();
            //Quote the name if it has spaces in it
            if(boost::find_first(name, " ") || boost::find_first(name, "\""))
            {
                std::string escaped_name;
                EscapeText(name, escaped_name);
                out << "\"" << escaped_name << "\"";
            }
            else
                out << name;

            //Quote the value if it is a string
            if(i->second->GetFloatValid() || i->second->GetIntValid())
                out << " = " << i->second->GetString() << std::endl;
            else
            {
                std::string escaped_value;
                EscapeText(i->second->GetString(), escaped_value);
                out << " = \"" << escaped_value << "\"" << std::endl;
            }
        }

        out << std::endl;

        SectionList const & subsections = current.GetSubsections();
        for(SectionList::const_iterator i = subsections.begin(); i != subsections.end(); i++)
        {
            Section const & subsection = *(i->second.get());
            SaveTreeAs(out, subsection);
        }


        return;
    }

//namespaces
}

