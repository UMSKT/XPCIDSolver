////////////////////////////////////////////////////////////
////  Algorithm by Matsuo Chao Tsujii  (Ants5)
////     (Low memory, parallelizable version)
//////////////////////////////////////////////////////////////

#include "NTL/ZZ_p.h"
#include "NTL/ZZ_pX.h"
#include "ZZ_pJac2.h"
#include <assert.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <fstream>


NTL_CLIENT


/***************************************************************************\
 *  Typedef for a divisor with alpha and beta
\***************************************************************************/

class DivAndTrack {
  public:
    ZZ_pJac2 D;
    ZZ alpha;
    ZZ beta;
    
    DivAndTrack() { }
    DivAndTrack(const DivAndTrack& dd) {
      D = dd.D;  alpha = dd.alpha;  beta = dd.beta;
    }
    
    ~DivAndTrack() { }
    
    inline DivAndTrack& operator=(const DivAndTrack& dd) {
      this->D = dd.D;
      this->alpha = dd.alpha;
      this->beta = dd.beta;
      return *this;
    }
    
};


NTL_vector_decl(DivAndTrack, vec_DivAndTrack);
NTL_vector_impl(DivAndTrack, vec_DivAndTrack);



/***************************************************************************\
 *  Typedef for the random walk data
\***************************************************************************/

typedef struct {
  double l1;
  double l2;
  int r;
  DivAndTrack **O;
  ZZ K;
  ZZ_pJac2 KBaseDivisor;
  ZZ_pJac2 mBaseDivisor;
  ZZ_pJac2 pp1mBaseDivisor;
  ZZ_pJac2 BaseDivisor;
  int pD;
  ZZ B1min, B1max;
  ZZ B2min, B2max;
} RWData;

inline int indexFromKKpBR(int k, int kp, int b, int r) {
  return (k + r*kp + r*r*b);
}

/***************************************************************************\
 *   Hash function.
\***************************************************************************/
unsigned int hashvalue(const ZZ_pJac2& D) {
  ZZ h = rep(D.u1) ^ rep(D.v0);
  ZZ hh = rep(D.u0) ^ rep(D.v1);
  h ^= hh << NumBits(h);
  unsigned int x;
  unsigned char *str = (unsigned char *) (&x);
  BytesFromZZ(str, h, 4);
  return x;
}

unsigned int hashvalue2(const ZZ_pJac2& D) {
  ZZ h = rep(D.u0) ^ rep(D.u1);
  ZZ hh = rep(D.v0) ^ rep(D.v1);
  h ^= hh << NumBits(h);
  unsigned int x;
  unsigned char *str = (unsigned char *) (&x);
  BytesFromZZ(str, h, 4);
  return x;
}

/***************************************************************************\
 *   Distinguished property
\***************************************************************************/

// proba gives the number of bits that have to be 0 for being
// distinguished
inline int IsDistinguished(const ZZ_pJac2& D, const int proba) {
  return (hashvalue2(D) & (((1UL)<<proba) - 1)) == 0;
}
  
/***************************************************************************\
 *   Random walk functions
\***************************************************************************/

// note: if l1 is too small, then k = 0 gives alpha_k = 0
void InitializeRandomWalk(RWData &rwdata, int r) {
  rwdata.r = r;
  rwdata.O = (DivAndTrack **)malloc(2*r*r*sizeof(DivAndTrack *));
  ZZ ak, bkp;
  ZZ_pJac2 akmP, bkpmP;
  for (int k = 0; k < r; ++k) {
    // select a random alpha_k of average value l1
    // If l1 is too small, then alpha_k is 0, 1, or 2.
    if (rwdata.l1 <= 2) {
      if (k == 0) 
	ak = 0;
      else
        // if (RandomWord() & 1UL)
        if (1)
	  ak = 1;
	else
	  ak = 2;
    } else {
      ak = RandomBnd(to_ZZ(2*rwdata.l1));
    }
    // compute alpha_k*(p+1)*m*P
    akmP = ak*rwdata.pp1mBaseDivisor;
    for (int kp = 0; kp < r; kp++) {
      // select a random betakp of average value l2
      bkp = RandomBnd(to_ZZ(2*rwdata.l2))+1;
      bkpmP = bkp*rwdata.mBaseDivisor;
 
      // Write the corresponding value in the rwdata.O table
      rwdata.O[indexFromKKpBR(k, kp, 0, r)] = new DivAndTrack;
      rwdata.O[indexFromKKpBR(k, kp, 0, r)]->D = akmP + bkpmP;
      rwdata.O[indexFromKKpBR(k, kp, 0, r)]->alpha = -ak;
      rwdata.O[indexFromKKpBR(k, kp, 0, r)]->beta = bkp;
      
      rwdata.O[indexFromKKpBR(k, kp, 1, r)] = new DivAndTrack;
      rwdata.O[indexFromKKpBR(k, kp, 1, r)]->D = bkpmP - akmP;
      rwdata.O[indexFromKKpBR(k, kp, 1, r)]->alpha = ak;
      rwdata.O[indexFromKKpBR(k, kp, 1, r)]->beta = bkp;
    }
  }
}

