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

#include "layerF.h"
#include "gdisplayF.h"

#include "toolsF.h"

#include <gtk/gtk.h>

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

  char *menu_path;  
  char *menu_accel; 

  char **icon_data;

  char *tool_desc;
  char *private_tip;

  gint tool_id;

  ToolInfoNewFunc new_func;
  ToolInfoFreeFunc free_func;
  ToolInfoInitFunc init_func;

  GtkWidget *tool_widget;
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
