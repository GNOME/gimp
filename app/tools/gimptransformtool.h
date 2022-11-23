/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TRANSFORM_TOOL_H__
#define __LIGMA_TRANSFORM_TOOL_H__


#include "ligmadrawtool.h"


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define LIGMA_TYPE_TRANSFORM_TOOL            (ligma_transform_tool_get_type ())
#define LIGMA_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TRANSFORM_TOOL, LigmaTransformTool))
#define LIGMA_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TRANSFORM_TOOL, LigmaTransformToolClass))
#define LIGMA_IS_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TRANSFORM_TOOL))
#define LIGMA_IS_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TRANSFORM_TOOL))
#define LIGMA_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TRANSFORM_TOOL, LigmaTransformToolClass))

#define LIGMA_TRANSFORM_TOOL_GET_OPTIONS(t)  (LIGMA_TRANSFORM_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaTransformToolClass LigmaTransformToolClass;

struct _LigmaTransformTool
{
  LigmaDrawTool       parent_instance;

  GList             *objects;            /*  List of LigmaObject initially
                                             selected and set for
                                             transform processing.        */

  gint               x1, y1;             /*  upper left hand coordinate   */
  gint               x2, y2;             /*  lower right hand coords      */

  LigmaMatrix3        transform;          /*  transformation matrix        */
  gboolean           transform_valid;    /*  whether the matrix is valid  */

  gboolean           restore_type;
  LigmaTransformType  saved_type;
};

struct _LigmaTransformToolClass
{
  LigmaDrawToolClass  parent_class;

  /*  virtual functions  */
  void                     (* recalc_matrix) (LigmaTransformTool  *tr_tool);
  gchar                  * (* get_undo_desc) (LigmaTransformTool  *tr_tool);
  LigmaTransformDirection   (* get_direction) (LigmaTransformTool  *tr_tool);
  GeglBuffer             * (* transform)     (LigmaTransformTool  *tr_tool,
                                              GList              *objects,
                                              GeglBuffer         *orig_buffer,
                                              gint                orig_offset_x,
                                              gint                orig_offset_y,
                                              LigmaColorProfile  **buffer_profile,
                                              gint               *new_offset_x,
                                              gint               *new_offset_y);

  const gchar *undo_desc;
  const gchar *progress_text;
};


GType        ligma_transform_tool_get_type            (void) G_GNUC_CONST;

GList     * ligma_transform_tool_get_selected_objects (LigmaTransformTool  *tr_tool,
                                                      LigmaDisplay        *display);
GList   * ligma_transform_tool_check_selected_objects (LigmaTransformTool  *tr_tool,
                                                      LigmaDisplay        *display,
                                                      GError            **error);

gboolean     ligma_transform_tool_bounds              (LigmaTransformTool  *tr_tool,
                                                      LigmaDisplay        *display);
void         ligma_transform_tool_recalc_matrix       (LigmaTransformTool  *tr_tool,
                                                      LigmaDisplay        *display);

gboolean     ligma_transform_tool_transform           (LigmaTransformTool  *tr_tool,
                                                      LigmaDisplay        *display);

void         ligma_transform_tool_set_type            (LigmaTransformTool  *tr_tool,
                                                      LigmaTransformType   type);


#endif  /*  __LIGMA_TRANSFORM_TOOL_H__  */
