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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"


static void          gimp_draw_tool_class_init (GimpDrawToolClass *klass);
static void          gimp_draw_tool_init       (GimpDrawTool      *draw_tool);

static void          gimp_draw_tool_finalize   (GObject           *object);

static void          gimp_draw_tool_control    (GimpTool          *tool,
                                                GimpToolAction     action,
                                                GimpDisplay       *gdisp);

static void          gimp_draw_tool_draw       (GimpDrawTool      *draw_tool);

static inline void   gimp_draw_tool_shift_to_north_west
                                               (gdouble            x,
                                                gdouble            y,
                                                gint               handle_width,
                                                gint               handle_height,
                                                GtkAnchorType      anchor,
                                                gdouble           *shifted_x,
                                                gdouble           *shifted_y);
static inline void   gimp_draw_tool_shift_to_center
                                               (gdouble            x,
                                                gdouble            y,
                                                gint               handle_width,
                                                gint               handle_height,
                                                GtkAnchorType      anchor,
                                                gdouble           *shifted_x,
                                                gdouble           *shifted_y);


static GimpToolClass *parent_class = NULL;


GType
gimp_draw_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpDrawToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_draw_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDrawTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_draw_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpDrawTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GObjectClass  *object_class;
  GimpToolClass *tool_class;

  object_class = G_OBJECT_CLASS (klass);
  tool_class   = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_draw_tool_finalize;

  tool_class->control    = gimp_draw_tool_control;

  klass->draw            = NULL;
}

static void
gimp_draw_tool_init (GimpDrawTool *draw_tool)
{
  draw_tool->gdisp        = NULL;
  draw_tool->win          = NULL;
  draw_tool->gc           = NULL;

  draw_tool->draw_state   = GIMP_DRAW_TOOL_STATE_INVISIBLE;
  draw_tool->paused_count = 0;

  draw_tool->line_width   = 0;
  draw_tool->line_style   = GDK_LINE_SOLID;
  draw_tool->cap_style    = GDK_CAP_NOT_LAST;
  draw_tool->join_style   = GDK_JOIN_MITER;
}

static void
gimp_draw_tool_finalize (GObject *object)
{
  GimpDrawTool *draw_tool;

  draw_tool = GIMP_DRAW_TOOL (object);

  if (draw_tool->gc)
    {
      g_object_unref (draw_tool->gc);
      draw_tool->gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_draw_tool_control (GimpTool       *tool,
			GimpToolAction  action,
			GimpDisplay    *gdisp)
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

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_draw_tool_draw (GimpDrawTool *draw_tool)
{
  if (draw_tool->gdisp)
    {
      GIMP_DRAW_TOOL_GET_CLASS (draw_tool)->draw (draw_tool);
    }
}

void
gimp_draw_tool_start (GimpDrawTool *draw_tool,
		      GimpDisplay  *gdisp)
{
  GimpDisplayShell *shell;
  GdkColor          fg, bg;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_draw_tool_stop (draw_tool);

  draw_tool->gdisp        = gdisp;
  draw_tool->win          = shell->canvas->window;
  draw_tool->gc           = gdk_gc_new (draw_tool->win);

  gdk_gc_set_function (draw_tool->gc, GDK_INVERT);
  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (draw_tool->gc, &fg);
  gdk_gc_set_background (draw_tool->gc, &bg);
  gdk_gc_set_line_attributes (draw_tool->gc,
                              draw_tool->line_width,
                              draw_tool->line_style,
			      draw_tool->cap_style,
                              draw_tool->join_style);

  gimp_draw_tool_draw (draw_tool);

  draw_tool->draw_state = GIMP_DRAW_TOOL_STATE_VISIBLE;
}

void
gimp_draw_tool_stop (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->draw_state == GIMP_DRAW_TOOL_STATE_VISIBLE)
    {
      gimp_draw_tool_draw (draw_tool);
    }

  draw_tool->draw_state   = GIMP_DRAW_TOOL_STATE_INVISIBLE;
  draw_tool->paused_count = 0;

  draw_tool->gdisp = NULL;
  draw_tool->win   = NULL;

  if (draw_tool->gc)
    {
      g_object_unref (draw_tool->gc);
      draw_tool->gc = NULL;
    }
}

gboolean
gimp_draw_tool_is_active (GimpDrawTool *draw_tool)
{
  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);

  return draw_tool->gdisp != NULL;
}

