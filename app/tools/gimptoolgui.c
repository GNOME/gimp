/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2003 Spencer Kimball, Peter Mattis, and others
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


static void          gimp_tool_gui_class_init     (GimpToolGuiClass *klass);
static void          gimp_tool_gui_init           (GimpToolGui      *tool_gui);

static void          gimp_tool_gui_finalize       (GObject          *object);

static void          gimp_tool_gui_default_start  (GimpToolGui      *tool,
                                                   GimpDisplay      *gdisp);
static void          gimp_tool_gui_default_stop   (GimpToolGui      *tool);
static void          gimp_tool_gui_default_pause  (GimpToolGui      *tool);
static void          gimp_tool_gui_default_resume (GimpToolGui      *tool);


static inline void   gimp_tool_gui_shift_to_north_west
                                               (gdouble          x,
                                                gdouble          y,
                                                gint             handle_width,
                                                gint             handle_height,
                                                GtkAnchorType    anchor,
                                                gdouble         *shifted_x,
                                                gdouble         *shifted_y);
static inline void   gimp_tool_gui_shift_to_center
                                               (gdouble          x,
                                                gdouble          y,
                                                gint             handle_width,
                                                gint             handle_height,
                                                GtkAnchorType    anchor,
                                                gdouble         *shifted_x,
                                                gdouble         *shifted_y);


static GimpToolClass *parent_class = NULL;


GType
gimp_tool_gui_get_type (void)
{
  static GType tool_gui_type = 0;

  if (! tool_gui_type)
    {
      static const GTypeInfo tool_gui_info =
      {
        sizeof (GimpToolGuiClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_tool_gui_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpToolGui),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_tool_gui_init,
      };

      tool_gui_type = g_type_register_static (GIMP_TYPE_OBJECT,
					      "GimpToolGui", 
                                              &tool_gui_info, 0);
    }

  return tool_gui_type;
}

static void
gimp_tool_gui_class_init (GimpToolGuiClass *klass)
{
  GObjectClass  *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_tool_gui_finalize;

  klass->draw            = NULL;
  klass->start           = gimp_tool_gui_default_start;
  klass->stop            = gimp_tool_gui_default_stop;
  klass->pause           = gimp_tool_gui_default_pause;
  klass->resume          = gimp_tool_gui_default_resume;
}

static void
gimp_tool_gui_init (GimpToolGui *tool_gui)
{
  tool_gui->gdisp        = NULL;
  tool_gui->win          = NULL;
  tool_gui->gc           = NULL;

  tool_gui->draw_state   = GIMP_TOOL_GUI_STATE_INVISIBLE;
  tool_gui->paused_count = 0;

  tool_gui->line_width   = 0;
  tool_gui->line_style   = GDK_LINE_SOLID;
  tool_gui->cap_style    = GDK_CAP_NOT_LAST;
  tool_gui->join_style   = GDK_JOIN_MITER;
}

