/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_commands.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_preview.h"

#include "libgimp/stdplugins-intl.h"

#define PREVIEW_MASK  (GDK_EXPOSURE_MASK       | \
                       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK   | \
                       GDK_BUTTON_RELEASE_MASK | \
                       GDK_BUTTON_MOTION_MASK  | \
                       GDK_KEY_PRESS_MASK      | \
                       GDK_KEY_RELEASE_MASK    | \
                       GDK_ENTER_NOTIFY_MASK   | \
                       GDK_LEAVE_NOTIFY_MASK)

#define PREVIEW_WIDTH  600
#define PREVIEW_HEIGHT 400

/*======================================================================
                Preview Rendering Util routine
=======================================================================*/

#define CHECKWIDTH 4
#define LIGHTCHECK 192
#define DARKCHECK  128
#ifndef OPAQUE
#define OPAQUE     255
#endif

static Object_t *_tmp_obj;

static Preview_t*
preview_user_data(GtkWidget *preview)
{
  return (Preview_t*) g_object_get_data (G_OBJECT (preview), "preview");
}

gint
preview_get_width(GtkWidget *preview)
{
   return preview_user_data(preview)->width;
}

gint
preview_get_height(GtkWidget *preview)
{
   return preview_user_data(preview)->height;
}

static void
render_background(Preview_t *preview_base)
{
   GtkWidget *preview = preview_base->preview;

   gimp_preview_area_fill (GIMP_PREVIEW_AREA (preview),
                           0, 0, G_MAXINT, G_MAXINT,
                           255, 255, 255);
}

static void
render_rgb_image (Preview_t *preview_base,
                  GimpDrawable *drawable)
{
  GeglBuffer *buffer;
  guchar     *dest_buffer;
  gint        dwidth, dheight, pwidth, pheight;
  GtkWidget  *preview = preview_base->preview;

  dwidth  = gimp_drawable_get_width (drawable);
  dheight = gimp_drawable_get_height (drawable);
  pwidth  = preview_base->widget_width;
  pheight = preview_base->widget_height;

  dest_buffer = g_new (guchar, pwidth * pheight * 4);

  buffer = gimp_drawable_get_buffer (drawable);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, pwidth, pheight),
                   MIN ((gdouble) pwidth / (gdouble) dwidth,
                        (gdouble) pheight / (gdouble) dheight),
                   babl_format ("R'G'B'A u8"), dest_buffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, pwidth, pheight,
                          GIMP_RGBA_IMAGE,
                          dest_buffer,
                          pwidth * 4);

  g_free (dest_buffer);
}

static void
render_preview (Preview_t *preview_base,
                GimpDrawable *drawable)
{
  render_background (preview_base);
  render_rgb_image (preview_base, drawable);
}

static gboolean
arrow_cb (GtkWidget      *widget,
          GdkEventButton *event,
          gpointer        data)
{
   if (event->button == 1)
      do_main_popup_menu (event, data);
   return TRUE;
}

static gboolean
preview_draw (GtkWidget *widget,
              cairo_t   *cr)
{
  gint width = preview_get_width (widget);
  gint height = preview_get_height (widget);

  cairo_set_line_width (cr, 1.);
  draw_grid (cr, width, height);

  draw_shapes (cr);

  if (_tmp_obj)
    {
      /* this is a bit of a hack */
      gdouble dash = 4.;
      _tmp_obj->selected |= 4;
      cairo_set_source_rgb (cr, 1., 0., 1.);
      cairo_set_dash (cr, &dash, 1, dash);
      object_draw (_tmp_obj, cr);
    }

  return FALSE;
}

void
preview_set_tmp_obj (Object_t *obj)
{
   _tmp_obj = obj;
}

void
preview_unset_tmp_obj (Object_t *obj)
{
   if (_tmp_obj == obj) _tmp_obj = NULL;
}

void
preview_zoom (Preview_t *preview,
              gfloat     zoom_factor)
{
  preview->widget_width  = ceil (preview->width * zoom_factor);
  preview->widget_height = ceil (preview->height * zoom_factor);

  gtk_widget_set_size_request (preview->preview, preview->widget_width,
                               preview->widget_height);
  gtk_widget_queue_resize (preview->window);
  render_preview (preview, preview->drawable);
  preview_redraw ();
}

GdkCursorType
preview_set_cursor(Preview_t *preview, GdkCursorType cursor_type)
{
   GdkCursorType  prev_cursor = preview->cursor;
   GdkDisplay    *display     = gtk_widget_get_display (preview->window);
   GdkCursor     *cursor      = gdk_cursor_new_for_display (display,
                                                            cursor_type);

   gdk_window_set_cursor(gtk_widget_get_window (preview->window), cursor);
   g_object_unref (cursor);

   preview->cursor = cursor_type;

   return prev_cursor;
}

static const GtkTargetEntry target_table[] =
{
   {"STRING",     0, 1 },
   {"text/plain", 0, 2 }
};

static void
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
            GtkSelectionData *data, guint info, guint time)
{
  gboolean success = FALSE;

  if (gtk_selection_data_get_length (data) >= 0 &&
      gtk_selection_data_get_format (data) == 8)
    {
      ObjectList_t *list = get_shapes();
      Object_t *obj;

      x = get_real_coord(x);
      y = get_real_coord(y);
      obj = object_list_find(list, x, y);
      if (obj && !obj->locked)
        {
          command_list_add(edit_object_command_new(obj));
          object_set_url(obj, (const gchar *) gtk_selection_data_get_data (data));
          object_emit_update_signal(obj);
          success = TRUE;
        }
    }
  gtk_drag_finish(context, success, FALSE, time);
}