void
gimp_draw_tool_pause (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->paused_count == 0)
    {
      draw_tool->draw_state = GIMP_DRAW_TOOL_STATE_INVISIBLE;

      gimp_draw_tool_draw (draw_tool);
    }

  draw_tool->paused_count++;
}

void
gimp_draw_tool_resume (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->paused_count > 0)
    {
      draw_tool->paused_count--;

      if (draw_tool->paused_count == 0)
        {
          draw_tool->draw_state = GIMP_DRAW_TOOL_STATE_VISIBLE;

          gimp_draw_tool_draw (draw_tool);
        }
    }
  else
    {
      g_warning ("gimp_draw_tool_resume(): "
                 "called with draw_tool->paused_count == 0");
    }
}

gdouble
gimp_draw_tool_calc_distance (GimpDrawTool *draw_tool,
                              GimpDisplay  *gdisp,
                              gdouble       x1,
                              gdouble       y1,
                              gdouble       x2,
                              gdouble       y2)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), 0.0);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), 0.0);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1, FALSE);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2, FALSE);

  return sqrt (SQR (tx2 - tx1) + SQR (ty2 - ty1));
}

gboolean
gimp_draw_tool_in_radius (GimpDrawTool *draw_tool,
                          GimpDisplay  *gdisp,
                          gdouble       x1,
                          gdouble       y1,
                          gdouble       x2,
                          gdouble       y2,
                          gint          radius)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1, FALSE);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2, FALSE);

  return (SQR (tx2 - tx1) + SQR (ty2 - ty1)) < SQR (radius);
}

void
gimp_draw_tool_draw_line (GimpDrawTool *draw_tool,
                          gdouble       x1,
                          gdouble       y1,
                          gdouble       x2,
                          gdouble       y2,
                          gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x1, y1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     x2, y2,
                                     &tx2, &ty2,
                                     use_offsets);

  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
                 RINT (tx1), RINT (ty1),
                 RINT (tx2), RINT (ty2));
}

void
gimp_draw_tool_draw_rectangle (GimpDrawTool *draw_tool,
                               gboolean      filled,
                               gdouble       x,
                               gdouble       y,
                               gdouble       width,
                               gdouble       height,
                               gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;
  guint             w, h;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  tx1 = MIN (x, x + width);
  ty1 = MIN (y, y + height);
  tx2 = MAX (x, x + width);
  ty2 = MAX (y, y + height);

  gimp_display_shell_transform_xy_f (shell,
                                     tx1, ty1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     tx2, ty2,
                                     &tx2, &ty2,
                                     use_offsets);

  tx1 = CLAMP (tx1, -1, shell->disp_width + 1);
  ty1 = CLAMP (ty1, -1, shell->disp_height + 1);
  tx2 = CLAMP (tx2, -1, shell->disp_width + 1);
  ty2 = CLAMP (ty2, -1, shell->disp_height + 1);

  tx2 -= tx1;
  ty2 -= ty1;
  w = (tx2 >= 0.0) ? RINT (tx2) : 0;
  h = (ty2 >= 0.0) ? RINT (ty2) : 0;

  if (w > 0 && h > 0)
    gdk_draw_rectangle (draw_tool->win,
                        draw_tool->gc,
                        filled,
                        RINT (tx1), RINT (ty1),
                        w - 1, h - 1);
}

void
gimp_draw_tool_draw_arc (GimpDrawTool *draw_tool,
                         gboolean      filled,
                         gdouble       x,
                         gdouble       y,
                         gdouble       width,
                         gdouble       height,
                         gint          angle1,
                         gint          angle2,
                         gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;
  guint             w, h;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  tx1 = MIN (x, x + width);
  ty1 = MIN (y, y + height);
  tx2 = MAX (x, x + width);
  ty2 = MAX (y, y + height);

  gimp_display_shell_transform_xy_f (shell,
                                     tx1, ty1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     tx2, ty2,
                                     &tx2, &ty2,
                                     use_offsets);

  tx2 -= tx1;
  ty2 -= ty1;
  w = (tx2 >= 0.0) ? RINT (tx2) : 0;
  h = (ty2 >= 0.0) ? RINT (ty2) : 0;

  if (w > 0 && h > 0)
    {
      if (w != 1 && h != 1)
        gdk_draw_arc (draw_tool->win,
                      draw_tool->gc,
                      filled,
                      RINT (tx1), RINT (ty1),
                      w - 1, h - 1,
                      angle1, angle2);
      else
        /* work around the problem of an 1xN or Nx1 arc not being shown
           properly */
        gdk_draw_rectangle (draw_tool->win,
                            draw_tool->gc,
                            filled,
                            RINT (tx1), RINT (ty1),
                            w - 1, h - 1);
    }
}

