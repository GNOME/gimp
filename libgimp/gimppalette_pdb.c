#include "gimp.h"


void
gimp_palette_get_background (guchar *red,
			     guchar *green,
			     guchar *blue)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_get_background",
                                    &nreturn_vals,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *red = return_vals[1].data.d_color.red;
      *green = return_vals[1].data.d_color.green;
      *blue = return_vals[1].data.d_color.blue;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_get_foreground (guchar *red,
			     guchar *green,
			     guchar *blue)
{
  GParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_palette_get_foreground",
                                    &nreturn_vals,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *red = return_vals[1].data.d_color.red;
      *green = return_vals[1].data.d_color.green;
      *blue = return_vals[1].data.d_color.blue;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_set_background (guchar red,
			     guchar green,
			     guchar blue)
{
  GParam *return_vals;
  int nreturn_vals;
  guchar color[3];

  color[0] = red;
  color[1] = green;
  color[2] = blue;

  return_vals = gimp_run_procedure ("gimp_palette_set_background",
                                    &nreturn_vals,
				    PARAM_COLOR, color,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_palette_set_foreground (guchar red,
			     guchar green,
			     guchar blue)
{
  GParam *return_vals;
  int nreturn_vals;
  guchar color[3];

  color[0] = red;
  color[1] = green;
  color[2] = blue;

  return_vals = gimp_run_procedure ("gimp_palette_set_foreground",
                                    &nreturn_vals,
				    PARAM_COLOR, color,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
