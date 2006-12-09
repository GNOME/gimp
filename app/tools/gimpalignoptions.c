/* GIMP - The GNU Image Manipulation Program
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

#include "tools-types.h"

#include "gimpalignoptions.h"
#include "gimptooloptions-gui.h"


G_DEFINE_TYPE (GimpAlignOptions, gimp_align_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_align_options_class_init (GimpAlignOptionsClass *klass)
{
}

static void
gimp_align_options_init (GimpAlignOptions *options)
{
}

GtkWidget *
gimp_align_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox = gimp_tool_options_gui (tool_options);
  GtkWidget *container;

  container = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), container, FALSE, FALSE, 0);
  gtk_widget_show (container);

  g_object_set_data (G_OBJECT (tool_options), "controls-container", container);

  return vbox;
}
