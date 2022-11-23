/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TOOL_OPTIONS_H__
#define __LIGMA_TOOL_OPTIONS_H__


#include "ligmacontext.h"


#define LIGMA_TYPE_TOOL_OPTIONS            (ligma_tool_options_get_type ())
#define LIGMA_TOOL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_OPTIONS, LigmaToolOptions))
#define LIGMA_TOOL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_OPTIONS, LigmaToolOptionsClass))
#define LIGMA_IS_TOOL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_OPTIONS))
#define LIGMA_IS_TOOL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_OPTIONS))
#define LIGMA_TOOL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_OPTIONS, LigmaToolOptionsClass))


typedef struct _LigmaToolOptionsClass LigmaToolOptionsClass;

struct _LigmaToolOptions
{
  LigmaContext   parent_instance;

  LigmaToolInfo *tool_info;

  /*  if TRUE this instance is the main tool options object used for
   *  the GUI, this is not exactly clean, but there are some things
   *  (like linking brush properties to the active brush, or properly
   *  maintaining global brush, pattern etc.) that can only be done
   *  right in the object, and not by signal connections from the GUI,
   *  or upon switching tools, all of which was much more horrible.
   */
  gboolean      gui_mode;
};

struct _LigmaToolOptionsClass
{
  LigmaContextClass parent_class;
};


GType      ligma_tool_options_get_type      (void) G_GNUC_CONST;

void       ligma_tool_options_set_gui_mode  (LigmaToolOptions   *tool_options,
                                            gboolean           gui_mode);
gboolean   ligma_tool_options_get_gui_mode  (LigmaToolOptions   *tool_options);

gboolean   ligma_tool_options_serialize     (LigmaToolOptions   *tool_options,
                                            GError           **error);
gboolean   ligma_tool_options_deserialize   (LigmaToolOptions   *tool_options,
                                            GError           **error);

gboolean   ligma_tool_options_delete        (LigmaToolOptions   *tool_options,
                                            GError           **error);
void       ligma_tool_options_create_folder (void);


#endif  /*  __LIGMA_TOOL_OPTIONS_H__  */
