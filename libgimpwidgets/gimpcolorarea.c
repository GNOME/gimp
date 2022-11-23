/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorarea.c
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmacairo-utils.h"
#include "ligmacolorarea.h"
#include "ligmawidgetsutils.h"
#include "ligmawidgets-private.h"


/**
 * SECTION: ligmacolorarea
 * @title: LigmaColorArea
 * @short_description: Displays a #LigmaRGB color, optionally with
 *                     alpha-channel.
 *
 * Displays a #LigmaRGB color, optionally with alpha-channel.
 **/


#define DRAG_PREVIEW_SIZE   32
#define DRAG_ICON_OFFSET    -8


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_COLOR,
  PROP_TYPE,
  PROP_DRAG_MASK,
  PROP_DRAW_BORDER
};


struct _LigmaColorAreaPrivate
{
  LigmaColorConfig    *config;
  LigmaColorTransform *transform;

  guchar             *buf;
  guint               width;
  guint               height;
  guint               rowstride;

  LigmaColorAreaType   type;
  LigmaRGB             color;
  guint               draw_border  : 1;
  guint               needs_render : 1;

  gboolean            out_of_gamut;
};

#define GET_PRIVATE(obj) (((LigmaColorArea *) (obj))->priv)


static void      ligma_color_area_dispose             (GObject           *object);
static void      ligma_color_area_finalize            (GObject           *object);
static void      ligma_color_area_get_property        (GObject           *object,
                                                      guint              property_id,
                                                      GValue            *value,
                                                      GParamSpec        *pspec);
static void      ligma_color_area_set_property        (GObject           *object,
                                                      guint              property_id,
                                                      const GValue      *value,
                                                      GParamSpec        *pspec);

static void      ligma_color_area_size_allocate       (GtkWidget         *widget,
                                                      GtkAllocation     *allocation);
static gboolean  ligma_color_area_draw                (GtkWidget         *widget,
                                                      cairo_t           *cr);
static void      ligma_color_area_render_buf          (GtkWidget         *widget,
                                                      LigmaColorAreaType  type,
                                                      guchar            *buf,
                                                      guint              width,
                                                      guint              height,
                                                      guint              rowstride,
                                                      LigmaRGB           *color);
static void      ligma_color_area_render              (LigmaColorArea     *area);

static void      ligma_color_area_drag_begin          (GtkWidget         *widget,
                                                      GdkDragContext    *context);
static void      ligma_color_area_drag_end            (GtkWidget         *widget,
                                                      GdkDragContext    *context);
static void      ligma_color_area_drag_data_received  (GtkWidget         *widget,
                                                      GdkDragContext    *context,
                                                      gint               x,
                                                      gint               y,
                                                      GtkSelectionData  *selection_data,
                                                      guint              info,
                                                      guint              time);
static void      ligma_color_area_drag_data_get       (GtkWidget         *widget,
                                                      GdkDragContext    *context,
                                                      GtkSelectionData  *selection_data,
                                                      guint              info,
                                                      guint              time);

static void      ligma_color_area_create_transform    (LigmaColorArea     *area);
static void      ligma_color_area_destroy_transform   (LigmaColorArea     *area);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorArea, ligma_color_area,
                            GTK_TYPE_DRAWING_AREA)

#define parent_class ligma_color_area_parent_class

static guint ligma_color_area_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry target = { "application/x-color", 0 };