static void
gimp_tool_gui_finalize (GObject *object)
{
  GimpToolGui *tool_gui;

  tool_gui = GIMP_TOOL_GUI (object);

  if (tool_gui->gc)
    {
      g_object_unref (tool_gui->gc);
      tool_gui->gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_tool_gui_draw (GimpToolGui *tool_gui)
{
  if (tool_gui->gdisp)
    {
      GIMP_TOOL_GUI_GET_CLASS (tool_gui)->draw (tool_gui);
    }
}

void
gimp_tool_gui_start (GimpToolGui *tool_gui,
                     GimpDisplay *gdisp)
{
  GIMP_TOOL_GUI_GET_CLASS (tool_gui)->start (tool_gui, gdisp);
}

void
gimp_tool_gui_stop (GimpToolGui *tool_gui)
{
  GIMP_TOOL_GUI_GET_CLASS (tool_gui)->stop (tool_gui);
}

void
gimp_tool_gui_pause (GimpToolGui *tool_gui)
{
  GIMP_TOOL_GUI_GET_CLASS (tool_gui)->pause (tool_gui);
}

void
gimp_tool_gui_resume (GimpToolGui *tool_gui)
{
  GIMP_TOOL_GUI_GET_CLASS (tool_gui)->resume (tool_gui);
}


static void
gimp_tool_gui_default_start (GimpToolGui *tool_gui,
		             GimpDisplay *gdisp)
{
  GimpDisplayShell *shell;
  GdkColor          fg, bg;

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_tool_gui_stop (tool_gui);

  tool_gui->gdisp        = gdisp;
  tool_gui->win          = shell->canvas->window;
  tool_gui->gc           = gdk_gc_new (tool_gui->win);

  gdk_gc_set_function (tool_gui->gc, GDK_INVERT);
  fg.pixel = 0xFFFFFFFF;
  bg.pixel = 0x00000000;
  gdk_gc_set_foreground (tool_gui->gc, &fg);
  gdk_gc_set_background (tool_gui->gc, &bg);
  gdk_gc_set_line_attributes (tool_gui->gc,
                              tool_gui->line_width,
                              tool_gui->line_style,
			      tool_gui->cap_style,
                              tool_gui->join_style);

  gimp_tool_gui_draw (tool_gui);

  tool_gui->draw_state = GIMP_TOOL_GUI_STATE_VISIBLE;
}

static void
gimp_tool_gui_default_stop (GimpToolGui *tool_gui)
{
  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  if (tool_gui->draw_state == GIMP_TOOL_GUI_STATE_VISIBLE)
    {
      gimp_tool_gui_draw (tool_gui);
    }

  tool_gui->draw_state   = GIMP_TOOL_GUI_STATE_INVISIBLE;
  tool_gui->paused_count = 0;

  tool_gui->gdisp = NULL;
  tool_gui->win   = NULL;

  if (tool_gui->gc)
    {
      g_object_unref (tool_gui->gc);
      tool_gui->gc = NULL;
    }
}

static void
gimp_tool_gui_pause (GimpToolGui *tool_gui)
{
  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  if (tool_gui->paused_count == 0)
    {
      tool_gui->draw_state = GIMP_TOOL_GUI_STATE_INVISIBLE;

      gimp_tool_gui_draw (tool_gui);
    }

  tool_gui->paused_count++;
}

static void
gimp_tool_gui_default_resume (GimpToolGui *tool_gui)
{
  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  if (tool_gui->paused_count > 0)
    {
      tool_gui->paused_count--;

      if (tool_gui->paused_count == 0)
        {
          tool_gui->draw_state = GIMP_TOOL_GUI_STATE_VISIBLE;

          gimp_tool_gui_draw (tool_gui);
        }
    }
  else
    {
      g_warning ("gimp_tool_gui_default_resume(): "
                 "called with tool_gui->paused_count == 0");
    }
}

gdouble
gimp_tool_gui_calc_distance (GimpToolGui  *tool_gui,
                             GimpDisplay  *gdisp,
                             gdouble       x1,
                             gdouble       y1,
                             gdouble       x2,
                             gdouble       y2)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (GIMP_IS_TOOL_GUI (tool_gui), 0.0);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), 0.0);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1, FALSE);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2, FALSE);

  return sqrt (SQR (tx2 - tx1) + SQR (ty2 - ty1));
}

gboolean
gimp_tool_gui_in_radius (GimpToolGui  *tool_gui,
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

  g_return_val_if_fail (GIMP_IS_TOOL_GUI (tool_gui), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1, FALSE);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2, FALSE);

  return (SQR (tx2 - tx1) + SQR (ty2 - ty1)) < SQR (radius);
}

void
gimp_tool_gui_draw_line (GimpToolGui  *tool_gui,
                         gdouble       x1,
                         gdouble       y1,
                         gdouble       x2,
                         gdouble       y2,
                         gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x1, y1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     x2, y2,
                                     &tx2, &ty2,
                                     use_offsets);

  gdk_draw_line (tool_gui->win,
                 tool_gui->gc,
                 RINT (tx1), RINT (ty1),
                 RINT (tx2), RINT (ty2));
}

void
gimp_tool_gui_draw_rectangle (GimpToolGui *tool_gui,
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

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

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

  gdk_draw_rectangle (tool_gui->win,
                      tool_gui->gc,
                      filled,
                      RINT (tx1), RINT (ty1),
                      RINT (tx2 - tx1), RINT (ty2 - ty1));
}

void
gimp_tool_gui_draw_arc (GimpToolGui  *tool_gui,
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

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

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

  gdk_draw_arc (tool_gui->win,
                tool_gui->gc,
                filled,
                RINT (tx1), RINT (ty1),
                RINT (tx2 - tx1), RINT (ty2 - ty1),
                angle1, angle2);
}

