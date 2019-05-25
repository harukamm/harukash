#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
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

    cout << s << endl;
  }
}
