/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print-preview.h"


enum
{
  OFFSETS_CHANGED,
  LAST_SIGNAL
};


#define SIZE_REQUEST  200


struct _PrintPreview
{
  GtkEventBox      parent_instance;

  GdkCursor       *cursor;

  GtkPageSetup    *page;
  cairo_surface_t *thumbnail;
  gboolean         dragging;
  gboolean         inside;

  gint32           drawable_id;

  gdouble          image_offset_x;
  gdouble          image_offset_y;
  gdouble          image_offset_x_max;
  gdouble          image_offset_y_max;
  gdouble          image_width;
  gdouble          image_height;

  gboolean         use_full_page;

  /* for mouse drags */
  gdouble          orig_offset_x;
  gdouble          orig_offset_y;
  gint             start_x;
  gint             start_y;
};

struct _PrintPreviewClass
{
  GtkEventBoxClass  parent_class;

  void (* offsets_changed)  (PrintPreview *print_preview,
                             gint          offset_x,
                             gint          offset_y);
};


static void      print_preview_finalize             (GObject          *object);

static void      print_preview_realize              (GtkWidget        *widget);
static void      print_preview_unrealize            (GtkWidget        *widget);
static void      print_preview_get_preferred_width  (GtkWidget        *widget,
                                                     gint             *minimum_width,
                                                     gint             *natural_width);
static void      print_preview_get_preferred_height (GtkWidget        *widget,
                                                     gint             *minimum_height,
                                                     gint             *natural_height);
static void      print_preview_size_allocate        (GtkWidget        *widget,
                                                     GtkAllocation    *allocation);
static gboolean  print_preview_draw                 (GtkWidget        *widget,
                                                     cairo_t          *cr);
static gboolean  print_preview_button_press_event   (GtkWidget        *widget,
                                                     GdkEventButton   *event);
static gboolean  print_preview_button_release_event (GtkWidget        *widget,
                                                     GdkEventButton   *event);
static gboolean  print_preview_motion_notify_event  (GtkWidget        *widget,
                                                     GdkEventMotion   *event);
static gboolean  print_preview_leave_notify_event   (GtkWidget        *widget,
                                                     GdkEventCrossing *event);

static gboolean  print_preview_is_inside            (PrintPreview     *preview,
                                                     gdouble           x,
                                                     gdouble           y);
static void      print_preview_set_inside           (PrintPreview     *preview,
                                                     gboolean          inside);

static gdouble   print_preview_get_scale            (PrintPreview     *preview);

static void      print_preview_get_page_size        (PrintPreview     *preview,
                                                     gdouble          *paper_width,
                                                     gdouble          *paper_height);
static void      print_preview_get_page_margins     (PrintPreview     *preview,
                                                     gdouble          *left_margin,
                                                     gdouble          *right_margin,
                                                     gdouble          *top_margin,
                                                     gdouble          *bottom_margin);
static cairo_surface_t * print_preview_get_thumbnail (gint32           drawable_id,
                                                      gint             width,
                                                      gint             height);


G_DEFINE_TYPE (PrintPreview, print_preview, GTK_TYPE_EVENT_BOX)

#define parent_class print_preview_parent_class

static guint print_preview_signals[LAST_SIGNAL] = { 0 };


#define g_marshal_value_peek_double(v)   (v)->data[0].v_double

static void
marshal_VOID__DOUBLE_DOUBLE (GClosure     *closure,
                             GValue       *return_value,
                             guint         n_param_values,
                             const GValue *param_values,
                             gpointer      invocation_hint,
                             gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__DOUBLE_DOUBLE) (gpointer     data1,
                                                    gdouble      arg_1,
                                                    gdouble      arg_2,
                                                    gpointer     data2);
  register GMarshalFunc_VOID__DOUBLE_DOUBLE callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }

  callback = (GMarshalFunc_VOID__DOUBLE_DOUBLE) (marshal_data ?
                                                 marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_double (param_values + 1),
            g_marshal_value_peek_double (param_values + 2),
            data2);
}

static void
print_preview_class_init (PrintPreviewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  print_preview_signals[OFFSETS_CHANGED] =
    g_signal_new ("offsets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (PrintPreviewClass, offsets_changed),
                  NULL, NULL,
                  marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE);

  object_class->finalize             = print_preview_finalize;

  widget_class->realize              = print_preview_realize;
  widget_class->unrealize            = print_preview_unrealize;
  widget_class->get_preferred_width  = print_preview_get_preferred_width;
  widget_class->get_preferred_height = print_preview_get_preferred_height;
  widget_class->size_allocate        = print_preview_size_allocate;
  widget_class->draw                 = print_preview_draw;
  widget_class->button_press_event   = print_preview_button_press_event;
  widget_class->button_release_event = print_preview_button_release_event;
  widget_class->motion_notify_event  = print_preview_motion_notify_event;
  widget_class->leave_notify_event   = print_preview_leave_notify_event;

  klass->offsets_changed = NULL;
}

