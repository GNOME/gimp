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
#include "gimpcontext.h"
#include "tool_options.h"
#include "channel.h"

#include "toolsF.h"

#include <gtk/gtk.h>

/*  The possible states for tools  */
typedef enum
{
  INACTIVE,
  ACTIVE,
  PAUSED
} ToolState;

/* Selection Boolean operations that rect, */
/* ellipse, freehand, and fuzzy tools may  */
/* perform.                                */

typedef enum
{
  SELECTION_ADD       = ADD,
  SELECTION_SUB       = SUB,
  SELECTION_REPLACE   = REPLACE,
  SELECTION_INTERSECT = INTERSECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE
} SelectOps;

/*  The possibilities for where the cursor lies  */
#define  ACTIVE_LAYER      (1 << 0)
#define  SELECTION         (1 << 1)
#define  NON_ACTIVE_LAYER  (1 << 2)

/*  The types of tools...  */
struct _Tool
{
  /*  Data  */
  ToolType   type;          /*  Tool type                                   */
  gint       ID;            /*  unique tool ID                              */

  ToolState  state;         /*  state of tool activity                      */
  gint       paused_count;  /*  paused control count                        */
  gboolean   scroll_lock;   /*  allow scrolling or not                      */
  gboolean   auto_snap_to;  /*  snap to guides automatically                */

  gboolean   preserve;      /*  Preserve this tool across drawable changes  */
  void      *gdisp_ptr;     /*  pointer to currently active gdisp           */
  void      *drawable;      /*  pointer to the tool's current drawable      */

  void      *private;       /*  Tool-specific information                   */

  /*  Action functions  */
  ButtonPressFunc    button_press_func;
  ButtonReleaseFunc  button_release_func;
  MotionFunc         motion_func;
  ArrowKeysFunc      arrow_keys_func;
  ModifierKeyFunc    modifier_key_func;
  CursorUpdateFunc   cursor_update_func;
  OperUpdateFunc     oper_update_func;
  ToolCtlFunc        control_func;
};

struct _ToolInfo
{
  ToolOptions *tool_options;

  gchar    *tool_name;

  gchar    *menu_path;  
  gchar    *menu_accel; 

  gchar   **icon_data;

  gchar    *tool_desc;
  gchar    *private_tip;

  ToolType  tool_id;

  ToolInfoNewFunc  new_func;
  ToolInfoFreeFunc free_func;
  ToolInfoInitFunc init_func;

  GtkWidget   *tool_widget;

  GimpContext *tool_context;
};

/*  Global Data Structures  */
extern Tool     * active_tool;
extern ToolInfo   tool_info[];
extern gint       num_tools;

/*  Function declarations  */
Tool   * tools_new_tool             (ToolType     tool_type);

void     tools_select               (ToolType     tool_type);
void     tools_initialize           (ToolType     tool_type,
				     GDisplay    *gdisplay);

void     active_tool_control        (ToolAction   action,
				     void        *gdisp_ptr);

void     tools_help_func            (gchar       *help_data);

void     tools_register             (ToolType     tool_type,
				     ToolOptions *tool_options);

void     tool_options_dialog_new   (void);
void     tool_options_dialog_show  (void);
void     tool_options_dialog_free  (void);

gchar  * tool_active_PDB_string    (void);

#endif  /*  __TOOLS_H__  */
