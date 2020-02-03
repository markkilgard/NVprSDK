
// cubic_solve.hpp - real coefficient cubic and quadratic equation solvers

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

void setcns();
int cubic(float p,float q,float r,float v3[3]);
int cubic(double p,double q,double r,double v3[3]);

int quadratic(float b,float c, float v2[2]);
int quadratic(double b, double c, double v2[2]);
