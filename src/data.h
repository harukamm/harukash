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
};
