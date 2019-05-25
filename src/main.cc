#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

struct parsed_obj {
  vector<string> token;
  vector<string> redirect;
};

void print_prompt() {
  cout << "^_^ > ";
}

bool is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

parsed_obj parse(const string& s) {
  char sep = ' ';
  parsed_obj* obj = new parsed_obj();
  stringstream ss(s);
  string item;
  while (getline(ss, item, sep)) {
    if (!item.empty()) {
      if (is_redirect_token(item)) {
        obj->redirect.push_back(item);
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
  assert(obj.redirect.size() == 0);
  const string& first = obj.token.at(0);
  if (first == "echo") {
    builtin_echo(obj.token);
    return true;
  }
  return false;
}

bool handle_command(const parsed_obj& obj) {
  if (obj.token.size() == 0) {
    return false;
  }

  assert(obj.redirect.size() == 0);
  pid_t pid = fork();
  if (pid < 0) { // When `fork` failed.
    perror("Fork failed.");
    exit(-1);
  } else if (pid == 0) { // For child process.
    const string& command = obj.token[0];
    char** args = c_str_arr(obj.token);
    execvp(command.c_str(), args);
    // cerr << strerror(errno);
    perror("Exec failed.");
    exit(-1);
  }

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
