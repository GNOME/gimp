/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "core/gimptooloptions.h"

#include "gimptooloptions-gui.h"


/*  public functions  */

GtkWidget *
gimp_tool_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  vbox = gtk_vbox_new (FALSE, 6);

  return vbox;
}
