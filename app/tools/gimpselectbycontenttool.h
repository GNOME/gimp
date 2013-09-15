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


#ifndef __GIMP_SELECT_BY_CONTENT_TOOL_H__
#define __GIMP_SELECT_BY_CONTENT_TOOL_H__


#include "gimpselectiontool.h"

/*  The possible states...  */
typedef enum
{
  NO_ACTION,
  SEED_PLACEMENT,
  SEED_ADJUSTMENT,
  WAITING
} SelectByContentState;

/*  For oper_update & cursor_update  */
typedef enum
{
  ISCISSORS_OP_NONE,
  ISCISSORS_OP_SELECT,
  ISCISSORS_OP_MOVE_POINT,
  ISCISSORS_OP_ADD_POINT,
  ISCISSORS_OP_CONNECT,
  ISCISSORS_OP_IMPOSSIBLE
} SelectByContentOps;

typedef struct _ICurve ICurve;

#define GIMP_TYPE_SELECT_BY_CONTENT_TOOL            (gimp_select_by_content_tool_get_type ())
#define GIMP_SELECT_BY_CONTENT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SELECT_BY_CONTENT_TOOL, GimpSelectByContentTool))
#define GIMP_SELECT_BY_CONTENT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SELECT_BY_CONTENT_TOOL, GimpSelectByContentToolClass))
#define GIMP_IS_SELECT_BY_CONTENT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SELECT_BY_CONTENT_TOOL))
#define GIMP_IS_SELECT_BY_CONTENT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SELECT_BY_CONTENT_TOOL))
#define GIMP_SELECT_BY_CONTENT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SELECT_BY_CONTENT_TOOL, GimpSelectByContentToolClass))

#define GIMP_SELECT_BY_CONTENT_TOOL_GET_OPTIONS(t)  (GIMP_SELECT_BY_CONTENT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpSelectByContentTool      GimpSelectByContentTool;
typedef struct _GimpSelectByContentToolClass GimpSelectByContentToolClass;

struct _GimpSelectByContentTool
{
  GimpSelectionTool  parent_instance;

  SelectByContentOps    op;

  gint            x, y;         /*  upper left hand coordinate            */
  gint            ix, iy;       /*  initial coordinates                   */
  gint            nx, ny;       /*  new coordinates                       */

  TempBuf        *dp_buf;       /*  dynamic programming buffer            */

  ICurve         *livewire;     /*  livewire boundary curve               */

  ICurve         *curve1;       /*  1st curve connected to current point  */
  ICurve         *curve2;       /*  2nd curve connected to current point  */

  GQueue         *curves;       /*  the list of curves                    */

  gboolean        first_point;  /*  is this the first point?              */
  gboolean        connected;    /*  is the region closed?                 */

  SelectByContentState  state;        /*  state of iscissors                    */

  /* XXX might be useful */
  GimpChannel    *mask;         /*  selection mask                        */
  TileManager    *gradient_map; /*  lazily filled gradient map            */
};

struct _GimpSelectByContentToolClass
{
  GimpSelectionToolClass parent_class;
  
  void (* select) (GimpSelectByContentTool *select_by_content_tool,
                   GimpDisplay        *display);
};


void    gimp_select_by_content_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data);

GType   gimp_select_by_content_tool_get_type (void) G_GNUC_CONST;

void    gimp_select_by_content_tool_select     (GimpSelectByContentTool       *free_sel,
                                          GimpDisplay              *display);

void    gimp_select_by_content_tool_get_points (GimpSelectByContentTool       *free_sel,
                                          const GimpVector2       **points,
                                          gint                     *n_points);


#endif  /*  __GIMP_SELECT_BY_CONTENT_TOOL_H__  */
