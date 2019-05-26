#pragma once

#include "data.h"
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;

class Util {
  public:
    static vector<string> split(const string& s, char sep);

    static int sysopen(const string& fname);

    static void sysclose(int fd);

    static void sysdup(int to, int from);

    static void sysexec(const vector<string>& token);

    static void syspipe(int* fd);

    static pid_t sysfork();

    static bool syswaitpid(pid_t pid);

    static char** c_str_arr(const vector<string>& arr);
};
