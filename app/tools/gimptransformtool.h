/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "gimpdrawtool.h"


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define GIMP_TYPE_TRANSFORM_TOOL            (gimp_transform_tool_get_type ())
#define GIMP_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformTool))
#define GIMP_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))
#define GIMP_IS_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_IS_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_TOOL))
#define GIMP_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_TOOL, GimpTransformToolClass))

#define GIMP_TRANSFORM_TOOL_GET_OPTIONS(t)  (GIMP_TRANSFORM_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTransformToolClass GimpTransformToolClass;

struct _GimpTransformTool
{
  GimpDrawTool       parent_instance;

  GList             *objects;            /*  List of GimpObject initially
                                             selected and set for
                                             transform processing.        */

  gint               x1, y1;             /*  upper left hand coordinate   */
  gint               x2, y2;             /*  lower right hand coords      */

  GimpMatrix3        transform;          /*  transformation matrix        */
  gboolean           transform_valid;    /*  whether the matrix is valid  */

  gboolean           restore_type;
  GimpTransformType  saved_type;
};

struct _GimpTransformToolClass
{
  GimpDrawToolClass  parent_class;

  /*  virtual functions  */
  void                     (* recalc_matrix) (GimpTransformTool  *tr_tool);
  gchar                  * (* get_undo_desc) (GimpTransformTool  *tr_tool);
  GimpTransformDirection   (* get_direction) (GimpTransformTool  *tr_tool);
  GeglBuffer             * (* transform)     (GimpTransformTool  *tr_tool,
                                              GList              *objects,
                                              GeglBuffer         *orig_buffer,
                                              gint                orig_offset_x,
                                              gint                orig_offset_y,
                                              GimpColorProfile  **buffer_profile,
                                              gint               *new_offset_x,
                                              gint               *new_offset_y);

  const gchar *undo_desc;
  const gchar *progress_text;
};


GType        gimp_transform_tool_get_type            (void) G_GNUC_CONST;

GList     * gimp_transform_tool_get_selected_objects (GimpTransformTool  *tr_tool,
                                                      GimpDisplay        *display);
GList   * gimp_transform_tool_check_selected_objects (GimpTransformTool  *tr_tool,
                                                      GimpDisplay        *display,
                                                      GError            **error);

gboolean     gimp_transform_tool_bounds              (GimpTransformTool  *tr_tool,
                                                      GimpDisplay        *display);
void         gimp_transform_tool_recalc_matrix       (GimpTransformTool  *tr_tool,
                                                      GimpDisplay        *display);

gboolean     gimp_transform_tool_transform           (GimpTransformTool  *tr_tool,
                                                      GimpDisplay        *display);

void         gimp_transform_tool_set_type            (GimpTransformTool  *tr_tool,
                                                      GimpTransformType   type);