void
gimp_draw_tool_draw_rectangle_by_anchor (GimpDrawTool   *draw_tool,
                                         gboolean        filled,
                                         gdouble         x,
                                         gdouble         y,
                                         gint            width,
                                         gint            height,
                                         GtkAnchorType   anchor,
                                         gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_draw_tool_shift_to_north_west (tx, ty,
                                      width, height,
                                      anchor,
                                      &tx, &ty);

  if (filled)
    {
      width++;
      height++;
    }

  gdk_draw_rectangle (draw_tool->win,
                      draw_tool->gc,
                      filled,
                      RINT (tx), RINT (ty),
                      width, height);
}

void
gimp_draw_tool_draw_arc_by_anchor (GimpDrawTool  *draw_tool,
                                   gboolean       filled,
                                   gdouble        x,
                                   gdouble        y,
                                   gint           radius_x,
                                   gint           radius_y,
                                   gint           angle1,
                                   gint           angle2,
                                   GtkAnchorType  anchor,
                                   gboolean       use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  /* well... */
  radius_x *= 2;
  radius_y *= 2;

  gimp_draw_tool_shift_to_north_west (tx, ty,
                                      radius_x, radius_y,
                                      anchor,
                                      &tx, &ty);

  if (filled)
    {
      radius_x += 1;
      radius_y += 1;
    }

  gdk_draw_arc (draw_tool->win,
                draw_tool->gc,
                filled,
                RINT (tx), RINT (ty),
                radius_x, radius_y,
                angle1, angle2);
}

void
gimp_draw_tool_draw_cross_by_anchor (GimpDrawTool  *draw_tool,
                                     gdouble        x,
                                     gdouble        y,
                                     gint           width,
                                     gint           height,
                                     GtkAnchorType  anchor,
                                     gboolean       use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_draw_tool_shift_to_center (tx, ty,
                                  width, height,
                                  anchor,
                                  &tx, &ty);

  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 RINT (tx), RINT (ty) - (height >> 1),
		 RINT (tx), RINT (ty) + (height >> 1));
  gdk_draw_line (draw_tool->win,
                 draw_tool->gc,
		 RINT (tx) - (width >> 1), RINT (ty),
		 RINT (tx) + (width >> 1), RINT (ty));
}

void
gimp_draw_tool_draw_handle (GimpDrawTool   *draw_tool, 
                            GimpHandleType  type,
                            gdouble         x,
                            gdouble         y,
                            gint            width,
                            gint            height,
                            GtkAnchorType   anchor,
                            gboolean        use_offsets)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
      gimp_draw_tool_draw_rectangle_by_anchor (draw_tool,
                                               FALSE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      gimp_draw_tool_draw_rectangle_by_anchor (draw_tool,
                                               TRUE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_CIRCLE:
      gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                         FALSE,
                                         x, y,
                                         width >> 1,
                                         height >> 1,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                         TRUE,
                                         x, y,
                                         width >> 1,
                                         height >> 1,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_CROSS:
      gimp_draw_tool_draw_cross_by_anchor (draw_tool,
                                           x, y,
                                           width,
                                           height,
                                           anchor,
                                           use_offsets);
      break;

    default:
      g_warning ("%s: invalid handle type %d", G_GNUC_PRETTY_FUNCTION, type);
      break;
    }
}

