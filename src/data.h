#pragma once

#include <map>
#include <vector>
#include <string>

using namespace std;

class CommandData {
  public:
    ~CommandData();

    typedef map<string, int> fdmap_t;
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
    static void parse_from(const string& s, CommandData* obj);

    void separate_command(CommandData* dest1, CommandData* dest2) const;

    void open_redirect_files();

  private:
    fdmap_t fdmap;

    void maybe_open_file(CommandData::file* file);

    redirect_pair parse_redirect(const string& s);

    static bool is_redirect_token(const string& s);
};
