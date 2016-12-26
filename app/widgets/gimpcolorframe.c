/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimpimage.h"

#include "gimpcolorframe.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MODE,
  PROP_HAS_NUMBER,
  PROP_NUMBER,
  PROP_HAS_COLOR_AREA
};


/*  local function prototypes  */

static void       gimp_color_frame_dispose           (GObject        *object);
static void       gimp_color_frame_finalize          (GObject        *object);
static void       gimp_color_frame_get_property      (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);
static void       gimp_color_frame_set_property      (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);

static void       gimp_color_frame_style_set         (GtkWidget      *widget,
                                                      GtkStyle       *prev_style);
static gboolean   gimp_color_frame_expose            (GtkWidget      *widget,
                                                      GdkEventExpose *eevent);

static void       gimp_color_frame_menu_callback     (GtkWidget      *widget,
                                                      GimpColorFrame *frame);
static void       gimp_color_frame_update            (GimpColorFrame *frame);

static void       gimp_color_frame_create_transform  (GimpColorFrame *frame);
static void       gimp_color_frame_destroy_transform (GimpColorFrame *frame);


G_DEFINE_TYPE (GimpColorFrame, gimp_color_frame, GIMP_TYPE_FRAME)

#define parent_class gimp_color_frame_parent_class


static void
gimp_color_frame_class_init (GimpColorFrameClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gimp_color_frame_dispose;
  object_class->finalize     = gimp_color_frame_finalize;
  object_class->get_property = gimp_color_frame_get_property;
  object_class->set_property = gimp_color_frame_set_property;

  widget_class->style_set    = gimp_color_frame_style_set;
  widget_class->expose_event = gimp_color_frame_expose;

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_COLOR_FRAME_MODE,
                                                      GIMP_COLOR_FRAME_MODE_PIXEL,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_NUMBER,
                                   g_param_spec_boolean ("has-number",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NUMBER,
                                   g_param_spec_int ("number",
                                                     NULL, NULL,
                                                     0, 256, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_COLOR_AREA,
                                   g_param_spec_boolean ("has-color-area",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_color_frame_init (GimpColorFrame *frame)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  gint       i;

  frame->sample_valid  = FALSE;
  frame->sample_format = babl_format ("R'G'B' u8");

  gimp_rgba_set (&frame->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  frame->menu = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_FRAME_MODE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame->menu);
  gtk_widget_show (frame->menu);

  g_signal_connect (frame->menu, "changed",
                    G_CALLBACK (gimp_color_frame_menu_callback),
                    frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame->color_area =
    g_object_new (GIMP_TYPE_COLOR_AREA,
                  "color",          &frame->color,
                  "type",           GIMP_COLOR_AREA_SMALL_CHECKS,
                  "drag-mask",      GDK_BUTTON1_MASK,
                  "draw-border",    TRUE,
                  "height-request", 20,
                  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame->color_area, FALSE, FALSE, 0);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_set_homogeneous (GTK_BOX (vbox2), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      GtkWidget *hbox;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      frame->name_labels[i] = gtk_label_new (" ");
      gtk_label_set_xalign (GTK_LABEL (frame->name_labels[i]), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), frame->name_labels[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (frame->name_labels[i]);

      frame->value_labels[i] = gtk_label_new (" ");
      gtk_label_set_selectable (GTK_LABEL (frame->value_labels[i]), TRUE);
      gtk_label_set_xalign (GTK_LABEL (frame->value_labels[i]), 1.0);
      gtk_box_pack_end (GTK_BOX (hbox), frame->value_labels[i],
                        FALSE, FALSE, 0);
      gtk_widget_show (frame->value_labels[i]);
    }
}

static void
gimp_color_frame_dispose (GObject *object)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  gimp_color_frame_set_color_config (frame, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_frame_finalize (GObject *object)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  if (frame->number_layout)
    {
      g_object_unref (frame->number_layout);
      frame->number_layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_frame_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, frame->frame_mode);
      break;

    case PROP_HAS_NUMBER:
      g_value_set_boolean (value, frame->has_number);
      break;

    case PROP_NUMBER:
      g_value_set_int (value, frame->number);
      break;

    case PROP_HAS_COLOR_AREA:
      g_value_set_boolean (value, frame->has_color_area);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_frame_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  switch (property_id)
    {
    case PROP_MODE:
      gimp_color_frame_set_mode (frame, g_value_get_enum (value));
      break;

    case PROP_HAS_NUMBER:
      gimp_color_frame_set_has_number (frame, g_value_get_boolean (value));
      break;

    case PROP_NUMBER:
      gimp_color_frame_set_number (frame, g_value_get_int (value));
      break;

    case PROP_HAS_COLOR_AREA:
      gimp_color_frame_set_has_color_area (frame, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_frame_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (frame->number_layout)
    {
      g_object_unref (frame->number_layout);
      frame->number_layout = NULL;
    }
}

static gboolean
gimp_color_frame_expose (GtkWidget      *widget,
                         GdkEventExpose *eevent)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (widget);

  if (frame->has_number)
    {
      GtkStyle      *style = gtk_widget_get_style (widget);
      GtkAllocation  allocation;
      GtkAllocation  menu_allocation;
      GtkAllocation  color_area_allocation;
      cairo_t       *cr;
      gchar          buf[8];
      gint           w, h;
      gdouble        scale;

      gtk_widget_get_allocation (widget, &allocation);
      gtk_widget_get_allocation (frame->menu, &menu_allocation);
      gtk_widget_get_allocation (frame->color_area, &color_area_allocation);

      cr = gdk_cairo_create (gtk_widget_get_window (widget));
      gdk_cairo_region (cr, eevent->region);
      cairo_clip (cr);

      cairo_translate (cr, allocation.x, allocation.y);

      gdk_cairo_set_source_color (cr, &style->light[GTK_STATE_NORMAL]);

      g_snprintf (buf, sizeof (buf), "%d", frame->number);

      if (! frame->number_layout)
        frame->number_layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (frame->number_layout, buf, -1);
      pango_layout_get_pixel_size (frame->number_layout, &w, &h);

      scale = ((gdouble) (allocation.height -
                          menu_allocation.height -
                          color_area_allocation.height) /
               (gdouble) h);

      cairo_scale (cr, scale, scale);

      cairo_move_to (cr,
                     (allocation.width / 2.0) / scale - w / 2.0,
                     (allocation.height / 2.0 +
                      menu_allocation.height / 2.0 +
                      color_area_allocation.height / 2.0) / scale - h / 2.0);
      pango_cairo_show_layout (cr, frame->number_layout);

      cairo_destroy (cr);
    }

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);
}


/*  public functions  */

/**
 * gimp_color_frame_new:
 *
 * Creates a new #GimpColorFrame widget.
 *
 * Return value: The new #GimpColorFrame widget.
 **/
GtkWidget *
gimp_color_frame_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_FRAME, NULL);
}


/**
 * gimp_color_frame_set_mode:
 * @frame: The #GimpColorFrame.
 * @mode:  The new @mode.
 *
 * Sets the #GimpColorFrame's color @mode. Calling this function does
 * the same as selecting the @mode from the frame's #GtkOptionMenu.
 **/
void
gimp_color_frame_set_mode (GimpColorFrame     *frame,
                           GimpColorFrameMode  mode)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (frame->menu), mode);

  g_object_notify (G_OBJECT (frame), "mode");
}

void
gimp_color_frame_set_has_number (GimpColorFrame *frame,
                                 gboolean        has_number)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (has_number != frame->has_number)
    {
      frame->has_number = has_number ? TRUE : FALSE;

      gtk_widget_queue_draw (GTK_WIDGET (frame));

      g_object_notify (G_OBJECT (frame), "has-number");
    }
}

