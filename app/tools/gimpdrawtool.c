/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "config.h"

#include <gtk/gtk.h>

#include "apptypes.h"
 
#include "gimpdrawtool.h"


enum
{
  DRAW,
  LAST_SIGNAL
};

static void   gimp_draw_tool_class_init (GimpDrawToolClass *klass);
static void   gimp_draw_tool_init       (GimpDrawTool      *draw_tool);

static void   gimp_draw_tool_destroy    (GtkObject         *object);

static void   gimp_draw_tool_control    (GimpTool          *tool,
					 ToolAction         action,
					 GDisplay          *gdisp);


static guint gimp_draw_tool_signals[LAST_SIGNAL] = { 0 };

static GimpToolClass *parent_class = NULL;



GtkType
gimp_draw_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpDrawTool",
        sizeof (GimpDrawTool),
        sizeof (GimpDrawToolClass),
        (GtkClassInitFunc) gimp_draw_tool_class_init,
        (GtkObjectInitFunc) gimp_draw_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL /* (GtkClassInitFunc) gimp_tool_class_init, */
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GtkObjectClass *object_class;
  GimpToolClass  *tool_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TOOL);

  gimp_draw_tool_signals[DRAW] =
    gtk_signal_new ("draw",
    		    GTK_RUN_FIRST,
    		    object_class->type,
    		    GTK_SIGNAL_OFFSET (GimpDrawToolClass,
    		    		       draw),
    		    gtk_marshal_NONE__NONE,
    		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_draw_tool_signals,
				LAST_SIGNAL);

  object_class->destroy = gimp_draw_tool_destroy;

  tool_class->control   = gimp_draw_tool_control;
}

static void
gimp_draw_tool_init (GimpDrawTool *tool)
{
  tool->draw_state   = INVISIBLE;
  tool->gc           = NULL;
  tool->paused_count = 0;
  tool->line_width   = 0;
  tool->line_style   = GDK_LINE_SOLID;
  tool->cap_style    = GDK_CAP_NOT_LAST;
  tool->join_style   = GDK_JOIN_MITER;
}

static void
gimp_draw_tool_destroy (GtkObject *object)
{
  GimpDrawTool *draw_tool;

  draw_tool = GIMP_DRAW_TOOL (object);

  if (draw_tool->draw_state == VISIBLE)
    gimp_draw_tool_stop (draw_tool);

  if (draw_tool->gc)
    gdk_gc_destroy (draw_tool->gc);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_draw_tool_control (GimpTool   *tool,
			ToolAction  action,
			GDisplay   *gdisp)
{
  GimpDrawTool *draw_tool;

  draw_tool = GIMP_DRAW_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      gimp_draw_tool_pause (draw_tool);
      break;

    case RESUME:
      gimp_draw_tool_resume (draw_tool);
      break;

    case HALT:
      gimp_draw_tool_stop (draw_tool);
      break;

    default:
      break;
    }
}

void
gimp_draw_tool_start (GimpDrawTool *core,
		      GdkWindow    *win)
{
  GdkColor fg, bg;

  if (core->draw_state != INVISIBLE)
    gimp_draw_tool_stop (core);  /* this seems backwards ;) */

  core->win   = win;
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

  gtk_signal_emit (GTK_OBJECT(core), gimp_draw_tool_signals[DRAW]);

  core->draw_state = VISIBLE;
}


void
gimp_draw_tool_stop (GimpDrawTool *core)
{
  if (core->draw_state == INVISIBLE)
    return;

  gtk_signal_emit (GTK_OBJECT(core), gimp_draw_tool_signals[DRAW]);

  core->draw_state = INVISIBLE;
}


void
gimp_draw_tool_resume (GimpDrawTool *core)
{
  core->paused_count = (core->paused_count > 0) ? core->paused_count - 1 : 0;

  if (core->paused_count == 0)
    {
      core->draw_state = VISIBLE;

      gtk_signal_emit(GTK_OBJECT(core), gimp_draw_tool_signals[DRAW]);
    }
}


void
gimp_draw_tool_pause (GimpDrawTool *core)
{
  if (core->paused_count == 0)
    {
      core->draw_state = INVISIBLE;

      gtk_signal_emit (GTK_OBJECT(core), gimp_draw_tool_signals[DRAW]);
    }

  core->paused_count++;
}
