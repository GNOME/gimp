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

#include "tools-types.h"

#include "gimpimagemaptool.h"


static GimpToolClass *parent_class = NULL;


/*  local function prototypes  */

static void   gimp_image_map_tool_class_init (GimpImageMapToolClass *klass);
static void   gimp_image_map_tool_init       (GimpImageMapTool      *image_map_tool);

static void   gimp_image_map_tool_destroy    (GtkObject             *object);


/*  functions  */

GtkType
gimp_image_map_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpImageMapTool",
        sizeof (GimpImageMapTool),
        sizeof (GimpImageMapToolClass),
        (GtkClassInitFunc) gimp_image_map_tool_class_init,
        (GtkObjectInitFunc) gimp_image_map_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  GtkObjectClass *object_class;
  GimpToolClass  *tool_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TOOL);

  object_class->destroy = gimp_image_map_tool_destroy;
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (image_map_tool);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */
}

static void
gimp_image_map_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}
