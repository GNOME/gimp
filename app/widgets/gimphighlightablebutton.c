/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphighlightablebutton.c
 * Copyright (C) 2018 Ell
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

#include "core/gimp-cairo.h"

#include "gimphighlightablebutton.h"


#define DEFAULT_HIGHLIGHT_COLOR {0.20, 0.70, 0.20, 0.65}

#define PADDING       1
#define BORDER_WIDTH  1
#define CORNER_RADIUS 2

#define REV (2.0 * G_PI)


enum
{
  PROP_0,
  PROP_HIGHLIGHT,
  PROP_HIGHLIGHT_COLOR
};


struct _GimpHighlightableButtonPrivate
{
  gboolean highlight;
  GimpRGB  highlight_color;
};


static void       gimp_highlightable_button_set_property (GObject      *object,
                                                          guint         property_id,
                                                          const GValue *value,
                                                          GParamSpec   *pspec);
static void       gimp_highlightable_button_get_property (GObject      *object,
                                                          guint         property_id,
                                                          GValue       *value,
                                                          GParamSpec   *pspec);

static gboolean   gimp_highlightable_button_draw         (GtkWidget    *widget,
                                                          cairo_t      *cr);


G_DEFINE_TYPE (GimpHighlightableButton, gimp_highlightable_button, GIMP_TYPE_BUTTON)

#define parent_class gimp_highlightable_button_parent_class


