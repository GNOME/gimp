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


#define GIMP_TYPE_TOOL            (gimp_tool_get_type ())
#define GIMP_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_TOOL, GimpTool))
#define GIMP_IS_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_TOOL))
#define GIMP_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL, GimpToolClass))
#define GIMP_IS_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL))


/*  The possibilities for where the cursor lies  */
#define  ACTIVE_LAYER      (1 << 0)
#define  SELECTION         (1 << 1)
#define  NON_ACTIVE_LAYER  (1 << 2)


/*  Tool action function declarations  */
typedef void   (* ButtonPressFunc)    (GimpTool       *tool,
				       GdkEventButton *bevent,
				       GDisplay       *gdisp);
typedef void   (* ButtonReleaseFunc)  (GimpTool       *tool,
				       GdkEventButton *bevent,
				       GDisplay       *gdisp);
typedef void   (* MotionFunc)         (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);
typedef void   (* ArrowKeysFunc)      (GimpTool       *tool,
				       GdkEventKey    *kevent,
				       GDisplay       *gdisp);
typedef void   (* ModifierKeyFunc)    (GimpTool       *tool,
				       GdkEventKey    *kevent,
				       GDisplay       *gdisp);
typedef void   (* CursorUpdateFunc)   (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);
typedef void   (* OperUpdateFunc)     (GimpTool       *tool,
				       GdkEventMotion *mevent,
				       GDisplay       *gdisp);
typedef void   (* ToolCtlFunc)        (GimpTool       *tool,
				       ToolAction     action,
				       GDisplay       *gdisp);

typedef GimpTool * (* GimpToolNewFunc) (void);
typedef void   (* GimpToolUnrefFunc)   (GimpTool      *tool);
typedef void   (* GimpToolInitFunc)    (GimpTool      *tool,
					GDisplay      *gdisp);

typedef struct _GimpToolClass GimpToolClass;

/*  The types of tools...  */
struct _GimpTool
{
  GimpObject	parent_instance;
  /*  Data  */

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
  PaintCore	*paintcore;  
};

struct _GimpToolClass
{
  GimpObjectClass  parent_class;

  /* stuff to be filled in by child classes */

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
  
  /*  Action functions  */
  ButtonPressFunc    button_press_func;
  ButtonReleaseFunc  button_release_func;
  MotionFunc         motion_func;
  ArrowKeysFunc      arrow_keys_func;
  ModifierKeyFunc    modifier_key_func;
  CursorUpdateFunc   cursor_update_func;
  OperUpdateFunc     oper_update_func;
  ToolCtlFunc        control_func;


/* put lots of interesting signals here */
  ToolCtlFunc        reserved1;
  ToolCtlFunc        reserved2;
  ToolCtlFunc        reserved3;
};


/*  Function declarations  */
GtkType    gimp_tool_get_type (void);

GimpTool  * gimp_tool_new             (void);
void	    gimp_tool_control	      (GimpTool *tool,
				       ToolAction action,
				       GDisplay *gdisp);
void	    gimp_tool_initialize      (GimpTool	*tool);		
void        gimp_tool_old_initialize  (GimpTool *tool,
				       GDisplay *gdisplay);
 
gchar *     gimp_tool_get_help_data   (GimpTool *tool);
void	    gimp_tool_help_func       (const gchar *help_data);

void	    gimp_tool_show_options    (GimpTool *tool);

gchar     * tool_get_PDB_string       (GimpTool	*tool);

/*  don't unref these pixmaps, they are static!  */
GdkPixmap * gimp_tool_get_pixmap           (GimpToolClass     *tool);
GdkBitmap * gimp_tool_get_mask             (GimpToolClass     *tool);


#endif  /*  __TOOL_H__  */