static void
ligma_color_area_class_init (LigmaColorAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaRGB         color;

  ligma_color_area_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorAreaClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose             = ligma_color_area_dispose;
  object_class->finalize            = ligma_color_area_finalize;
  object_class->get_property        = ligma_color_area_get_property;
  object_class->set_property        = ligma_color_area_set_property;

  widget_class->size_allocate       = ligma_color_area_size_allocate;
  widget_class->draw                = ligma_color_area_draw;

  widget_class->drag_begin          = ligma_color_area_drag_begin;
  widget_class->drag_end            = ligma_color_area_drag_end;
  widget_class->drag_data_received  = ligma_color_area_drag_data_received;
  widget_class->drag_data_get       = ligma_color_area_drag_data_get;

  klass->color_changed              = NULL;

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  /**
   * LigmaColorArea:color:
   *
   * The color displayed in the color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_COLOR,
                                   ligma_param_spec_rgb ("color",
                                                        "Color",
                                                        "The displayed color",
                                                        TRUE, &color,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * LigmaColorArea:type:
   *
   * The type of the color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type",
                                                      "Type",
                                                      "The type of the color area",
                                                      LIGMA_TYPE_COLOR_AREA_TYPE,
                                                      LIGMA_COLOR_AREA_FLAT,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  /**
   * LigmaColorArea:drag-type:
   *
   * The modifier mask that should trigger drags.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAG_MASK,
                                   g_param_spec_flags ("drag-mask",
                                                       "Drag Mask",
                                                       "The modifier mask that triggers dragging the color",
                                                       GDK_TYPE_MODIFIER_TYPE,
                                                       0,
                                                       LIGMA_PARAM_WRITABLE));
  /**
   * LigmaColorArea:draw-border:
   *
   * Whether to draw a thin border in the foreground color around the area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAW_BORDER,
                                   g_param_spec_boolean ("draw-border",
                                                         "Draw Border",
                                                         "Whether to draw a thin border in the foreground color around the area",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_color_area_init (LigmaColorArea *area)
{
  LigmaColorAreaPrivate *priv;

  area->priv = ligma_color_area_get_instance_private (area);

  priv = GET_PRIVATE (area);

  priv->buf         = NULL;
  priv->width       = 0;
  priv->height      = 0;
  priv->rowstride   = 0;
  priv->draw_border = FALSE;

  gtk_drag_dest_set (GTK_WIDGET (area),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  ligma_widget_track_monitor (GTK_WIDGET (area),
                             G_CALLBACK (ligma_color_area_destroy_transform),
                             NULL, NULL);
}

static void
ligma_color_area_dispose (GObject *object)
{
  LigmaColorArea *area = LIGMA_COLOR_AREA (object);

  ligma_color_area_set_color_config (area, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_area_finalize (GObject *object)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->buf, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_area_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, &priv->color);
      break;

    case PROP_TYPE:
      g_value_set_enum (value, priv->type);
      break;

    case PROP_DRAW_BORDER:
      g_value_set_boolean (value, priv->draw_border);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_area_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaColorArea   *area = LIGMA_COLOR_AREA (object);
  GdkModifierType  drag_mask;

  switch (property_id)
    {
    case PROP_COLOR:
      ligma_color_area_set_color (area, g_value_get_boxed (value));
      break;

    case PROP_TYPE:
      ligma_color_area_set_type (area, g_value_get_enum (value));
      break;

    case PROP_DRAG_MASK:
      drag_mask = g_value_get_flags (value) & (GDK_BUTTON1_MASK |
                                               GDK_BUTTON2_MASK |
                                               GDK_BUTTON3_MASK);
      if (drag_mask)
        gtk_drag_source_set (GTK_WIDGET (area),
                             drag_mask,
                             &target, 1,
                             GDK_ACTION_COPY | GDK_ACTION_MOVE);
      else
        gtk_drag_source_unset (GTK_WIDGET (area));
      break;

    case PROP_DRAW_BORDER:
      ligma_color_area_set_draw_border (area, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_area_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (allocation->width  != priv->width ||
      allocation->height != priv->height)
    {
      priv->width  = allocation->width;
      priv->height = allocation->height;

      priv->rowstride = priv->width * 4 + 4;

      g_free (priv->buf);
      priv->buf = g_new (guchar, priv->rowstride * priv->height);

      priv->needs_render = TRUE;
    }
}

static gboolean
ligma_color_area_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  LigmaColorArea        *area    = LIGMA_COLOR_AREA (widget);
  LigmaColorAreaPrivate *priv    = GET_PRIVATE (area);
  GtkStyleContext      *context = gtk_widget_get_style_context (widget);
  cairo_surface_t      *buffer;

  if (! priv->buf)
    return FALSE;

  if (priv->needs_render)
    ligma_color_area_render (area);

  if (! priv->transform)
    ligma_color_area_create_transform (area);

  if (priv->transform)
    {
      const Babl *format = babl_format ("cairo-RGB24");
      guchar     *buf    = g_new (guchar, priv->rowstride * priv->height);
      guchar     *src    = priv->buf;
      guchar     *dest   = buf;
      guint       i;

      for (i = 0; i < priv->height; i++)
        {
          ligma_color_transform_process_pixels (priv->transform,
                                               format, src,
                                               format, dest,
                                               priv->width);

          src  += priv->rowstride;
          dest += priv->rowstride;
        }

      buffer = cairo_image_surface_create_for_data (buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    priv->width,
                                                    priv->height,
                                                    priv->rowstride);
      cairo_surface_set_user_data (buffer, NULL,
                                   buf, (cairo_destroy_func_t) g_free);
    }
  else
    {
      buffer = cairo_image_surface_create_for_data (priv->buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    priv->width,
                                                    priv->height,
                                                    priv->rowstride);
    }

  cairo_set_source_surface (cr, buffer, 0.0, 0.0);
  cairo_surface_destroy (buffer);

  if (! gtk_widget_is_sensitive (widget))
    {
      static cairo_pattern_t *pattern = NULL;

      if (! pattern)
        {
          static const guchar  stipple[] = { 0,   255, 0, 0,
                                             255, 0,   0, 0 };
          cairo_surface_t     *surface;
          gint                 stride;

          stride = cairo_format_stride_for_width (CAIRO_FORMAT_A8, 2);

          surface = cairo_image_surface_create_for_data ((guchar *) stipple,
                                                         CAIRO_FORMAT_A8,
                                                         2, 2, stride);
          pattern = cairo_pattern_create_for_surface (surface);
          cairo_surface_destroy (surface);

          cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
        }

      cairo_mask (cr, pattern);
    }
  else
    {
      cairo_paint (cr);
    }

  if (priv->config &&
      ((priv->color.r < 0.0 || priv->color.r > 1.0 ||
        priv->color.g < 0.0 || priv->color.g > 1.0 ||
        priv->color.b < 0.0 || priv->color.b > 1.0) ||
       priv->out_of_gamut))
    {
      LigmaRGB color;
      gint    side = MIN (priv->width, priv->height) * 2 / 3;

      cairo_move_to (cr, priv->width, 0);
      cairo_line_to (cr, priv->width - side, 0);
      cairo_line_to (cr, priv->width, side);
      cairo_line_to (cr, priv->width, 0);

      ligma_color_config_get_out_of_gamut_color (priv->config, &color);
      ligma_cairo_set_source_rgb (cr, &color);

      cairo_fill (cr);
    }

  if (priv->draw_border)
    {
      GdkRGBA color;

      cairo_set_line_width (cr, 1.0);

      gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);

      cairo_rectangle (cr, 0.5, 0.5, priv->width - 1, priv->height - 1);

      cairo_stroke (cr);
    }

  return FALSE;
}

/**
 * ligma_color_area_new:
 * @color:     A pointer to a #LigmaRGB struct.
 * @type:      The type of color area to create.
 * @drag_mask: The event_mask that should trigger drags.
 *
 * Creates a new #LigmaColorArea widget.
 *
 * This returns a preview area showing the color. It handles color
 * DND. If the color changes, the "color_changed" signal is emitted.
 *
 * Returns: Pointer to the new #LigmaColorArea widget.
 **/
GtkWidget *
ligma_color_area_new (const LigmaRGB     *color,
                     LigmaColorAreaType  type,
                     GdkModifierType    drag_mask)
{
  return g_object_new (LIGMA_TYPE_COLOR_AREA,
                       "color",     color,
                       "type",      type,
                       "drag-mask", drag_mask,
                       NULL);
}

/**
 * ligma_color_area_set_color:
 * @area: Pointer to a #LigmaColorArea.
 * @color: Pointer to a #LigmaRGB struct that defines the new color.
 *
 * Sets @area to a different @color.
 **/
void
ligma_color_area_set_color (LigmaColorArea *area,
                           const LigmaRGB *color)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (area);

  if (ligma_rgba_distance (&priv->color, color) < LIGMA_RGBA_EPSILON)
    return;

  priv->color = *color;

  priv->needs_render = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (area));

  g_object_notify (G_OBJECT (area), "color");

  g_signal_emit (area, ligma_color_area_signals[COLOR_CHANGED], 0);
}

