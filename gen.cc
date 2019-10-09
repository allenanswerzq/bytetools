#include "array.h"
#include "rnds.h"
#include "graph.h"
using namespace std;
typedef long long ll;

template<class T> void puts(T t) { cout << t << "\n"; }
template<class T, class U, class... G>
void puts(T t, U u, G... g) { cout << t << " "; puts(u, g...); }

template<class T> void trace(T t) { cerr << t << "\n"; }
template<class T, class U, class... G>
void trace(T t, U u, G... g) { cerr << t << " "; puts(u, g...); }

// The number of test cases.
const int T = 1;

void gen() {
  // Write your own test generating code here.
}

int main() {
  for (int t = 0; t < T; ++t) {
    if (t > 0) {
      cout << '\n';
    }
    gen();
  }
  return 0;
}
