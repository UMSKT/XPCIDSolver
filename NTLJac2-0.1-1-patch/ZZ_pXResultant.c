
#include <NTL/ZZ_pX.h>

#include <NTL/new.h>
#include <assert.h>

// include code from NTL's official ZZ_pX.c
NTL_OPEN_NNS 
  void mul(ZZ_pX& U, ZZ_pX& V, const ZZ_pXMatrix& M);
  void mul(ZZ_pXMatrix& A, ZZ_pXMatrix& B, ZZ_pXMatrix& C);
  void PlainResultant(ZZ_p& rres, const ZZ_pX& a, const ZZ_pX& b);
  void ResIterHalfGCD(ZZ_pXMatrix& M_out, ZZ_pX& U, ZZ_pX& V, long d_red,
      vec_ZZ_p& cvec, vec_long& dvec);
  void ResHalfGCD(ZZ_pXMatrix& M_out, const ZZ_pX& U, const ZZ_pX& V,
      long d_red, vec_ZZ_p& cvec, vec_long& dvec);
  void ResHalfGCD(ZZ_pX& U, ZZ_pX& V, vec_ZZ_p& cvec, vec_long& dvec);
NTL_CLOSE_NNS


NTL_START_IMPL

// Euclidian algorithm between U and V
// Fill in cvec and dvec with degrees and LeadCoeffs of all the
// polynomials, starting with V and finishing with the last non-zero
// poly.
// U and V are modified: at the end, V is 0 and U is the previous poly
// (usually a constant). UU is filled with the ante-previous poly
// (usually of degree 1: the last subresultant up to a mult. factor)
void PlainResultant(ZZ_pX& UU, ZZ_pX& U, ZZ_pX& V, vec_ZZ_p& cvec,
    vec_long& dvec) {
  ZZ_pX Q;
  ZZVec tmp(deg(U)+1, ZZ_pInfo->ExtendedModulusSize);
  
  while (deg(V) >= 0) {
    append(cvec, LeadCoeff(V));
    append(dvec, deg(V));
    UU = U;
    PlainDivRem(Q, U, U, V, tmp);
    swap(U, V);
  }
}



// returns the resultant and the last non-trivial sub-resultant
//   rres     is the resultant
//   subres1  is the last subresultant (of degree <= 1)
//   subres2  is filled if the subres1 is degenerated (of degree 0). Then
//            subres2 is the last non-trivial subresultant.
// All of this might fail, in the sense that subres1 and subres2 could
// be 0 instead of having meaningful values. In particular, if rres = 0,
// no further information is obtained. However, if a subres is <> 0, then
// it is (should be?) exact.
//
// Warning: subres1 and subres2 are not allowed to be aliases of the
// input parameters.
void PlainResultantWithSubRes(ZZ_p& rres, ZZ_pX& subres1, ZZ_pX& subres2,
    const ZZ_pX& a, const ZZ_pX& b)
{
   ZZ_p res, saved_res;
   
   clear(subres1);
   clear(subres2);
 
   if (IsZero(a) || IsZero(b))
      clear(res);
   else if (deg(a) == 0 && deg(b) == 0) 
      set(res);
   else {
      long d0, d1, d2;
      long sg = 1;
      ZZ_p lc;
      set(res);

      long n = max(deg(a),deg(b)) + 1;
      ZZ_pX u(INIT_SIZE, n), v(INIT_SIZE, n), saved_u(INIT_SIZE, n);
      ZZVec tmp(n, ZZ_pInfo->ExtendedModulusSize);

      u = a;
      v = b;

      for (;;) {
         d0 = deg(u);
         d1 = deg(v);
         lc = LeadCoeff(v);

	 saved_u = u;
         PlainRem(u, u, v, tmp);
         swap(u, v);
 
         d2 = deg(v);
         if (d2 >= 0) {
            power(lc, lc, d0-d2);
	    saved_res = res;
            mul(res, res, lc);
            if (d0 & d1 & 1) negate(res, res);
	    if (!((d0-d1) & 1)) {
	      sg = -sg;
	      cerr << "sg = " << sg << endl;
	    }
//	    cerr << saved_res*u << endl;
         }
         else {
            if (d1 == 0) {
	      cerr << "d0 = " << d0 << endl;
	       subres1 = sg*saved_res*saved_u;
               if (deg(subres1) > 1) { // degenerated case
		 subres2 = subres1;
		 if (deg(subres1) > 2)
		   clear(subres1);
		 else
		   subres1 = saved_res*lc*power(LeadCoeff(saved_u),d0);
	       }
	       power(lc, lc, d0);
               mul(res, res, lc);
            }
            else {
	      subres1 = sg*res*u;
	      if (deg(subres1) > 1) { // *very* degenerated case
		cerr << "WARNING: This case is not fully implemented!!!\n";
		subres2 = subres1;
		if (deg(subres1) > 2)
		  clear(subres1);
		else
		  subres1 = res*lc*power(LeadCoeff(u),d0);
	      }
	      clear(res);
	    }
            break;
         }
      }

      rres = res;
   }
}

