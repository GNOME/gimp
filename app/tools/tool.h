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

#ifndef __TOOL_H__
#define __TOOL_H__


#include "gimpobject.h"


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

  gboolean      toggled;      /*  Bad hack to let the paint_core show the
			       *  right toggle cursors
			       */
  PaintCore    *paintcore;  
};

struct _GimpToolClass
{
  GimpObjectClass  parent_class;

  /* stuff to be filled in by child classes */

  /* FIXME: most of this stuff must go away */
  ToolOptions *tool_options;

  gchar	      *pdb_string;

  gchar       *tool_name;

  gchar       *menu_path;  
  gchar       *menu_accel; 

  gchar      **icon_data;
  GdkPixmap   *icon_pixmap;
  GdkBitmap   *icon_mask;

  gchar       *tool_desc;
  const gchar *help_data;

  ToolType     tool_id;

  GtkWidget   *tool_widget;

  GimpContext *tool_context;

  BitmapCursor *tool_cursor;
  BitmapCursor *toggle_cursor;


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


/*  Function declarations  */

GtkType     gimp_tool_get_type        (void);

void        gimp_tool_initialize      (GimpTool       *tool,
				       GDisplay       *gdisplay);
void	    gimp_tool_control	      (GimpTool       *tool,
				       ToolAction      action,
				       GDisplay       *gdisp);
void        gimp_tool_button_press    (GimpTool       *tool,
				       GdkEventButton *bevent,
				       GDisplay       *gdisp);
void        gimp_tool_button_release  (GimpTool       *tool,
				       GdkEventButton *bevent,
				       GDisplay       *gdisp);
void        gimp_tool_motion          (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);
void        gimp_tool_arrow_key       (GimpTool       *tool,
				       GdkEventKey    *kevent,
				       GDisplay       *gdisp);
void        gimp_tool_modifier_key    (GimpTool       *tool,
				       GdkEventKey    *kevent,
				       GDisplay       *gdisp);
void        gimp_tool_cursor_update   (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);
void        gimp_tool_oper_update     (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);




gchar     * tool_get_PDB_string       (GimpTool	*tool);




gchar *     gimp_tool_get_help_data   (GimpTool *tool);
void	    gimp_tool_help_func       (const gchar *help_data);

/*  don't unref these pixmaps, they are static!  */
GdkPixmap * gimp_tool_get_pixmap           (GimpToolClass     *tool);
GdkBitmap * gimp_tool_get_mask             (GimpToolClass     *tool);

void      gimp_tool_show_options           (GimpTool *);


#endif  /*  __TOOL_H__  */
