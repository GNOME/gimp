/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpresets.h
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TOOL_PRESETS_H__
#define __GIMP_TOOL_PRESETS_H__


#include "core/gimplist.h"


#define GIMP_TYPE_TOOL_PRESETS            (gimp_tool_presets_get_type ())
#define GIMP_TOOL_PRESETS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_PRESETS, GimpToolPresets))
#define GIMP_TOOL_PRESETS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_PRESETS, GimpToolPresetsClass))
#define GIMP_IS_TOOL_PRESETS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_PRESETS))
#define GIMP_IS_TOOL_PRESETS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_PRESETS))
#define GIMP_TOOL_PRESETS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_PRESETS, GimpToolPresetsClass))


typedef struct _GimpToolPresetsClass GimpToolPresetsClass;

struct _GimpToolPresets
{
  GimpList      parent_instance;

  GimpToolInfo *tool_info;
};

struct _GimpToolPresetsClass
{
  GimpListClass  parent_class;
};


GType             gimp_tool_presets_get_type    (void) G_GNUC_CONST;

GimpToolPresets * gimp_tool_presets_new         (GimpToolInfo     *tool_info);

GimpToolOptions * gimp_tool_presets_get_options (GimpToolPresets  *presets,
                                                 gint              index);

gboolean          gimp_tool_presets_save        (GimpToolPresets  *presets,
                                                 gboolean          be_verbose,
                                                 GError          **error);
gboolean          gimp_tool_presets_load        (GimpToolPresets  *presets,
                                                 gboolean          be_verbose,
                                                 GError          **error);

#endif  /*  __GIMP_TOOL_PRESETS_H__  */
