#include "gimp.h"


gint32
gimp_display_new (gint32 image_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 display_ID;

  return_vals = gimp_run_procedure ("gimp_display_new",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  display_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    display_ID = return_vals[1].data.d_display;

  gimp_destroy_params (return_vals, nreturn_vals);

  return display_ID;
}

void
gimp_display_delete (gint32 display_ID)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_display_delete",
                                    &nreturn_vals,
                                    PARAM_DISPLAY, display_ID,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_displays_flush ()
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_displays_flush",
                                    &nreturn_vals,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
