#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP
// Config Class
// Author: Charles Gruenwald III

/*! \file config_file.hpp
 * This file contains the class responsible for 'flat file' configurations.
 * It accepts files very similar to the windows .reg file format, as in sub-sections
 * are collapsed like the following:
 * \verbatim
  ---------------------
  [main]
  value_in_main = 1
 
  [main/subsection]
  value_in_sub = "asdf"
  "value with spaces" = 4
  --------------------- \endverbatim
 *
 * It uses boost::regex to perform the parsing. Note that comments are not preserved
 * across saves.
 */
   
#include <fstream>
#include "config.hpp"
#include "config_file_grammar.hpp"

// #define BOOST_SPIRIT_DEBUG        // define this for debug output
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_escape_char.hpp>
#include <boost/spirit/include/classic_chset.hpp>

#include <boost/thread/mutex.hpp>

namespace config
{
    using namespace boost::spirit::classic;

    /*! \brief ConfigFile: A flat-file interface for the Config Class.
     * This file contains the class that is used to interface a flat
     * file for config input / output. It uses boost::spirit for the
     * config grammar.
     *
     * The grammar is as follows: \verbatim
    config       =  key* >> section* >> end
    section      =  section_name >> key*
    key          =  key_name >> '=' >> value
    key_name     =  string!
    value        =  real | int | string
    section_name = '[' >> *lex_escape_ch_p >> ']'
    string       = '"' >> *lex_escape_ch_p >> '"' | +(alnum | punct) >> *(alnum | punct ) \endverbatim
     */
    class ConfigFile : public config::Config
    {


        typedef tree_parse_info<config_parser::iterator_t, config_parser::factory_t> parse_info_t;
        typedef tree_match<config_parser::iterator_t, config_parser::factory_t> tree_match_t;
        typedef tree_match_t::node_t node_t;
        typedef tree_match_t::const_tree_iterator tree_iter_t;


        public:
            ConfigFile(bool case_sensitive = false);
            ConfigFile(const Section & root, bool case_sensitive = false);
            ~ConfigFile(){}

            void SaveAs(const std::string &path);

            void LoadConfigFromString(const std::string & cfg);
        private:
            void SaveTreeAs(std::ofstream &out, const Section &current);

            /*! LoadConfig() Function which loads the Config tree with the entries extracted from the
             * file located at the m_path member variable.
             */
            void LoadConfig();

            //! LoadFileToString() Function that reads a given filename into a std::string
            void LoadFileToString(std::string &s, const std::string &filename);

            //! Parse() The function that is responsible for recursively building the tree
            void Parse(const std::string &source, Section & current);
            void EvalTree(Section & current, tree_iter_t const& node, int depth = 0);

            void ShowParseTree(tree_iter_t const& node, int depth = 0);

            // This function is used to turn \" into " and \\ into \  and the like
            void UnEscapeText(const std::string & source, std::string & dest);
            void EscapeText(const std::string & source, std::string & dest);

            std::string GetNodeValue(tree_iter_t const& node);
            RuleID GetNodeID(tree_iter_t const& node);


            //Hooks for creating appropriate objects when they are parse
            void CreateStringKey(Section & current, const std::string key_name, const std::string & value);
            void CreateIntKey(Section & current, const std::string key_name, int value);
            void CreateFloatKey(Section & current, const std::string key_name, double value);

            //The following code is commented out until full thread protection is provided
            //in the get/set functions as well as block writing, preferably through a shared
            //mutex.
//            boost::mutex m_fileWriterMux;
    };
}

#endif