/**
 * ligma_color_area_get_color:
 * @area:  Pointer to a #LigmaColorArea.
 * @color: (out caller-allocates): Pointer to a #LigmaRGB struct
 *         that is used to return the color.
 *
 * Retrieves the current color of the @area.
 **/
void
ligma_color_area_get_color (LigmaColorArea *area,
                           LigmaRGB       *color)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (area);

  *color = priv->color;
}

/**
 * ligma_color_area_has_alpha:
 * @area: Pointer to a #LigmaColorArea.
 *
 * Checks whether the @area shows transparency information. This is determined
 * via the @area's #LigmaColorAreaType.
 *
 * Returns: %TRUE if @area shows transparency information, %FALSE otherwise.
 **/
gboolean
ligma_color_area_has_alpha (LigmaColorArea *area)
{
  LigmaColorAreaPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_AREA (area), FALSE);

  priv = GET_PRIVATE (area);

  return priv->type != LIGMA_COLOR_AREA_FLAT;
}

/**
 * ligma_color_area_set_type:
 * @area: Pointer to a #LigmaColorArea.
 * @type: A #LigmaColorAreaType.
 *
 * Changes the type of @area. The #LigmaColorAreaType determines
 * whether the widget shows transparency information and chooses the
 * size of the checkerboard used to do that.
 **/
