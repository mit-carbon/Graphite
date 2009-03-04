
#include <iostream>
#include "config_file.hpp"

using namespace config;

int main(int argc, char **argv)
{
    ConfigFile cfg;
    cfg.Load("test.cfg");

    cout << cfg.ShowFullTree() << endl;
}

