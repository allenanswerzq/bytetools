#include <bits/stdc++.h>
#include "array.h"
#include "rnds.h"
#include "graph.h"
using namespace std;
#define x first
#define y second
typedef long long ll;

template<class T> void puts(T t) { cout << t << "\n"; }
template<class T, class U, class... G>
void puts(T t, U u, G... g) { cout << t << " "; puts(u, g...); }

#define assertm(exp, msg) assert(((void)msg, exp))

//NOTE: Be aware jngen has its own ostream, need to fix this issue.

int xy = -1;

void WriteFile(const string &name, const string& data) {
  ofstream ofs(FLAGS_elf + ".in");
  if (!ofs.is_open()) {
    LOG_ERRR("File cant open!");
    exit(1);
  }
  ofs << data << '\n';
  ofs.close();
}


void gen() {
  // Write your own test generating code here.
}

int main(int argc, char** argv) {
  assert(argc == 2);
  xy = atoi(argv[1]);
  gen();
  return 0;
}
