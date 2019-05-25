#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

struct redirect_exp {
  int from;
  int to;
};

struct parsed_obj {
  vector<string> token;
  vector<redirect_exp> redirect;
  map<string, int>* fdmap;
};

void print_prompt() {
  cout << "^_^ > ";
}

bool is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

int open_wrap(const string& fname) {
  int fd = open(fname.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open failed.");
    exit(-1);
  }
  return fd;
}

redirect_exp parse_and_open_redirect(const string& s, map<string, int>* m) {
  assert(m != nullptr);
  // Currently only support `1>some-file`
  smatch results;
  assert(regex_match(s, results, regex("1?>(\\w+)")));
  const string& fname = results[1];
  if (m->find(fname) == m->end()) {
    m->insert(make_pair(fname, open_wrap(fname)));
  }
  return (redirect_exp){.from=1, .to=m->at(fname)};
}

void close_files(const map<string, int>& m) {
  for (const auto &pair: m) {
    int st = close(pair.second);
    if (st == -1) {
      perror("close failed.");
      exit(-1);
    }
  }
}

void dupall(const vector<redirect_exp>& redirect) {
  for (const auto &r: redirect) {
    int fd = dup2(r.to, r.from);
    if (fd == -1) {
      perror("dup2 failed.");
      exit(-1);
    }
  }
}

parsed_obj parse(const string& s) {
  char sep = ' ';
  parsed_obj* obj = new parsed_obj();
  map<string, int>* fdmap = new map<string, int>();
  obj->fdmap = fdmap;
  stringstream ss(s);
  string item;
  while (getline(ss, item, sep)) {
    if (!item.empty()) {
      if (is_redirect_token(item)) {
        obj->redirect.push_back(parse_and_open_redirect(item, fdmap));
      } else {
        obj->token.push_back(item);
      }
    }
  }
  return *obj;
}

char** c_str_arr(const vector<string>& arr) {
  char** args = new char*[arr.size() + 1];
  for (int i = 0; i < arr.size(); i++) {
    args[i] = (char*)arr[i].c_str();
  }
  args[arr.size()] = nullptr;
  return args;
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

bool handle_builtin(const parsed_obj& obj) {
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

bool handle_command(const parsed_obj& obj) {
  if (obj.token.size() == 0) {
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) { // When `fork` failed.
    perror("Fork failed.");
    exit(-1);
  } else if (pid == 0) { // For child process.
    const string& command = obj.token[0];
    char** args = c_str_arr(obj.token);
    dupall(obj.redirect);
    execvp(command.c_str(), args);
    // cerr << strerror(errno);
    perror("Exec failed.");
    exit(-1);
  }

  close_files(*obj.fdmap);
  int status;
  pid_t r = waitpid(pid, &status, 0); // Wait for child process.
  if (r < 0) {
    perror("Waitpid failed.");
    exit(-1);
  }
  if (WIFEXITED(status)) { // Child process ends successfully.
    return true;
  }
  cerr << "child status=" << status << endl;
  perror("child process failed.");
  return false;
}

int main() {
  while (!cin.eof()) {
    print_prompt();

    string s;
    getline(cin, s);
    const parsed_obj& obj = parse(s);

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
