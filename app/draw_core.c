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
#include <stdlib.h>
#include "appenv.h"
#include "draw_core.h"


DrawCore *
draw_core_new (draw_func)
     DrawCoreDraw draw_func;
{
  DrawCore * core;

  core = (DrawCore *) g_malloc (sizeof (DrawCore));

  core->draw_func    = draw_func;
  core->draw_state   = INVISIBLE;
  core->gc           = NULL;
  core->paused_count = 0;
  core->data         = NULL;
  core->line_width   = 1;
  core->line_style   = GDK_LINE_SOLID;
  core->cap_style    = GDK_CAP_NOT_LAST;
  core->join_style   = GDK_JOIN_MITER;

  return core;
}


void
draw_core_start (core, win, tool)
     DrawCore *core;
     GdkWindow *win;
     Tool *tool;
{
  GdkColor fg, bg;

  if (core->draw_state != INVISIBLE)
    draw_core_stop (core, tool);

  core->win   = win;
  core->data  = (void *) tool;
  core->paused_count = 0;  /*  reset pause counter to 0  */

  /*  create a new graphics context  */
  if (! core->gc)
    core->gc = gdk_gc_new (win);

  gdk_gc_set_function (core->gc, GDK_INVERT);
  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (core->gc, &fg);
  gdk_gc_set_background (core->gc, &bg);
  gdk_gc_set_line_attributes (core->gc, core->line_width, core->line_style, 
			      core->cap_style, core->join_style);

  (* core->draw_func) (tool);

  core->draw_state = VISIBLE;
}


void
draw_core_stop (core, tool)
     DrawCore * core;
     Tool * tool;
{
  if (core->draw_state == INVISIBLE)
    return;

  (* core->draw_func) (tool);

  core->draw_state = INVISIBLE;
}


void
draw_core_resume (core, tool)
     DrawCore * core;
     Tool * tool;
{
  core->paused_count = (core->paused_count > 0) ? core->paused_count - 1 : 0;
  if (core->paused_count == 0)
    {
      core->draw_state = VISIBLE;
      (* core->draw_func) (tool);
    }
}


void
draw_core_pause (core, tool)
     DrawCore * core;
     Tool * tool;
{
  if (core->paused_count == 0)
    {
      core->draw_state = INVISIBLE;
      (* core->draw_func) (tool);
    }
  core->paused_count++;
}


void
draw_core_free (core)
     DrawCore * core;
{
  if (core)
    {
      if (core->gc)
	gdk_gc_destroy (core->gc);
      g_free (core);
    }
}


