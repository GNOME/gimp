/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorarea.c
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcairo-utils.h"
#include "gimpcolorarea.h"
#include "gimpwidgetsutils.h"


/**
 * SECTION: gimpcolorarea
 * @title: GimpColorArea
 * @short_description: Displays a #GimpRGB color, optionally with
 *                     alpha-channel.
 *
 * Displays a #GimpRGB color, optionally with alpha-channel.
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


typedef struct _GimpColorAreaPrivate GimpColorAreaPrivate;

struct _GimpColorAreaPrivate
{
  GimpColorConfig    *config;
  GimpColorTransform *transform;
};

#define GET_PRIVATE(obj) \
        G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                     GIMP_TYPE_COLOR_AREA, \
                                     GimpColorAreaPrivate)


static void      gimp_color_area_dispose       (GObject            *object);
static void      gimp_color_area_finalize      (GObject            *object);
static void      gimp_color_area_get_property  (GObject            *object,
                                                guint               property_id,
                                                GValue             *value,
                                                GParamSpec         *pspec);
static void      gimp_color_area_set_property  (GObject            *object,
                                                guint               property_id,
                                                const GValue       *value,
                                                GParamSpec         *pspec);

static void      gimp_color_area_size_allocate (GtkWidget          *widget,
                                                GtkAllocation      *allocation);
static void      gimp_color_area_state_changed (GtkWidget          *widget,
                                                GtkStateType        previous_state);
static gboolean  gimp_color_area_draw          (GtkWidget          *widget,
                                                cairo_t            *cr);
static void      gimp_color_area_render_buf    (GtkWidget          *widget,
                                                gboolean            insensitive,
                                                GimpColorAreaType   type,
                                                guchar             *buf,
                                                guint               width,
                                                guint               height,
                                                guint               rowstride,
                                                GimpRGB            *color);
static void      gimp_color_area_render        (GimpColorArea      *area);

static void  gimp_color_area_drag_begin         (GtkWidget        *widget,
                                                 GdkDragContext   *context);
static void  gimp_color_area_drag_end           (GtkWidget        *widget,
                                                 GdkDragContext   *context);
static void  gimp_color_area_drag_data_received (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 GtkSelectionData *selection_data,
                                                 guint             info,
                                                 guint             time);
static void  gimp_color_area_drag_data_get      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GtkSelectionData *selection_data,
                                                 guint             info,
                                                 guint             time);

static void  gimp_color_area_create_transform   (GimpColorArea    *area);
static void  gimp_color_area_destroy_transform  (GimpColorArea    *area);


G_DEFINE_TYPE (GimpColorArea, gimp_color_area, GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_color_area_parent_class

static guint gimp_color_area_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry target = { "application/x-color", 0 };


static void
gimp_color_area_class_init (GimpColorAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpRGB         color;

  gimp_color_area_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorAreaClass, color_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose            = gimp_color_area_dispose;
  object_class->finalize           = gimp_color_area_finalize;
  object_class->get_property       = gimp_color_area_get_property;
  object_class->set_property       = gimp_color_area_set_property;

  widget_class->size_allocate      = gimp_color_area_size_allocate;
  widget_class->state_changed      = gimp_color_area_state_changed;
  widget_class->draw               = gimp_color_area_draw;

  widget_class->drag_begin         = gimp_color_area_drag_begin;
  widget_class->drag_end           = gimp_color_area_drag_end;
  widget_class->drag_data_received = gimp_color_area_drag_data_received;
  widget_class->drag_data_get      = gimp_color_area_drag_data_get;

  klass->color_changed             = NULL;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  /**
   * GimpColorArea:color:
   *
   * The color displayed in the color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color", NULL, NULL,
                                                        TRUE, &color,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * GimpColorArea:type:
   *
   * The type of the color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_COLOR_AREA_TYPE,
                                                      GIMP_COLOR_AREA_FLAT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  /**
   * GimpColorArea:drag-type:
   *
   * The event_mask that should trigger drags.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAG_MASK,
                                   g_param_spec_flags ("drag-mask", NULL, NULL,
                                                       GDK_TYPE_MODIFIER_TYPE,
                                                       0,
                                                       GIMP_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  /**
   * GimpColorArea:draw-border:
   *
   * Whether to draw a thin border in the foreground color around the area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAW_BORDER,
                                   g_param_spec_boolean ("draw-border",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GimpColorAreaPrivate));
}

static void
gimp_color_area_init (GimpColorArea *area)
{
  area->buf         = NULL;
  area->width       = 0;
  area->height      = 0;
  area->rowstride   = 0;
  area->draw_border = FALSE;

  gtk_drag_dest_set (GTK_WIDGET (area),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  gimp_widget_track_monitor (GTK_WIDGET (area),
                             G_CALLBACK (gimp_color_area_destroy_transform),
                             NULL);
}

static void
gimp_color_area_dispose (GObject *object)
{
  GimpColorArea *area = GIMP_COLOR_AREA (object);

  gimp_color_area_set_color_config (area, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_area_finalize (GObject *object)
{
  GimpColorArea *area = GIMP_COLOR_AREA (object);

  if (area->buf)
    {
      g_free (area->buf);
      area->buf = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_area_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpColorArea *area = GIMP_COLOR_AREA (object);

  switch (property_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, &area->color);
      break;

    case PROP_TYPE:
      g_value_set_enum (value, area->type);
      break;

    case PROP_DRAW_BORDER:
      g_value_set_boolean (value, area->draw_border);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_area_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpColorArea   *area = GIMP_COLOR_AREA (object);
  GdkModifierType  drag_mask;

  switch (property_id)
    {
    case PROP_COLOR:
      gimp_color_area_set_color (area, g_value_get_boxed (value));
      break;

    case PROP_TYPE:
      gimp_color_area_set_type (area, g_value_get_enum (value));
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
      break;

    case PROP_DRAW_BORDER:
      gimp_color_area_set_draw_border (area, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_area_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  GimpColorArea *area = GIMP_COLOR_AREA (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (allocation->width  != area->width ||
      allocation->height != area->height)
    {
      area->width  = allocation->width;
      area->height = allocation->height;

      area->rowstride = area->width * 4 + 4;

      g_free (area->buf);
      area->buf = g_new (guchar, area->rowstride * area->height);

      area->needs_render = TRUE;
    }
}

static void
gimp_color_area_state_changed (GtkWidget    *widget,
                               GtkStateType  previous_state)
{
  if (gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE ||
      previous_state == GTK_STATE_INSENSITIVE)
    {
      GIMP_COLOR_AREA (widget)->needs_render = TRUE;
    }

  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, previous_state);
}

static gboolean
gimp_color_area_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GimpColorArea        *area  = GIMP_COLOR_AREA (widget);
  GimpColorAreaPrivate *priv  = GET_PRIVATE (widget);
  GtkStyle             *style = gtk_widget_get_style (widget);
  cairo_surface_t      *buffer;

  if (! area->buf)
    return FALSE;

  if (area->needs_render)
    gimp_color_area_render (area);

  if (! priv->transform)
    gimp_color_area_create_transform (area);

  if (priv->transform)
    {
      const Babl *format = babl_format ("cairo-RGB24");
      guchar     *buf    = g_new (guchar, area->rowstride * area->height);
      guchar     *src    = area->buf;
      guchar     *dest   = buf;
      gint        i;

      for (i = 0; i < area->height; i++)
        {
          gimp_color_transform_process_pixels (priv->transform,
                                               format, src,
                                               format, dest,
                                               area->width);

          src  += area->rowstride;
          dest += area->rowstride;
        }

      buffer = cairo_image_surface_create_for_data (buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    area->width,
                                                    area->height,
                                                    area->rowstride);
      cairo_surface_set_user_data (buffer, NULL,
                                   buf, (cairo_destroy_func_t) g_free);
    }
  else
    {
      buffer = cairo_image_surface_create_for_data (area->buf,
                                                    CAIRO_FORMAT_RGB24,
                                                    area->width,
                                                    area->height,
                                                    area->rowstride);
    }

  cairo_set_source_surface (cr, buffer, 0.0, 0.0);
  cairo_surface_destroy (buffer);

  cairo_paint (cr);

  if (area->draw_border)
    {
      cairo_set_line_width (cr, 1.0);
      gdk_cairo_set_source_color (cr,
                                  &style->fg[gtk_widget_get_state (widget)]);

      cairo_rectangle (cr, 0.5, 0.5, area->width - 1, area->height - 1);

      cairo_stroke (cr);
    }

  return FALSE;
}

/**
 * gimp_color_area_new:
 * @color:     A pointer to a #GimpRGB struct.
 * @type:      The type of color area to create.
 * @drag_mask: The event_mask that should trigger drags.
 *
 * Creates a new #GimpColorArea widget.
 *
 * This returns a preview area showing the color. It handles color
 * DND. If the color changes, the "color_changed" signal is emitted.
 *
 * Returns: Pointer to the new #GimpColorArea widget.
 **/