static void
preview_size_allocate (GtkWidget *widget,
                       GtkAllocation *allocation,
                       gpointer preview_void)
{
  Preview_t *preview = preview_void;

  render_preview (preview, preview->drawable);
}

static void
scroll_adj_changed (GtkAdjustment *adj,
                    GimpRuler     *ruler)
{
  gimp_ruler_set_range (ruler,
                        gtk_adjustment_get_value (adj),
                        gtk_adjustment_get_value (adj) +
                        gtk_adjustment_get_page_size (adj),
                        gtk_adjustment_get_upper (adj));
}

Preview_t *
make_preview (GimpDrawable *drawable,
              gpointer      imap)
{
   Preview_t *data = g_new (Preview_t, 1);
   GtkAdjustment *hadj;
   GtkAdjustment *vadj;
   GtkWidget *preview;
   GtkWidget *window;
   GtkWidget *viewport;
   GtkWidget *button, *arrow;
   GtkWidget *ruler;
   GtkWidget *grid;
   GtkWidget *scrollbar;
   gint width, height;

   data->drawable = drawable;
   data->preview = preview = gimp_preview_area_new ();

   g_object_set_data (G_OBJECT (preview), "preview", data);
   gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);

   g_signal_connect_after (preview, "draw",
                           G_CALLBACK (preview_draw),
                           data);
   g_signal_connect (preview, "size-allocate",
                     G_CALLBACK (preview_size_allocate),
                     data);

   /* Handle drop of links in preview widget */
   gtk_drag_dest_set (preview, GTK_DEST_DEFAULT_ALL, target_table,
                      2, GDK_ACTION_COPY);

   g_signal_connect (preview, "drag-data-received",
                     G_CALLBACK (handle_drop),
                     NULL);

   data->widget_width  = data->width  = gimp_drawable_get_width (drawable);
   data->widget_height = data->height = gimp_drawable_get_height (drawable);
   gtk_widget_set_size_request (preview, data->widget_width,
                                data->widget_height);

   /* The main grid */
   data->window = grid = gtk_grid_new ();
   gtk_grid_set_row_spacing (GTK_GRID (grid), 1);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 1);

   /* Create button with arrow */
   button = gtk_button_new ();
   gtk_widget_set_can_focus (button, FALSE);
   gtk_widget_set_tooltip_text (button, _("Shortcut Menu"));
   gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
                     // GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_set_events (button,
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
   gtk_widget_show (button);

   g_signal_connect (button, "button-press-event",
                     G_CALLBACK (arrow_cb),
                     imap);

   arrow = gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT,
                                         GTK_ICON_SIZE_BUTTON);
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);

   /* Create horizontal ruler */
   data->hruler = ruler = gimp_ruler_new (GTK_ORIENTATION_HORIZONTAL);
   g_signal_connect_swapped (preview, "motion-notify-event",
                             G_CALLBACK (GTK_WIDGET_GET_CLASS (ruler)->motion_notify_event),
                             ruler);

   gtk_widget_set_hexpand (ruler, TRUE);
   gtk_grid_attach (GTK_GRID (grid), ruler, 1, 0, 1, 1);
   gtk_widget_show (ruler);

   /* Create vertical ruler */
   data->vruler = ruler = gimp_ruler_new (GTK_ORIENTATION_VERTICAL);
   g_signal_connect_swapped (preview, "motion-notify-event",
                             G_CALLBACK (GTK_WIDGET_GET_CLASS (ruler)->motion_notify_event),
                            ruler);
   gtk_widget_set_vexpand (ruler, TRUE);
   gtk_grid_attach (GTK_GRID (grid), ruler, 0, 1, 1, 1);
   gtk_widget_show (ruler);

   window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window),
                                   GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
   width = (data->width > PREVIEW_WIDTH) ? PREVIEW_WIDTH : data->width;
   height = (data->height > PREVIEW_HEIGHT) ? PREVIEW_HEIGHT : data->height;
   gtk_widget_set_size_request (window, width, height);
   gtk_grid_attach (GTK_GRID (grid), window, 1, 1, 1, 1);
   gtk_widget_show (window);

   viewport = gtk_viewport_new (NULL, NULL);
   gtk_container_add (GTK_CONTAINER (window), viewport);
   gtk_widget_show (viewport);

   hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (window));

   g_signal_connect (hadj, "changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->hruler);
   g_signal_connect (hadj, "value-changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->hruler);

   vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (window));

   g_signal_connect (vadj, "changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->vruler);
   g_signal_connect (vadj, "value-changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->vruler);

   gtk_container_add (GTK_CONTAINER (viewport), preview);

   scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, hadj);
   gtk_grid_attach (GTK_GRID (grid), scrollbar, 1, 2, 1, 1);
                    // GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
   gtk_widget_show (scrollbar);

   scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, vadj);
   gtk_grid_attach (GTK_GRID (grid), scrollbar,  2, 1, 1, 1);
                     // GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
   gtk_widget_show (scrollbar);

   gtk_widget_show (preview);

   render_preview (data, drawable);

   gtk_widget_show (grid);

   return data;
}
