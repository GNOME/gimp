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

#include "gimpsmudgeoptions.h"


#define SMUDGE_DEFAULT_RATE 50.0


static void   gimp_smudge_options_init       (GimpSmudgeOptions      *options);
static void   gimp_smudge_options_class_init (GimpSmudgeOptionsClass *options_class);


GType
gimp_smudge_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpSmudgeOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_smudge_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpSmudgeOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_smudge_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpSmudgeOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_smudge_options_class_init (GimpSmudgeOptionsClass *klass)
{
}

static void
gimp_smudge_options_init (GimpSmudgeOptions *options)
{
  options->rate = options->rate_d = SMUDGE_DEFAULT_RATE;
}
