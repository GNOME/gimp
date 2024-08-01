/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcellrenderertoggle.c
 * Copyright (C) 2003-2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpwidgetsmarshal.h"
#include "gimpcellrenderertoggle.h"


/**
 * SECTION: gimpcellrenderertoggle
 * @title: GimpCellRendererToggle
 * @short_description: A #GtkCellRendererToggle that displays icons instead
 *                     of a checkbox.
 *
 * A #GtkCellRendererToggle that displays icons instead of a checkbox.
 **/


#define DEFAULT_ICON_SIZE 16


enum
{
  CLICKED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ICON_NAME,
  PROP_ICON_SIZE,
  PROP_OVERRIDE_BACKGROUND
};


typedef struct _GimpCellRendererTogglePrivate
{
  gchar       *icon_name;
  gint         icon_size;
  gboolean     override_background;

  GdkPixbuf   *pixbuf;
} GimpCellRendererTogglePrivate;


static void gimp_cell_renderer_toggle_finalize     (GObject              *object);
static void gimp_cell_renderer_toggle_get_property (GObject              *object,
                                                    guint                 param_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);
static void gimp_cell_renderer_toggle_set_property (GObject              *object,
                                                    guint                 param_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);
static void gimp_cell_renderer_toggle_get_size     (GtkCellRenderer      *cell,
                                                    GtkWidget            *widget,
                                                    const GdkRectangle   *rectangle,
                                                    gint                 *x_offset,
                                                    gint                 *y_offset,
                                                    gint                 *width,
                                                    gint                 *height);
static void gimp_cell_renderer_toggle_render       (GtkCellRenderer      *cell,
                                                    cairo_t              *cr,
                                                    GtkWidget            *widget,
                                                    const GdkRectangle   *background_area,
                                                    const GdkRectangle   *cell_area,
                                                    GtkCellRendererState  flags);
static gboolean gimp_cell_renderer_toggle_activate (GtkCellRenderer      *cell,
                                                    GdkEvent             *event,
                                                    GtkWidget            *widget,
                                                    const gchar          *path,
                                                    const GdkRectangle   *background_area,
                                                    const GdkRectangle   *cell_area,
                                                    GtkCellRendererState  flags);
static void gimp_cell_renderer_toggle_create_pixbuf (GimpCellRendererToggle *toggle,
                                                     GtkWidget              *widget);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCellRendererToggle, gimp_cell_renderer_toggle,
                            GTK_TYPE_CELL_RENDERER_TOGGLE)

#define parent_class gimp_cell_renderer_toggle_parent_class

static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };


