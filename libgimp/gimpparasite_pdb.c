#include "gimp.h"

Parasite *
gimp_find_parasite (const char *name)
{
  GParam *return_vals;
  int nreturn_vals;
  Parasite *parasite;
  return_vals = gimp_run_procedure ("gimp_find_parasite",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
  {
    parasite = parasite_copy(&return_vals[1].data.d_parasite);
  }
  else
    parasite = NULL;

  gimp_destroy_params (return_vals, nreturn_vals);
  
  return parasite;
}


void
gimp_attach_parasite (const Parasite *p)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_attach_parasite",
				    &nreturn_vals,
				    PARAM_PARASITE, p,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_attach_new_parasite (const char *name, int flags,
			  int size, const void *data)
{
  GParam *return_vals;
  int nreturn_vals;
  Parasite *p = parasite_new(name, flags, size, data);

  return_vals = gimp_run_procedure ("gimp_attach_parasite",
				    &nreturn_vals,
				    PARAM_PARASITE, p,
				    PARAM_END);

  parasite_free(p);
  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_detach_parasite (const char *name)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_detach_parasite",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