void
ligma_color_area_set_type (LigmaColorArea     *area,
                          LigmaColorAreaType  type)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));

  priv = GET_PRIVATE (area);

  if (priv->type != type)
    {
      priv->type = type;

      priv->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (area));

      g_object_notify (G_OBJECT (area), "type");
    }
}

/**
 * ligma_color_area_enable_drag:
 * @area:      A #LigmaColorArea.
 * @drag_mask: The bitmask of buttons that can start the drag.
 *
 * Allows dragging the color displayed with buttons identified by
 * @drag_mask. The drag supports targets of type "application/x-color".
 *
 * Note that setting a @drag_mask of 0 disables the drag ability.
 **/
void
ligma_color_area_enable_drag (LigmaColorArea   *area,
                             GdkModifierType  drag_mask)
{
  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));

  g_object_set (area,
                "drag-mask", drag_mask,
                NULL);
}

/**
 * ligma_color_area_set_draw_border:
 * @area: Pointer to a #LigmaColorArea.
 * @draw_border: whether to draw a border or not
 *
 * The @area can draw a thin border in the foreground color around
 * itself.  This function toggles this behaviour on and off. The
 * default is not draw a border.
 **/
void
ligma_color_area_set_draw_border (LigmaColorArea *area,
                                 gboolean       draw_border)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));

  priv = GET_PRIVATE (area);

  draw_border = draw_border ? TRUE : FALSE;

  if (priv->draw_border != draw_border)
    {
      priv->draw_border = draw_border;

      gtk_widget_queue_draw (GTK_WIDGET (area));

      g_object_notify (G_OBJECT (area), "draw-border");
    }
}

/**
 * ligma_color_area_set_out_of_gamut:
 * @area:         a #LigmaColorArea widget.
 * @out_of_gamut: whether to show an out-of-gamut indicator
 *
 * Sets the color area to render as an out-of-gamut color, i.e. with a
 * small triangle on a corner using the color management out of gamut
 * color (as per ligma_color_area_set_color_config()).
 *
 * By default, @area will render as out-of-gamut for any RGB color with
 * a channel out of the [0; 1] range. This function allows to consider
 * more colors out of gamut (for instance non-gray colors on a grayscale
 * image, or colors absent of palettes in indexed images, etc.)
 *
 * Since: 2.10.10
 */
