/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#ifndef __TOOLS_F_H__
#define __TOOLS_F_H__

#include <gdk/gdk.h>

#include "gdisplayF.h"

/*  Tool control actions  */
typedef enum
{
  PAUSE,
  RESUME,
  HALT,
  CURSOR_UPDATE,
  DESTROY,
  RECREATE
} ToolAction;

/*  Tool types  */
typedef enum
{
  FIRST_TOOLBOX_TOOL,
  RECT_SELECT = FIRST_TOOLBOX_TOOL,
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
  FLIP,
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
  INK,
  DODGEBURN,
  SMUDGE,
  XINPUT_AIRBRUSH,
  MEASURE,
  PATH_TOOL,
  LAST_TOOLBOX_TOOL = PATH_TOOL,

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

/*  Structure definitions  */
typedef struct _Tool Tool;
typedef struct _ToolInfo ToolInfo;

/*  Tool action function declarations  */
typedef void   (* ButtonPressFunc)    (Tool *, GdkEventButton *, gpointer);
typedef void   (* ButtonReleaseFunc)  (Tool *, GdkEventButton *, gpointer);
typedef void   (* MotionFunc)         (Tool *, GdkEventMotion *, gpointer);
typedef void   (* ArrowKeysFunc)      (Tool *, GdkEventKey *,    gpointer);
typedef void   (* ModifierKeyFunc)    (Tool *, GdkEventKey *,    gpointer);
typedef void   (* CursorUpdateFunc)   (Tool *, GdkEventMotion *, gpointer);
typedef void   (* ToolCtlFunc)        (Tool *, ToolAction,       gpointer);

/*  ToolInfo function declarations  */
typedef Tool * (* ToolInfoNewFunc)    (void);
typedef void   (* ToolInfoFreeFunc)   (Tool *);
typedef void   (* ToolInfoInitFunc)   (GDisplay *);

#endif /* __TOOLS_F_H__ */
