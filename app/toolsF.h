#ifndef __TOOLS_F_H__
#define __TOOLS_F_H__

#include <gdk/gdk.h>

#include "gdisplayF.h"

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
  INK,
  LAST_TOOLBOX_TOOL = INK,

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

typedef struct _tool Tool;
typedef struct _ToolInfo ToolInfo;
typedef void (* ButtonPressFunc)       (Tool *, GdkEventButton *, gpointer);
typedef void (* ButtonReleaseFunc)     (Tool *, GdkEventButton *, gpointer);
typedef void (* MotionFunc)            (Tool *, GdkEventMotion *, gpointer);
typedef void (* ArrowKeysFunc)         (Tool *, GdkEventKey *, gpointer);
typedef void (* CursorUpdateFunc)      (Tool *, GdkEventMotion *, gpointer);
typedef void (* ToolCtlFunc)           (Tool *, int, gpointer);

/*  ToolInfo function declarations  */
typedef Tool *(* ToolInfoNewFunc)      (void);
typedef void  (* ToolInfoFreeFunc)     (Tool *);
typedef void  (* ToolInfoInitFunc)     (GDisplay *);

/*  Tool options function declarations  */
typedef void  (* ToolOptionsResetFunc) (void);

#endif
