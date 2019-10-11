#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>

#include "ccl.h"

static double nfw_fx(double x)
{
  if(x>0.01) {
    double xp1=1+x;
    double lxp1=log(xp1);
    double den=1./(xp1*lxp1-x);
    return xp1*x*x*x*den;
  }
  else {
    return 2*x;
  }
}

static double nfw_f(double x,void *params)
{
  double offset = *((double *)params);
  return nfw_fx(x)-offset;
}

static double nfw_df(double x,void *params)
{
  if(x>0.01) {
    double xp1=1+x;
    double lxp1=log(xp1);
    double den=1./(xp1*lxp1-x);
    return x*x*(3*xp1*xp1*lxp1-x*(4*x+3))*den*den;
  }
  else {
    return 2;
  }
}

static void nfw_fdf(double x,void *params,
		    double *y, double *dy)
{
  double offset = *((double *)params);
  if(x>0.01) {
    double xp1=1+x;
    double lxp1=log(xp1);
    double den=1./(xp1*lxp1-x);
    *y=xp1*x*x*x*den-offset;
    *dy=x*x*(3*xp1*xp1*lxp1-x*(4*x+3))*den*den;
  }
  else {
    *y=2*x-offset;
    *dy=2;
  }
}

static int get_new_concentration_single(double d_factor, double c_old,
					double *c_new, double c_start)
{
  double c0, offset = d_factor * nfw_fx(c_old);
  int status, iter=0, max_iter=100;
  gsl_function_fdf FDF;
  gsl_root_fdfsolver *s=NULL;
  FDF.f = &nfw_f;
  FDF.df = &nfw_df;
  FDF.fdf = &nfw_fdf;
  FDF.params = &offset;
  
  s=gsl_root_fdfsolver_alloc(gsl_root_fdfsolver_newton);
  if (s==NULL)
    return CCL_ERROR_MEMORY;
  
  gsl_root_fdfsolver_set (s, &FDF, c_start);
  *c_new = c_start;
  do
    {
      iter++;
      c0 = *c_new;
      status = gsl_root_fdfsolver_iterate (s);
      *c_new = gsl_root_fdfsolver_root (s);
      status = gsl_root_test_delta (*c_new, c0, 0, 1e-4);

    }
  while (status == GSL_CONTINUE && iter < max_iter);
  gsl_root_fdfsolver_free (s);

  return status;
}

void ccl_get_new_concentration(double delta_old, int nc, double c_old[],
			       double delta_new, double c_new[],int *status)
{
  if(nc<=0)
    return;

  int ii,st=0;
  double d_factor = delta_old/delta_new;
  for(ii=0;ii<nc;ii++) {
    double c_new_h;
    st=get_new_concentration_single(d_factor, c_old[ii], &(c_new[ii]), c_old[ii]);
    if(st!=GSL_SUCCESS) {
      *status=blah;
      ccl_cosmology_set_status_message(cosmo,
				       "ccl_mass_conversion.c: NR solver failed to find a root\n");
      return;
    }
  }
}