gboolean
gimp_draw_tool_on_handle (GimpDrawTool   *draw_tool,
                          GimpDisplay    *gdisp,
                          gdouble         x,
                          gdouble         y,
                          GimpHandleType  type,
                          gdouble         handle_x,
                          gdouble         handle_y,
                          gint            width,
                          gint            height,
                          GtkAnchorType   anchor,
                          gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     handle_x, handle_y,
                                     &handle_tx, &handle_ty,
                                     use_offsets);

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
    case GIMP_HANDLE_CROSS:
      gimp_draw_tool_shift_to_north_west (handle_tx, handle_ty,
                                          width, height,
                                          anchor,
                                          &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_draw_tool_shift_to_center (handle_tx, handle_ty,
                                      width, height,
                                      anchor,
                                      &handle_tx, &handle_ty);

      /* FIXME */
      if (width != height)
        width = (width + height) / 2;

      width /= 2;

      return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));

    default:
      g_warning ("%s: invalid handle type %d", G_GNUC_PRETTY_FUNCTION, type);
      break;
    }

  return FALSE;
}

void
gimp_draw_tool_draw_lines (GimpDrawTool *draw_tool, 
			   gdouble      *points,
			   gint          n_points,
			   gboolean      filled,
                           gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;
  gint              i;
  gdouble           sx, sy;

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  coords = g_new (GdkPoint, n_points);

  for (i = 0; i < n_points ; i++)
    {
      gimp_display_shell_transform_xy_f (shell,
                                         points[i*2], points[i*2+1],
                                         &sx, &sy,
                                         use_offsets);
      coords[i].x = ROUND (sx);
      coords[i].y = ROUND (sy);
    }

  if (filled)
    {
      gdk_draw_polygon (draw_tool->win,
                        draw_tool->gc, TRUE,
                        coords, n_points);
    }
  else
    {
      gdk_draw_lines (draw_tool->win,
                      draw_tool->gc,
                      coords, n_points);
    }

  g_free (coords);
}

void
gimp_draw_tool_draw_strokes (GimpDrawTool *draw_tool, 
			     GimpCoords   *points,
			     gint          n_points,
			     gboolean      filled,
                             gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;
  gint              i;
  gdouble           sx, sy;

  shell = GIMP_DISPLAY_SHELL (draw_tool->gdisp->shell);

  coords = g_new (GdkPoint, n_points);

  for (i = 0; i < n_points ; i++)
    {
      gimp_display_shell_transform_xy_f (shell,
                                         points[i].x, points[i].y,
                                         &sx, &sy,
                                         use_offsets);
      coords[i].x = ROUND (sx);
      coords[i].y = ROUND (sy);
    }

  if (filled)
    {
      gdk_draw_polygon (draw_tool->win,
                        draw_tool->gc, TRUE,
                        coords, n_points);
    }
  else
    {
      gdk_draw_lines (draw_tool->win,
                      draw_tool->gc,
                      coords, n_points);
    }

  g_free (coords);
}


/*  private functions  */

static inline void
gimp_draw_tool_shift_to_north_west (gdouble        x,
                                    gdouble        y,
                                    gint           handle_width,
                                    gint           handle_height,
                                    GtkAnchorType  anchor,
                                    gdouble       *shifted_x,
                                    gdouble       *shifted_y)
{
  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      x -= (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH:
      x -= (handle_width >> 1);
      break;

    case GTK_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case GTK_ANCHOR_NORTH_EAST:
      x -= handle_width;
      break;

    case GTK_ANCHOR_SOUTH:
      x -= (handle_width >> 1);
      y -= handle_height;
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      y -= handle_height;
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      x -= handle_width;
      y -= handle_height;
      break;

    case GTK_ANCHOR_WEST:
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_EAST:
      x -= handle_width;
      y -= (handle_height >> 1);
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
                                    
static inline void
gimp_draw_tool_shift_to_center (gdouble        x,
                                gdouble        y,
                                gint           handle_width,
                                gint           handle_height,
                                GtkAnchorType  anchor,
                                gdouble       *shifted_x,
                                gdouble       *shifted_y)
{
  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case GTK_ANCHOR_NORTH:
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH_WEST:
      x += (handle_width >> 1);
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH_EAST:
      x -= (handle_width >> 1);
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH:
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      x += (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      x -= (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_WEST:
      x += (handle_width >> 1);
      break;

    case GTK_ANCHOR_EAST:
      x -= (handle_width >> 1);
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