void
gimp_color_frame_set_number (GimpColorFrame *frame,
                             gint            number)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (number != frame->number)
    {
      frame->number = number;

      gtk_widget_queue_draw (GTK_WIDGET (frame));

      g_object_notify (G_OBJECT (frame), "number");
    }
}

void
gimp_color_frame_set_has_color_area (GimpColorFrame *frame,
                                     gboolean        has_color_area)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (has_color_area != frame->has_color_area)
    {
      frame->has_color_area = has_color_area ? TRUE : FALSE;

      g_object_set (frame->color_area, "visible", frame->has_color_area, NULL);

      g_object_notify (G_OBJECT (frame), "has-color-area");
    }
}

/**
 * gimp_color_frame_set_color:
 * @frame:          The #GimpColorFrame.
 * @sample_average: The set @color is the result of averaging
 * @sample_format:  The format of the #GimpDrawable or #GimpImage the @color
 *                  was picked from.
 * @pixel:          The raw pixel in @sample_format.
 * @color:          The @color to set.
 *
 * Sets the color sample to display in the #GimpColorFrame. if
 * @sample_average is %TRUE, @pixel represents the sample at the
 * center of the average area and will not be displayed.
 **/
void
gimp_color_frame_set_color (GimpColorFrame *frame,
                            gboolean        sample_average,
                            const Babl     *sample_format,
                            gpointer        pixel,
                            const GimpRGB  *color)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));
  g_return_if_fail (color != NULL);

  if (frame->sample_valid                     &&
      frame->sample_average == sample_average &&
      frame->sample_format == sample_format   &&
      gimp_rgba_distance (&frame->color, color) < 0.0001)
    {
      frame->color = *color;
      return;
    }

  frame->sample_valid   = TRUE;
  frame->sample_average = sample_average;
  frame->sample_format  = sample_format;
  frame->color          = *color;

  memcpy (frame->pixel, pixel, babl_format_get_bytes_per_pixel (sample_format));

  gimp_color_frame_update (frame);
}

