
#include "thread_test.h"
#include <iostream>
using namespace std;

void TestThread::RunThread(OS_SERVICES::ITHREAD *me)
{
    int i = 5;
    while(i--)
    {
        cout << "TestThread::RunThread" << endl;
        sleep(1);
    }
}

