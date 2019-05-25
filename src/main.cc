#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;

void print_prompt() {
  cout << "^_^ > ";
}

int main() {
  while (!cin.eof()) {
    print_prompt();

    string s;
    getline(cin, s);
    cout << s << endl;
  }
}
