#ifndef __DSENTCONFIG_H__
#define __DSENTCONFIG_H__

#include <iostream>
#include <vector>
#include "../db_utils/api.h"
#include "Type.h"

namespace dsent_contrib
{
    // This class contains the file locations of DSENT config files, some helpers
    // used for getting data in/out of DSENT, and global technology overwrites used for all
    // DSENT calls.  Maybe other stuff will be added if the need should arise
    class DSENTInterface
    {
        public:
            class Overwrite
            {
                public:
                    Overwrite(const String& var_, const String& val_);
                    ~Overwrite();

                public:
                    inline String get_string() const { return (get_var() + "=" + get_val()); }
                    inline const String& get_var() const { return m_var_; }
                    inline const String& get_val() const { return m_val_; }

                private:
                    String m_var_;
                    String m_val_;
            };

        public:
            DSENTInterface(const String& dsent_path_, unsigned int tech_node_);
            ~DSENTInterface();

        public:
            // Allocate/release singletons
            static void allocate(const string& dsent_path_, unsigned int tech_node_);
            static void release();
            static DSENTInterface* getSingleton();

            // Get the file name of the config files used for various models
            inline const String& get_el_link_cfg_file_path() const { return m_el_link_cfg_file_path_; }
            inline const String& get_op_link_cfg_file_path() const { return m_op_link_cfg_file_path_; }
            inline const String& get_router_cfg_file_path() const { return m_router_cfg_file_path_; }
            // Get the file name of the technology parameters
            inline const String& get_elec_tech_file_path() const { return m_elec_tech_file_path_; }
            inline const String& get_phot_tech_file_path() const { return m_phot_tech_file_path_; }

            // Add global tech overwrite
            void add_global_tech_overwrite(const String& var_, const String& val_);

            // Run DSENT with some specified arguments, returning outputs
            std::vector<String> run_dsent(const String& cfg_file_path_, const std::vector<String>& evals_,
                const std::vector<Overwrite>& overwrites_, const std::vector<Overwrite>& overwrites_tech_) const;

            // Get BerkeleyDB database
            DB* getDatabase() const    { return m_database; }

        private:
            // Config file paths
            String m_el_link_cfg_file_path_;
            String m_op_link_cfg_file_path_;
            String m_router_cfg_file_path_;
            // Tech file paths
            String m_elec_tech_file_path_;
            String m_phot_tech_file_path_;
            // Global tech overwrites
            std::vector<Overwrite>* m_overwrites_tech_;
            // BerkeleyDB Database
            DB* m_database;

        private:        
            // Singleton
            static DSENTInterface* m_singleton;
            // Constants
            const static String el_link_cfg_file_name;
            const static String op_link_cfg_file_name;
            const static String router_cfg_file_name;
            
    };
}
#endif

