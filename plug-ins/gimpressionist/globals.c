#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"

GtkWidget *presetsavebutton = NULL;
GtkWidget *presetdesctext = NULL;
GtkObject *devthreshadjust = NULL;
gint  brushfile = 2;
ppm_t brushppm  = {0, 0, NULL};
gboolean img_has_alpha = FALSE;
gimpressionist_vals_t pcvals;
ppm_t infile = {0,0,NULL};
ppm_t inalpha = {0,0,NULL};
GRand *gr;

GtkWidget        *previewbutton = NULL;
GtkObject *colornoiseadjust = NULL;



/*
 * The default values for the application, to be initialized at startup.
 * */
static const gimpressionist_vals_t defaultpcvals = {
  4,
  0.0,
  60.0,
  0,
  12.0,
  20.0,
  20.0,
  1.0,
  1,
  0.1,
  0.0,
  30.0,
  0,
  0,
  "defaultbrush.pgm",
  "defaultpaper.pgm",
  {0,0,0,1.0},
  1,
  0,
  { { 0.5, 0.5, 0.0, 0.0, 1.0, 1.0, 0 } },
  1,
  0,
  0.0,
  0.0,
  1.0,
  0,
  0,
  0,
  0,
  0,
  20.0,
  1,
  10.0,
  20.0,
  0,
  0.1,

  { { 0.5, 0.5, 50.0, 1.0 } },
  1,
  1.0,
  0,

  10,
  4,

  0, 0.0
};

void restore_default_values(void)
{
    pcvals = defaultpcvals;
}
