/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "paint-types.h"

#include "gimpconvolveoptions.h"


#define DEFAULT_CONVOLVE_RATE  50.0
#define DEFAULT_CONVOLVE_TYPE  GIMP_BLUR_CONVOLVE


static void   gimp_convolve_options_init       (GimpConvolveOptions      *options);
static void   gimp_convolve_options_class_init (GimpConvolveOptionsClass *options_class);


GType
gimp_convolve_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpConvolveOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_convolve_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpConvolveOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_convolve_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpConvolveOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_convolve_options_class_init (GimpConvolveOptionsClass *klass)
{
}

static void
gimp_convolve_options_init (GimpConvolveOptions *options)
{
  options->type = options->type_d = DEFAULT_CONVOLVE_TYPE;
  options->rate = options->rate_d = DEFAULT_CONVOLVE_RATE;
}