static void
gimp_cell_renderer_toggle_class_init (GimpCellRendererToggleClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  toggle_cell_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpCellRendererToggleClass, clicked),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__STRING_FLAGS,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  GDK_TYPE_MODIFIER_TYPE);

  object_class->finalize     = gimp_cell_renderer_toggle_finalize;
  object_class->get_property = gimp_cell_renderer_toggle_get_property;
  object_class->set_property = gimp_cell_renderer_toggle_set_property;

  cell_class->get_size       = gimp_cell_renderer_toggle_get_size;
  cell_class->render         = gimp_cell_renderer_toggle_render;
  cell_class->activate       = gimp_cell_renderer_toggle_activate;

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        "Icon Name",
                                                        "The icon to display",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ICON_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     "Icon Size",
                                                     "The desired icon size to use in pixel (before applying scaling factor)",
                                                     0, G_MAXINT,
                                                     DEFAULT_ICON_SIZE,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OVERRIDE_BACKGROUND,
                                   g_param_spec_boolean ("override-background",
                                                         "Override Background",
                                                         "Draw the background if the row is selected",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_cell_renderer_toggle_init (GimpCellRendererToggle *toggle)
{
}

static void
gimp_cell_renderer_toggle_finalize (GObject *object)
{
  GimpCellRendererToggle        *toggle = GIMP_CELL_RENDERER_TOGGLE (object);
  GimpCellRendererTogglePrivate *priv   = gimp_cell_renderer_toggle_get_instance_private (toggle);

  g_clear_pointer (&priv->icon_name, g_free);
  g_clear_object (&priv->pixbuf);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cell_renderer_toggle_get_property (GObject    *object,
                                        guint       param_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpCellRendererToggle        *toggle = GIMP_CELL_RENDERER_TOGGLE (object);
  GimpCellRendererTogglePrivate *priv   = gimp_cell_renderer_toggle_get_instance_private (toggle);

  switch (param_id)
    {
    case PROP_ICON_NAME:
      g_value_set_string (value, priv->icon_name);
      break;

    case PROP_ICON_SIZE:
      g_value_set_int (value, priv->icon_size);
      break;

    case PROP_OVERRIDE_BACKGROUND:
      g_value_set_boolean (value, priv->override_background);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_cell_renderer_toggle_set_property (GObject      *object,
                                        guint         param_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpCellRendererToggle        *toggle = GIMP_CELL_RENDERER_TOGGLE (object);
  GimpCellRendererTogglePrivate *priv   = gimp_cell_renderer_toggle_get_instance_private (toggle);

  switch (param_id)
    {
    case PROP_ICON_NAME:
      if (priv->icon_name)
        g_free (priv->icon_name);
      priv->icon_name = g_value_dup_string (value);
      break;

    case PROP_ICON_SIZE:
      priv->icon_size = g_value_get_int (value);
      break;

    case PROP_OVERRIDE_BACKGROUND:
      priv->override_background = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

  g_clear_object (&priv->pixbuf);
}

static void
gimp_cell_renderer_toggle_get_size (GtkCellRenderer    *cell,
                                    GtkWidget          *widget,
                                    const GdkRectangle *cell_area,
                                    gint               *x_offset,
                                    gint               *y_offset,
                                    gint               *width,
                                    gint               *height)
{
  GimpCellRendererToggle        *toggle  = GIMP_CELL_RENDERER_TOGGLE (cell);
  GimpCellRendererTogglePrivate *priv    = gimp_cell_renderer_toggle_get_instance_private (toggle);
  GtkStyleContext               *context = gtk_widget_get_style_context (widget);
  GtkBorder                      border;
  gint                           scale_factor;
  gint                           calc_width;
  gint                           calc_height;
  gint                           pixbuf_width;
  gint                           pixbuf_height;
  gfloat                         xalign;
  gfloat                         yalign;
  gint                           xpad;
  gint                           ypad;

  scale_factor = gtk_widget_get_scale_factor (widget);

  if (! priv->icon_name)
    {
      GTK_CELL_RENDERER_CLASS (parent_class)->get_size (cell,
                                                        widget,
                                                        cell_area,
                                                        x_offset, y_offset,
                                                        width, height);
      return;
    }

  gtk_style_context_save (context);

  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  if (! priv->pixbuf)
    gimp_cell_renderer_toggle_create_pixbuf (toggle, widget);

  pixbuf_width  = gdk_pixbuf_get_width  (priv->pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (priv->pixbuf);

  /* The pixbuf size may be bigger than the logical size. */
  calc_width  = pixbuf_width / scale_factor + (gint) xpad * 2;
  calc_height = pixbuf_height / scale_factor + (gint) ypad * 2;

  gtk_style_context_add_class (context, GTK_STYLE_CLASS_BUTTON);

  gtk_style_context_get_border (context, 0, &border);
  calc_width  += border.left + border.right;
  calc_height += border.top + border.bottom;

  if (width)
    *width  = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
    {
      if (x_offset)
        {
          *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        (1.0 - xalign) : xalign) *
                       (cell_area->width - calc_width));
          *x_offset = MAX (*x_offset, 0);
        }

      if (y_offset)
        {
          *y_offset = yalign * (cell_area->height - calc_height);
          *y_offset = MAX (*y_offset, 0);
        }
    }

  gtk_style_context_restore (context);
}

static void
gimp_cell_renderer_toggle_render (GtkCellRenderer      *cell,
                                  cairo_t              *cr,
                                  GtkWidget            *widget,
                                  const GdkRectangle   *background_area,
                                  const GdkRectangle   *cell_area,
                                  GtkCellRendererState  flags)
{
  GimpCellRendererToggle        *toggle  = GIMP_CELL_RENDERER_TOGGLE (cell);
  GimpCellRendererTogglePrivate *priv    = gimp_cell_renderer_toggle_get_instance_private (toggle);
  GtkStyleContext               *context = gtk_widget_get_style_context (widget);
  GdkRectangle                   toggle_rect;
  GtkStateFlags                  state;
  gboolean                       active;
  gint                           scale_factor;
  gint                           xpad;
  gint                           ypad;

  scale_factor = gtk_widget_get_scale_factor (widget);

  if (! priv->icon_name)
    {
      GTK_CELL_RENDERER_CLASS (parent_class)->render (cell, cr, widget,
                                                      background_area,
                                                      cell_area,
                                                      flags);
      return;
    }

  if ((flags & GTK_CELL_RENDERER_SELECTED) &&
      priv->override_background)
    {
      gboolean background_set;

      g_object_get (cell,
                    "cell-background-set", &background_set,
                    NULL);

      if (background_set)
        {
          GdkRGBA *color;

          g_object_get (cell,
                        "cell-background-rgba", &color,
                        NULL);

          gdk_cairo_rectangle (cr, background_area);
          gdk_cairo_set_source_rgba (cr, color);
          cairo_fill (cr);

          gdk_rgba_free (color);
        }
    }

  gimp_cell_renderer_toggle_get_size (cell, widget, cell_area,
                                      &toggle_rect.x,
                                      &toggle_rect.y,
                                      &toggle_rect.width,
                                      &toggle_rect.height);
  gtk_style_context_save (context);

  gtk_style_context_add_class (context, GTK_STYLE_CLASS_BUTTON);
  gtk_style_context_add_class (context, "toggle-icon");

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  toggle_rect.x      += cell_area->x + xpad;
  toggle_rect.y      += cell_area->y + ypad;
  toggle_rect.width  -= xpad * 2;
  toggle_rect.height -= ypad * 2;

  if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
    return;

  state = gtk_cell_renderer_get_state (cell, widget, flags);

  active =
    gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (cell));

  if (active)
    {
      gtk_style_context_add_class (context, "visible");
      state |= GTK_STATE_FLAG_ACTIVE;
    }

  if (! gtk_cell_renderer_toggle_get_activatable (GTK_CELL_RENDERER_TOGGLE (cell)))
    state |= GTK_STATE_FLAG_INSENSITIVE;

  gtk_style_context_set_state (context, state);

  if (state & GTK_STATE_FLAG_PRELIGHT)
    gtk_render_frame (context, cr,
                      toggle_rect.x,     toggle_rect.y,
                      toggle_rect.width, toggle_rect.height);

  if (active)
    {
      GtkBorder border;
      gboolean  inconsistent;

      gtk_style_context_get_border (context, 0, &border);
      toggle_rect.x      += border.left;
      toggle_rect.x      *= scale_factor;
      toggle_rect.y      += border.top;
      toggle_rect.y      *= scale_factor;
      toggle_rect.width  -= border.left + border.right;
      toggle_rect.height -= border.top + border.bottom;

      /* For high DPI displays, pixbuf size is bigger than logical size. */
      cairo_scale (cr, (gdouble) 1.0 / scale_factor, (gdouble) 1.0 / scale_factor);
      gdk_cairo_set_source_pixbuf (cr, priv->pixbuf,
                                   toggle_rect.x, toggle_rect.y);
      cairo_paint (cr);

      g_object_get (cell,
                    "inconsistent", &inconsistent,
                    NULL);

      if (inconsistent)
        {
          GdkRGBA color;

          gtk_style_context_get_color (context, state, &color);
          gdk_cairo_set_source_rgba (cr, &color);
          cairo_set_line_width (cr, scale_factor * 1.5);
          cairo_move_to (cr,
                         toggle_rect.x + scale_factor * (toggle_rect.width - 1),
                         toggle_rect.y + scale_factor);
          cairo_line_to (cr,
                         toggle_rect.x + scale_factor,
                         toggle_rect.y + scale_factor * (toggle_rect.height - 1));
          cairo_stroke (cr);
        }
    }

  gtk_style_context_restore (context);
}

static gboolean
gimp_cell_renderer_toggle_activate (GtkCellRenderer      *cell,
                                    GdkEvent             *event,
                                    GtkWidget            *widget,
                                    const gchar          *path,
                                    const GdkRectangle   *background_area,
                                    const GdkRectangle   *cell_area,
                                    GtkCellRendererState  flags)
{
  GtkCellRendererToggle  *toggle = GTK_CELL_RENDERER_TOGGLE (cell);

  if (gtk_cell_renderer_toggle_get_activatable (toggle))
    {
      GdkModifierType state = 0;

      if (event && ((GdkEventAny *) event)->type == GDK_BUTTON_PRESS)
        state = ((GdkEventButton *) event)->state;

      gimp_cell_renderer_toggle_clicked (GIMP_CELL_RENDERER_TOGGLE (cell),
                                         path, state);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_cell_renderer_toggle_create_pixbuf (GimpCellRendererToggle *toggle,
                                         GtkWidget              *widget)
{
  GimpCellRendererTogglePrivate *priv = gimp_cell_renderer_toggle_get_instance_private (toggle);

  g_clear_object (&priv->pixbuf);

  if (priv->icon_name)
    {
      GdkPixbuf    *pixbuf;
      GdkScreen    *screen;
      GtkIconTheme *icon_theme;
      GtkIconInfo  *icon_info;
      gchar        *icon_name;
      gint          scale_factor;

      scale_factor = gtk_widget_get_scale_factor (widget);
      screen       = gtk_widget_get_screen (widget);
      icon_theme   = gtk_icon_theme_get_for_screen (screen);

      /* Look for symbolic and fallback to color icon. */
      icon_name = g_strdup_printf ("%s-symbolic", priv->icon_name);
      icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme, icon_name,
                                                        priv->icon_size, scale_factor,
                                                        GTK_ICON_LOOKUP_GENERIC_FALLBACK);

      g_free (icon_name);
      if (! icon_info)
        {
          icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme, "image-missing",
                                                            priv->icon_size, scale_factor,
                                                            GTK_ICON_LOOKUP_GENERIC_FALLBACK |
                                                            GTK_ICON_LOOKUP_USE_BUILTIN);
        }

      g_return_if_fail (icon_info != NULL);

      pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info,
                                                        gtk_widget_get_style_context (widget),
                                                        NULL, NULL);
      priv->pixbuf = pixbuf;
      g_object_unref (icon_info);
    }
}


/**
 * gimp_cell_renderer_toggle_new:
 * @icon_name: the icon name of the icon to use for the active state
 *
 * Creates a custom version of the #GtkCellRendererToggle. Instead of
 * showing the standard toggle button, it shows a named icon if the
 * cell is active and no icon otherwise. This cell renderer is for
 * example used in the Layers treeview to indicate and control the
 * layer's visibility by showing %GIMP_STOCK_VISIBLE.
 *
 * Returns: a new #GimpCellRendererToggle
 *
 * Since: 2.2
 **/
GtkCellRenderer *
gimp_cell_renderer_toggle_new (const gchar *icon_name)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_TOGGLE,
                       "icon-name", icon_name,
                       NULL);
}

/**
 * gimp_cell_renderer_toggle_clicked:
 * @cell:  a #GimpCellRendererToggle
 * @path:  the path to the clicked row
 * @state: the modifier state
 *
 * Emits the "clicked" signal from a #GimpCellRendererToggle.
 *
 * Since: 2.2
 **/
void
gimp_cell_renderer_toggle_clicked (GimpCellRendererToggle *cell,
                                   const gchar            *path,
                                   GdkModifierType         state)
{
  g_return_if_fail (GIMP_IS_CELL_RENDERER_TOGGLE (cell));
  g_return_if_fail (path != NULL);

  g_signal_emit (cell, toggle_cell_signals[CLICKED], 0, path, state);
}
