/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "gimpvectoroptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_vector_options_init       (GimpVectorOptions      *options);
static void   gimp_vector_options_class_init (GimpVectorOptionsClass *options_class);

static void   gimp_vector_options_reset      (GimpToolOptions *tool_options);


GType
gimp_vector_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpVectorOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_vector_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpVectorOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_vector_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpVectorOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_vector_options_class_init (GimpVectorOptionsClass *klass)
{
}

static void
gimp_vector_options_init (GimpVectorOptions *options)
{
}

void
gimp_vector_options_gui (GimpToolOptions *tool_options)
{
  GimpVectorOptions *options;
  GtkWidget         *vbox;

  options = GIMP_VECTOR_OPTIONS (tool_options);

  gimp_selection_options_gui (tool_options);

  tool_options->reset_func = gimp_vector_options_reset;

  vbox = tool_options->main_vbox;
}

static void
gimp_vector_options_reset (GimpToolOptions *tool_options)
{
  GimpVectorOptions *options;

  options = GIMP_VECTOR_OPTIONS (tool_options);

  gimp_selection_options_reset (tool_options);
}
