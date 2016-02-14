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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontext.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"

#include "tool-presets-commands.h"


/*  public functions  */

void
tool_presets_restore_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpToolPreset      *preset;

  context = gimp_container_view_get_context (editor->view);

  preset = gimp_context_get_tool_preset (context);

  if (preset)
    gimp_context_tool_preset_changed (context);
}
