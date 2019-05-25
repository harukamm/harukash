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

bool is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

CommandData::redirect_pair parse_and_open_redirect(const string& s, map<string, int>* m) {
  assert(m != nullptr);
  // Currently only support `1>some-file`
  smatch results;
  assert(regex_match(s, results, regex("1?>(\\w+)")));
  const string& fname = results[1];
  if (m->find(fname) == m->end()) {
    m->insert(make_pair(fname, Util::sysopen(fname)));
  }
  return (CommandData::redirect_pair){.from=1, .to=m->at(fname)};
}

void parse(const string& s, CommandData* obj) {
  char sep = ' ';
  stringstream ss(s);
  string item;
  while (getline(ss, item, sep)) {
    if (!item.empty()) {
      if (is_redirect_token(item)) {
        obj->redirect.push_back(parse_and_open_redirect(item, &obj->fdmap));
      } else {
        obj->token.push_back(item);
      }
    }
  }
}

void separate_command(const CommandData& src, CommandData* dest1,
    CommandData* dest2) {
  assert(dest1 != nullptr);
  assert(dest2 != nullptr);

  auto it = find(src.token.begin(), src.token.end(), "|");
  copy(src.token.begin(), it, back_inserter(dest1->token));
  copy(it + 1, src.token.end(), back_inserter(dest2->token));

  copy(src.redirect.begin(), src.redirect.end(), back_inserter(dest1->redirect));
  copy(src.redirect.begin(), src.redirect.end(), back_inserter(dest2->redirect));

  dest1->fdmap.insert(src.fdmap.begin(), src.fdmap.end());
  dest2->fdmap.insert(src.fdmap.begin(), src.fdmap.end());
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
  separate_command(obj, &obj1, &obj2);

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
    parse(s, &obj);

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
