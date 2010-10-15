/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgview.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdnd.h"
#include "gimpfgbgview.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void     gimp_fg_bg_view_dispose           (GObject        *object);
static void     gimp_fg_bg_view_set_property      (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void     gimp_fg_bg_view_get_property      (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);

static gboolean gimp_fg_bg_view_draw              (GtkWidget      *widget,
                                                   cairo_t        *cr);

static void     gimp_fg_bg_view_create_transform  (GimpFgBgView   *view);
static void     gimp_fg_bg_view_destroy_transform (GimpFgBgView   *view);


G_DEFINE_TYPE (GimpFgBgView, gimp_fg_bg_view, GTK_TYPE_WIDGET)

#define parent_class gimp_fg_bg_view_parent_class


static void
gimp_fg_bg_view_class_init (GimpFgBgViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gimp_fg_bg_view_dispose;
  object_class->set_property = gimp_fg_bg_view_set_property;
  object_class->get_property = gimp_fg_bg_view_get_property;

  widget_class->draw         = gimp_fg_bg_view_draw;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_fg_bg_view_init (GimpFgBgView *view)
{
  gtk_widget_set_has_window (GTK_WIDGET (view), FALSE);

  gimp_widget_track_monitor (GTK_WIDGET (view),
                             G_CALLBACK (gimp_fg_bg_view_destroy_transform),
                             NULL);
}

static void
gimp_fg_bg_view_dispose (GObject *object)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  if (view->context)
    gimp_fg_bg_view_set_context (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_fg_bg_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_fg_bg_view_set_context (view, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fg_bg_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, view->context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_fg_bg_view_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GimpFgBgView *view  = GIMP_FG_BG_VIEW (widget);
  GtkStyle     *style = gtk_widget_get_style (widget);
  GtkAllocation allocation;
  gint          rect_w, rect_h;
  GimpRGB       color;

  gtk_widget_get_allocation (widget, &allocation);

  rect_w = allocation.width  * 3 / 4;
  rect_h = allocation.height * 3 / 4;

  if (! view->transform)
    gimp_fg_bg_view_create_transform (view);

  /*  draw the background area  */

  if (view->context)
    {
      gimp_context_get_background (view->context, &color);

      if (view->transform)
        gimp_color_transform_process_pixels (view->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             1);

      gimp_cairo_set_source_rgb (cr, &color);

      cairo_rectangle (cr,
                       allocation.width  - rect_w + 1,
                       allocation.height - rect_h + 1,
                       rect_w - 2,
                       rect_h - 2);
      cairo_fill (cr);
    }

  gtk_paint_shadow (style, cr, GTK_STATE_NORMAL,
                    GTK_SHADOW_IN,
                    widget, NULL,
                    allocation.width  - rect_w,
                    allocation.height - rect_h,
                    rect_w, rect_h);

  /*  draw the foreground area  */

  if (view->context)
    {
      gimp_context_get_foreground (view->context, &color);

      if (view->transform)
        gimp_color_transform_process_pixels (view->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             1);

      gimp_cairo_set_source_rgb (cr, &color);

      cairo_rectangle (cr, 1, 1, rect_w - 2, rect_h - 2);
      cairo_fill (cr);
    }

  gtk_paint_shadow (style, cr, GTK_STATE_NORMAL,
                    GTK_SHADOW_OUT,
                    widget, NULL,
                    0, 0, rect_w, rect_h);

  return TRUE;
}

static void
gimp_fg_bg_view_create_transform (GimpFgBgView *view)
{
  if (view->color_config)
    {
      static GimpColorProfile *profile = NULL;

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      view->transform =
        gimp_widget_get_color_transform (GTK_WIDGET (view),
                                         view->color_config,
                                         profile,
                                         babl_format ("R'G'B'A double"),
                                         babl_format ("R'G'B'A double"));
    }
}

static void
gimp_fg_bg_view_destroy_transform (GimpFgBgView *view)
{
  if (view->transform)
    {
      g_object_unref (view->transform);
      view->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}


/*  public functions  */

GtkWidget *
gimp_fg_bg_view_new (GimpContext *context)
{
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_FG_BG_VIEW,
                       "context", context,
                       NULL);
}

void
gimp_fg_bg_view_set_context (GimpFgBgView *view,
                             GimpContext  *context)
{
  g_return_if_fail (GIMP_IS_FG_BG_VIEW (view));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != view->context)
    {
      if (view->context)
        {
          g_signal_handlers_disconnect_by_func (view->context,
                                                gtk_widget_queue_draw,
                                                view);
          g_object_unref (view->context);
          view->context = NULL;

          g_signal_handlers_disconnect_by_func (view->color_config,
                                                gimp_fg_bg_view_destroy_transform,
                                                view);
          g_object_unref (view->color_config);
          view->color_config = NULL;
        }

      view->context = context;

      if (context)
        {
          g_object_ref (context);

          g_signal_connect_swapped (context, "foreground-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    view);
          g_signal_connect_swapped (context, "background-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    view);

          view->color_config = g_object_ref (context->gimp->config->color_management);

          g_signal_connect_swapped (view->color_config, "notify",
                                    G_CALLBACK (gimp_fg_bg_view_destroy_transform),
                                    view);
        }

      gimp_fg_bg_view_destroy_transform (view);

      g_object_notify (G_OBJECT (view), "context");
    }
}
