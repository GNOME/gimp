/*
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

#include "tool_options.h"

#include "libgimp/gimpintl.h"


void
tool_options_init (GimpToolOptions      *options,
		   ToolOptionsResetFunc  reset_func)
{
  options->main_vbox  = gtk_vbox_new (FALSE, 2);
  options->reset_func = reset_func;
}

GimpToolOptions *
tool_options_new (void)
{
  GimpToolOptions *options;
  GtkWidget       *label;

  options = g_new0 (GimpToolOptions, 1);
  tool_options_init (options, NULL);

  label = gtk_label_new (_("This tool has no options."));
  gtk_box_pack_start (GTK_BOX (options->main_vbox), label, FALSE, FALSE, 6);
  gtk_widget_show (label);

  return options;
}
