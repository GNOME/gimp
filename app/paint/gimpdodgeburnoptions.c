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

#include "gimpdodgeburnoptions.h"


#define DODGE_BURN_DEFAULT_EXPOSURE 50.0
#define DODGE_BURN_DEFAULT_TYPE     GIMP_DODGE
#define DODGE_BURN_DEFAULT_MODE     GIMP_MIDTONES


static void   gimp_dodge_burn_options_init       (GimpDodgeBurnOptions      *options);
static void   gimp_dodge_burn_options_class_init (GimpDodgeBurnOptionsClass *options_class);


GType
gimp_dodge_burn_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpDodgeBurnOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_dodge_burn_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDodgeBurnOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_dodge_burn_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpDodgeBurnOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_dodge_burn_options_class_init (GimpDodgeBurnOptionsClass *klass)
{
}

static void
gimp_dodge_burn_options_init (GimpDodgeBurnOptions *options)
{
  options->type     = options->type_d     = DODGE_BURN_DEFAULT_TYPE;
  options->exposure = options->exposure_d = DODGE_BURN_DEFAULT_EXPOSURE;
  options->mode     = options->mode_d     = DODGE_BURN_DEFAULT_MODE;
}
