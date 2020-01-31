
// cubic_solve.cpp - real coefficient cubic and quadratic equation solvers

// http://www-staff.it.uts.edu.au/~don/src/quartic.c
// XXX dead link now

// Related link:
// Schwarze, Jochen, Cubic and Quartic Roots, Graphics Gems, p. 404-407, code: p. 738-786
// http://tog.acm.org/resources/GraphicsGems/gems/Roots3And4.c

#include <cmath>  // makes sure sqrt is properly overloaded for double & float

#include "common/cubic_solve.hpp"

template <typename REAL>
REAL compute_realmax()
/* 
     set up constants.

     called by main, quartictest.
*/
{
      REAL doubmin = (1/REAL(2));
      for (int j = 1; j <= 100; ++ j)
      {
          doubmin=doubmin*doubmin;
          if ((doubmin*doubmin) <= (doubmin*doubmin*(1/REAL(2))))
              break;
      }
      return 1/sqrt(2*doubmin);

} /* setcns */

double doubmax;
float floatmax;

template <typename REAL>
struct realmax { };

template <>
struct realmax<float> {
    float value() { return floatmax; }
};

template <>
struct realmax<double> {
    double value() { return doubmax; }
};

void setcns()
{
    doubmax = compute_realmax<double>();
    floatmax = compute_realmax<float>();
}

template <typename REAL>
REAL acos3(REAL x)
/* 
     find cos(acos(x)/3) 
    
     16 Jul 1981   Don Herbison-Evans 

     called by cubic . 
*/
{
   REAL value;

   value = cos(acos(x)*(1/REAL(3)));
   return(value);
} /* acos3 */
/***************************************/

template <typename REAL>
REAL curoot(REAL x)
/* 
     find cube root of x.

     30 Jan 1989   Don Herbison-Evans 

     called by cubic . 
*/
{
   REAL value;
   REAL absx;
   int neg;

   neg = 0;
   absx = x;
   if (x < 0)
   {
      absx = -x;
      neg = 1;
   }
   if (absx != 0) value = exp( log(absx)*(1/REAL(3)) );
      else value = 0;
   if (neg == 1) value = -value;
   return(value);
} /* curoot */

template <typename REAL>
int quadratic(REAL b,REAL c,REAL rts[2])
/* 
     solve the quadratic equation - 

         x**2 + b*x + c = 0 

     14 Jan 2004   cut determinant in quadratic call
     29 Nov 2003   improved
     16 Jul 1981   Don Herbison-Evans

     called by  cubic,quartic,chris,descartes,ferrari,neumark.
*/
{
   int nquad;
   REAL dis,rtdis ;

   dis = b*b - 4*c;
   rts[0] = 0;
   rts[1] = 0;
   if (b == 0)
   {
      if (c == 0)
      {
         nquad = 2;
      }
      else
      {
         if (c < 0)
         {
            nquad = 2;
            rts[0] = sqrt(-c);
            rts[1] = -rts[0];
         }
         else
         {
            nquad = 0;
         }         
      }
   }
   else
   if (c == 0)
   {
      nquad = 2;
      rts[0] = -b;
   }
   else
   if (dis >= 0)
   {
      nquad = 2 ;
      rtdis = sqrt(dis);
      if (b > 0) 
      {
         rts[0] = ( -b - rtdis)*(1/REAL(2));
      }
      else
      {
         rts[0] = ( -b + rtdis)*(1/REAL(2));
      }
      if (rts[0] == 0)
      {
         rts[1] =  -b;
      }
      else
      {
         rts[1] = c/rts[0];
      }
   }
   else
   {
      nquad = 0;
   }
#if 0
   if (debug < 1)
   {
      printf("quad  b %g   c %g  dis %g\n",
         b,c,dis);
      printf("      %d %g %g\n",
         nquad,rts[0],rts[1]);
   }
#endif
   return(nquad);
} /* quadratic */


