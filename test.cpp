#include <vector>
#include <string>
#include <iostream>
using namespace std;

class Solution{
  public:
    bool wordBreak(string s, vector<string> &wordDict) {
      for (unsigned long i = 0; i < wordDict.size(); i++) {
        string tmpStr = s;
        vector<string>::iterator  dictBeginIt = wordDict.begin();
        while (dictBeginIt != wordDict.end()) {
          string::iterator dictSingle = (*dictBeginIt).begin();


          dictBeginIt++;
        }
      }
    }
};

int main() {
  Solution Sol;
  string s = "leetCode";
  vector<string> vec;
  vec.push_back("leet");
  vec.push_back("Code");
  bool ret = Sol.wordBreak(s, vec);
  cout << ret << endl;
  
  return 0;
}