static void
print_preview_init (PrintPreview *preview)
{
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (preview), FALSE);

  gtk_widget_add_events (GTK_WIDGET (preview),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);
}


static void
print_preview_finalize (GObject *object)
{
  PrintPreview *preview = PRINT_PREVIEW (object);

  if (preview->thumbnail)
    {
      cairo_surface_destroy (preview->thumbnail);
      preview->thumbnail = NULL;
    }

  if (preview->page)
    {
      g_object_unref (preview->page);
      preview->page = NULL;
    }

  G_OBJECT_CLASS (print_preview_parent_class)->finalize (object);
}

static void
print_preview_realize (GtkWidget *widget)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  GTK_WIDGET_CLASS (print_preview_parent_class)->realize (widget);

  preview->cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                                GDK_HAND1);
}

static void
print_preview_unrealize (GtkWidget *widget)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  if (preview->cursor)
    gdk_cursor_unref (preview->cursor);

  GTK_WIDGET_CLASS (print_preview_parent_class)->unrealize (widget);
}

static void
print_preview_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);
  gdouble       paper_width;
  gdouble       paper_height;
  gint          border;

  border = gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  print_preview_get_page_size (preview, &paper_width, &paper_height);

  if (paper_width > paper_height)
    {
      requisition->height = SIZE_REQUEST;
      requisition->width  = paper_width * SIZE_REQUEST / paper_height;
      requisition->width  = MIN (requisition->width, 2 * SIZE_REQUEST);
    }
  else
    {
      requisition->width  = SIZE_REQUEST;
      requisition->height = paper_height * SIZE_REQUEST / paper_width;
      requisition->height = MIN (requisition->height, 2 * SIZE_REQUEST);
    }

  requisition->width  += 2 * border;
  requisition->height += 2 * border;
}

static void
print_preview_get_preferred_width (GtkWidget *widget,
                                   gint      *minimum_width,
                                   gint      *natural_width)
{
  GtkRequisition requisition;

  print_preview_size_request (widget, &requisition);

  *minimum_width = *natural_width = requisition.width;
}

static void
print_preview_get_preferred_height (GtkWidget *widget,
                                    gint      *minimum_height,
                                    gint      *natural_height)
{
  GtkRequisition requisition;

  print_preview_size_request (widget, &requisition);

  *minimum_height = *natural_height = requisition.height;
}

static void
print_preview_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  GTK_WIDGET_CLASS (print_preview_parent_class)->size_allocate (widget,
                                                                allocation);

  if (preview->thumbnail)
    {
      cairo_surface_destroy (preview->thumbnail);
      preview->thumbnail = NULL;
    }
}

static gboolean
print_preview_button_press_event (GtkWidget      *widget,
                                  GdkEventButton *event)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  if (event->type == GDK_BUTTON_PRESS && event->button == 1 && preview->inside)
    {
      GdkCursor *cursor;

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                           GDK_FLEUR);

      if (gdk_pointer_grab (event->window, FALSE,
                            (GDK_BUTTON1_MOTION_MASK |
                             GDK_BUTTON_RELEASE_MASK),
                            NULL, cursor, event->time) == GDK_GRAB_SUCCESS)
        {
          preview->orig_offset_x = preview->image_offset_x;
          preview->orig_offset_y = preview->image_offset_y;

          preview->start_x = event->x;
          preview->start_y = event->y;

          preview->dragging = TRUE;
        }

      gdk_cursor_unref (cursor);
    }

  return FALSE;
}

static gboolean
print_preview_button_release_event (GtkWidget      *widget,
                                    GdkEventButton *event)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  if (preview->dragging)
    {
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  event->time);
      preview->dragging = FALSE;

      print_preview_set_inside (preview,
                                print_preview_is_inside (preview,
                                                         event->x, event->y));
    }

  return FALSE;
}