template <typename REAL>
int cubic(REAL p,REAL q,REAL r,REAL v3[3])
/* 
   find the real roots of the cubic - 
       x**3 + p*x**2 + q*x + r = 0 

     12 Dec 2003 initialising n,m,po3
     12 Dec 2003 allow return of 3 zero roots if p=q=r=0
      2 Dec 2003 negating j if p>0
      1 Dec 2003 changing v from (sinsqk > 0) to (sinsqk >= 0)
      1 Dec 2003 test changing v from po3sq+po3sq to doub2*po3sq
     16 Jul 1981 Don Herbison-Evans

   input parameters - 
     p,q,r - coeffs of cubic equation. 

   output- 
     the number of real roots
     v3 - the roots. 

   global constants -
     rt3 - sqrt(3) 
     (1/double(3)) - 1/3 
     doubmax - square root of largest number held by machine 

     method - 
     see D.E. Littlewood, "A University Algebra" pp.173 - 6 

     15 Nov 2003 output 3 real roots: Don Herbison-Evans
        Apr 1981 initial version: Charles Prineas

     called by  cubictest,quartic,chris,yacfraid,neumark,descartes,ferrari.
     calls      quadratic,acos3,curoot,cubnewton. 
*/
{
   int    n3;
   REAL po3,po3sq,qo3;
   REAL uo3,u2o3,uo3sq4,uo3cu4;
   REAL v,vsq,wsq;
   REAL m1,m2,mcube;
   REAL muo3,s,scube,t,cosk,rt3sink,sinsqk;

   m1=0;  m2=0;  po3=0;
   v=0;  uo3=0; cosk=0;
   if (r == 0)
   {
      n3 = quadratic(p,q,v3);
      v3[n3++] = 0;
      goto done;
   }
   if ((p == 0) && (q == 0))
   {
      v3[0] = curoot(-r);
      v3[1] = v3[0];
      v3[2] = v3[0];
      n3 = 3;
      goto done;
   }
   n3 = 1;
   realmax<REAL> realmax_value;
   if ((p > realmax_value.value()) || (p <  -realmax_value.value()))
   {
      v3[0] = -p;
      goto done;
   }
   if ((q > realmax_value.value()) || (q <  -realmax_value.value()))
   {
       if (q > 0)
       {
          v3[0] =  -r/q;
          goto done;
       }
       else
       if (q < 0)
       {
          v3[0] = -sqrt(-q);
          goto done; 
       }
       else
       {
          v3[0] = 0;
          goto done;
       }
   }
   else
   if ((r > doubmax)|| (r < -doubmax))
   {
      v3[0] =  -curoot(r);
      goto done;
   }
   else
   {
      po3 = p*(1/REAL(3));
      po3sq = po3*po3;
      if (po3sq > doubmax)
      {
         v3[0] = -p;
         goto done;
      }
      else
      {
         v = r + po3*(po3sq+po3sq - q);
         if ((v > doubmax) || (v < -doubmax))
         {
            v3[0] = -p;
            goto done;
         }
         else
         {
            vsq = v*v;
            qo3 = q*(1/REAL(3));
            uo3 = qo3 - po3sq;
            u2o3 = uo3 + uo3;
            if ((u2o3 > doubmax) || (u2o3 < -doubmax))
            {
               if (p == 0)
               {
                  if (q > 0)
                  {
                     v3[0] =  -r/q;
                     goto done;
                  }
		      else
                  if (q < 0)
                  {
                     v3[0] =  -sqrt(-q);
                     goto done;
                  }
                  else
                  {
                     v3[0] = 0;
                     goto done;
                  }
               }
               else
               {
                  v3[0] = -q/p;
                  goto done;
               }
            }
            uo3sq4 = u2o3*u2o3;
            if (uo3sq4 > doubmax)
            {
               if (p == 0)
               {
                  if (q > 0)
                  {
                     v3[0] = -r/q;
                     goto done;
                  }
                  else
	  	      if (q < 0)
                  {
                     v3[0] = -sqrt(-q);
                     goto done;
                  }
		      else
                  {
                     v3[0] = 0;
                     goto done;
                  }
               }
               else
               {
                  v3[0] = -q/p;
                  goto done;
               }
            }
            uo3cu4 = uo3sq4*uo3;
            wsq = uo3cu4 + vsq;
            if (wsq > 0)
            {
/* 
     cubic has one real root -
*/
               if (v <= 0)
               {
                  mcube = ( -v + sqrt(wsq))*(1/REAL(2));
               }
               else
               {
                  mcube = ( -v - sqrt(wsq))*(1/REAL(2));
               }
               m1 = curoot(mcube);
               if (m1 != 0)
               {
                  m2 = -uo3/m1;
               }
               else
               {
                  m2 = 0;
               }
               v3[0] = m1 + m2 - po3;
            }
            else
            {
/* 
     cubic has three real roots -
*/
               if (uo3 < 0)
               {
                  muo3 = -uo3;
                  if (muo3 > 0)
                  {
                     s = sqrt(muo3);
                     if (p > 0)
                     {
                        s = -s;
                     }
                  }
		      else
                  {
                     s = 0;
                  }
                  scube = s*muo3;
		      if (scube == 0)
		      {
                     v3[0] = m1 + m2 - po3;
                     n3 = 1;
		      }
                  else
                  {
                     t =  -v/(scube+scube);
                     cosk = acos3(t);
                     v3[0] = (s+s)*cosk - po3;
                     n3 = 1 ;
                     sinsqk = 1 - cosk*cosk;
		         if (sinsqk >= 0)
		         {
                        rt3sink = sqrt(REAL(3))*sqrt(sinsqk);
                        v3[1] = s*(-cosk + rt3sink) - po3;
                        v3[2] = s*(-cosk - rt3sink) - po3;
                        n3 = 3;
                     }
                  }
               }
               else
/* 
     cubic has multiple root -  
*/
               {
                  v3[0] = curoot(v) - po3;
                  v3[1] = v3[0];
                  v3[2] = v3[0];
                  n3 = 3;
               }
            }
         }
      }
   }
done:
#if 0
   if (debug < 1)
   {
      printf("cubic %d %g %g %g\n",n3,p,q,r);
      for (j = 0; j < n3; ++j)
         printf("   %d %13g %13g\n",j,v3[j],r+v3[j]*(q+v3[j]*(p+v3[j])));
      printf("v %g,  uo3 %g,  m1 %g,   m2 %g,  po3 %g, cosk %g\n",
         v,uo3,m1,m2,po3,cosk);
      for (j = 0; j < 28; ++j)
      {
         printf("  %d",ncub[j]);
         if ((j%10) == 9) printf("\n");
      }
      printf("\n");
   }
   if (iterate == TRUE) cubnewton(p,q,r,n3,v3);
#endif
   return(n3) ;
} /* cubic */

int cubic(float p,float q,float r,float v3[3])
{
    return cubic<float>(p,q,r,v3);
}

int cubic(double p,double q,double r,double v3[3])
{
    return cubic<double>(p,q,r,v3);
}

int quadratic(float b,float c,float v2[2])
{
    return quadratic<float>(b,c,v2);
}

int quadratic(double b,double c,double v2[2])
{
    return quadratic<double>(b,c,v2);
}
