#include <NTL/ZZ_pX.h>
#include "elltorsion.h"
#include "schoof.h"
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include <fstream>

NTL_CLIENT

// Convert a string to a polynomial.
// The strings contains the coeffs in the magma format:
//   [ a0, a1, ... , an ]
ZZ_pX PolyMagmaToNTL(char *str) {
  size_t n = strlen(str);
  char *str2 = (char *)malloc(sizeof(char)*(n+1));

  char *ptr1, *ptr2;
  ptr1 = str;
  ptr2 = str2;

  for (unsigned int i = 0; i < n; ++i) {
    if (*ptr1 != ',')
      *ptr2++ = *ptr1;
    ++ptr1;
  }
  *ptr2 = '\0';

  string tmp(str2); istringstream myin(tmp);
  ZZ_pX f;
  myin >> f;
  free(str2);
  return f;
}


// read 
//   p
//   f
//   ell
// on stdin.

#define MAX_STR_LENGTH 1000000    /* should be enough for l=23 */

int main(int argc, char** argv) {
  char str[MAX_STR_LENGTH];
  bool output_to_file = false;
  char* filename = NULL;
  
  if (argc >= 3) {
    if (!strcmp(argv[1], "-o")) {
      output_to_file = true;
      filename = argv[2];
    }
  }
  
  cin.getline(str, MAX_STR_LENGTH);
  ZZ p = to_ZZ(str);
  ZZ_p::init(p);
	
  ZZ_pX f;
  cin.getline(str, MAX_STR_LENGTH);
  f = PolyMagmaToNTL(str);
      
  int ell;
  cin.getline(str, MAX_STR_LENGTH);
  ell = atoi(str);
	
  ZZ_pX Res, param, V12, V0overV1;
  EllTorsionIdeal(Res, param, V12, V0overV1, f, ell);

  vec_pair_long_long Candidates;
  Schoof(Candidates, Res, param, V12, V0overV1, f, ell);

  if (Candidates.length() == 1) {
    cout << "(s1, s2) mod ell = " << Candidates[0].a 
      << ", " << Candidates[0].b << endl;
    
    if (output_to_file) {
      ofstream file;
      file.open(filename, ofstream::trunc);
      file << ell << " " << Candidates[0].a << " " << Candidates[0].b << " " << endl;
      file.close();
    }
    
  } else {
    cout << "there are " << Candidates.length() << " candidates remaining\n";
  }
  return 0;
}

