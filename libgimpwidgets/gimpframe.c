/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpframe.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <babl/babl.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpframe.h"
#include "gimpwidgetsutils.h"


/**
 * SECTION: gimpframe
 * @title: GimpFrame
 * @short_description: A widget providing a HIG-compliant subclass
 *                     of #GtkFrame.
 *
 * A widget providing a HIG-compliant subclass of #GtkFrame.
 **/


#define DEFAULT_LABEL_SPACING       6
#define DEFAULT_LABEL_BOLD          TRUE

#define GIMP_FRAME_INDENT_KEY       "gimp-frame-indent"
#define GIMP_FRAME_IN_EXPANDER_KEY  "gimp-frame-in-expander"


static void      gimp_frame_style_updated        (GtkWidget      *widget);
static gboolean  gimp_frame_draw                 (GtkWidget      *widget,
                                                  cairo_t        *cr);
static void      gimp_frame_label_widget_notify  (GimpFrame      *frame);
static void      gimp_frame_child_added          (GimpFrame      *frame,
                                                  GtkWidget      *child,
                                                  gpointer        user_data);
static void      gimp_frame_apply_margins        (GimpFrame      *frame);
static gint      gimp_frame_get_indent           (GimpFrame      *frame);
static gint      gimp_frame_get_label_spacing    (GimpFrame      *frame);


G_DEFINE_TYPE (GimpFrame, gimp_frame, GTK_TYPE_FRAME)

#define parent_class gimp_frame_parent_class


static void
gimp_frame_class_init (GimpFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->style_updated = gimp_frame_style_updated;
  widget_class->draw          = gimp_frame_draw;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("label-bold",
                                                                 "Label Bold",
                                                                 "Whether the frame's label should be bold",
                                                                 DEFAULT_LABEL_BOLD,
                                                                 G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("label-spacing",
                                                             "Label Spacing",
                                                             "The spacing between the label and the frame content",
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_LABEL_SPACING,
                                                             G_PARAM_READABLE));
}


static void
gimp_frame_init (GimpFrame *frame)
{
  g_signal_connect (frame, "notify::label-widget",
                    G_CALLBACK (gimp_frame_label_widget_notify),
                    NULL);
  g_signal_connect (frame, "add",
                    G_CALLBACK (gimp_frame_child_added),
                    NULL);
}

static void
gimp_frame_style_updated (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  /*  font changes invalidate the indentation  */
  g_object_set_data (G_OBJECT (widget), GIMP_FRAME_INDENT_KEY, NULL);

  gimp_frame_label_widget_notify (GIMP_FRAME (widget));
  gimp_frame_apply_margins (GIMP_FRAME (widget));
}

static gboolean
gimp_frame_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  GtkWidgetClass *widget_class = g_type_class_peek_parent (parent_class);

  return widget_class->draw (widget, cr);
}

static void
gimp_frame_label_widget_notify (GimpFrame *frame)
{
  GtkWidget *label_widget = gtk_frame_get_label_widget (GTK_FRAME (frame));

  if (label_widget)
    {
      GtkLabel *label = NULL;

      if (GTK_IS_LABEL (label_widget))
        {
          gfloat xalign, yalign;

          label = GTK_LABEL (label_widget);

          gtk_frame_get_label_align (GTK_FRAME (frame), &xalign, &yalign);
          gtk_label_set_xalign (GTK_LABEL (label), xalign);
          gtk_label_set_yalign (GTK_LABEL (label), yalign);
        }
      else if (GTK_IS_BIN (label_widget))
        {
          GtkWidget *child = gtk_bin_get_child (GTK_BIN (label_widget));

          if (GTK_IS_LABEL (child))
            label = GTK_LABEL (child);
        }

      if (label)
        {
          gboolean bold;

          gtk_widget_style_get (GTK_WIDGET (frame),
                                "label-bold", &bold,
                                NULL);

          gimp_label_set_attributes (label,
                                     PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                     -1);
        }
    }
}

static void
gimp_frame_child_added (GimpFrame *frame,
                        GtkWidget *child,
                        gpointer   user_data)
{
  gimp_frame_apply_margins (frame);
}

static void
gimp_frame_apply_margins (GimpFrame *frame)
{
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (frame));

  if (child)
    {
      gtk_widget_set_margin_start (child, gimp_frame_get_indent (frame));
      gtk_widget_set_margin_top (child, gimp_frame_get_label_spacing (frame));
    }
}

static gint
gimp_frame_get_indent (GimpFrame *frame)
{
  gint width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (frame),
                                                   GIMP_FRAME_INDENT_KEY));

  if (! width)
    {
      PangoLayout *layout;

      /*  the HIG suggests to use four spaces so do just that  */
      layout = gtk_widget_create_pango_layout (GTK_WIDGET (frame), "    ");
      pango_layout_get_pixel_size (layout, &width, NULL);
      g_object_unref (layout);

      g_object_set_data (G_OBJECT (frame),
                         GIMP_FRAME_INDENT_KEY, GINT_TO_POINTER (width));
    }

  return width;
}

static gint
gimp_frame_get_label_spacing (GimpFrame *frame)
{
  GtkWidget *label_widget = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gint       spacing      = 0;

  if ((label_widget && gtk_widget_get_visible (label_widget)) ||
      (g_object_get_data (G_OBJECT (frame), GIMP_FRAME_IN_EXPANDER_KEY)))
    {
      gtk_widget_style_get (GTK_WIDGET (frame),
                            "label-spacing", &spacing,
                            NULL);
    }

  return spacing;
}

/**
 * gimp_frame_new:
 * @label: (nullable): text to set as the frame's title label (or %NULL for no title)
 *
 * Creates a #GimpFrame widget. A #GimpFrame is a HIG-compliant
 * variant of #GtkFrame. It doesn't render a frame at all but
 * otherwise behaves like a frame. The frame's title is rendered in
 * bold and the frame content is indented four spaces as suggested by
 * the GNOME HIG (see https://developer.gnome.org/hig/stable/).
 *
 * Returns: a new #GimpFrame widget
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_frame_new (const gchar *label)
{
  GtkWidget *frame;
  gboolean   expander = FALSE;

  /*  somewhat hackish, should perhaps be an object property of GimpFrame  */
  if (label && strcmp (label, "<expander>") == 0)
    {
      expander = TRUE;
      label    = NULL;
    }

  frame = g_object_new (GIMP_TYPE_FRAME,
                        "label", label,
                        NULL);

  if (expander)
    g_object_set_data (G_OBJECT (frame),
                       GIMP_FRAME_IN_EXPANDER_KEY, (gpointer) TRUE);

  return frame;
}
