
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int main(int argc, char **argv)
{
    int i = open("./input", O_RDONLY);
    cout << "fid: " << i << endl;
}
