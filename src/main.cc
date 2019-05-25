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
  map<string, int> fdmap;
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
    perror("open failed");
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

void close_wrap(int fd) {
  if (close(fd) == -1) {
    perror("close failed");
    exit(-1);
  }
}

void close_files(const map<string, int>& m) {
  for (const auto &pair: m) {
    close_wrap(pair.second);
  }
}

void dup_wrap(int to, int from) {
  // Call dup_wrap(2, 1) if 1>&2
  int fd = dup2(to, from);
  if (fd == -1) {
    perror("dup2 failed");
    exit(-1);
  }
}

void dupall(const vector<redirect_exp>& redirect) {
  for (const auto &r: redirect) {
    dup_wrap(r.to, r.from);
  }
}

void parse(const string& s, parsed_obj* obj) {
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

void separate_parsed_obj(const parsed_obj& src, parsed_obj* dest1,
    parsed_obj* dest2) {
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

char** c_str_arr(const vector<string>& arr) {
  char** args = new char*[arr.size() + 1];
  for (int i = 0; i < arr.size(); i++) {
    args[i] = (char*)arr[i].c_str();
  }
  args[arr.size()] = nullptr;
  return args;
}

void exec_command(const parsed_obj& obj) {
  const vector<string>& token = obj.token;
  const string& command = token[0];
  char** args = c_str_arr(token);
  dupall(obj.redirect);
  execvp(command.c_str(), args);
  perror("Exec failed");
  exit(-1);
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

void check_fork_failure(pid_t pid) {
  if (pid < 0) {
    perror("Fork failed");
    exit(-1);
  }
}

bool wait_pid(pid_t pid) {
  int status;
  pid_t r = waitpid(pid, &status, 0); // Wait for child process.
  if (r < 0) {
    perror("Waitpid failed");
    exit(-1);
  }
  if (WIFEXITED(status)) { // Child process ends successfully.
    return true;
  }
  cerr << "child status=" << status << endl;
  perror("child process failed");
  return false;
}

bool handle_command_with_pipe(const parsed_obj& obj) {
  const vector<string>& token = obj.token;
  int cnt = count(token.begin(), token.end(), "|");
  if (cnt == 0) {
    return false;
  }
  assert(cnt == 1);

  parsed_obj obj1, obj2;
  separate_parsed_obj(obj, &obj1, &obj2);

  int fds[2];
  int st = pipe(fds);
  if (st == -1) {
    perror("Pipe failed");
    exit(-1);
  }

  int pipe_r = fds[0];
  int pipe_w = fds[1];

  pid_t pid1 = fork();
  check_fork_failure(pid1);
  if (pid1 == 0) {
    dup_wrap(pipe_w, 1);
    close_wrap(pipe_r);
    close_wrap(pipe_w);
    exec_command(obj1);
  }
  pid_t pid2 = fork();
  check_fork_failure(pid2);
  if (pid2 == 0) {
    dup_wrap(pipe_r, 0);
    close_wrap(pipe_r);
    close_wrap(pipe_w);
    exec_command(obj2);
  }
  close_files(obj.fdmap);
  return wait_pid(pid1) && wait_pid(pid2);
}

bool handle_command(const parsed_obj& obj) {
  if (obj.token.size() == 0) {
    return false;
  }

  if (handle_command_with_pipe(obj)) {
    return true;
  }

  pid_t pid = fork();
  check_fork_failure(pid);
  if (pid == 0) { // For child process.
    exec_command(obj);
  }
  close_files(obj.fdmap);
  return wait_pid(pid);
}

int main() {
  while (!cin.eof()) {
    print_prompt();

    string s;
    getline(cin, s);
    parsed_obj obj;
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