GtkWidget *
gimp_color_area_new (const GimpRGB     *color,
                     GimpColorAreaType  type,
                     GdkModifierType    drag_mask)
{
  return g_object_new (GIMP_TYPE_COLOR_AREA,
                       "color",     color,
                       "type",      type,
                       "drag-mask", drag_mask,
                       NULL);
}

/**
 * gimp_color_area_set_color:
 * @area: Pointer to a #GimpColorArea.
 * @color: Pointer to a #GimpRGB struct that defines the new color.
 *
 * Sets @area to a different @color.
 **/
void
gimp_color_area_set_color (GimpColorArea *area,
                           const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  if (gimp_rgba_distance (&area->color, color) < 0.000001)
    return;

  area->color = *color;

  area->needs_render = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (area));

  g_object_notify (G_OBJECT (area), "color");

  g_signal_emit (area, gimp_color_area_signals[COLOR_CHANGED], 0);
}

/**
 * gimp_color_area_get_color:
 * @area: Pointer to a #GimpColorArea.
 * @color: Pointer to a #GimpRGB struct that is used to return the color.
 *
 * Retrieves the current color of the @area.
 **/
void
gimp_color_area_get_color (GimpColorArea *area,
                           GimpRGB       *color)
{
  g_return_if_fail (GIMP_IS_COLOR_AREA (area));
  g_return_if_fail (color != NULL);

  *color = area->color;
}

