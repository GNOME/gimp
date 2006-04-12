/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientmenu.c
 * Copyright (C) 1998 Andy Thomas
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


#define GRADIENT_SELECT_DATA_KEY "gimp-gradient-select-data"
#define CELL_HEIGHT              18
#define CELL_WIDTH               84


typedef struct _GradientSelect GradientSelect;

struct _GradientSelect
{
  gchar                   *title;
  GimpRunGradientCallback  callback;
  gpointer                 data;

  GtkWidget               *preview;
  GtkWidget               *button;

  gchar                   *gradient_name;      /* Local copy */
  gint                     sample_size;
  gboolean                 reverse;
  gint                     n_samples;
  gdouble                 *gradient_data;      /* Local copy */

  const gchar             *temp_gradient_callback;
};


/*  local function prototypes  */

static void gimp_gradient_select_widget_callback (const gchar    *name,
                                                  gint            n_samples,
                                                  const gdouble  *gradient_data,
                                                  gboolean        closing,
                                                  gpointer        data);
static void gimp_gradient_select_widget_clicked  (GtkWidget      *widget,
                                                  GradientSelect *gradient_sel);
static void gimp_gradient_select_widget_destroy  (GtkWidget      *widget,
                                                  GradientSelect *gradient_sel);
static void gimp_gradient_select_preview_size_allocate
                                                 (GtkWidget      *widget,
                                                  GtkAllocation  *allocation,
                                                  GradientSelect *gradient_sel);
static void gimp_gradient_select_preview_expose  (GtkWidget      *preview,
                                                  GdkEventExpose *event,
                                                  GradientSelect *gradient_sel);

static void gimp_gradient_select_drag_data_received (GtkWidget        *widget,
                                                     GdkDragContext   *context,
                                                     gint              x,
                                                     gint              y,
                                                     GtkSelectionData *selection,
                                                     guint             info,
                                                     guint             time);


static const GtkTargetEntry target = { "application/x-gimp-gradient-name", 0 };


/**
 * gimp_gradient_select_widget_new:
 * @title:         Title of the dialog to use or %NULL to use the default title.
 * @gradient_name: Initial gradient name or %NULL to use current selection.
 * @callback:      A function to call when the selected gradient changes.
 * @data:          A pointer to arbitary data to be used in the call to
 *                 @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a gradient.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_gradient_select_widget_new (const gchar             *title,
                                 const gchar             *gradient_name,
                                 GimpRunGradientCallback  callback,
                                 gpointer                 data)
{
  GradientSelect *gradient_sel;

  g_return_val_if_fail (callback != NULL, NULL);

  if (! title)
    title = _("Gradient Selection");

  gradient_sel = g_new0 (GradientSelect, 1);

  gradient_sel->title    = g_strdup (title);
  gradient_sel->callback = callback;
  gradient_sel->data     = data;

  gradient_sel->sample_size = CELL_WIDTH;
  gradient_sel->reverse     = FALSE;

  if (! gradient_name || ! strlen (gradient_name))
    gradient_sel->gradient_name = gimp_context_get_gradient ();
  else
    gradient_sel->gradient_name = g_strdup (gradient_name);

  gradient_sel->button = gtk_button_new ();

  g_signal_connect (gradient_sel->button, "clicked",
                    G_CALLBACK (gimp_gradient_select_widget_clicked),
                    gradient_sel);
  g_signal_connect (gradient_sel->button, "destroy",
                    G_CALLBACK (gimp_gradient_select_widget_destroy),
                    gradient_sel);

  gtk_drag_dest_set (GTK_WIDGET (gradient_sel->button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect (gradient_sel->button, "drag-data-received",
                    G_CALLBACK (gimp_gradient_select_drag_data_received),
                    NULL);

  gradient_sel->preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (gradient_sel->preview, CELL_WIDTH, CELL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (gradient_sel->button),
                     gradient_sel->preview);
  gtk_widget_show (gradient_sel->preview);

  g_signal_connect (gradient_sel->preview, "size-allocate",
                    G_CALLBACK (gimp_gradient_select_preview_size_allocate),
                    gradient_sel);

  g_signal_connect (gradient_sel->preview, "expose-event",
                    G_CALLBACK (gimp_gradient_select_preview_expose),
                    gradient_sel);

  g_object_set_data (G_OBJECT (gradient_sel->button),
                     GRADIENT_SELECT_DATA_KEY, gradient_sel);

  return gradient_sel->button;
}


/**
 * gimp_gradient_select_widget_close;
 * @widget: A gradient select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_gradient_select_widget_close (GtkWidget *widget)
{
  GradientSelect *gradient_sel;

  gradient_sel = g_object_get_data (G_OBJECT (widget),
                                    GRADIENT_SELECT_DATA_KEY);

  g_return_if_fail (gradient_sel != NULL);

  if (gradient_sel->temp_gradient_callback)
    {
      gimp_gradient_select_destroy (gradient_sel->temp_gradient_callback);
      gradient_sel->temp_gradient_callback = NULL;
    }
}

/**
 * gimp_gradient_select_widget_set:
 * @widget:        A gradient select widget.
 * @gradient_name: Gradient name to set.
 *
 * Sets the current gradient for the gradient select widget.  Calls
 * the callback function if one was supplied in the call to
 * gimp_gradient_select_widget_new().
 */