/**
 * gimp_color_frame_set_invalid:
 * @frame: The #GimpColorFrame.
 *
 * Tells the #GimpColorFrame that the current sample is invalid. All labels
 * visible for the current color space will show "n/a" (not available).
 *
 * There is no special API for setting the frame to "valid" again because
 * this happens automatically when calling gimp_color_frame_set_color().
 **/
void
gimp_color_frame_set_invalid (GimpColorFrame *frame)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (! frame->sample_valid)
    return;

  frame->sample_valid = FALSE;

  gimp_color_frame_update (frame);
}

void
gimp_color_frame_set_color_config (GimpColorFrame  *frame,
                                   GimpColorConfig *config)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  if (config != frame->config)
    {
      if (frame->config)
        {
          g_signal_handlers_disconnect_by_func (frame->config,
                                                gimp_color_frame_destroy_transform,
                                                frame);
          g_object_unref (frame->config);

          gimp_color_frame_destroy_transform (frame);
        }

      frame->config = config;

      if (frame->config)
        {
          g_object_ref (frame->config);

          g_signal_connect_swapped (frame->config, "notify",
                                    G_CALLBACK (gimp_color_frame_destroy_transform),
                                    frame);
        }

      gimp_color_area_set_color_config (GIMP_COLOR_AREA (frame->color_area),
                                        config);
    }
}


/*  private functions  */

static void
gimp_color_frame_menu_callback (GtkWidget      *widget,
                                GimpColorFrame *frame)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      frame->frame_mode = value;
      gimp_color_frame_update (frame);
    }
}

