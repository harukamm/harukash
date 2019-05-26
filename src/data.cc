#include "util.h"
#include "data.h"

#include <assert.h>
#include <regex>
#include <sstream>

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
      obj->parse_and_store_redirect(item);
    } else {
      obj->token.push_back(item);
    }
  }
}

bool CommandData::is_redirect_token(const string& s) {
  // TODO: Fix it.
  return s.find('<') != string::npos || s.find('>') != string::npos;
}

void CommandData::parse_and_store_redirect(const string& s) {
  // Currently only support `1>some-file`
  smatch results;
  assert(regex_match(s, results, regex("1?>(\\w+)")));
  const string& fname = results[1];
  auto& m = this->fdmap;
  if (m.find(fname) == m.end()) {
    m.insert(make_pair(fname, Util::sysopen(fname)));
  }
  const auto redirect = (CommandData::redirect_pair){.from=1, .to=m.at(fname)};
  this->redirect.push_back(redirect);
}

void CommandData::separate_command(CommandData* dest1, CommandData* dest2) const {
  assert(dest1 != nullptr);
  assert(dest2 != nullptr);

  auto it = find(this->token.begin(), this->token.end(), "|");
  copy(this->token.begin(), it, back_inserter(dest1->token));
  copy(it + 1, this->token.end(), back_inserter(dest2->token));

  copy(this->redirect.begin(), this->redirect.end(), back_inserter(dest1->redirect));
  copy(this->redirect.begin(), this->redirect.end(), back_inserter(dest2->redirect));

  dest1->fdmap.insert(this->fdmap.begin(), this->fdmap.end());
  dest2->fdmap.insert(this->fdmap.begin(), this->fdmap.end());
}