void
ligma_color_area_set_out_of_gamut (LigmaColorArea *area,
                                  gboolean       out_of_gamut)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));

  priv = GET_PRIVATE (area);
  if (priv->out_of_gamut != out_of_gamut)
    {
      priv->out_of_gamut = out_of_gamut;
      gtk_widget_queue_draw (GTK_WIDGET (area));
    }
}

/**
 * ligma_color_area_set_color_config:
 * @area:   a #LigmaColorArea widget.
 * @config: a #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this color area.
 *
 * Since: 2.10
 */
void
ligma_color_area_set_color_config (LigmaColorArea   *area,
                                  LigmaColorConfig *config)
{
  LigmaColorAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_AREA (area));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (area);

  if (config != priv->config)
    {
      if (priv->config)
        {
          g_signal_handlers_disconnect_by_func (priv->config,
                                                ligma_color_area_destroy_transform,
                                                area);

          ligma_color_area_destroy_transform (area);
        }

      g_set_object (&priv->config, config);

      if (priv->config)
        {
          g_signal_connect_swapped (priv->config, "notify",
                                    G_CALLBACK (ligma_color_area_destroy_transform),
                                    area);
        }
    }
}


/*  private functions  */

static void
ligma_color_area_render_buf (GtkWidget         *widget,
                            LigmaColorAreaType  type,
                            guchar            *buf,
                            guint              width,
                            guint              height,
                            guint              rowstride,
                            LigmaRGB           *color)
{
  guint    x, y;
  guint    check_size = 0;
  guchar   light[3];
  guchar   dark[3];
  guchar   opaque[3];
  guchar  *p;
  gdouble  frac;

  switch (type)
    {
    case LIGMA_COLOR_AREA_FLAT:
      check_size = 0;
      break;

    case LIGMA_COLOR_AREA_SMALL_CHECKS:
      check_size = LIGMA_CHECK_SIZE_SM;
      break;

    case LIGMA_COLOR_AREA_LARGE_CHECKS:
      check_size = LIGMA_CHECK_SIZE;
      break;
    }

  ligma_rgb_get_uchar (color, opaque, opaque + 1, opaque + 2);

  if (check_size == 0 || color->a == 1.0)
    {
      for (y = 0; y < height; y++)
        {
          p = buf + y * rowstride;

          for (x = 0; x < width; x++)
            {
              LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                          opaque[0],
                                          opaque[1],
                                          opaque[2]);

              p += 4;
            }
        }

      return;
    }

  light[0] = (LIGMA_CHECK_LIGHT +
              (color->r - LIGMA_CHECK_LIGHT) * color->a) * 255.999;
  light[1] = (LIGMA_CHECK_LIGHT +
              (color->g - LIGMA_CHECK_LIGHT) * color->a) * 255.999;
  light[2] = (LIGMA_CHECK_LIGHT +
              (color->b - LIGMA_CHECK_LIGHT) * color->a) * 255.999;

  dark[0] = (LIGMA_CHECK_DARK +
             (color->r - LIGMA_CHECK_DARK)  * color->a) * 255.999;
  dark[1] = (LIGMA_CHECK_DARK +
             (color->g - LIGMA_CHECK_DARK)  * color->a) * 255.999;
  dark[2] = (LIGMA_CHECK_DARK +
             (color->b - LIGMA_CHECK_DARK)  * color->a) * 255.999;

  for (y = 0; y < height; y++)
    {
      p = buf + y * rowstride;

      for (x = 0; x < width; x++)
        {
          if ((width - x) * height > y * width)
            {
              LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                          opaque[0],
                                          opaque[1],
                                          opaque[2]);
              p += 4;

              continue;
            }

          frac = y - (gdouble) ((width - x) * height) / (gdouble) width;

          if (((x / check_size) ^ (y / check_size)) & 1)
            {
              if ((gint) frac)
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                              light[0],
                                              light[1],
                                              light[2]);
                }
              else
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                              ((gdouble) light[0]  * frac +
                                               (gdouble) opaque[0] * (1.0 - frac)),
                                              ((gdouble) light[1]  * frac +
                                               (gdouble) opaque[1] * (1.0 - frac)),
                                              ((gdouble) light[2]  * frac +
                                               (gdouble) opaque[2] * (1.0 - frac)));
                }
            }
          else
            {
              if ((gint) frac)
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                              dark[0],
                                              dark[1],
                                              dark[2]);
                }
              else
                {
                  LIGMA_CAIRO_RGB24_SET_PIXEL (p,
                                              ((gdouble) dark[0] * frac +
                                               (gdouble) opaque[0] * (1.0 - frac)),
                                              ((gdouble) dark[1] * frac +
                                               (gdouble) opaque[1] * (1.0 - frac)),
                                              ((gdouble) dark[2] * frac +
                                               (gdouble) opaque[2] * (1.0 - frac)));
                }
            }

          p += 4;
        }
    }
}

