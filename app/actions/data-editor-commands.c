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

#include "actions-types.h"

#include "widgets/gimpdataeditor.h"

#include "data-editor-commands.h"


/*  public functionss */

void
data_editor_edit_active_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (data);
  gboolean        edit_active;

  edit_active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_data_editor_set_edit_active (editor, edit_active);
}
