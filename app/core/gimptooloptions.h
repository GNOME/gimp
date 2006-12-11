/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_TOOL_OPTIONS_H__
#define __GIMP_TOOL_OPTIONS_H__


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
};

struct _GimpToolOptionsClass
{
  GimpContextClass parent_class;

  void (* reset) (GimpToolOptions *tool_options);
};


GType      gimp_tool_options_get_type      (void) G_GNUC_CONST;

void       gimp_tool_options_reset         (GimpToolOptions  *tool_options);

gboolean   gimp_tool_options_serialize     (GimpToolOptions   *tool_options,
                                            GError           **error);
gboolean   gimp_tool_options_deserialize   (GimpToolOptions   *tool_options,
                                            GError           **error);

gboolean   gimp_tool_options_delete        (GimpToolOptions   *tool_options,
                                            GError           **error);
void       gimp_tool_options_create_folder (void);


#endif  /*  __GIMP_TOOL_OPTIONS_H__  */