static gboolean
print_preview_motion_notify_event (GtkWidget      *widget,
                                   GdkEventMotion *event)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  if (preview->dragging)
    {
      gdouble scale = print_preview_get_scale (preview);
      gdouble offset_x;
      gdouble offset_y;

      offset_x = (preview->orig_offset_x +
                  (event->x - preview->start_x) / scale);
      offset_y = (preview->orig_offset_y +
                  (event->y - preview->start_y) / scale);

      offset_x = CLAMP (offset_x, 0, preview->image_offset_x_max);
      offset_y = CLAMP (offset_y, 0, preview->image_offset_y_max);

      if (preview->image_offset_x != offset_x ||
          preview->image_offset_y != offset_y)
        {
          print_preview_set_image_offsets (preview, offset_x, offset_y);

          g_signal_emit (preview,
                         print_preview_signals[OFFSETS_CHANGED], 0,
                         preview->image_offset_x,
                         preview->image_offset_y);
        }
    }
  else
    {
      print_preview_set_inside (preview,
                                print_preview_is_inside (preview,
                                                         event->x, event->y));
    }

  return FALSE;
}

static gboolean
print_preview_leave_notify_event (GtkWidget        *widget,
                                  GdkEventCrossing *event)
{
  PrintPreview *preview = PRINT_PREVIEW (widget);

  if (event->mode == GDK_CROSSING_NORMAL)
    print_preview_set_inside (preview, FALSE);

  return FALSE;
}

static gboolean
print_preview_draw (GtkWidget *widget,
                    cairo_t   *cr)
{
  PrintPreview  *preview = PRINT_PREVIEW (widget);
  GtkStyle      *style   = gtk_widget_get_style (widget);
  GtkAllocation  allocation;
  gdouble        paper_width;
  gdouble        paper_height;
  gdouble        left_margin;
  gdouble        right_margin;
  gdouble        top_margin;
  gdouble        bottom_margin;
  gdouble        scale;
  gint           border;

  gtk_widget_get_allocation (widget, &allocation);

  border = gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  print_preview_get_page_size (preview, &paper_width, &paper_height);
  print_preview_get_page_margins (preview,
                                  &left_margin, &right_margin,
                                  &top_margin,  &bottom_margin);

  scale = print_preview_get_scale (preview);

  cairo_translate (cr, border, border);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      gint width = allocation.width - 2 * border;

      cairo_translate (cr, width - scale * paper_width, 0);
    }

  cairo_set_line_width (cr, 1.0);

  /* draw page background */
  cairo_rectangle (cr, 0, 0, scale * paper_width, scale * paper_height);

  gdk_cairo_set_source_color (cr, &style->black);
  cairo_stroke_preserve (cr);

  gdk_cairo_set_source_color (cr, &style->white);
  cairo_fill (cr);

  /* draw page_margins */
  cairo_rectangle (cr,
                   scale * left_margin,
                   scale * top_margin,
                   scale * (paper_width - left_margin - right_margin),
                   scale * (paper_height - top_margin - bottom_margin));

  gdk_cairo_set_source_color (cr, &style->mid[gtk_widget_get_state (widget)]);
  cairo_stroke (cr);

  cairo_translate (cr,
                   scale * (left_margin + preview->image_offset_x),
                   scale * (top_margin  + preview->image_offset_y));

  if (preview->dragging || preview->inside)
    {
      cairo_rectangle (cr,
                       0, 0,
                       scale * preview->image_width,
                       scale * preview->image_height);

      gdk_cairo_set_source_color (cr, &style->black);
      cairo_stroke (cr);
    }

  if (preview->thumbnail == NULL &&
      gimp_item_is_valid (preview->drawable_id))
    {
      preview->thumbnail =
        print_preview_get_thumbnail (preview->drawable_id,
                                     MIN (allocation.width,  1024),
                                     MIN (allocation.height, 1024));
    }

  if (preview->thumbnail != NULL)
    {
      gdouble scale_x;
      gdouble scale_y;

      scale_x = (preview->image_width  /
                 cairo_image_surface_get_width (preview->thumbnail));
      scale_y = (preview->image_height /
                 cairo_image_surface_get_height (preview->thumbnail));

      cairo_rectangle (cr, 0, 0, preview->image_width, preview->image_height);

      cairo_scale (cr, scale_x * scale, scale_y * scale);

      cairo_set_source_surface (cr, preview->thumbnail, 0, 0);
      cairo_fill (cr);
    }

  return FALSE;
}

/**
 * print_preview_new:
 * @page: page setup
 * @drawable_id: the drawable to print
 *
 * Creates a new #PrintPreview widget.
 *
 * Return value: the new #PrintPreview widget.
 **/
GtkWidget *
print_preview_new (GtkPageSetup *page,
                   gint32        drawable_id)
{
  PrintPreview *preview;

  g_return_val_if_fail (GTK_IS_PAGE_SETUP (page), NULL);

  preview = g_object_new (PRINT_TYPE_PREVIEW, NULL);

  preview->drawable_id = drawable_id;

  print_preview_set_page_setup (preview, page);

  return GTK_WIDGET (preview);
}

