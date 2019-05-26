#include "util.h"
#include "data.h"

#include <assert.h>
#include <regex>
#include <sstream>

CommandData::~CommandData() {
  for (const auto& pair: fdmap) {
    Util::sysclose(pair.second);
  }
}

void CommandData::parse_from(const string& s, CommandData* obj) {
  assert(obj != nullptr);
  char sep = ' ';
  stringstream ss(s);
  string item;
  while (getline(ss, item, sep)) {
    if (item.empty()) {
      continue;
    }
    if (is_redirect_token(item)) {
      const auto& r = obj->parse_redirect(item);
      obj->redirect.push_back(r);
    } else {
      obj->token.push_back(item);
    }
  }
}

bool CommandData::is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

CommandData::redirect_pair CommandData::parse_redirect(const string& s) {
  // Currently only support `1>some-file`
  smatch results;
  assert(regex_match(s, results, regex("1?>(\\w+)")));
  const auto& from = file(1);
  const auto& to = file(results[1]);
  return (CommandData::redirect_pair){.from=from, .to=to};
}

void CommandData::separate_command(CommandData* dest1, CommandData* dest2) const {
  assert(dest1 != nullptr);
  assert(dest2 != nullptr);

  auto it = find(this->token.begin(), this->token.end(), "|");
  copy(this->token.begin(), it, back_inserter(dest1->token));
  copy(it + 1, this->token.end(), back_inserter(dest2->token));

  copy(this->redirect.begin(), this->redirect.end(), back_inserter(dest1->redirect));
  copy(this->redirect.begin(), this->redirect.end(), back_inserter(dest2->redirect));
}

void CommandData::maybe_open_file(CommandData::file* file) {
  assert(file != nullptr);
  int fd;
  const string& fname = file->fname;
  auto& m = this->fdmap;
  if (0 <= file->fd) {
    fd = file->fd;
  } else if (m.find(fname) != m.end()) {
    fd = m.at(fname);
  } else {
    assert(fname != "");
    fd = Util::sysopen(file->fname);
    m.insert(make_pair(fname, fd));
  }
  file->fd = fd;
}

void CommandData::open_redirect_files() {
  auto m = CommandData::fdmap_t();
  for (auto& r: this->redirect) {
    maybe_open_file(&r.to);
    maybe_open_file(&r.from);
  }
}
