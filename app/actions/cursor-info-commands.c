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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpcursorview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "cursor-info-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
cursor_info_sample_merged_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (data);
  gboolean        active;

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_cursor_view_set_sample_merged (view, active);
}
