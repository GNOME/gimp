/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafgbgview.c
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmadnd.h"
#include "ligmafgbgview.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void     ligma_fg_bg_view_dispose           (GObject        *object);
static void     ligma_fg_bg_view_set_property      (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void     ligma_fg_bg_view_get_property      (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);

static gboolean ligma_fg_bg_view_draw              (GtkWidget      *widget,
                                                   cairo_t        *cr);

static void     ligma_fg_bg_view_create_transform  (LigmaFgBgView   *view);
static void     ligma_fg_bg_view_destroy_transform (LigmaFgBgView   *view);


G_DEFINE_TYPE (LigmaFgBgView, ligma_fg_bg_view, GTK_TYPE_WIDGET)

#define parent_class ligma_fg_bg_view_parent_class


static void
ligma_fg_bg_view_class_init (LigmaFgBgViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = ligma_fg_bg_view_dispose;
  object_class->set_property = ligma_fg_bg_view_set_property;
  object_class->get_property = ligma_fg_bg_view_get_property;

  widget_class->draw         = ligma_fg_bg_view_draw;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE));

  gtk_widget_class_set_css_name (widget_class, "LigmaFgBgView");
}

static void
ligma_fg_bg_view_init (LigmaFgBgView *view)
{
  gtk_widget_set_has_window (GTK_WIDGET (view), FALSE);

  ligma_widget_track_monitor (GTK_WIDGET (view),
                             G_CALLBACK (ligma_fg_bg_view_destroy_transform),
                             NULL, NULL);
}

static void
ligma_fg_bg_view_dispose (GObject *object)
{
  LigmaFgBgView *view = LIGMA_FG_BG_VIEW (object);

  if (view->context)
    ligma_fg_bg_view_set_context (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_fg_bg_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaFgBgView *view = LIGMA_FG_BG_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      ligma_fg_bg_view_set_context (view, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_fg_bg_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaFgBgView *view = LIGMA_FG_BG_VIEW (object);

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
ligma_fg_bg_view_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  LigmaFgBgView    *view   = LIGMA_FG_BG_VIEW (widget);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GtkBorder        border;
  GtkBorder        padding;
  GdkRectangle     rect;
  LigmaRGB          color;

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

  if (! view->transform)
    ligma_fg_bg_view_create_transform (view);

  /*  draw the background area  */

  rect.x = allocation.width  - rect.width  - border.right;
  rect.y = allocation.height - rect.height - border.bottom;

  if (view->context)
    {
      ligma_context_get_background (view->context, &color);

      if (view->transform)
        ligma_color_transform_process_pixels (view->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             1);

      ligma_cairo_set_source_rgb (cr, &color);

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
      ligma_context_get_foreground (view->context, &color);

      if (view->transform)
        ligma_color_transform_process_pixels (view->transform,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             babl_format ("R'G'B'A double"),
                                             &color,
                                             1);

      ligma_cairo_set_source_rgb (cr, &color);

      cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
      cairo_fill (cr);
    }

  gtk_render_frame (style, cr, rect.x, rect.y, rect.width, rect.height);

  gtk_style_context_restore (style);

  return TRUE;
}

static void
ligma_fg_bg_view_create_transform (LigmaFgBgView *view)
{
  if (view->color_config)
    {
      static LigmaColorProfile *profile = NULL;

      if (G_UNLIKELY (! profile))
        profile = ligma_color_profile_new_rgb_srgb ();

      view->transform =
        ligma_widget_get_color_transform (GTK_WIDGET (view),
                                         view->color_config,
                                         profile,
                                         babl_format ("R'G'B'A double"),
                                         babl_format ("R'G'B'A double"),
                                         NULL,
                                         LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                         FALSE);
    }
}

static void
ligma_fg_bg_view_destroy_transform (LigmaFgBgView *view)
{
  g_clear_object (&view->transform);

  gtk_widget_queue_draw (GTK_WIDGET (view));
}


/*  public functions  */

GtkWidget *
ligma_fg_bg_view_new (LigmaContext *context)
{
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_FG_BG_VIEW,
                       "context", context,
                       NULL);
}

void
ligma_fg_bg_view_set_context (LigmaFgBgView *view,
                             LigmaContext  *context)
{
  g_return_if_fail (LIGMA_IS_FG_BG_VIEW (view));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  if (context != view->context)
    {
      if (view->context)
        {
          g_signal_handlers_disconnect_by_func (view->context,
                                                gtk_widget_queue_draw,
                                                view);
          g_clear_object (&view->context);

          g_signal_handlers_disconnect_by_func (view->color_config,
                                                ligma_fg_bg_view_destroy_transform,
                                                view);
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

          view->color_config = g_object_ref (context->ligma->config->color_management);

          g_signal_connect_swapped (view->color_config, "notify",
                                    G_CALLBACK (ligma_fg_bg_view_destroy_transform),
                                    view);
        }

      ligma_fg_bg_view_destroy_transform (view);

      g_object_notify (G_OBJECT (view), "context");
    }
}
