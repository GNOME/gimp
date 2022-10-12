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

#ifndef __GIMP_TRANSFORM_GRID_TOOL_H__
#define __GIMP_TRANSFORM_GRID_TOOL_H__


#include "gimptransformtool.h"


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define GIMP_TYPE_TRANSFORM_GRID_TOOL            (gimp_transform_grid_tool_get_type ())
#define GIMP_TRANSFORM_GRID_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL, GimpTransformGridTool))
#define GIMP_TRANSFORM_GRID_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_GRID_TOOL, GimpTransformGridToolClass))
#define GIMP_IS_TRANSFORM_GRID_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL))
#define GIMP_IS_TRANSFORM_GRID_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_GRID_TOOL))
#define GIMP_TRANSFORM_GRID_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL, GimpTransformGridToolClass))

#define GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS(t)  (GIMP_TRANSFORM_GRID_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTransformGridToolClass GimpTransformGridToolClass;

struct _GimpTransformGridTool
{
  GimpTransformTool   parent_instance;

  TransInfo           init_trans_info;  /*  initial transformation info           */
  TransInfo           trans_infos[2];   /*  forward/backward transformation info  */
  gdouble            *trans_info;       /*  current transformation info           */
  GList              *undo_list;        /*  list of all states,
                                            head is current == prev_trans_info,
                                            tail is original == old_trans_info    */
  GList              *redo_list;        /*  list of all undone states,
                                            NULL when nothing undone */

  GList              *hidden_objects;   /*  the objects that was hidden during
                                            the transform                         */

  GimpToolWidget     *widget;
  GimpToolWidget     *grab_widget;
  GList              *previews;
  GimpCanvasItem     *boundary_in;
  GimpCanvasItem     *boundary_out;
  GPtrArray          *strokes;

  GHashTable         *filters;
  GList              *preview_drawables;

  GimpToolGui        *gui;
};

struct _GimpTransformGridToolClass
{
  GimpTransformToolClass  parent_class;

  /*  virtual functions  */
  gboolean         (* info_to_matrix) (GimpTransformGridTool  *tg_tool,
                                       GimpMatrix3            *transform);
  void             (* matrix_to_info) (GimpTransformGridTool  *tg_tool,
                                       const GimpMatrix3      *transform);
  void             (* apply_info)     (GimpTransformGridTool  *tg_tool,
                                       const TransInfo         info);
  gchar          * (* get_undo_desc)  (GimpTransformGridTool  *tg_tool);
  void             (* dialog)         (GimpTransformGridTool  *tg_tool);
  void             (* dialog_update)  (GimpTransformGridTool  *tg_tool);
  void             (* prepare)        (GimpTransformGridTool  *tg_tool);
  void             (* readjust)       (GimpTransformGridTool  *tg_tool);
  GimpToolWidget * (* get_widget)     (GimpTransformGridTool  *tg_tool);
  void             (* update_widget)  (GimpTransformGridTool  *tg_tool);
  void             (* widget_changed) (GimpTransformGridTool  *tg_tool);
  GeglBuffer     * (* transform)      (GimpTransformGridTool  *tg_tool,
                                       GList                  *objects,
                                       GeglBuffer             *orig_buffer,
                                       gint                    orig_offset_x,
                                       gint                    orig_offset_y,
                                       GimpColorProfile      **buffer_profile,
                                       gint                   *new_offset_x,
                                       gint                   *new_offset_y);

  const gchar *ok_button_label;
};


GType      gimp_transform_grid_tool_get_type           (void) G_GNUC_CONST;

gboolean   gimp_transform_grid_tool_info_to_matrix     (GimpTransformGridTool *tg_tool,
                                                        GimpMatrix3           *transform);
void       gimp_transform_grid_tool_matrix_to_info     (GimpTransformGridTool *tg_tool,
                                                        const GimpMatrix3     *transform);

void       gimp_transform_grid_tool_push_internal_undo (GimpTransformGridTool *tg_tool,
                                                        gboolean               compress);


#endif  /*  __GIMP_TRANSFORM_GRID_TOOL_H__  */