static void
gimp_highlightable_button_class_init (GimpHighlightableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gimp_highlightable_button_get_property;
  object_class->set_property = gimp_highlightable_button_set_property;

  widget_class->draw         = gimp_highlightable_button_draw;

  g_object_class_install_property (object_class, PROP_HIGHLIGHT,
                                   g_param_spec_boolean ("highlight",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGHLIGHT_COLOR,
                                   gimp_param_spec_rgb ("highlight-color",
                                                        NULL, NULL,
                                                        TRUE,
                                                        &(GimpRGB) DEFAULT_HIGHLIGHT_COLOR,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (klass, sizeof (GimpHighlightableButtonPrivate));
}

static void
gimp_highlightable_button_init (GimpHighlightableButton *button)
{
  button->priv = G_TYPE_INSTANCE_GET_PRIVATE (button,
                                              GIMP_TYPE_HIGHLIGHTABLE_BUTTON,
                                              GimpHighlightableButtonPrivate);
}

static void
gimp_highlightable_button_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpHighlightableButton *button = GIMP_HIGHLIGHTABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      gimp_highlightable_button_set_highlight (button,
                                               g_value_get_boolean (value));
      break;

    case PROP_HIGHLIGHT_COLOR:
      gimp_highlightable_button_set_highlight_color (button,
                                                     g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_highlightable_button_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpHighlightableButton *button = GIMP_HIGHLIGHTABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, button->priv->highlight);
      break;

    case PROP_HIGHLIGHT_COLOR:
      g_value_set_boxed (value, &button->priv->highlight_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_highlightable_button_draw (GtkWidget *widget,
                                cairo_t   *cr)
{
  GimpHighlightableButton *button = GIMP_HIGHLIGHTABLE_BUTTON (widget);
  GtkStyle                *style  = gtk_widget_get_style (widget);
  GtkStateType             state  = gtk_widget_get_state (widget);
  GtkAllocation            allocation;
  gboolean                 border;
  gdouble                  lightness;
  gdouble                  opacity;
  gdouble                  x;
  gdouble                  y;
  gdouble                  width;
  gdouble                  height;

  if (! button->priv->highlight)
    return GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  gtk_widget_get_allocation (widget, &allocation);

  border =
    (state                                       == GTK_STATE_ACTIVE   ||
     state                                       == GTK_STATE_PRELIGHT ||
     gtk_button_get_relief (GTK_BUTTON (button)) == GTK_RELIEF_NORMAL);

  lightness = 1.00;
  opacity   = 1.00;

  switch (state)
    {
    case GTK_STATE_ACTIVE:      lightness = 0.80; break;
    case GTK_STATE_PRELIGHT:    lightness = 1.25; break;
    case GTK_STATE_INSENSITIVE: opacity   = 0.50; break;
    default:                                      break;
    }

  x      = allocation.x      +       PADDING;
  y      = allocation.y      +       PADDING;
  width  = allocation.width  - 2.0 * PADDING;
  height = allocation.height - 2.0 * PADDING;

  if (border)
    {
      x      += BORDER_WIDTH / 2.0;
      y      += BORDER_WIDTH / 2.0;
      width  -= BORDER_WIDTH;
      height -= BORDER_WIDTH;
    }

  cairo_set_source_rgba (cr,
                         button->priv->highlight_color.r * lightness,
                         button->priv->highlight_color.g * lightness,
                         button->priv->highlight_color.b * lightness,
                         button->priv->highlight_color.a * opacity);

  gimp_cairo_rounded_rectangle (cr,
                                x, y, width, height, CORNER_RADIUS);

  cairo_fill_preserve (cr);

  if (border)
    {
      gdk_cairo_set_source_color (cr, &style->fg[state]);

      cairo_set_line_width (cr, BORDER_WIDTH);

      cairo_stroke (cr);
    }

  if (gtk_widget_has_focus (widget))
    {
      gboolean interior_focus;
      gint     focus_width;
      gint     focus_pad;
      gint     child_displacement_x;
      gint     child_displacement_y;
      gboolean displace_focus;
      gint     x;
      gint     y;
      gint     width;
      gint     height;

      gtk_widget_style_get (widget,
                            "interior-focus",       &interior_focus,
                            "focus-line-width",     &focus_width,
                            "focus-padding",        &focus_pad,
                            "child-displacement-y", &child_displacement_y,
                            "child-displacement-x", &child_displacement_x,
                            "displace-focus",       &displace_focus,
                            NULL);

      x      = allocation.x      +     PADDING;
      y      = allocation.y      +     PADDING;
      width  = allocation.width  - 2 * PADDING;
      height = allocation.height - 2 * PADDING;

      if (interior_focus)
        {
          x      += style->xthickness + focus_pad;
          y      += style->ythickness + focus_pad;
          width  -= 2 * (style->xthickness + focus_pad);
          height -= 2 * (style->ythickness + focus_pad);
        }
      else
        {
          x      -= focus_width + focus_pad;
          y      -= focus_width + focus_pad;
          width  += 2 * (focus_width + focus_pad);
          height += 2 * (focus_width + focus_pad);
        }

      if (state == GTK_STATE_ACTIVE && displace_focus)
        {
          x += child_displacement_x;
          y += child_displacement_y;
        }

      gtk_paint_focus (style, cr, state,
                       widget, "button",
                       x, y, width, height);
    }

  gtk_container_propagate_draw (GTK_CONTAINER (button),
                                gtk_bin_get_child (GTK_BIN (button)),
                                cr);

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_highlightable_button_new (void)
{
  return g_object_new (GIMP_TYPE_HIGHLIGHTABLE_BUTTON, NULL);
}

void
gimp_highlightable_button_set_highlight (GimpHighlightableButton *button,
                                         gboolean                 highlight)
{
  g_return_if_fail (GIMP_IS_HIGHLIGHTABLE_BUTTON (button));

  if (button->priv->highlight != highlight)
    {
      button->priv->highlight = highlight;

      gtk_widget_queue_draw (GTK_WIDGET (button));

      g_object_notify (G_OBJECT (button), "highlight");
    }
}

gboolean
gimp_highlightable_button_get_highlight (GimpHighlightableButton *button)
{
  g_return_val_if_fail (GIMP_IS_HIGHLIGHTABLE_BUTTON (button), FALSE);

  return button->priv->highlight;
}

void
gimp_highlightable_button_set_highlight_color (GimpHighlightableButton *button,
                                               const GimpRGB           *color)
{
  g_return_if_fail (GIMP_IS_HIGHLIGHTABLE_BUTTON (button));
  g_return_if_fail (color != NULL);

  if (memcmp (&button->priv->highlight_color, color, sizeof (GimpRGB)))
    {
      button->priv->highlight_color = *color;

      if (button->priv->highlight)
        gtk_widget_queue_draw (GTK_WIDGET (button));

      g_object_notify (G_OBJECT (button), "highlight-color");
    }
}

void
gimp_highlightable_button_get_highlight_color (GimpHighlightableButton *button,
                                               GimpRGB                 *color)
{
  g_return_if_fail (GIMP_IS_HIGHLIGHTABLE_BUTTON (button));
  g_return_if_fail (color != NULL);

  *color = button->priv->highlight_color;
}
