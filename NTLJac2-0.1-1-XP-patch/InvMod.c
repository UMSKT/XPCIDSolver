#include "NTL/ZZ.h"
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

NTL_CLIENT

string ZZ_to_hex_string(ZZ number) {
  stringstream ss;
  string str;
  
  int size = NumBytes(number);
  vector<unsigned char> vector(size);

  BytesFromZZ(vector.data(), number, size);
  reverse(vector.begin(), vector.end());

  for (auto& element : vector) {
    ss << setfill('0') << setw(2) << hex << (int)element << dec;
  }
  
  str = ss.str();
  
  return str;
}

int main(int argc, char **argv) {
  ZZ a, n, res;
  
  if (argc < 3) return -1;
  
  a = to_ZZ(argv[1]);
  n = to_ZZ(argv[2]);
  
  res = InvMod(a, n);
  
  cout << "0x" << ZZ_to_hex_string(res) << endl;
  return 0;
}
