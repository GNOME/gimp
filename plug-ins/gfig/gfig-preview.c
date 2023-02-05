/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-grid.h"
#include "gfig-dobject.h"
#include "gfig-preview.h"

#include "libgimp/stdplugins-intl.h"

#define PREVIEW_MASK  (GDK_EXPOSURE_MASK       | \
                       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK   | \
                       GDK_BUTTON_RELEASE_MASK | \
                       GDK_BUTTON_MOTION_MASK  | \
                       GDK_KEY_PRESS_MASK      | \
                       GDK_KEY_RELEASE_MASK)

static gint       x_pos_val;
static gint       y_pos_val;
static gint       pos_tag = -1;
GtkWidget        *status_label_dname;
GtkWidget        *status_label_fname;
static GtkWidget *pos_label;       /* XY pos marker */


static void       gfig_preview_realize  (GtkWidget *widget);
static gboolean   gfig_preview_events   (GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   data);
static gboolean   gfig_preview_draw     (GtkWidget *widget,
                                         cairo_t   *cr);

static gint       gfig_invscale_x        (gint      x);
static gint       gfig_invscale_y        (gint      y);
static GtkWidget *gfig_pos_labels        (void);
static GtkWidget *make_pos_info          (void);

static void       gfig_pos_update        (gint      x,
                                          gint      y);
static void       gfig_pos_update_labels (gpointer  data);

GtkWidget *
make_preview (GimpGfig *gfig)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *ruler;

  gfig_context->preview = gtk_drawing_area_new ();
  gtk_widget_set_events (GTK_WIDGET (gfig_context->preview), PREVIEW_MASK);

  g_signal_connect (gfig_context->preview , "realize",
                    G_CALLBACK (gfig_preview_realize),
                    NULL);

  g_signal_connect (gfig_context->preview , "event",
                    G_CALLBACK (gfig_preview_events),
                    gfig);

  g_signal_connect_after (gfig_context->preview , "draw",
                          G_CALLBACK (gfig_preview_draw),
                          NULL);

  gtk_widget_set_size_request (gfig_context->preview,
                               preview_width, preview_height);

  frame = gtk_frame_new (NULL);

  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (grid), gfig_context->preview, 1, 1, 1, 1);
  gtk_container_add (GTK_CONTAINER (frame), grid);

  ruler = gimp_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, preview_width, PREVIEW_SIZE);
  g_signal_connect_swapped (gfig_context->preview, "motion-notify-event",
                            G_CALLBACK (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (ruler))->motion_notify_event),
                            ruler);
  gtk_grid_attach (GTK_GRID (grid), ruler, 1, 0, 1, 1);
  gtk_widget_show (ruler);

  ruler = gimp_ruler_new (GTK_ORIENTATION_VERTICAL);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, preview_height, PREVIEW_SIZE);
  g_signal_connect_swapped (gfig_context->preview, "motion-notify-event",
                            G_CALLBACK (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (ruler))->motion_notify_event),
                            ruler);
  gtk_grid_attach (GTK_GRID (grid), ruler, 0, 1, 1, 1);
  gtk_widget_show (ruler);

  gtk_widget_show (frame);
  gtk_widget_show (grid);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  frame = make_pos_info ();
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (hbox);

  return vbox;
}

static void
gfig_preview_realize (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  gdk_window_set_cursor (gtk_widget_get_window (gfig_context->preview),
                         gdk_cursor_new_for_display (display, GDK_CROSSHAIR));
  gfig_grid_colors (widget);
}

static void
draw_background (cairo_t  *cr)
{
  if (! back_pixbuf)
    back_pixbuf = gimp_image_get_thumbnail (gfig_context->image,
                                            preview_width, preview_height,
                                            GIMP_PIXBUF_LARGE_CHECKS);

  if (back_pixbuf)
    {
      gdk_cairo_set_source_pixbuf (cr, back_pixbuf, 0, 0);
      cairo_paint (cr);
    }
}

static gboolean
gfig_preview_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  if (gfig_context->show_background)
    draw_background (cr);

  draw_grid (cr);
  draw_objects (gfig_context->current_obj->obj_list, TRUE, cr);

  if (obj_creating)
    {
      GList *single = g_list_prepend (NULL, obj_creating);
      draw_objects (single, TRUE, cr);
      g_list_free (single);
    }

  return FALSE;
}

