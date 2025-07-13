/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#pragma once

#include "gimpcontext.h"


#define GIMP_TYPE_TOOL_OPTIONS            (gimp_tool_options_get_type ())
#define GIMP_TOOL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_OPTIONS, GimpToolOptions))
#define GIMP_TOOL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_OPTIONS, GimpToolOptionsClass))
#define GIMP_IS_TOOL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_OPTIONS))
#define GIMP_IS_TOOL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_OPTIONS))
#define GIMP_TOOL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_OPTIONS, GimpToolOptionsClass))


typedef struct _GimpToolOptionsClass GimpToolOptionsClass;

struct _GimpToolOptions
{
  GimpContext   parent_instance;

  GimpToolInfo *tool_info;

  /*  if TRUE this instance is the main tool options object used for
   *  the GUI, this is not exactly clean, but there are some things
   *  (like linking brush properties to the active brush, or properly
   *  maintaining global brush, pattern etc.) that can only be done
   *  right in the object, and not by signal connections from the GUI,
   *  or upon switching tools, all of which was much more horrible.
   */
  gboolean      gui_mode;
};

struct _GimpToolOptionsClass
{
  GimpContextClass parent_class;
};


GType      gimp_tool_options_get_type      (void) G_GNUC_CONST;

void       gimp_tool_options_set_gui_mode  (GimpToolOptions   *tool_options,
                                            gboolean           gui_mode);
gboolean   gimp_tool_options_get_gui_mode  (GimpToolOptions   *tool_options);

gboolean   gimp_tool_options_serialize     (GimpToolOptions   *tool_options,
                                            GError           **error);
gboolean   gimp_tool_options_deserialize   (GimpToolOptions   *tool_options,
                                            GError           **error);

gboolean   gimp_tool_options_delete        (GimpToolOptions   *tool_options,
                                            GError           **error);
void       gimp_tool_options_create_folder (void);