/**
 * gimp_color_area_has_alpha:
 * @area: Pointer to a #GimpColorArea.
 *
 * Checks whether the @area shows transparency information. This is determined
 * via the @area's #GimpColorAreaType.
 *
 * Returns: %TRUE if @area shows transparency information, %FALSE otherwise.
 **/
gboolean
gimp_color_area_has_alpha (GimpColorArea *area)
{
  g_return_val_if_fail (GIMP_IS_COLOR_AREA (area), FALSE);

  return area->type != GIMP_COLOR_AREA_FLAT;
}

/**
 * gimp_color_area_set_type:
 * @area: Pointer to a #GimpColorArea.
 * @type: A #GimpColorAreaType.
 *
 * Changes the type of @area. The #GimpColorAreaType determines
 * whether the widget shows transparency information and chooses the
 * size of the checkerboard used to do that.
 **/
void
gimp_color_area_set_type (GimpColorArea     *area,
                          GimpColorAreaType  type)
{
  g_return_if_fail (GIMP_IS_COLOR_AREA (area));

  if (area->type != type)
    {
      area->type = type;

      area->needs_render = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (area));

      g_object_notify (G_OBJECT (area), "type");
    }
}

/**
 * gimp_color_area_set_draw_border:
 * @area: Pointer to a #GimpColorArea.
 * @draw_border: whether to draw a border or not
 *
 * The @area can draw a thin border in the foreground color around
 * itself.  This function toggles this behaviour on and off. The
 * default is not draw a border.
 **/
