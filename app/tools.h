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
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "layer.h"
#include "gdisplay.h"

/*  The possible states for tools  */
#define  INACTIVE               0
#define  ACTIVE                 1
#define  PAUSED                 2


/*  Tool control actions  */
#define  PAUSE                  0
#define  RESUME                 1
#define  HALT                   2
#define  CURSOR_UPDATE          3
#define  DESTROY                4
#define  RECREATE               5


/*  The possibilities for where the cursor lies  */
#define  ACTIVE_LAYER           (1 << 0)
#define  SELECTION              (1 << 1)
#define  NON_ACTIVE_LAYER       (1 << 2)


/*  The types of tools...  */
typedef enum
{
  RECT_SELECT,
  ELLIPSE_SELECT,
  FREE_SELECT,
  FUZZY_SELECT,
  BEZIER_SELECT,
  ISCISSORS,
  MOVE,
  MAGNIFY,
  CROP,
  ROTATE,
  SCALE,
  SHEAR,
  PERSPECTIVE,
  FLIP_HORZ,
  FLIP_VERT,
  TEXT,
  COLOR_PICKER,
  BUCKET_FILL,
  BLEND,
  PENCIL,
  PAINTBRUSH,
  ERASER,
  AIRBRUSH,
  CLONE,
  CONVOLVE,

  /*  Non-toolbox tools  */
  BY_COLOR_SELECT,
  COLOR_BALANCE,
  BRIGHTNESS_CONTRAST,
  HUE_SATURATION,
  POSTERIZE,
  THRESHOLD,
  CURVES,
  LEVELS,
  HISTOGRAM
} ToolType;

#define XButtonEvent GdkEventButton
#define XMotionEvent GdkEventMotion

/*  Structure definitions  */

typedef struct _tool Tool;
typedef struct _ToolInfo ToolInfo;
typedef void (* ButtonPressFunc)   (Tool *, GdkEventButton *, gpointer);
typedef void (* ButtonReleaseFunc) (Tool *, GdkEventButton *, gpointer);
typedef void (* MotionFunc)        (Tool *, GdkEventMotion *, gpointer);
typedef void (* ArrowKeysFunc)     (Tool *, GdkEventKey *, gpointer);
typedef void (* CursorUpdateFunc)  (Tool *, GdkEventMotion *, gpointer);
typedef void (* ToolCtlFunc)       (Tool *, int, gpointer);


struct _tool
{
  /*  Data  */
  ToolType       type;                 /*  Tool type  */
  int            state;                /*  state of tool activity  */
  int            paused_count;         /*  paused control count  */
  int            scroll_lock;          /*  allow scrolling or not  */
  int            auto_snap_to;         /*  should the mouse snap to guides automatically */
  void *         private;              /*  Tool-specific information  */
  void *         gdisp_ptr;            /*  pointer to currently active gdisp  */
  void *         drawable;             /*  pointer to the drawable that was
					   active when the tool was created */
  int            ID;                   /*  unique tool ID  */

  int            preserve;             /*  Perserve this tool through the current image changes */

  /*  Action functions  */
  ButtonPressFunc    button_press_func;
  ButtonReleaseFunc  button_release_func;
  MotionFunc         motion_func;
  ArrowKeysFunc      arrow_keys_func;
  CursorUpdateFunc   cursor_update_func;
  ToolCtlFunc        control_func;
};


struct _ToolInfo
{
  GtkWidget *tool_options;
  char *tool_name;
  int toolbar_position;
};


/*  Global Data Structure  */

extern Tool * active_tool;
extern ToolType active_tool_type;
extern Layer * active_tool_layer;
extern ToolInfo tool_info[];

/*  Function declarations  */

void     tools_select              (ToolType);
void     tools_initialize          (ToolType, GDisplay *);
void     tools_options_dialog_new  (void);
void     tools_options_dialog_show (void);
void     tools_options_dialog_free (void);
void     tools_register_options    (ToolType, GtkWidget *);
void *   tools_register_no_options (ToolType, char *);
void     active_tool_control       (int, void *);


/*  Standard member functions  */
void     standard_arrow_keys_func  (Tool *, GdkEventKey *, gpointer);

#endif  /*  __TOOLS_H__  */
