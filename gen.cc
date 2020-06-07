#include <bits/stdc++.h>
#include "array.h"
using namespace std;
#define x first
#define y second
#define mt make_tuple
#define all(x) (x).begin(), (x).end()
typedef long long ll;

int xy = -1;

void WriteFile(const string& data) {
  if (xy == -1) return;
  ofstream ofs(to_string(xy) + ".gb");
  if (!ofs.is_open()) {
    assertm(0, "File cant open!");
    exit(1);
  }
  ofs << data << '\n';
  ofs.close();
}

// Write your own test generating code here.
void Generate() {

}

int main(int argc, char** argv) {
  if (argc == 2) {
    // Used to write out a file for comparison.
    xy = atoi(argv[1]);
  }
  Generate();
  return 0;
}
