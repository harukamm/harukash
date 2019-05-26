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

bool handle_builtin(const CommandData& obj) {
  if (obj.token.size() == 0) {
    return false;
  }
  const string& first = obj.token.at(0);
  if (first == "echo") {
    assert(obj.redirect.size() == 0);
    builtin_echo(obj.token);
    return true;
  }
  return false;
}

bool handle_command_with_pipe(const CommandData& obj) {
  const vector<string>& token = obj.token;
  int cnt = count(token.begin(), token.end(), "|");
  if (cnt == 0) {
    return false;
  }
  assert(cnt == 1);
  // TODO: redirect 先をパイプごとに分けないと
  assert(obj.redirect.size() == 0);

  CommandData obj1, obj2;
  obj.separate_command(&obj1, &obj2);

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
  for (const auto& pair: obj.fdmap) {
    Util::sysclose(pair.second);
  }
  // Note: Close pipe otherwise sub process does not end due to blocking read
  // on pipe.
  Util::sysclose(pipe_r);
  Util::sysclose(pipe_w);
  return Util::syswaitpid(pid1) && Util::syswaitpid(pid2);
}

bool handle_command(const CommandData& obj) {
  if (obj.token.size() == 0) {
    return false;
  }

  if (handle_command_with_pipe(obj)) {
    return true;
  }

  pid_t pid = Util::sysfork();
  if (pid == 0) { // For child process.
    for (const auto& r: obj.redirect) {
      Util::sysdup(r.to, r.from);
    }
    Util::sysexec(obj.token);
  }
  for (const auto& pair: obj.fdmap) {
    Util::sysclose(pair.second);
  }
  return Util::syswaitpid(pid);
}

int main() {
  while (!cin.eof()) {
    print_prompt();

    string s;
    getline(cin, s);
    CommandData obj;
    CommandData::parse_from(s, &obj);

    if (obj.token.size() == 0) {
      continue;
    }
    if (handle_builtin(obj)) {
      continue;
    }
    if (handle_command(obj)) {
      continue;
    }

    cout << "Unknown: " << s << endl;
  }
}