void
gimp_color_area_set_draw_border (GimpColorArea *area,
                                 gboolean       draw_border)
{
  g_return_if_fail (GIMP_IS_COLOR_AREA (area));

  draw_border = draw_border ? TRUE : FALSE;

  if (area->draw_border != draw_border)
    {
      area->draw_border = draw_border;

      gtk_widget_queue_draw (GTK_WIDGET (area));

      g_object_notify (G_OBJECT (area), "draw-border");
    }
}

/**
 * gimp_color_area_set_color_config:
 * @area:   a #GimpColorArea widget.
 * @config: a #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color area.
 *
 * Since: 2.10
 */
void
gimp_color_area_set_color_config (GimpColorArea   *area,
                                  GimpColorConfig *config)
{
  GimpColorAreaPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_AREA (area));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (area);

  if (config != priv->config)
    {
      if (priv->config)
        {
          g_signal_handlers_disconnect_by_func (priv->config,
                                                gimp_color_area_destroy_transform,
                                                area);
          g_object_unref (priv->config);

          gimp_color_area_destroy_transform (area);
        }

      priv->config = config;

      if (priv->config)
        {
          g_object_ref (priv->config);

          g_signal_connect_swapped (priv->config, "notify",
                                    G_CALLBACK (gimp_color_area_destroy_transform),
                                    area);
        }
    }
}


/*  private functions  */

