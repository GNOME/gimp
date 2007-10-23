#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"

gboolean               img_has_alpha = FALSE;
GRand                 *random_generator;
gimpressionist_vals_t  pcvals;

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

void
restore_default_values (void)
{
  pcvals = defaultpcvals;
}