static gboolean
gfig_preview_events (GtkWidget *widget,
                     GdkEvent  *event,
                     gpointer   data)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GdkPoint        point;
  static gint     tmp_show_single = 0;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      point.x = bevent->x;
      point.y = bevent->y;

      g_assert (need_to_scale == 0); /* If not out of step some how */

      /* Start drawing of object */
      if (selvals.otype >= MOVE_OBJ)
        {
          if (!selvals.scaletoimage)
            {
              point.x = gfig_invscale_x (point.x);
              point.y = gfig_invscale_y (point.y);
            }
          object_operation_start (GIMP_GFIG (data), &point,
                                  bevent->state & GDK_SHIFT_MASK);

          /* If constraining save start pnt */
          if (selvals.opts.snap2grid)
            {
              /* Save point to constrained point ... if button 3 down */
              if (bevent->button == 3)
                {
                  find_grid_pos (&point, &point, FALSE);
                }
            }
        }
      else
        {
          if (selvals.opts.snap2grid)
            find_grid_pos (&point, &point, FALSE);
          object_start (&point, bevent->state & GDK_SHIFT_MASK);

          gtk_widget_queue_draw (widget);
        }

      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      point.x = bevent->x;
      point.y = bevent->y;

      if (selvals.opts.snap2grid)
        find_grid_pos (&point, &point, bevent->button == 3);

      /* Still got shift down ?*/
      if (selvals.otype >= MOVE_OBJ)
        {
          if (!selvals.scaletoimage)
            {
              point.x = gfig_invscale_x (point.x);
              point.y = gfig_invscale_y (point.y);
            }
          object_operation_end (&point, bevent->state & GDK_SHIFT_MASK);
        }
      else
        {
          if (obj_creating)
            {
              object_end (GIMP_GFIG (data), &point, bevent->state & GDK_SHIFT_MASK);
            }
          else
            break;
        }

      gfig_paint_callback ();
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      point.x = mevent->x;
      point.y = mevent->y;

      if (selvals.opts.snap2grid)
        find_grid_pos (&point, &point, mevent->state & GDK_BUTTON3_MASK);

      if (selvals.otype >= MOVE_OBJ)
        {
          /* Moving objects around */
          if (!selvals.scaletoimage)
            {
              point.x = gfig_invscale_x (point.x);
              point.y = gfig_invscale_y (point.y);
            }
          object_operation (&point, mevent->state & GDK_SHIFT_MASK);
          gfig_pos_update (point.x, point.y);
          return FALSE;
        }

      if (obj_creating)
        {
          obj_creating->class->update (&point);
          gtk_widget_queue_draw (widget);
        }
      gfig_pos_update (point.x, point.y);
      break;

    case GDK_KEY_PRESS:
      if ((tmp_show_single = obj_show_single) != -1)
        {
          obj_show_single = -1;
          draw_grid_clear ();
        }
      break;

    case GDK_KEY_RELEASE:
      if (tmp_show_single != -1)
        {
          obj_show_single = tmp_show_single;
          draw_grid_clear ();
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static GtkWidget *
make_pos_info (void)
{
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;

  frame = gimp_frame_new (_("Object Details"));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Add labels */
  label = gfig_pos_labels ();
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gfig_pos_enable (NULL, NULL);

#if 0
  label = gfig_obj_size_label ();
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
#endif /* 0 */

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  return frame;
}

static gint
gfig_invscale_x (gint x)
{
  if (!selvals.scaletoimage)
    return (gint) (x * scale_x_factor);
  else
    return x;
}

static gint
gfig_invscale_y (gint y)
{
  if (!selvals.scaletoimage)
    return (gint) (y * scale_y_factor);
  else
    return y;
}

static GtkWidget *
gfig_pos_labels (void)
{
  GtkWidget *label;
  GtkWidget *hbox;
  gchar      buf[256];

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (hbox);

  /* Position labels */
  label = gtk_label_new (_("XY position:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pos_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), pos_label, FALSE, FALSE, 0);
  gtk_widget_show (pos_label);

  g_snprintf (buf, sizeof (buf), "%d, %d", 0, 0);
  gtk_label_set_text (GTK_LABEL (pos_label), buf);

  return hbox;
}

void
gfig_pos_enable (GtkWidget *widget,
                 gpointer   data)
{
  gboolean enable = selvals.showpos;

  gtk_widget_set_sensitive (GTK_WIDGET (pos_label), enable);
}

static void
gfig_pos_update_labels (gpointer data)
{
  static gchar buf[256];

  pos_tag = -1;

  g_snprintf (buf, sizeof (buf), "%d, %d", x_pos_val, y_pos_val);
  gtk_label_set_text (GTK_LABEL (pos_label), buf);
}

static void
gfig_pos_update (gint x,
                 gint y)
{
  if ((x_pos_val !=x || y_pos_val != y) && pos_tag == -1 && selvals.showpos)
    {
      x_pos_val = x;
      y_pos_val = y;
      gfig_pos_update_labels (NULL);
    }
}
