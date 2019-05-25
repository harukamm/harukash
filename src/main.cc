#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

void print_prompt() {
  cout << "^_^ > ";
}

vector<string> split(const string& s, char sep) {
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, sep)) {
    if (!item.empty()) {
      elems.push_back(item);
    }
  }
  return elems;
}

vector<string> parse(string& s) {
  return split(s, ' ');
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

bool handle_builtin(const vector<string>& tokens) {
  if (tokens.size() == 0) {
    return false;
  }
  const string& first = tokens.at(0);
  if (first == "echo") {
    builtin_echo(tokens);
    return true;
  }
  return false;
}

bool handle_command(const vector<string>& tokens) {
  if (tokens.size() == 0) {
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) { // When `fork` failed.
    perror("Fork failed.");
    exit(-1);
  } else if (pid == 0) { // For child process.
    const string& command = tokens[0];
    char** args = c_str_arr(tokens);
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
    const vector<string>& tokens = parse(s);

    if (tokens.size() == 0) {
      continue;
    }
    if (handle_builtin(tokens)) {
      continue;
    }
    if (handle_command(tokens)) {
      continue;
    }

    cout << "Unknown: " << s << endl;
  }
}
