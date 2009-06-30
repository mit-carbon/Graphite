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

#include <boost/thread/mutex.hpp>

namespace config
{
#if (BOOST_VERSION==103500)
    using namespace boost::spirit;
#else
    using namespace boost::spirit::classic;
#endif

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

            void saveAs(const std::string &path);

            void loadConfigFromString(const std::string & cfg);
        private:
            void SaveTreeAs(std::ofstream &out, const Section &current);

            /*! loadConfig() Function which loads the Config tree with the entries extracted from the
             * file located at the m_path member variable.
             */
            void loadConfig();

            //! loadFileToString() Function that reads a given filename into a std::string
            void loadFileToString(std::string &s, const std::string &filename);

            //! parse() The function that is responsible for recursively building the tree
            void parse(const std::string &source, Section & current);
            void evalTree(Section & current, tree_iter_t const& node, int depth = 0);

            void showParseTree(tree_iter_t const& node, int depth = 0);

            // This function is used to turn \" into " and \\ into \  and the like
            void unEscapeText(const std::string & source, std::string & dest);
            void escapeText(const std::string & source, std::string & dest);

            std::string getNodeValue(tree_iter_t const& node);
            RuleID getNodeID(tree_iter_t const& node);


            //Hooks for creating appropriate objects when they are parse
            void createStringKey(Section & current, const std::string key_name, const std::string & value);
            void createIntKey(Section & current, const std::string key_name, int value);
            void createFloatKey(Section & current, const std::string key_name, double value);

            //The following code is commented out until full thread protection is provided
            //in the get/set functions as well as block writing, preferably through a shared
            //mutex.
//            boost::mutex m_fileWriterMux;
    };
}

#endif

