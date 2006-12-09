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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsamplepointeditor.h"

#include "sample-points-commands.h"

#include "gimp-intl.h"


/*  public functions  */

void
sample_points_sample_merged_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (data);
  gboolean               active;

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_sample_point_editor_set_sample_merged (editor, active);
}
