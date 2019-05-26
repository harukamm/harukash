#include "util.h"
#include "data.h"

#include <assert.h>
#include <regex>

void CommandList::parse_from(const string& s, CommandList* obj) {
  assert(obj != nullptr);
  for (const auto& item: Util::split(s, '|')) {
    auto unit = CommandUnit();
    CommandUnit::parse_from(item, &unit);
    obj->units.push_back(unit);
  }
}

void CommandUnit::parse_from(const string& s, CommandUnit* obj) {
  assert(obj != nullptr);
  const auto& items = Util::split(s, ' ');
  assert(items.size() != 0);
  for (const auto& item: items) {
    if (is_redirect_token(item)) {
      const auto& r = obj->parse_redirect(item);
      obj->redirect.push_back(r);
    } else {
      obj->token.push_back(item);
    }
  }
}

bool CommandUnit::is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

CommandUnit::redirect_pair CommandUnit::parse_redirect(const string& s) {
  // Currently only support `1>some-file`
  smatch results;
  assert(regex_match(s, results, regex("1?>(\\w+)")));
  const auto& from = file(1);
  const auto& to = file(results[1]);
  return (CommandUnit::redirect_pair){.from=from, .to=to};
}

void CommandList::maybe_open_file(CommandUnit::file* file) {
  assert(file != nullptr);
  int fd;
  const string& fname = file->fname;
  if (0 <= file->fd) {
    fd = file->fd;
  } else {
    assert(fname != "");
    fd = Util::sysopen(file->fname);
    this->add_to_fdlst(1, fd);
  }
  file->fd = fd;
}

void CommandList::open_redirect_files() {
  for (auto& unit: this->units) {
    for (auto& r: unit.redirect) {
      maybe_open_file(&r.to);
      maybe_open_file(&r.from);
    }
  }
}

void CommandList::add_to_fdlst(int size, ...) {
  va_list args;
  va_start(args, size);
  for(int i = 0; i < size; i++) {
    int fd = va_arg(args, int);
    fdlst.push_back(fd);
  }
  va_end(args);
}

void CommandList::close_all() {
  for (const auto& fd: fdlst) {
    Util::sysclose(fd);
  }
}
