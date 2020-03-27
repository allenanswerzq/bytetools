#include <bits/stdc++.h>
#include "array.h"
#include "rnds.h"
#include "graph.h"
using namespace std;
#define x first
#define y second
#define mt make_tuple
#define all(x) (x).begin(), (x).end()
typedef long long ll;

template<class T> void puts(T t) { cout << t << "\n"; }
template<class T, class U, class... G>
void puts(T t, U u, G... g) { cout << t << " "; puts(u, g...); }

#define assertm(exp, msg) assert(((void)msg, exp))

//NOTE: Be aware jngen has its own ostream, need to fix this issue.

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

void gen() {
  // Write your own test generating code here.
}

int main(int argc, char** argv) {
  if (argc == 2) {
    // Let's say we might want to write out a file for comparison.
    xy = atoi(argv[1]);
  }
  gen();
  return 0;
}
