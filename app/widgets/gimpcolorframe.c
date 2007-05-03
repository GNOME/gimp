/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

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

static void   gimp_color_frame_get_property  (GObject        *object,
                                              guint           property_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);
static void   gimp_color_frame_set_property  (GObject        *object,
                                              guint           property_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void   gimp_color_frame_menu_callback (GtkWidget      *widget,
                                              GimpColorFrame *frame);
static void   gimp_color_frame_update        (GimpColorFrame *frame);


G_DEFINE_TYPE (GimpColorFrame, gimp_color_frame, GIMP_TYPE_FRAME)

#define parent_class gimp_color_frame_parent_class


static void
gimp_color_frame_class_init (GimpColorFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_color_frame_get_property;
  object_class->set_property = gimp_color_frame_set_property;

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
  GtkWidget *hbox;
  gint       i;

  frame->sample_valid = FALSE;
  frame->sample_type  = GIMP_RGB_IMAGE;

  gimp_rgba_set (&frame->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  frame->menu = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_FRAME_MODE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame->menu);
  gtk_widget_show (frame->menu);

  g_signal_connect (frame->menu, "changed",
                    G_CALLBACK (gimp_color_frame_menu_callback),
                    frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame->number_label = gtk_label_new ("0");
  gimp_label_set_attributes (GTK_LABEL (frame->number_label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (frame->number_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), frame->number_label, FALSE, FALSE, 0);

  frame->color_area =
    g_object_new (GIMP_TYPE_COLOR_AREA,
                  "color",          &frame->color,
                  "type",           GIMP_COLOR_AREA_SMALL_CHECKS,
                  "drag-mask",      GDK_BUTTON1_MASK,
                  "draw-border",    TRUE,
                  "height-request", 20,
                  NULL);

  gtk_box_pack_end (GTK_BOX (hbox), frame->color_area, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      hbox = gtk_hbox_new (FALSE, 6);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      frame->name_labels[i] = gtk_label_new (" ");
      gtk_misc_set_alignment (GTK_MISC (frame->name_labels[i]), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), frame->name_labels[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (frame->name_labels[i]);

      frame->value_labels[i] = gtk_label_new (" ");
      gtk_label_set_selectable (GTK_LABEL (frame->value_labels[i]), TRUE);
      gtk_misc_set_alignment (GTK_MISC (frame->value_labels[i]), 1.0, 0.5);
      gtk_box_pack_end (GTK_BOX (hbox), frame->value_labels[i],
                        FALSE, FALSE, 0);
      gtk_widget_show (frame->value_labels[i]);
    }
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

      g_object_set (frame->number_label, "visible", frame->has_number, NULL);

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
      gchar str[8];

      frame->number = number;

      g_snprintf (str, sizeof (str), "%d", number);
      gtk_label_set_text (GTK_LABEL (frame->number_label), str);

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
 * @frame:       The #GimpColorFrame.
 * @sample_type: The type of the #GimpDrawable or #GimpImage the @color
 *               was picked from.
 * @color:       The @color to set.
 * @color_index: The @color's index. This value is ignored unless
 *               @sample_type equals to #GIMP_INDEXED_IMAGE or
 *               #GIMP_INDEXEDA_IMAGE.
 *
 * Sets the color sample to display in the #GimpColorFrame.
 **/
void
gimp_color_frame_set_color (GimpColorFrame *frame,
                            GimpImageType   sample_type,
                            const GimpRGB  *color,
                            gint            color_index)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));
  g_return_if_fail (color != NULL);

  if (frame->sample_valid               &&
      frame->sample_type == sample_type &&
      frame->color_index == color_index &&
      gimp_rgba_distance (&frame->color, color) < 0.0001)
    {
      frame->color = *color;
      return;
    }

  frame->sample_valid = TRUE;
  frame->sample_type  = sample_type;
  frame->color        = *color;
  frame->color_index  = color_index;

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
  const gchar *names[GIMP_COLOR_FRAME_ROWS]  = { NULL, };
  gchar       *values[GIMP_COLOR_FRAME_ROWS] = { NULL, };
  gboolean     has_alpha;
  gint         alpha_row = 3;
  guchar       r, g, b, a;
  gint         i;

  has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (frame->sample_type);

  if (frame->sample_valid)
    {
      gimp_rgba_get_uchar (&frame->color, &r, &g, &b, &a);

      gimp_color_area_set_color (GIMP_COLOR_AREA (frame->color_area),
                                 &frame->color);
    }

  switch (frame->frame_mode)
    {
    case GIMP_COLOR_FRAME_MODE_PIXEL:
      switch (GIMP_IMAGE_TYPE_BASE_TYPE (frame->sample_type))
        {
        case GIMP_INDEXED:
          names[4] = _("Index:");

          if (frame->sample_valid)
            {
              /* color_index will be -1 for an averaged sample */
              if (frame->color_index < 0)
                names[4] = NULL;
              else
                values[4] = g_strdup_printf ("%d", frame->color_index);
            }
          /* fallthrough */

        case GIMP_RGB:
          names[0] = _("Red:");
          names[1] = _("Green:");
          names[2] = _("Blue:");

          if (frame->sample_valid)
            {
              values[0] = g_strdup_printf ("%d", r);
              values[1] = g_strdup_printf ("%d", g);
              values[2] = g_strdup_printf ("%d", b);
            }

          alpha_row = 3;
          break;

        case GIMP_GRAY:
          names[0]  = _("Value:");

          if (frame->sample_valid)
            values[0] = g_strdup_printf ("%d", r);

          alpha_row = 1;
          break;
        }
      break;

    case GIMP_COLOR_FRAME_MODE_RGB:
      names[0] = _("Red:");
      names[1] = _("Green:");
      names[2] = _("Blue:");

      if (frame->sample_valid)
        {
          values[0] = g_strdup_printf ("%d %%", ROUND (frame->color.r * 100.0));
          values[1] = g_strdup_printf ("%d %%", ROUND (frame->color.g * 100.0));
          values[2] = g_strdup_printf ("%d %%", ROUND (frame->color.b * 100.0));
        }

      alpha_row = 3;

      names[4]  = _("Hex:");

      if (frame->sample_valid)
        values[4] = g_strdup_printf ("%.2x%.2x%.2x", r, g, b);
      break;

    case GIMP_COLOR_FRAME_MODE_HSV:
      names[0] = _("Hue:");
      names[1] = _("Sat.:");
      names[2] = _("Value:");

      if (frame->sample_valid)
        {
          GimpHSV hsv;

          gimp_rgb_to_hsv (&frame->color, &hsv);

          values[0] = g_strdup_printf ("%d \302\260", ROUND (hsv.h * 360.0));
          values[1] = g_strdup_printf ("%d %%",       ROUND (hsv.s * 100.0));
          values[2] = g_strdup_printf ("%d %%",       ROUND (hsv.v * 100.0));
        }

      alpha_row = 3;
      break;

    case GIMP_COLOR_FRAME_MODE_CMYK:
      names[0] = _("Cyan:");
      names[1] = _("Magenta:");
      names[2] = _("Yellow:");
      names[3] = _("Black:");

      if (frame->sample_valid)
        {
          GimpCMYK cmyk;

          gimp_rgb_to_cmyk (&frame->color, 1.0, &cmyk);

          values[0] = g_strdup_printf ("%d %%", ROUND (cmyk.c * 100.0));
          values[1] = g_strdup_printf ("%d %%", ROUND (cmyk.m * 100.0));
          values[2] = g_strdup_printf ("%d %%", ROUND (cmyk.y * 100.0));
          values[3] = g_strdup_printf ("%d %%", ROUND (cmyk.k * 100.0));
        }

      alpha_row = 4;
      break;
    }

  if (has_alpha)
    {
      names[alpha_row] = _("Alpha:");

      if (frame->sample_valid)
        {
          if (frame->frame_mode == GIMP_COLOR_FRAME_MODE_PIXEL)
            values[alpha_row] = g_strdup_printf ("%d", a);
          else
            values[alpha_row] = g_strdup_printf ("%d %%",
                                                 (gint) (frame->color.a * 100.0));
        }
    }

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      if (names[i])
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]), names[i]);

          if (frame->sample_valid)
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), values[i]);
          else
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), _("n/a"));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]),  " ");
          gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), " ");
        }

      g_free (values[i]);
    }
}
