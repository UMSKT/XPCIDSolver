#include "NTL/ZZ.h"
#include <unistd.h>
#include <cassert>
#include <vector>
#include <sstream>
#include <cstring>

NTL_CLIENT

// read 
//   ell list
//   s1p list
//   s2p list
// on stdin.

#define MAX_STR_LENGTH 1000000    /* should be enough for l=23 */

int main(int argc, char **argv) {
  vector<unsigned long> primesList, s1pList, s2pList;
  char str[MAX_STR_LENGTH];
  bool quiet = false;

  if (argc >= 2) {
    if (!strcmp(argv[1], "-q")) {
      quiet = true;
    }
  }

  //unsigned long primesList[] =   {5, 11, 13, 17, 23}; // m (ell)
  //unsigned long s1pList[] =      {4,  1,  5, 16,  8}; // s1p
  //unsigned long s2pList[] =      {0,  2, 10, 16,  7}; // s2p

  //size_t arraySize = sizeof(primesList) / sizeof(primesList[0]);



  cin.getline(str, MAX_STR_LENGTH);

  stringstream pListStream(str);
  string pNum;

  while(getline(pListStream, pNum, ',')) {
    primesList.push_back(stol(pNum));
  }


  cin.getline(str, MAX_STR_LENGTH);

  stringstream s1pListStream(str);
  string s1pNum;

  while(getline(s1pListStream, s1pNum, ',')) {
    s1pList.push_back(stol(s1pNum));
  }


  cin.getline(str, MAX_STR_LENGTH);

  stringstream s2pListStream(str);
  string s2pNum;

  while(getline(s2pListStream, s2pNum, ',')) {
    s2pList.push_back(stol(s2pNum));
  }


  size_t arraySize = primesList.size();



  ZZ a1, p1, A1, P1;
  ZZ a2, p2, A2, P2;

  a1 = to_ZZ(s1pList[0]); p1 = to_ZZ(primesList[0]);
  a2 = to_ZZ(s2pList[0]); p2 = to_ZZ(primesList[0]);

  for (unsigned long i = 1; i < (unsigned long)arraySize; i++) {
    A1 = to_ZZ(s1pList[i]); P1 = to_ZZ(primesList[i]);
    CRT(a1, p1, A1, P1);

    A2 = to_ZZ(s2pList[i]); P2 = to_ZZ(primesList[i]);
    CRT(a2, p2, A2, P2);
  }

  a1 = a1 % p1;
  a2 = a2 % p2;

  assert(p1 == p2);

  if (quiet)
    cout << a1 << " " << a2 << " " << p1 << endl;
  else
    cout << a1 << ", " << a2 << " mod " << p1 << endl;

  return 0;
}
