#include <map>
#include <vector>

using namespace std;

class CommandData {
  public:
    struct redirect_pair {
      int from;
      int to;
    };
    vector<string> token;
    vector<redirect_pair> redirect;
    map<string, int> fdmap;

    static void parse_from(const string& s, CommandData* obj);

    void separate_command(CommandData* dest1, CommandData* dest2) const;

  private:
    void parse_and_store_redirect(const string& s);

    static bool is_redirect_token(const string& s);
};
