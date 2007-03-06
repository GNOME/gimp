/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_EDIT_SELECTION_TOOL_H__
#define __GIMP_EDIT_SELECTION_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_EDIT_SELECTION_TOOL            (gimp_edit_selection_tool_get_type ())
#define GIMP_EDIT_SELECTION_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionTool))
#define GIMP_EDIT_SELECTION_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL, GimpEditSelectionToolClass))
#define GIMP_IS_EDIT_SELECTION_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_EDIT_SELECTION_TOOL))
#define GIMP_IS_EDIT_SELECTION_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EDIT_SELECTION_TOOL))


typedef struct _GimpEditSelectionTool      GimpEditSelectionTool;
typedef struct _GimpEditSelectionToolClass GimpEditSelectionToolClass;

struct _GimpEditSelectionTool
{
  GimpDrawTool        parent_instance;

  gint                origx, origy;    /*  Last x and y coords               */
  gint                cumlx, cumly;    /*  Cumulative changes to x and yed   */
  gint                x, y;            /*  Current x and y coords            */
  gint                num_segs_in;     /*  Num seg in selection boundary     */
  gint                num_segs_out;    /*  Num seg in selection boundary     */
  BoundSeg           *segs_in;         /*  Pointer to the channel sel. segs  */
  BoundSeg           *segs_out;        /*  Pointer to the channel sel. segs  */

  gint                x1, y1;          /*  Bounding box of selection mask    */
  gint                x2, y2;

  GimpTranslateMode   edit_mode;       /*  Translate the mask or layer?      */

  gboolean            first_move;      /*  Don't push undos after the first  */

  gboolean            propagate_release;
};

struct _GimpEditSelectionToolClass
{
  GimpDrawToolClass   parent_class;
};


GType      gimp_edit_selection_tool_get_type  (void) G_GNUC_CONST;

void       gimp_edit_selection_tool_start     (GimpTool          *parent_tool,
                                               GimpDisplay       *display,
                                               GimpCoords        *coords,
                                               GimpTranslateMode  edit_mode,
                                               gboolean           propagate_release);

gboolean   gimp_edit_selection_tool_key_press (GimpTool          *tool,
                                               GdkEventKey       *kevent,
                                               GimpDisplay       *display);

/* could move this function to a more central location
 * so it can be used by other tools?
 */
gint       process_event_queue_keys           (GdkEventKey *kevent,
                                               ... /* GdkKeyType, GdkModifierType, value ... 0 */);

#endif  /*  __GIMP_EDIT_SELECTION_TOOL_H__  */
