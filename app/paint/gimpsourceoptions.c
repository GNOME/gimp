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

#include "gimpcloneoptions.h"


#define CLONE_DEFAULT_TYPE     GIMP_IMAGE_CLONE
#define CLONE_DEFAULT_ALIGNED  GIMP_CLONE_ALIGN_NO


static void   gimp_clone_options_init       (GimpCloneOptions      *options);
static void   gimp_clone_options_class_init (GimpCloneOptionsClass *options_class);


GType
gimp_clone_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpCloneOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_clone_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCloneOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_clone_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_OPTIONS,
                                     "GimpCloneOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_clone_options_class_init (GimpCloneOptionsClass *klass)
{
}

static void
gimp_clone_options_init (GimpCloneOptions *options)
{
  options->type    = options->type_d    = CLONE_DEFAULT_TYPE;
  options->aligned = options->aligned_d = CLONE_DEFAULT_ALIGNED;
}
