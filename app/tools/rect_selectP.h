#ifndef __RECT_SELECTP_H__
#define __RECT_SELECTP_H__

#include "draw_core.h"

typedef struct _rect_select RectSelect, EllipseSelect;

struct _rect_select
{
  DrawCore *      core;       /*  Core select object                      */

  int             op;         /*  selection operation (SELECTION_ADD etc.) */

  int             x, y;       /*  upper left hand coordinate              */
  int             w, h;       /*  width and height                        */
  int             center;     /*  is the selection being created from the center out? */

  int             fixed_size;
  int             fixed_width;
  int             fixed_height;
  guint           context_id; /*  for the statusbar                       */ 
};

#endif  /*  __RECT_SELECTP_H__  */