static void
gimp_color_area_render_buf (GtkWidget         *widget,
                            gboolean           insensitive,
                            GimpColorAreaType  type,
                            guchar            *buf,
                            guint              width,
                            guint              height,
                            guint              rowstride,
                            GimpRGB           *color)
{
  GtkStyle *style = gtk_widget_get_style (widget);
  guint     x, y;
  guint     check_size = 0;
  guchar    light[3];
  guchar    dark[3];
  guchar    opaque[3];
  guchar    insens[3];
  guchar   *p;
  gdouble   frac;

  switch (type)
    {
    case GIMP_COLOR_AREA_FLAT:
      check_size = 0;
      break;

    case GIMP_COLOR_AREA_SMALL_CHECKS:
      check_size = GIMP_CHECK_SIZE_SM;
      break;

    case GIMP_COLOR_AREA_LARGE_CHECKS:
      check_size = GIMP_CHECK_SIZE;
      break;
    }

  gimp_rgb_get_uchar (color, opaque, opaque + 1, opaque + 2);

  insens[0] = style->bg[GTK_STATE_INSENSITIVE].red   >> 8;
  insens[1] = style->bg[GTK_STATE_INSENSITIVE].green >> 8;
  insens[2] = style->bg[GTK_STATE_INSENSITIVE].blue  >> 8;

  if (insensitive || check_size == 0 || color->a == 1.0)
    {
      for (y = 0; y < height; y++)
        {
          p = buf + y * rowstride;

          for (x = 0; x < width; x++)
            {
              if (insensitive && ((x + y) % 2))
                {
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
                                              insens[0],
                                              insens[1],
                                              insens[2]);
                }
              else
                {
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
                                              opaque[0],
                                              opaque[1],
                                              opaque[2]);
                }

              p += 4;
            }
        }

      return;
    }

  light[0] = (GIMP_CHECK_LIGHT +
              (color->r - GIMP_CHECK_LIGHT) * color->a) * 255.999;
  light[1] = (GIMP_CHECK_LIGHT +
              (color->g - GIMP_CHECK_LIGHT) * color->a) * 255.999;
  light[2] = (GIMP_CHECK_LIGHT +
              (color->b - GIMP_CHECK_LIGHT) * color->a) * 255.999;

  dark[0] = (GIMP_CHECK_DARK +
             (color->r - GIMP_CHECK_DARK)  * color->a) * 255.999;
  dark[1] = (GIMP_CHECK_DARK +
             (color->g - GIMP_CHECK_DARK)  * color->a) * 255.999;
  dark[2] = (GIMP_CHECK_DARK +
             (color->b - GIMP_CHECK_DARK)  * color->a) * 255.999;

  for (y = 0; y < height; y++)
    {
      p = buf + y * rowstride;

      for (x = 0; x < width; x++)
        {
          if ((width - x) * height > y * width)
            {
              GIMP_CAIRO_RGB24_SET_PIXEL (p,
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
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
                                              light[0],
                                              light[1],
                                              light[2]);
                }
              else
                {
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
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
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
                                              dark[0],
                                              dark[1],
                                              dark[2]);
                }
              else
                {
                  GIMP_CAIRO_RGB24_SET_PIXEL (p,
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
gimp_color_area_render (GimpColorArea *area)
{
  if (! area->buf)
    return;

  gimp_color_area_render_buf (GTK_WIDGET (area),
                              ! gtk_widget_is_sensitive (GTK_WIDGET (area)),
                              area->type,
                              area->buf,
                              area->width, area->height, area->rowstride,
                              &area->color);

  area->needs_render = FALSE;
}

static void
gimp_color_area_drag_begin (GtkWidget      *widget,
                            GdkDragContext *context)
{
  GimpRGB    color;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *color_area;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (widget));

  gtk_widget_realize (window);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);

  color_area = gimp_color_area_new (&color,
                                    GIMP_COLOR_AREA (widget)->type,
                                    0);

  gtk_widget_set_size_request (color_area,
                               DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_area);
  gtk_widget_show (color_area);
  gtk_widget_show (frame);

  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-color-area-drag-window",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
                            DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
}

static void
gimp_color_area_drag_end (GtkWidget      *widget,
                          GdkDragContext *context)
{
  g_object_set_data (G_OBJECT (widget),
                     "gimp-color-area-drag-window", NULL);
}

static void
gimp_color_area_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time)
{
  GimpColorArea *area = GIMP_COLOR_AREA (widget);
  const guint16 *vals;
  GimpRGB        color;

  if (gtk_selection_data_get_length (selection_data) != 8 ||
      gtk_selection_data_get_format (selection_data) != 16)
    {
      g_warning ("%s: received invalid color data", G_STRFUNC);
      return;
    }

  vals = (const guint16 *) gtk_selection_data_get_data (selection_data);

  gimp_rgba_set (&color,
                 (gdouble) vals[0] / 0xffff,
                 (gdouble) vals[1] / 0xffff,
                 (gdouble) vals[2] / 0xffff,
                 (gdouble) vals[3] / 0xffff);

  gimp_color_area_set_color (area, &color);
}

static void
gimp_color_area_drag_data_get (GtkWidget        *widget,
                               GdkDragContext   *context,
                               GtkSelectionData *selection_data,
                               guint             info,
                               guint             time)
{
  GimpColorArea *area = GIMP_COLOR_AREA (widget);
  guint16        vals[4];

  vals[0] = area->color.r * 0xffff;
  vals[1] = area->color.g * 0xffff;
  vals[2] = area->color.b * 0xffff;

  if (area->type == GIMP_COLOR_AREA_FLAT)
    vals[3] = 0xffff;
  else
    vals[3] = area->color.a * 0xffff;

  gtk_selection_data_set (selection_data,
                          gdk_atom_intern ("application/x-color", FALSE),
                          16, (guchar *) vals, 8);
}

static void
gimp_color_area_create_transform (GimpColorArea *area)
{
  GimpColorAreaPrivate *priv = GET_PRIVATE (area);

  if (priv->config)
    {
      static GimpColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      priv->transform = gimp_widget_get_color_transform (GTK_WIDGET (area),
                                                         priv->config,
                                                         profile,
                                                         format,
                                                         format);
    }
}

static void
gimp_color_area_destroy_transform (GimpColorArea *area)
{
  GimpColorAreaPrivate *priv = GET_PRIVATE (area);

  if (priv->transform)
    {
      g_object_unref (priv->transform);
      priv->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (area));
}
