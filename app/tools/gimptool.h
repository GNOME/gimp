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

#ifndef __GIMP_TOOL_H__
#define __GIMP_TOOL_H__


#include "gimpobject.h"


#define GIMP_TYPE_BY_COLOR_SELECT_TOOL GTK_TYPE_NONE
#define GIMP_TYPE_AIRBRUSH_TOOL        GTK_TYPE_NONE
#define GIMP_TYPE_CLONE_TOOL           GTK_TYPE_NONE
#define GIMP_TYPE_CONVOLVE_TOOL        GTK_TYPE_NONE
#define GIMP_TYPE_DODGEBURN_TOOL       GTK_TYPE_NONE
#define GIMP_TYPE_SMUDGE_TOOL          GTK_TYPE_NONE


/*  The possibilities for where the cursor lies  */
#define  ACTIVE_LAYER      (1 << 0)
#define  SELECTION         (1 << 1)
#define  NON_ACTIVE_LAYER  (1 << 2)


#define GIMP_TYPE_TOOL            (gimp_tool_get_type ())
#define GIMP_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_TOOL, GimpTool))
#define GIMP_IS_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_TOOL))
#define GIMP_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL, GimpToolClass))
#define GIMP_IS_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL))


typedef struct _GimpToolClass GimpToolClass;

struct _GimpTool
{
  GimpObject	parent_instance;

  ToolState     state;        /*  state of tool activity                      */
  gint          paused_count; /*  paused control count                        */
  gboolean      scroll_lock;  /*  allow scrolling or not                      */
  gboolean      auto_snap_to; /*  snap to guides automatically                */

  gboolean      preserve;     /*  Preserve this tool across drawable changes  */
  GDisplay     *gdisp;        /*  pointer to currently active gdisp           */
  GimpDrawable *drawable;     /*  pointer to the tool's current drawable      */

  GimpToolCursorType tool_cursor;
  GimpToolCursorType toggle_cursor;  /* one of these or both will go
				      * away once all cursor_update
				      * functions are properly
				      * virtualized
				      */

  gboolean           toggled; /*  Bad hack to let the paint_core show the
			       *  right toggle cursors
			       */
};

struct _GimpToolClass
{
  GimpObjectClass  parent_class;

  /* stuff to be filled in by child classes */

  /* FIXME: most of this stuff must go away */
  gchar	      *pdb_string;

  void (* initialize)     (GimpTool       *tool,
			   GDisplay       *gdisp);
  void (* control)        (GimpTool       *tool,
			   ToolAction      action,
			   GDisplay       *gdisp);
  void (* button_press)   (GimpTool       *tool,
			   GdkEventButton *bevent,
			   GDisplay       *gdisp);
  void (* button_release) (GimpTool       *tool,
			   GdkEventButton *bevent,
			   GDisplay       *gdisp);
  void (* motion)         (GimpTool       *tool,
			   GdkEventMotion *mevent,
			   GDisplay       *gdisp);
  void (* arrow_key)      (GimpTool       *tool,
			   GdkEventKey    *kevent,
			   GDisplay       *gdisp);
  void (* modifier_key)   (GimpTool       *tool,
			   GdkEventKey    *kevent,
			   GDisplay       *gdisp);
  void (* cursor_update)  (GimpTool       *tool,
			   GdkEventMotion *mevent,
			   GDisplay       *gdisp);
  void (* oper_update)    (GimpTool       *tool,
			   GdkEventMotion *mevent,
			   GDisplay       *gdisp);
};
GtkType       gimp_tool_get_type        (void);

void          gimp_tool_initialize      (GimpTool       *tool,
					 GDisplay       *gdisplay);
void	      gimp_tool_control         (GimpTool       *tool,
					 ToolAction      action,
					 GDisplay       *gdisp);
void          gimp_tool_button_press    (GimpTool       *tool,
					 GdkEventButton *bevent,
					 GDisplay       *gdisp);
void          gimp_tool_button_release  (GimpTool       *tool,
					 GdkEventButton *bevent,
					 GDisplay       *gdisp);
void          gimp_tool_motion          (GimpTool       *tool,
					 GdkEventMotion *mevent,
					 GDisplay       *gdisp);
void          gimp_tool_arrow_key       (GimpTool       *tool,
					 GdkEventKey    *kevent,
					 GDisplay       *gdisp);
void          gimp_tool_modifier_key    (GimpTool       *tool,
					 GdkEventKey    *kevent,
					 GDisplay       *gdisp);
void          gimp_tool_cursor_update   (GimpTool       *tool,
					 GdkEventMotion *mevent,
					 GDisplay       *gdisp);
void          gimp_tool_oper_update     (GimpTool       *tool,
					 GdkEventMotion *mevent,
					 GDisplay       *gdisp);
const gchar * gimp_tool_get_PDB_string  (GimpTool       *tool);


#endif  /*  __GIMP_TOOL_H__  */
