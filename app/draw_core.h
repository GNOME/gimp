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
#ifndef __DRAW_CORE_H__
#define __DRAW_CORE_H__

#include "tools.h"

/*  drawing states  */
#define INVISIBLE   0
#define VISIBLE     1

/*  Structure definitions  */

typedef struct _draw_core  DrawCore;
typedef void (* DrawCoreDraw) (Tool *);

struct _draw_core
{
  GdkGC *         gc;           /*  Grahpics context for drawing functions  */
  GdkWindow *     win;          /*  Window to draw draw operation to      */

  int             draw_state;   /*  Current state in the drawing process    */

  int             line_width;   /**/
  int             line_style;   /**/
  int             cap_style;    /*  line attributes                         */
  int             join_style;   /**/

  int             paused_count; /*  count to keep track of multiple pauses  */

  gpointer        data;         /*  data to pass to draw_func               */

  DrawCoreDraw    draw_func;    /*  Member function for actual drawing      */
};


/*  draw core functions  */

DrawCore *    draw_core_new          (DrawCoreDraw);
void          draw_core_start        (DrawCore *, GdkWindow *, Tool *);
void          draw_core_stop         (DrawCore *, Tool *);
void          draw_core_pause        (DrawCore *, Tool *);
void          draw_core_resume       (DrawCore *, Tool *);
void          draw_core_free         (DrawCore *);




#endif  /*  __DRAW_CORE_H__  */
