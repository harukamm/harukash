#include "data.h"
#include "util.h"

#include <assert.h>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

template<typename T>
ostream& operator<< (ostream& out, const vector<T>& v) {
  out << "{";
  size_t last = v.size() - 1;
  for(size_t i = 0; i < v.size(); ++i) {
    out << v[i];
    if (i != last) {
      out << ", ";
    }
  }
  out << "}";
  return out;
}

void print_prompt() {
  cout << "^_^ > ";
}

void builtin_echo(const vector<string>& tokens) {
  for (int i = 1; i < tokens.size(); i++) {
    cout << tokens[i];
    if (i + 1 != tokens.size()) {
      cout << " ";
    }
  }
  cout << endl;
}

bool handle_builtin(const CommandList& obj) {
  if (obj.units.size() != 1) {
    return false;
  }
  const CommandUnit& unit = obj.units.at(0);
  if (unit.token.size() == 0) {
    return false;
  }
  const string& first = unit.token.at(0);
  if (first == "echo") {
    assert(unit.redirect.size() == 0);
    builtin_echo(unit.token);
    return true;
  }
  return false;
}

bool handle_command_with_pipe(const CommandList& obj) {
  if (obj.units.size() < 2) {
    return false;
  }
  assert(obj.units.size() == 2);

  const auto& obj1 = obj.units.at(0);
  const auto& obj2 = obj.units.at(1);

  int fds[2];
  Util::syspipe(fds);
  int pipe_r = fds[0];
  int pipe_w = fds[1];

  pid_t pid1 = Util::sysfork();
  if (pid1 == 0) {
    Util::sysdup(pipe_w, 1);
    Util::sysclose(pipe_r);
    Util::sysclose(pipe_w);
    Util::sysexec(obj1.token);
  }
  pid_t pid2 = Util::sysfork();
  if (pid2 == 0) {
    Util::sysdup(pipe_r, 0);
    Util::sysclose(pipe_r);
    Util::sysclose(pipe_w);
    Util::sysexec(obj2.token);
  }
  // Note: Close pipe otherwise sub process does not end due to blocking read
  // on pipe.
  Util::sysclose(pipe_r);
  Util::sysclose(pipe_w);
  return Util::syswaitpid(pid1) && Util::syswaitpid(pid2);
}

bool handle_command(const CommandList& obj) {
  if (obj.units.size() == 0) {
    return false;
  }
  if (handle_command_with_pipe(obj)) {
    return true;
  }

  const auto& unit = obj.units.at(0);
  if (unit.token.size() == 0) {
    return false;
  }

  pid_t pid = Util::sysfork();
  if (pid == 0) { // For child process.
    for (const auto& r: unit.redirect) {
      Util::sysdup(r.to.fd, r.from.fd);
    }
    Util::sysexec(unit.token);
  }
  return Util::syswaitpid(pid);
}

int main() {
  while (!cin.eof()) {
    print_prompt();

    string s;
    getline(cin, s);
    CommandList obj;
    CommandList::parse_from(s, &obj);

    if (obj.units.size() == 0) {
      continue;
    }
    obj.open_redirect_files();
    if (handle_builtin(obj)) {
      continue;
    }
    if (handle_command(obj)) {
      continue;
    }

    cout << "Unknown: " << s << endl;
  }
}