static void Coef(ZZ_p& Coe, vec_ZZ_p& cvec, vec_long& dvec) {
  int r = cvec.length();
  ZZ_p t;
  
  set(Coe);
  for (int l = 3; l <= r-1; ++l) {
    power(t, cvec[l-2], dvec[l-3]-dvec[l-1]);
    mul(Coe, Coe, t);
  }
}

static void Signs(long& sig1, long& sig2, vec_long& dvec) { 
  int r = dvec.length();
  sig1 = 0; sig2 = 0;

  for (int l = 3; l <= r-1; ++l) {
    sig1 = (sig1 + dvec[l-3]*(dvec[l-2] & 1L)) & 1L;
    sig2 = (sig2 + 1L + dvec[l-3] + dvec[l-2]) & 1L;
  }
}


// For the moment, I assume that deg(u) > deg(v) and v1 <> 0
void resultantWithSubRes(ZZ_p& rres, ZZ_pX& subres, 
    const ZZ_pX& u, const ZZ_pX& v)
{
#if 0
   if (deg(u) <= NTL_ZZ_pX_GCD_CROSSOVER || deg(v) <= NTL_ZZ_pX_GCD_CROSSOVER) { 
      PlainResultantWithSubRes(rres, subres1, subres2, u, v);
      return;
   }
#endif
   ZZ_pX u1, v1;

   u1 = u;
   v1 = v;

   ZZ_p t;
   ZZ_pX res;
#if 0
   clear(subres1);
   clear(subres2);

   if (deg(u1) == deg(v1)) {
      rem(u1, u1, v1);
      swap(u1, v1);

      if (IsZero(v1)) {
         clear(rres);
         return;
      }

      power(t, LeadCoeff(u1), deg(u1) - deg(v1));
      mul(res, res, t);
      if (deg(u1) & 1)
         negate(res, res);
   }
   else if (deg(u1) < deg(v1)) {
      swap(u1, v1);
      if (deg(u1) & deg(v1) & 1)
         negate(res, res);
   }
#endif
   assert ((deg(u1) >= deg(v1)) && (v1 != 0));

   vec_ZZ_p cvec;
   vec_long  dvec;

   cvec.SetMaxLength(deg(v1)+2);
   dvec.SetMaxLength(deg(v1)+2);

   append(cvec, LeadCoeff(u1));
   append(dvec, deg(u1));


   while (deg(u1) > NTL_ZZ_pX_GCD_CROSSOVER && !IsZero(v1)) { 
      ResHalfGCD(u1, v1, cvec, dvec);

      if (!IsZero(v1)) {
         append(cvec, LeadCoeff(v1));
         append(dvec, deg(v1));
         rem(u1, u1, v1);
         swap(u1, v1);
      }
   }

   if (IsZero(v1) && deg(u1) > 0) {
      clear(rres);
      return;
   }


   if (deg(u1) <= 1) {
      // we went all the way...
      // Too Bad!!! we have skipped the interesting subresultant.
      //   ==> in this rare case, we go back to the naive algorithm
      //   (in principle we should detect exactly which step of the
      //   recursive call went too fast and to redo just that one... but
      //   this is too much work for something that nether (?) happens.)
      u1 = u; v1 = v;
      cvec.SetLength(0);
      dvec.SetLength(0);
      append(cvec, LeadCoeff(u1));
      append(dvec, deg(u1));
   }
   
   ZZ_pX UU;
   ZZ_p Coe;
   long sig1, sig2;
   PlainResultant(UU, u1, v1, cvec, dvec);
   
   long r = dvec.length();
   assert (cvec.length() == dvec.length());
   
   Coef(Coe, cvec, dvec);
   Signs(sig1, sig2, dvec);
   power(t, cvec[r-2], dvec[r-3] - dvec[r-2] - 1);
   mul(Coe, Coe, t);

   mul(subres, Coe, UU);
   if ((sig1 + dvec[r-2]*sig2) & 1UL)
     negate(subres, subres);

   sig1 = (sig1 + dvec[r-3]*(dvec[r-2] & 1L)) & 1L;
   sig2 = (sig2 + 1L + dvec[r-3] + dvec[r-2]) & 1L;
   power(t, cvec[r-2], dvec[r-2] - dvec[r-1] + 1);
   mul(Coe, Coe, t);

   mul(res, u1, Coe);
   power(t, cvec[r-1], dvec[r-2] - dvec[r-1] - 1);
   mul(res, res, t);
   if ((sig1 + dvec[r-1]*sig2) & 1L)
     negate(res, res);

   if (deg(res) >= 1) {
     subres = res;
     res = 0;
     if (deg(subres) >= 2)
       subres = 0;
   } else if (deg(subres) > 2) {
     subres = 0;
   } else if (deg(subres) == 2) {
     mul(subres, Coe, u1);
     power(t, cvec[r-2], 1-(dvec[r-2] - dvec[r-1]));
     mul(subres, subres, t);
     if ((sig1 + (dvec[r-2]+1)*sig2) & 1L)
       negate(subres, subres);
   }
     
   rres = coeff(res,0);
}

NTL_END_IMPL
