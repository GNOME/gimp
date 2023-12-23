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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

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

  gtk_widget_class_set_css_name (widget_class, "GimpFgBgView");
}

static void
gimp_fg_bg_view_init (GimpFgBgView *view)
{
  gtk_widget_set_has_window (GTK_WIDGET (view), FALSE);
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
  GimpFgBgView    *view   = GIMP_FG_BG_VIEW (widget);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GtkBorder        border;
  GtkBorder        padding;
  GdkRectangle     rect;
  GeglColor       *color;

  gtk_widget_get_allocation (widget, &allocation);

  gtk_style_context_save (style);

  gtk_style_context_get_border (style, gtk_style_context_get_state (style),
                                &border);
  gtk_style_context_get_padding (style, gtk_style_context_get_state (style),
                                 &padding);

  border.left   += padding.left;
  border.right  += padding.right;
  border.top    += padding.top;
  border.bottom += padding.bottom;

  rect.width  = (allocation.width  - border.left - border.right)  * 3 / 4;
  rect.height = (allocation.height - border.top  - border.bottom) * 3 / 4;

  /*  draw the background area  */

  rect.x = allocation.width  - rect.width  - border.right;
  rect.y = allocation.height - rect.height - border.bottom;

  if (view->context)
    {
      color = gimp_context_get_background (view->context);

      gimp_cairo_set_source_color (cr, color, view->color_config, FALSE, widget);

      cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
      cairo_fill (cr);
    }

  gtk_style_context_add_class (style, GTK_STYLE_CLASS_FRAME);

  gtk_render_frame (style, cr, rect.x, rect.y, rect.width, rect.height);

  /*  draw the foreground area  */

  rect.x = border.left;
  rect.y = border.top;

  if (view->context)
    {
      color = gimp_context_get_foreground (view->context);

      gimp_cairo_set_source_color (cr, color, view->color_config, FALSE, widget);

      cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
      cairo_fill (cr);
    }

  gtk_render_frame (style, cr, rect.x, rect.y, rect.width, rect.height);

  gtk_style_context_restore (style);

  return TRUE;
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
          g_clear_object (&view->context);
          g_clear_object (&view->color_config);
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
        }

      g_object_notify (G_OBJECT (view), "context");
    }
}