static void
gimp_color_frame_update (GimpColorFrame *frame)
{
  const gchar  *names[GIMP_COLOR_FRAME_ROWS]  = { NULL, };
  gchar       **values = NULL;
  gboolean      has_alpha;
  gint          i;

  has_alpha = babl_format_has_alpha (frame->sample_format);

  if (frame->sample_valid)
    {
      gimp_color_area_set_color (GIMP_COLOR_AREA (frame->color_area),
                                 &frame->color);
    }

  switch (frame->frame_mode)
    {
    case GIMP_COLOR_FRAME_MODE_PIXEL:
      {
        GimpImageBaseType base_type;

        base_type = gimp_babl_format_get_base_type (frame->sample_format);

        if (frame->sample_valid)
          {
            const Babl *print_format = NULL;
            guchar      print_pixel[32];

            switch (gimp_babl_format_get_precision (frame->sample_format))
              {
              case GIMP_PRECISION_U8_GAMMA:
                if (babl_format_is_palette (frame->sample_format))
                  {
                    print_format = gimp_babl_format (GIMP_RGB,
                                                     GIMP_PRECISION_U8_GAMMA,
                                                     has_alpha);
                    break;
                  }
                /* else fall thru */

              case GIMP_PRECISION_U8_LINEAR:
              case GIMP_PRECISION_U16_LINEAR:
              case GIMP_PRECISION_U16_GAMMA:
              case GIMP_PRECISION_U32_LINEAR:
              case GIMP_PRECISION_U32_GAMMA:
              case GIMP_PRECISION_FLOAT_LINEAR:
              case GIMP_PRECISION_FLOAT_GAMMA:
              case GIMP_PRECISION_DOUBLE_LINEAR:
              case GIMP_PRECISION_DOUBLE_GAMMA:
                print_format = frame->sample_format;
                break;

              case GIMP_PRECISION_HALF_GAMMA:
                print_format = gimp_babl_format (base_type,
                                                 GIMP_PRECISION_FLOAT_GAMMA,
                                                 has_alpha);
                break;

              case GIMP_PRECISION_HALF_LINEAR:
                print_format = gimp_babl_format (base_type,
                                                 GIMP_PRECISION_FLOAT_LINEAR,
                                                 has_alpha);
                break;
              }

            if (frame->sample_average)
              {
                /* FIXME: this is broken: can't use the averaged sRGB GimpRGB
                 * value for displaying pixel values when color management
                 * is enabled
                 */
                gimp_rgba_get_pixel (&frame->color, print_format, print_pixel);
              }
            else
              {
                babl_process (babl_fish (frame->sample_format, print_format),
                              frame->pixel, print_pixel, 1);
              }

            values = gimp_babl_print_pixel (print_format, print_pixel);
          }

        if (base_type == GIMP_GRAY)
          {
            names[0] = _("Value:");

            if (has_alpha)
              names[1] = _("Alpha:");
          }
        else
          {
            names[0] = _("Red:");
            names[1] = _("Green:");
            names[2] = _("Blue:");

            if (has_alpha)
              names[3] = _("Alpha:");

            if (babl_format_is_palette (frame->sample_format))
              {
                names[4] = _("Index:");

                if (frame->sample_valid)
                  {
                    gchar **v   = g_new0 (gchar *, 6);
                    gchar **tmp = values;

                    memcpy (v, values, 4 * sizeof (gchar *));
                    values = v;

                    g_free (tmp);

                    if (! frame->sample_average)
                      values[4] = g_strdup_printf ("%d", frame->pixel[0]);
                  }
              }
          }
      }
      break;

    case GIMP_COLOR_FRAME_MODE_RGB:
      names[0] = _("Red:");
      names[1] = _("Green:");
      names[2] = _("Blue:");

      if (has_alpha)
        names[3] = _("Alpha:");

      if (frame->sample_valid)
        {
          values = g_new0 (gchar *, 6);

          values[0] = g_strdup_printf ("%.01f %%", frame->color.r * 100.0);
          values[1] = g_strdup_printf ("%.01f %%", frame->color.g * 100.0);
          values[2] = g_strdup_printf ("%.01f %%", frame->color.b * 100.0);
          values[3] = g_strdup_printf ("%.01f %%", frame->color.a * 100.0);
        }

      names[4] = _("Hex:");

      if (frame->sample_valid)
        {
          guchar r, g, b;

          gimp_rgb_get_uchar (&frame->color, &r, &g, &b);

          values[4] = g_strdup_printf ("%.2x%.2x%.2x", r, g, b);
        }
      break;

    case GIMP_COLOR_FRAME_MODE_HSV:
      names[0] = _("Hue:");
      names[1] = _("Sat.:");
      names[2] = _("Value:");

      if (has_alpha)
        names[3] = _("Alpha:");

      if (frame->sample_valid)
        {
          GimpHSV hsv;

          gimp_rgb_to_hsv (&frame->color, &hsv);
          hsv.a = frame->color.a;

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f \302\260", hsv.h * 360.0);
          values[1] = g_strdup_printf ("%.01f %%",       hsv.s * 100.0);
          values[2] = g_strdup_printf ("%.01f %%",       hsv.v * 100.0);
          values[3] = g_strdup_printf ("%.01f %%",       hsv.a * 100.0);
        }
      break;

    case GIMP_COLOR_FRAME_MODE_CMYK:
      names[0] = _("Cyan:");
      names[1] = _("Magenta:");
      names[2] = _("Yellow:");
      names[3] = _("Black:");

      if (has_alpha)
        names[4] = _("Alpha:");

      if (frame->sample_valid)
        {
          GimpCMYK cmyk;

          if (! frame->transform)
            gimp_color_frame_create_transform (frame);

          if (frame->transform)
            {
              gdouble rgb_values[3];
              gdouble cmyk_values[4];

              rgb_values[0] = frame->color.r;
              rgb_values[1] = frame->color.g;
              rgb_values[2] = frame->color.b;

              gimp_color_transform_process_pixels (frame->transform,
                                                   babl_format ("R'G'B' double"),
                                                   rgb_values,
                                                   babl_format ("CMYK double"),
                                                   cmyk_values,
                                                   1);

              cmyk.c = cmyk_values[0] / 100.0;
              cmyk.m = cmyk_values[1] / 100.0;
              cmyk.y = cmyk_values[2] / 100.0;
              cmyk.k = cmyk_values[3] / 100.0;
            }
          else
            {
              gimp_rgb_to_cmyk (&frame->color, 1.0, &cmyk);
            }

          cmyk.a = frame->color.a;

          values = g_new0 (gchar *, 6);

          values[0] = g_strdup_printf ("%.01f %%", cmyk.c * 100.0);
          values[1] = g_strdup_printf ("%.01f %%", cmyk.m * 100.0);
          values[2] = g_strdup_printf ("%.01f %%", cmyk.y * 100.0);
          values[3] = g_strdup_printf ("%.01f %%", cmyk.k * 100.0);
          values[4] = g_strdup_printf ("%.01f %%", cmyk.a * 100.0);
        }
      break;
    }

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      if (names[i])
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]), names[i]);

          if (frame->sample_valid && values[i])
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), values[i]);
          else
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), _("n/a"));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]),  " ");
          gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), " ");
        }
    }

  g_strfreev (values);
}

static void
gimp_color_frame_create_transform (GimpColorFrame *frame)
{
  if (frame->config)
    {
      GimpColorProfile *cmyk_profile;

      cmyk_profile = gimp_color_config_get_cmyk_color_profile (frame->config,
                                                               NULL);

      if (cmyk_profile)
        {
          static GimpColorProfile *rgb_profile = NULL;

          if (G_UNLIKELY (! rgb_profile))
            rgb_profile = gimp_color_profile_new_rgb_srgb ();

          frame->transform =
            gimp_color_transform_new (rgb_profile,
                                      babl_format ("R'G'B' double"),
                                      cmyk_profile,
                                      babl_format ("CMYK double"),
                                      GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                      GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE |
                                      GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION);
        }
    }
}

static void
gimp_color_frame_destroy_transform (GimpColorFrame *frame)
{
  if (frame->transform)
    {
      g_object_unref (frame->transform);
      frame->transform = NULL;
    }

  gimp_color_frame_update (frame);
}