void
gimp_tool_gui_draw_rectangle_by_anchor (GimpToolGui    *tool_gui,
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

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_tool_gui_shift_to_north_west (tx, ty,
                                      width, height,
                                      anchor,
                                      &tx, &ty);

  if (filled)
    {
      width++;
      height++;
    }

  gdk_draw_rectangle (tool_gui->win,
                      tool_gui->gc,
                      filled,
                      RINT (tx), RINT (ty),
                      width, height);
}

void
gimp_tool_gui_draw_arc_by_anchor (GimpToolGui   *tool_gui,
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

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  /* well... */
  radius_x *= 2;
  radius_y *= 2;

  gimp_tool_gui_shift_to_north_west (tx, ty,
                                      radius_x, radius_y,
                                      anchor,
                                      &tx, &ty);

  if (filled)
    {
      radius_x += 1;
      radius_y += 1;
    }

  gdk_draw_arc (tool_gui->win,
                tool_gui->gc,
                filled,
                RINT (tx), RINT (ty),
                radius_x, radius_y,
                angle1, angle2);
}

void
gimp_tool_gui_draw_cross_by_anchor (GimpToolGui   *tool_gui,
                                    gdouble        x,
                                    gdouble        y,
                                    gint           width,
                                    gint           height,
                                    GtkAnchorType  anchor,
                                    gboolean       use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_tool_gui_shift_to_center (tx, ty,
                                  width, height,
                                  anchor,
                                  &tx, &ty);

  gdk_draw_line (tool_gui->win,
                 tool_gui->gc,
		 RINT (tx), RINT (ty) - (height >> 1),
		 RINT (tx), RINT (ty) + (height >> 1));
  gdk_draw_line (tool_gui->win,
                 tool_gui->gc,
		 RINT (tx) - (width >> 1), RINT (ty),
		 RINT (tx) + (width >> 1), RINT (ty));
}

void
gimp_tool_gui_draw_handle (GimpToolGui    *tool_gui, 
                           GimpHandleType  type,
                           gdouble         x,
                           gdouble         y,
                           gint            width,
                           gint            height,
                           GtkAnchorType   anchor,
                           gboolean        use_offsets)
{
  g_return_if_fail (GIMP_IS_TOOL_GUI (tool_gui));

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
      gimp_tool_gui_draw_rectangle_by_anchor (tool_gui,
                                               FALSE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      gimp_tool_gui_draw_rectangle_by_anchor (tool_gui,
                                               TRUE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_CIRCLE:
      gimp_tool_gui_draw_arc_by_anchor (tool_gui,
                                         FALSE,
                                         x, y,
                                         width >> 1,
                                         height >> 1,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_tool_gui_draw_arc_by_anchor (tool_gui,
                                         TRUE,
                                         x, y,
                                         width >> 1,
                                         height >> 1,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_CROSS:
      gimp_tool_gui_draw_cross_by_anchor (tool_gui,
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
gimp_tool_gui_on_handle (GimpToolGui    *tool_gui,
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

  g_return_val_if_fail (GIMP_IS_TOOL_GUI (tool_gui), FALSE);
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
      gimp_tool_gui_shift_to_north_west (handle_tx, handle_ty,
                                          width, height,
                                          anchor,
                                          &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_tool_gui_shift_to_center (handle_tx, handle_ty,
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
gimp_tool_gui_draw_lines (GimpToolGui  *tool_gui, 
                          gdouble      *points,
                          gint          n_points,
                          gboolean      filled,
                          gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;
  gint              i;
  gdouble           sx, sy;

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

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
      gdk_draw_polygon (tool_gui->win,
                        tool_gui->gc, TRUE,
                        coords, n_points);
    }
  else
    {
      gdk_draw_lines (tool_gui->win,
                      tool_gui->gc,
                      coords, n_points);
    }

  g_free (coords);
}

void
gimp_tool_gui_draw_strokes (GimpToolGui  *tool_gui, 
                            GimpCoords   *points,
                            gint          n_points,
                            gboolean      filled,
                            gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;
  gint              i;
  gdouble           sx, sy;

  shell = GIMP_DISPLAY_SHELL (tool_gui->gdisp->shell);

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
      gdk_draw_polygon (tool_gui->win,
                        tool_gui->gc, TRUE,
                        coords, n_points);
    }
  else
    {
      gdk_draw_lines (tool_gui->win,
                      tool_gui->gc,
                      coords, n_points);
    }

  g_free (coords);
}


/*  private functions  */

static inline void
gimp_tool_gui_shift_to_north_west (gdouble        x,
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
gimp_tool_gui_shift_to_center (gdouble        x,
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