static void
ligma_color_area_render (LigmaColorArea *area)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (area);

  if (! priv->buf)
    return;

  ligma_color_area_render_buf (GTK_WIDGET (area),
                              priv->type,
                              priv->buf,
                              priv->width, priv->height, priv->rowstride,
                              &priv->color);

  priv->needs_render = FALSE;
}

static void
ligma_color_area_drag_begin (GtkWidget      *widget,
                            GdkDragContext *context)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (widget);
  LigmaRGB               color;
  GtkWidget            *window;
  GtkWidget            *frame;
  GtkWidget            *color_area;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (widget));

  gtk_widget_realize (window);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);

  ligma_color_area_get_color (LIGMA_COLOR_AREA (widget), &color);

  color_area = ligma_color_area_new (&color, priv->type, 0);

  gtk_widget_set_size_request (color_area,
                               DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_area);
  gtk_widget_show (color_area);
  gtk_widget_show (frame);

  g_object_set_data_full (G_OBJECT (widget),
                          "ligma-color-area-drag-window",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
                            DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
}

static void
ligma_color_area_drag_end (GtkWidget      *widget,
                          GdkDragContext *context)
{
  g_object_set_data (G_OBJECT (widget),
                     "ligma-color-area-drag-window", NULL);
}

static void
ligma_color_area_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time)
{
  LigmaColorArea *area = LIGMA_COLOR_AREA (widget);
  const guint16 *vals;
  LigmaRGB        color;

  if (gtk_selection_data_get_length (selection_data) != 8 ||
      gtk_selection_data_get_format (selection_data) != 16)
    {
      g_warning ("%s: received invalid color data", G_STRFUNC);
      return;
    }

  vals = (const guint16 *) gtk_selection_data_get_data (selection_data);

  ligma_rgba_set (&color,
                 (gdouble) vals[0] / 0xffff,
                 (gdouble) vals[1] / 0xffff,
                 (gdouble) vals[2] / 0xffff,
                 (gdouble) vals[3] / 0xffff);

  ligma_color_area_set_color (area, &color);
}

static void
ligma_color_area_drag_data_get (GtkWidget        *widget,
                               GdkDragContext   *context,
                               GtkSelectionData *selection_data,
                               guint             info,
                               guint             time)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (widget);
  guint16               vals[4];

  vals[0] = priv->color.r * 0xffff;
  vals[1] = priv->color.g * 0xffff;
  vals[2] = priv->color.b * 0xffff;

  if (priv->type == LIGMA_COLOR_AREA_FLAT)
    vals[3] = 0xffff;
  else
    vals[3] = priv->color.a * 0xffff;

  gtk_selection_data_set (selection_data,
                          gdk_atom_intern ("application/x-color", FALSE),
                          16, (guchar *) vals, 8);
}

static void
ligma_color_area_create_transform (LigmaColorArea *area)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (area);

  if (priv->config)
    {
      static LigmaColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = ligma_color_profile_new_rgb_srgb ();

      priv->transform = ligma_widget_get_color_transform (GTK_WIDGET (area),
                                                         priv->config,
                                                         profile,
                                                         format,
                                                         format,
                                                         NULL,
                                                         LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                         FALSE);
    }
}

static void
ligma_color_area_destroy_transform (LigmaColorArea *area)
{
  LigmaColorAreaPrivate *priv = GET_PRIVATE (area);

  g_clear_object (&priv->transform);

  gtk_widget_queue_draw (GTK_WIDGET (area));
}