void JumpOneStep(DivAndTrack& Dp, const DivAndTrack& D, const RWData &rwdata) {
  unsigned int hash = hashvalue(D.D);
  int r = rwdata.r;
  int k, kp, b;

  unsigned int il1 = 0;
  if (rwdata.l1 <= 2) {
    il1 = (unsigned int)(1/rwdata.l1);
  }
  
  // Select k, kp, b according to hash.
  //      hash >>= (rwdata.pD+1);  this is now useless
  b = hash & 1; hash >>= 1;
  kp = hash % r; hash >>= 4;
  if (rwdata.l1 <= 2) {
    if ((hash % il1) == 0) {
      hash >>= 3;
      k = (hash % (rwdata.r - 1)) + 1;
    } else
      k = 0;
  } else {
    k = hash % r;
  }
//  cout << k << " " << kp << " " << b<< " "  << r << endl;
//  cout << indexFromKKpBR(k, kp, b, r) << endl;
  Dp.D = D.D + rwdata.O[indexFromKKpBR(k, kp, b, r)]->D;
  Dp.alpha = D.alpha + rwdata.O[indexFromKKpBR(k, kp, b, r)]->alpha;
  Dp.beta = D.beta + rwdata.O[indexFromKKpBR(k, kp, b, r)]->beta;
}

/***************************************************************************\
 *   One thread
\***************************************************************************/

// Starting point of a new thread.  Bit b indicates Wild or Tame
// kangaroo.
void SelectRandomStartingPoint(DivAndTrack& D, const int b,
    const RWData &rwdata) {
  D.alpha = rwdata.B1min + RandomBnd(rwdata.B1max - rwdata.B1min);
  D.beta = rwdata.B2min + RandomBnd(rwdata.B2max - rwdata.B2min);
  D.D = (-D.alpha)*rwdata.pp1mBaseDivisor + D.beta*rwdata.mBaseDivisor;

  if (b)
    D.D += rwdata.KBaseDivisor;
}

void RunThread(DivAndTrack& distD, int b, const RWData &rwdata, ZZ& nb_jump) {
  DivAndTrack D;
  ZZ no_cycle_bound = to_ZZ((1UL)<<rwdata.pD)*10;
  ZZ cpt;
  
  do {  
    SelectRandomStartingPoint(D, b, rwdata);
    cpt = 0;
    while (!IsDistinguished(D.D, rwdata.pD) && (cpt < no_cycle_bound)) {
      JumpOneStep(D, D, rwdata);
      cpt ++;
    }
  } while (cpt == no_cycle_bound);
  //  cerr << "Distinguished point hit after " << cpt << " jumps.\n";
  distD = D;
  nb_jump = cpt;
}

/***************************************************************************\
 *   Initialize parameters
\***************************************************************************/

void InitializeParameters(RWData &rwdata,
    const ZZ& m, const ZZ& s1p, const ZZ& s2p) {
  ZZ p = ZZ_p::modulus();
  
  // Select a random base divisor
  random(rwdata.BaseDivisor);
  
  // B[12]min/max
  rwdata.B1max = (5*SqrRoot(p))/(2*m);
  rwdata.B1min = - rwdata.B1max;
  rwdata.B2min = - ((2*p)/m);
  rwdata.B2max = (3*p)/m;

  // compute K and K*BaseDivisor and m*BaseDivisor
  rwdata.K = p*p+1 - s1p*(p+1) + s2p + m*((rwdata.B2max + rwdata.B2min)/2);
  rwdata.KBaseDivisor = rwdata.K*rwdata.BaseDivisor;
  rwdata.mBaseDivisor = m*rwdata.BaseDivisor;
  rwdata.pp1mBaseDivisor = (p+1)*rwdata.mBaseDivisor;

  // proba of being distinguished
  // NB: the integer pD means that the proba is 2^-pD
  ZZ aux;
  aux = 3*SqrRoot( (rwdata.B1max-rwdata.B1min) * (rwdata.B2max-rwdata.B2min) );
  aux /= 1000;
  rwdata.pD = NumBits(aux) - 2;

  // l1 and l2
  rwdata.l1 = to_double(rwdata.B1max-rwdata.B1min)/9.0
    / sqrt(to_double(1UL<<rwdata.pD));
  rwdata.l2 = to_double(rwdata.B2max-rwdata.B2min)/10.0
    / to_double(1UL<<rwdata.pD);

  // in the case where l1 << 1, the random walk is different, and we have
  // to choose another value for l1
  if (rwdata.l1 <= 2) {
    rwdata.l1 = to_double(rwdata.B1max-rwdata.B1min)/9.0
      / (to_double(1UL<<rwdata.pD));
    assert (rwdata.l1 <= 2);
  }
  
}

