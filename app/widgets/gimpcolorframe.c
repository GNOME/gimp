/* The GIMP -- an image manipulation program
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"

#include "gimpcolorframe.h"
#include "gimpenumcombobox.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_color_frame_class_init    (GimpColorFrameClass *klass);
static void   gimp_color_frame_init          (GimpColorFrame      *frame);

static void   gimp_color_frame_menu_callback (GtkWidget           *widget,
                                              GimpColorFrame      *frame);
static void   gimp_color_frame_update        (GimpColorFrame      *frame);


static GimpFrameClass *parent_class = NULL;


GType
gimp_color_frame_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo frame_info =
      {
        sizeof (GimpColorFrameClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_color_frame_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpColorFrame),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_color_frame_init,
      };

      type = g_type_register_static (GIMP_TYPE_FRAME,
                                     "GimpColorFrame",
                                     &frame_info, 0);
    }

  return type;
}

static void
gimp_color_frame_class_init (GimpColorFrameClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_color_frame_init (GimpColorFrame *frame)
{
  GtkWidget *table;
  gint       i;

  frame->sample_valid = FALSE;
  frame->sample_type  = GIMP_RGB_IMAGE;

  gimp_rgba_set (&frame->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  frame->menu = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_FRAME_MODE);
  g_signal_connect (frame->menu, "changed",
                    G_CALLBACK (gimp_color_frame_menu_callback),
                    frame);
  gtk_frame_set_label_widget (GTK_FRAME (frame), frame->menu);
  gtk_widget_show (frame->menu);

  table = gtk_table_new (GIMP_COLOR_FRAME_ROWS, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      frame->name_labels[i] = gtk_label_new (" ");
      gtk_misc_set_alignment (GTK_MISC (frame->name_labels[i]), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), frame->name_labels[i],
                        0, 1, i, i + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (frame->name_labels[i]);

      frame->value_labels[i] = gtk_label_new (" ");
      gtk_label_set_selectable (GTK_LABEL (frame->value_labels[i]), TRUE);
      gtk_misc_set_alignment (GTK_MISC (frame->value_labels[i]), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), frame->value_labels[i],
                        1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (frame->value_labels[i]);
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
  frame->frame_mode = mode;

  gimp_color_frame_update (frame);
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

  frame->sample_valid = FALSE;

  gimp_color_frame_update (frame);
}


/*  private functions  */

static void
gimp_color_frame_menu_callback (GtkWidget      *widget,
                                GimpColorFrame *frame)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 (gint *) &frame->frame_mode);
  gimp_color_frame_update (frame);
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

  gimp_rgba_get_uchar (&frame->color, &r, &g, &b, &a);

  switch (frame->frame_mode)
    {
    case GIMP_COLOR_FRAME_MODE_PIXEL:
      switch (GIMP_IMAGE_TYPE_BASE_TYPE (frame->sample_type))
        {
        case GIMP_INDEXED:
          names[4]  = _("Index:");
          values[4] = g_strdup_printf ("%d", frame->color_index);

        case GIMP_RGB:
          names[0] = _("Red:");
          names[1] = _("Green:");
          names[2] = _("Blue:");

          values[0] = g_strdup_printf ("%d", r);
          values[1] = g_strdup_printf ("%d", g);
          values[2] = g_strdup_printf ("%d", b);

          alpha_row = 3;
          break;

        case GIMP_GRAY:
          names[0]  = _("Value:");
          values[0] = g_strdup_printf ("%d", r);

          alpha_row = 1;
          break;
        }
      break;

    case GIMP_COLOR_FRAME_MODE_RGB:
      names[0] = _("Red:");
      names[1] = _("Green:");
      names[2] = _("Blue:");

      values[0] = g_strdup_printf ("%d %%", ROUND (frame->color.r * 100.0));
      values[1] = g_strdup_printf ("%d %%", ROUND (frame->color.g * 100.0));
      values[2] = g_strdup_printf ("%d %%", ROUND (frame->color.b * 100.0));

      alpha_row = 3;

      names[4]  = _("Hex:");
      values[4] = g_strdup_printf ("%.2x%.2x%.2x", r, g, b);
      break;

    case GIMP_COLOR_FRAME_MODE_HSV:
      {
        GimpHSV hsv;

        gimp_rgb_to_hsv (&frame->color, &hsv);

        names[0] = _("Hue:");
        names[1] = _("Sat.:");
        names[2] = _("Value:");

        values[0] = g_strdup_printf ("%d \302\260", ROUND (hsv.h * 360.0));
        values[1] = g_strdup_printf ("%d %%",       ROUND (hsv.s * 100.0));
        values[2] = g_strdup_printf ("%d %%",       ROUND (hsv.v * 100.0));

        alpha_row = 3;
      }
      break;

    case GIMP_COLOR_FRAME_MODE_CMYK:
      {
        GimpCMYK cmyk;

        gimp_rgb_to_cmyk (&frame->color, 1.0, &cmyk);

        names[0] = _("Cyan:");
        names[1] = _("Magenta:");
        names[2] = _("Yellow:");
        names[3] = _("Black:");

        values[0] = g_strdup_printf ("%d %%", ROUND (cmyk.c * 100.0));
        values[1] = g_strdup_printf ("%d %%", ROUND (cmyk.m * 100.0));
        values[2] = g_strdup_printf ("%d %%", ROUND (cmyk.y * 100.0));
        values[3] = g_strdup_printf ("%d %%", ROUND (cmyk.k * 100.0));

        alpha_row = 4;
      }
      break;
    }

  if (has_alpha)
    {
      names[alpha_row]  = _("Alpha:");

      if (frame->frame_mode == GIMP_COLOR_FRAME_MODE_PIXEL)
        values[alpha_row] = g_strdup_printf ("%d", a);
      else
        values[alpha_row] = g_strdup_printf ("%d %%",
                                             (gint) (frame->color.a * 100.0));
    }

  for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
    {
      if (names[i] && values[i])
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
    }
}
