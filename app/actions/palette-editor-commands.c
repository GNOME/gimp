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

#include "widgets/gimppaletteeditor.h"

#include "palette-editor-commands.h"


/*  public functions  */

void
palette_editor_edit_color_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->edit_button))
    gtk_button_clicked (GTK_BUTTON (editor->edit_button));
}

void
palette_editor_new_color_fg_cmd_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->new_button))
    gimp_button_extended_clicked (GIMP_BUTTON (editor->new_button), 0);
}

void
palette_editor_new_color_bg_cmd_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->new_button))
    gimp_button_extended_clicked (GIMP_BUTTON (editor->new_button),
                                  GDK_CONTROL_MASK);
}

void
palette_editor_delete_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->delete_button))
    gtk_button_clicked (GTK_BUTTON (editor->delete_button));
}

void
palette_editor_zoom_in_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->zoom_in_button))
    gtk_button_clicked (GTK_BUTTON (editor->zoom_in_button));
}

void
palette_editor_zoom_out_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->zoom_out_button))
    gtk_button_clicked (GTK_BUTTON (editor->zoom_out_button));
}

void
palette_editor_zoom_all_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->zoom_all_button))
    gtk_button_clicked (GTK_BUTTON (editor->zoom_all_button));
}
