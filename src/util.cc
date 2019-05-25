#include "util.h"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

int Util::sysopen(const string& fname) {
  int fd = open(fname.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open failed");
    exit(-1);
  }
  return fd;
}

void Util::sysclose(int fd) {
  if (close(fd) == -1) {
    perror("close failed");
    exit(-1);
  }
}

void Util::sysdup(int to, int from) {
  // Call dup(2, 1) if 1>&2
  int fd = dup2(to, from);
  if (fd == -1) {
    perror("dup2 failed");
    exit(-1);
  }
}

void Util::sysexec(const vector<string>& token) {
  const string& command = token[0];
  char** args = c_str_arr(token);
  execvp(command.c_str(), args);
  perror("Exec failed");
  exit(-1);
}

void Util::syspipe(int* fd) {
  assert(fd != nullptr);
  if (pipe(fd) == -1) {
    perror("Pipe failed");
    exit(-1);
  }
}

pid_t Util::sysfork() {
  pid_t pid = fork();
  if (pid < 0) {
    perror("Fork failed");
    exit(-1);
  }
  return pid;
}

bool Util::syswaitpid(pid_t pid) {
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

char** Util::c_str_arr(const vector<string>& arr) {
  char** args = new char*[arr.size() + 1];
  for (int i = 0; i < arr.size(); i++) {
    args[i] = (char*)arr[i].c_str();
  }
  args[arr.size()] = nullptr;
  return args;
}