/**
 * print_preview_set_image_dpi:
 * @preview: a #PrintPreview.
 * @xres: the X resolution
 * @yres: the Y resolution
 *
 * Sets the resolution of the image/drawable displayed by the
 * #PrintPreview.
 **/
void
print_preview_set_image_dpi (PrintPreview *preview,
                             gdouble       xres,
                             gdouble       yres)
{
  gdouble width;
  gdouble height;

  g_return_if_fail (PRINT_IS_PREVIEW (preview));
  g_return_if_fail (xres > 0.0 && yres > 0.0);

  width  = gimp_drawable_width  (preview->drawable_id) * 72.0 / xres;
  height = gimp_drawable_height (preview->drawable_id) * 72.0 / yres;

  if (width != preview->image_width || height != preview->image_height)
    {
      preview->image_width  = width;
      preview->image_height = height;

      gtk_widget_queue_draw (GTK_WIDGET (preview));
    }
}

/**
 * print_preview_set_page_setup:
 * @preview: a #PrintPreview.
 * @page: the page setup to use
 *
 * Sets the page setup to use by the #PrintPreview.
 **/
void
print_preview_set_page_setup (PrintPreview *preview,
                              GtkPageSetup *page)
{
  g_return_if_fail (PRINT_IS_PREVIEW (preview));
  g_return_if_fail (GTK_IS_PAGE_SETUP (page));

  if (preview->page)
    g_object_unref (preview->page);

  preview->page = gtk_page_setup_copy (page);

  gtk_widget_queue_resize (GTK_WIDGET (preview));
}

/**
 * print_preview_set_image_offsets:
 * @preview: a #PrintPreview.
 * @offset_x: the X offset
 * @offset_y: the Y offset
 *
 * Sets the offsets of the image/drawable displayed by the #PrintPreview.
 * It does not emit the "offsets-changed" signal.
 **/
void
print_preview_set_image_offsets (PrintPreview *preview,
                                 gdouble       offset_x,
                                 gdouble       offset_y)
{
  g_return_if_fail (PRINT_IS_PREVIEW (preview));

  preview->image_offset_x = offset_x;
  preview->image_offset_y = offset_y;

  gtk_widget_queue_draw (GTK_WIDGET (preview));
}

/**
 * print_preview_set_image_offsets_max:
 * @preview: a #PrintPreview.
 * @offset_x_max: the maximum X offset allowed
 * @offset_y_max: the maximum Y offset allowed
 *
 * Sets the maximum offsets of the image/drawable displayed by the
 * #PrintPreview.  It does not emit the "offsets-changed" signal.
 **/
void
print_preview_set_image_offsets_max (PrintPreview *preview,
                                     gdouble       offset_x_max,
                                     gdouble       offset_y_max)
{
  g_return_if_fail (PRINT_IS_PREVIEW (preview));

  preview->image_offset_x_max = offset_x_max;
  preview->image_offset_y_max = offset_y_max;

  gtk_widget_queue_draw (GTK_WIDGET (preview));
}

/**
 * print_preview_set_use_full_page:
 * @preview: a #PrintPreview.
 * @full_page: TRUE to ignore the page margins
 *
 * If @full_page is TRUE, the page margins are ignored and the full page
 * can be used to setup printing.
 **/
void
print_preview_set_use_full_page (PrintPreview *preview,
                                 gboolean      full_page)
{
  g_return_if_fail (PRINT_IS_PREVIEW (preview));

  preview->use_full_page = full_page;

  gtk_widget_queue_draw (GTK_WIDGET (preview));
}

static gboolean
print_preview_is_inside (PrintPreview *preview,
                         gdouble       x,
                         gdouble       y)
{
  GtkWidget     *widget = GTK_WIDGET (preview);
  GtkAllocation  allocation;
  gdouble        left_margin;
  gdouble        right_margin;
  gdouble        top_margin;
  gdouble        bottom_margin;
  gdouble        scale;
  gint           border;

  gtk_widget_get_allocation (widget, &allocation);

  border = gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  x -= border;

  scale = print_preview_get_scale (preview);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      gdouble paper_width;
      gdouble paper_height;
      gint    width = allocation.width - 2 * border;

      print_preview_get_page_size (preview, &paper_width, &paper_height);

      x -= width - scale * paper_width;
    }

  print_preview_get_page_margins (preview,
                                  &left_margin, &right_margin,
                                  &top_margin,  &bottom_margin);

  x = x / scale - left_margin;
  y = y / scale - top_margin;

  return (x > preview->image_offset_x                        &&
          x < preview->image_offset_x + preview->image_width &&
          y > preview->image_offset_y                        &&
          y < preview->image_offset_y + preview->image_height);
}

