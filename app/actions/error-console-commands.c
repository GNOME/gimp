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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimperrorconsole.h"

#include "error-console-commands.h"


/*  public functions  */

void
error_console_clear_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);

  if (GTK_WIDGET_IS_SENSITIVE (console->clear_button))
    gtk_button_clicked (GTK_BUTTON (console->clear_button));
}

void
error_console_save_all_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);

  if (GTK_WIDGET_IS_SENSITIVE (console->save_button))
    gtk_button_clicked (GTK_BUTTON (console->save_button));
}

void
error_console_save_selection_cmd_callback (GtkAction *action,
                                           gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);

  if (GTK_WIDGET_IS_SENSITIVE (console->save_button))
    gimp_button_extended_clicked (GIMP_BUTTON (console->save_button),
                                  GDK_SHIFT_MASK);
}
