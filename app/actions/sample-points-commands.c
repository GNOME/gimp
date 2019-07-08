/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "actions-types.h"

#include "widgets/gimpsamplepointeditor.h"

#include "sample-points-commands.h"


/*  public functions  */

void
sample_points_sample_merged_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (data);
  gboolean               active = g_variant_get_boolean (value);

  gimp_sample_point_editor_set_sample_merged (editor, active);
}