/***************************************************************************\
 ***************************************************************************
 *   MAIN
 ***************************************************************************
\***************************************************************************/

void initSeedTime() {
  pid_t pid = getpid();
  long int tm = time(NULL);
  
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  tm += tv.tv_usec;
  SetSeed(to_ZZ(pid+tm));
}



// Default curve: 
/*
p := 10000000000000000051; q := p;
s1 := 1712898036;
s2 := 11452277089352355350;
f0 := 2089986280348253421;
f1 := 4944592307816406286;
f2 := 9716939937510582097;
f3 := 4626433832795028841;
f4 := 3141592653589793238;
m := 500000000;
s1p := 212898036;
s2p := 352355350;
*/

////////////////////////////////////////////////////////////////////
// 
// Read on stdin:
//   p
//   f0, f1, f2, f3, f4
//   m
//   s1 mod m
//   s2 mod m
//

int main(int argc, char **argv) {
  assert (sizeof(int)==4);
  
  bool output_to_file = false;
  char* filename = NULL;
  
  if (argc >= 3) {
    if (!strcmp(argv[1], "-o")) {
      output_to_file = true;
      filename = argv[2];
    }
  }
  
  // read parameters from stdin
  ZZ p;
  cin >> p;
  ZZ_p::init(p);
  
  ZZ_p f0, f1, f2, f3, f4;
  
  ZZ m, s1p, s2p;
  cin >> f0;
  cin >> f1;
  cin >> f2;
  cin >> f3;
  cin >> f4;
  cin >> m;
  cin >> s1p;
  cin >> s2p;

  ZZ_pJac2::init(f0, f1, f2, f3, f4);

  // choose a seed
  initSeedTime();
  
  // initialize the parameters
  RWData rwdata;
  InitializeParameters(rwdata, m, s1p, s2p);
  InitializeRandomWalk(rwdata, 15);

  cerr << "pD = " << rwdata.pD << endl;
  cerr << "l1 = " << rwdata.l1 << endl;
  cerr << "l2 = " << rwdata.l2 << endl;
  cerr << "range B1 = " << rwdata.B1max - rwdata.B1min << endl;
  cerr << "range B2 = " << rwdata.B2max - rwdata.B2min << endl;

  // receive dist points from slaves 
  vec_DivAndTrack ListWild;
  vec_DivAndTrack ListTame;
  DivAndTrack distP, otherdistP;
  int found = 0;
  int b;
  ZZ nb_jump;
  ZZ tot_jumps;
  tot_jumps = 0;
  
  b = (RandomWord() & 1UL);
  while (!found) {
    b = !b;
    RunThread(distP, b, rwdata, nb_jump);
    
    // cerr << "Dist point hit after " << nb_jump << " jumps" << endl;
    // cerr << "Coordinates: "
    //      << to_double(distP.alpha - rwdata.B1min) / 
    //           to_double(rwdata.B1max-rwdata.B1min) 
    //      << " "
    //      << to_double(distP.beta - rwdata.B2min) / 
    //           to_double(rwdata.B2max-rwdata.B2min) << endl;
    tot_jumps += nb_jump;
    if (b) {
      for (int i = 0; i < ListTame.length(); ++i) {
	if (distP.D == ListTame[i].D) {
	  found = 1;
	  otherdistP = ListTame[i];
	  break;
	}
      }
      append(ListWild, distP);
    }
    else {
      for (int i = 0; i < ListWild.length(); ++i) {
	if (distP.D == ListWild[i].D) {
	  found = 1;
	  otherdistP = ListWild[i];
	  break;
	}
      }
      append(ListTame, distP);
    }
    cerr << "We have now " << ListWild.length() << " wilds and " 
      << ListTame.length() << " tames\n";
  }
  
  // found a match: compute the result
  if (!b) {
    DivAndTrack tmp;
    tmp = distP;
    distP = otherdistP;
    otherdistP = tmp;
  } 

  cerr << "Total nb of jumps: " << tot_jumps << endl;
  double ratio;
  ratio = to_double(p);
  ratio = pow(ratio, 0.75) / to_double(m);
  ratio = to_double(tot_jumps) / ratio;
  cerr << "Ratio: " << ratio << endl;
  

  ZZ N = rwdata.K - (distP.alpha - otherdistP.alpha)*(p+1)*m + 
    (distP.beta - otherdistP.beta)*m;

  cout << N << endl;
  
  if (output_to_file) {
    ofstream file;
    file.open(filename, ofstream::trunc);
    file << N << endl;
    file.close();
  }
  
  return 0;
}
