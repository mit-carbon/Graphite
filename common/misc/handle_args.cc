#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include "handle_args.h"

static char*prog_name;

using namespace boost::algorithm;


void handle_generic_arg(const std::string &str, config::ConfigFile & cfg);
void handle_args(const string_vec & args);

void usage_error(const char*error_msg, ...)
{
   fprintf(stderr, "Error: ");
   va_list args;
   va_start(args, error_msg);
   vfprintf(stderr,  error_msg, args);
   va_end(args);
   fprintf(stderr, "\n");

   fprintf(stderr, "Usage: %s -c config [extra_options]\n", prog_name);
   exit(-1);
}

void parse_args(string_vec &args, std::string & config_path, int argc, char **argv)
{
   prog_name = argv[0];

   for(int i = 1; i < argc; i++)
   {
       if(strcmp(argv[i], "-c") == 0)
       {
           if(i + 1 >= argc)
               usage_error("Should have provided another argument to the -c parameter.\n");
           config_path = argv[i+1];
           i++;
       }
       else if(strncmp(argv[i],"--config", strlen("--config")) == 0)
       {
           string_vec split_args;
           std::string config_arg(argv[i]);
           boost::split( split_args, config_arg, boost::algorithm::is_any_of("=") );

           if(split_args.size() != 2)
               usage_error("Error parsing argument: %s (%s)\n", config_arg.c_str(), argv[i]);

           config_path = split_args[1];
       }
       else if(strcmp(argv[i], "--") == 0)
           return;
       else if(strncmp(argv[i], "--", strlen("--")) == 0)
       {
           args.push_back(argv[i]);
       }
   }

   if(config_path == "")
       usage_error("Should have specified config argument.\n");
   else
       fprintf(stderr, "Config path set to: %s\n", config_path.c_str());

}


void handle_args(const string_vec & args, config::ConfigFile & cfg)
{
    for(string_vec::const_iterator i = args.begin();
            i != args.end();
            i++)
    {
        handle_generic_arg(*i, cfg);
    }
}

void handle_generic_arg(const std::string &str, config::ConfigFile & cfg)
{
   string_vec split_args;

   boost::split( split_args, str, boost::algorithm::is_any_of("=") );

   if(split_args.size() != 2 || split_args[0].size() <= 2 || 
           split_args[0].c_str()[0] != '-' || split_args[0].c_str()[1] != '-')
       usage_error("Error parsing argument: %s\n", str.c_str());

   std::string setting(split_args[0].substr(2));
   std::string & value(split_args[1]);

   cfg.set(setting, value);
}

