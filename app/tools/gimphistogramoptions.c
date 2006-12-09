/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-utils.h"

#include "widgets/gimphistogramview.h"

#include "gimphistogramoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SCALE
};


static void   gimp_histogram_options_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   gimp_histogram_options_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);


G_DEFINE_TYPE (GimpHistogramOptions, gimp_histogram_options,
               GIMP_TYPE_COLOR_OPTIONS)


static void
gimp_histogram_options_class_init (GimpHistogramOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_histogram_options_set_property;
  object_class->get_property = gimp_histogram_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_SCALE,
                                 "histogram-scale", NULL,
                                 GIMP_TYPE_HISTOGRAM_SCALE,
                                 GIMP_HISTOGRAM_SCALE_LINEAR,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_histogram_options_init (GimpHistogramOptions *options)
{
}

static void
gimp_histogram_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpHistogramOptions *options = GIMP_HISTOGRAM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SCALE:
      options->scale = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpHistogramOptions *options = GIMP_HISTOGRAM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SCALE:
      g_value_set_enum (value, options->scale);
      break;
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_histogram_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *frame;

  frame = gimp_prop_enum_radio_frame_new (config, "histogram-scale",
                                          _("Histogram Scale"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  return vbox;
}

void
gimp_histogram_options_connect_view (GimpHistogramOptions *options,
                                     GimpHistogramView    *view)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_OPTIONS (options));
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  gimp_config_connect (G_OBJECT (options), G_OBJECT (view), "histogram-scale");

  g_object_notify (G_OBJECT (options), "histogram-scale");
}
