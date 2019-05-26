#pragma once

#include <vector>
#include <string>

// <command> := <command-unit> <pipe> <command>
// <command-unit> := [<redirect_pair>|<token>]+
// <pipe> := |
// <redirect-pair> := \w+<\w+
// <token> := [^ ]+ and not matched with <pipe> nor <redirect-pair>

using namespace std;

class CommandUnit {
  public:
    struct file {
      int fd;
      string fname;
      file(string fname) : fd(-1), fname(fname) {};
      file(int fd): fd(fd) {};
    };
    struct redirect_pair {
      file from;
      file to;
    };
    vector<string> token;
    vector<redirect_pair> redirect;

    // TODO: Move functions related parsing to another file.
    static void parse_from(const string& s, CommandUnit* obj);

  private:
    redirect_pair parse_redirect(const string& s);

    static bool is_redirect_token(const string& s);
};

class CommandList {
  public:
    vector<CommandUnit> units;

    static void parse_from(const string& s, CommandList* obj);

    void open_redirect_files();

    void add_to_fdlst(int fds, ...);

    void close_all();

  private:
    vector<int> fdlst;

    void maybe_open_file(CommandUnit::file* file);
};