void
gimp_gradient_select_widget_set (GtkWidget   *widget,
                                 const gchar *gradient_name)
{
  GradientSelect *gradient_sel;

  gradient_sel = g_object_get_data (G_OBJECT (widget),
                                    GRADIENT_SELECT_DATA_KEY);

  g_return_if_fail (gradient_sel != NULL);

  if (gradient_sel->temp_gradient_callback)
    {
      gimp_gradients_set_popup (gradient_sel->temp_gradient_callback,
                                gradient_name);
    }
  else
    {
      gchar   *name;
      gdouble *samples;
      gint     n_samples;

      if (! gradient_name || ! strlen (gradient_name))
        name = gimp_context_get_gradient ();
      else
        name = g_strdup (gradient_name);

      if (gimp_gradient_get_uniform_samples (name,
                                             gradient_sel->sample_size,
                                             gradient_sel->reverse,
                                             &n_samples,
                                             &samples))
        {
          gimp_gradient_select_widget_callback (name,
                                                n_samples, samples,
                                                FALSE, gradient_sel);

          g_free (samples);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_gradient_select_widget_callback (const gchar   *name,
                                      gint           n_samples,
                                      const gdouble *gradient_data,
                                      gboolean       closing,
                                      gpointer       data)
{
  GradientSelect *gradient_sel = data;

  g_free (gradient_sel->gradient_name);
  g_free (gradient_sel->gradient_data);

  gradient_sel->gradient_name = g_strdup (name);
  gradient_sel->n_samples     = n_samples;
  gradient_sel->gradient_data = g_memdup (gradient_data,
                                          n_samples * sizeof (gdouble));

  gtk_widget_queue_draw (gradient_sel->preview);

  if (gradient_sel->callback)
    gradient_sel->callback (name,
                            n_samples, gradient_data, closing,
                            gradient_sel->data);

  if (closing)
    gradient_sel->temp_gradient_callback = NULL;
}

static void
gimp_gradient_select_widget_clicked (GtkWidget      *widget,
                                     GradientSelect *gradient_sel)
{
  if (gradient_sel->temp_gradient_callback)
    {
      /*  calling gimp_gradients_set_popup() raises the dialog  */
      gimp_gradients_set_popup (gradient_sel->temp_gradient_callback,
                                gradient_sel->gradient_name);
    }
  else
    {
      gradient_sel->temp_gradient_callback =
        gimp_gradient_select_new (gradient_sel->title,
                                  gradient_sel->gradient_name,
                                  gradient_sel->sample_size,
                                  gimp_gradient_select_widget_callback,
                                  gradient_sel);
    }
}

static void
gimp_gradient_select_widget_destroy (GtkWidget      *widget,
                                     GradientSelect *gradient_sel)
{
  if (gradient_sel->temp_gradient_callback)
    {
      gimp_gradient_select_destroy (gradient_sel->temp_gradient_callback);
      gradient_sel->temp_gradient_callback = NULL;
    }

  g_free (gradient_sel->title);
  g_free (gradient_sel->gradient_name);
  g_free (gradient_sel->gradient_data);
  g_free (gradient_sel);
}

static void
gimp_gradient_select_preview_size_allocate (GtkWidget      *widget,
                                            GtkAllocation  *allocation,
                                            GradientSelect *gradient_sel)
{
  gdouble *samples;
  gint     n_samples;

  if (gimp_gradient_get_uniform_samples (gradient_sel->gradient_name,
                                         allocation->width,
                                         gradient_sel->reverse,
                                         &n_samples,
                                         &samples))
    {
      g_free (gradient_sel->gradient_data);

      gradient_sel->sample_size   = allocation->width;
      gradient_sel->n_samples     = n_samples;
      gradient_sel->gradient_data = samples;
    }
}

static void
gimp_gradient_select_preview_expose (GtkWidget      *widget,
                                     GdkEventExpose *event,
                                     GradientSelect *gradient_sel)
{
  const gdouble *src;
  guchar        *p0;
  guchar        *p1;
  guchar        *even;
  guchar        *odd;
  gint           width;
  gint           x, y;

  src = gradient_sel->gradient_data;
  if (! src)
    return;

  width = gradient_sel->n_samples / 4;

  p0 = even = g_malloc (width * 3);
  p1 = odd  = g_malloc (width * 3);

  for (x = 0; x < width; x++)
    {
      gdouble  r, g, b, a;
      gdouble  c0, c1;

      r = src[x * 4 + 0];
      g = src[x * 4 + 1];
      b = src[x * 4 + 2];
      a = src[x * 4 + 3];

      if ((x / GIMP_CHECK_SIZE_SM) & 1)
        {
          c0 = GIMP_CHECK_LIGHT;
          c1 = GIMP_CHECK_DARK;
        }
      else
        {
          c0 = GIMP_CHECK_DARK;
          c1 = GIMP_CHECK_LIGHT;
        }

      *p0++ = (c0 + (r - c0) * a) * 255.0;
      *p0++ = (c0 + (g - c0) * a) * 255.0;
      *p0++ = (c0 + (b - c0) * a) * 255.0;

      *p1++ = (c1 + (r - c1) * a) * 255.0;
      *p1++ = (c1 + (g - c1) * a) * 255.0;
      *p1++ = (c1 + (b - c1) * a) * 255.0;
    }

  for (y = event->area.y; y < event->area.y + event->area.height; y++)
    {
      guchar *buf = ((y / GIMP_CHECK_SIZE_SM) & 1) ? odd : even;

      gdk_draw_rgb_image_dithalign (widget->window,
                                    widget->style->fg_gc[widget->state],
                                    event->area.x, y,
                                    event->area.width, 1,
                                    GDK_RGB_DITHER_MAX,
                                    buf + event->area.x * 3,
                                    width * 3,
                                    - event->area.x, - y);
    }

  g_free (odd);
  g_free (even);
}

static void
gimp_gradient_select_drag_data_received (GtkWidget        *widget,
                                         GdkDragContext   *context,
                                         gint              x,
                                         gint              y,
                                         GtkSelectionData *selection,
                                         guint             info,
                                         guint             time)
{
  gchar *str;

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid gradient data!");
      return;
    }

  str = g_strndup ((const gchar *) selection->data, selection->length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_gradient_select_widget_set (widget, name);
        }
    }

  g_free (str);
}