static void
print_preview_set_inside (PrintPreview *preview,
                          gboolean      inside)
{
  if (inside != preview->inside)
    {
      GtkWidget *widget = GTK_WIDGET (preview);

      preview->inside = inside;

      if (gtk_widget_is_drawable (widget))
        gdk_window_set_cursor (gtk_widget_get_window (widget),
                               inside ? preview->cursor : NULL);

      gtk_widget_queue_draw (widget);
    }
}

static gdouble
print_preview_get_scale (PrintPreview *preview)
{
  GtkWidget     *widget = GTK_WIDGET (preview);
  GtkAllocation  allocation;
  gdouble        paper_width;
  gdouble        paper_height;
  gdouble        scale_x;
  gdouble        scale_y;
  gint           border;

  gtk_widget_get_allocation (widget, &allocation);

  border = gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  print_preview_get_page_size (preview, &paper_width, &paper_height);

  scale_x = (gdouble) (allocation.width  - 2 * border) / paper_width;
  scale_y = (gdouble) (allocation.height - 2 * border) / paper_height;

  return MIN (scale_x, scale_y);
}

static void
print_preview_get_page_size (PrintPreview *preview,
                             gdouble      *paper_width,
                             gdouble      *paper_height)
{
  *paper_width  = gtk_page_setup_get_paper_width  (preview->page,
                                                   GTK_UNIT_POINTS);
  *paper_height = gtk_page_setup_get_paper_height (preview->page,
                                                   GTK_UNIT_POINTS);
}

static void
print_preview_get_page_margins (PrintPreview *preview,
                                gdouble      *left_margin,
                                gdouble      *right_margin,
                                gdouble      *top_margin,
                                gdouble      *bottom_margin)
{
  if (preview->use_full_page)
    {
      *left_margin   = 0.0;
      *right_margin  = 0.0;
      *top_margin    = 0.0;
      *bottom_margin = 0.0;
    }
  else
    {
      *left_margin   = gtk_page_setup_get_left_margin   (preview->page,
                                                         GTK_UNIT_POINTS);
      *right_margin  = gtk_page_setup_get_right_margin  (preview->page,
                                                         GTK_UNIT_POINTS);
      *top_margin    = gtk_page_setup_get_top_margin    (preview->page,
                                                         GTK_UNIT_POINTS);
      *bottom_margin = gtk_page_setup_get_bottom_margin (preview->page,
                                                         GTK_UNIT_POINTS);
    }
}


/*  This thumbnail code should eventually end up in libgimpui.  */

static cairo_surface_t *
print_preview_get_thumbnail (gint32 drawable_id,
                             gint   width,
                             gint   height)
{
  cairo_surface_t *surface;
  cairo_format_t   format;
  guchar          *data;
  guchar          *dest;
  const guchar    *src;
  gint             src_stride;
  gint             dest_stride;
  gint             y;
  gint             bpp;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_drawable_get_thumbnail_data (drawable_id,
                                           &width, &height, &bpp);

  switch (bpp)
    {
    case 1:
    case 3:
      format = CAIRO_FORMAT_RGB24;
      break;

    case 2:
    case 4:
      format = CAIRO_FORMAT_ARGB32;
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  surface = cairo_image_surface_create (format, width, height);

  src         = data;
  src_stride  = width * bpp;

  dest        = cairo_image_surface_get_data (surface);
  dest_stride = cairo_image_surface_get_stride (surface);

  for (y = 0; y < height; y++)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          w = width;

      switch (bpp)
        {
        case 1:
          while (w--)
            {
              GIMP_CAIRO_RGB24_SET_PIXEL (d, s[0], s[0], s[0]);
              s += 1;
              d += 4;
            }
          break;

        case 2:
          while (w--)
            {
              GIMP_CAIRO_ARGB32_SET_PIXEL (d, s[0], s[0], s[0], s[1]);
              s += 2;
              d += 4;
            }
          break;

        case 3:
          while (w--)
            {
              GIMP_CAIRO_RGB24_SET_PIXEL (d, s[0], s[1], s[2]);
              s += 3;
              d += 4;
            }
          break;

        case 4:
          while (w--)
            {
              GIMP_CAIRO_ARGB32_SET_PIXEL (d, s[0], s[1], s[2], s[3]);
              s += 4;
              d += 4;
            }
          break;
        }

      src  += src_stride;
      dest += dest_stride;
    }

  g_free (data);

  cairo_surface_mark_dirty (surface);

  return surface;
}
