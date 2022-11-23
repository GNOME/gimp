/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"

#include "ligmacolorframe.h"

#include "ligma-intl.h"


#define RGBA_EPSILON 1e-6

enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_MODE,
  PROP_HAS_NUMBER,
  PROP_NUMBER,
  PROP_HAS_COLOR_AREA,
  PROP_HAS_COORDS,
  PROP_ELLIPSIZE,
};


/*  local function prototypes  */

static void       ligma_color_frame_dispose             (GObject        *object);
static void       ligma_color_frame_finalize            (GObject        *object);
static void       ligma_color_frame_get_property        (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void       ligma_color_frame_set_property        (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);

static void       ligma_color_frame_style_updated       (GtkWidget      *widget);
static gboolean   ligma_color_frame_draw                (GtkWidget      *widget,
                                                        cairo_t        *cr);

static void       ligma_color_frame_combo_callback      (GtkWidget      *widget,
                                                        LigmaColorFrame *frame);
static void       ligma_color_frame_update              (LigmaColorFrame *frame);

static void       ligma_color_frame_image_changed       (LigmaColorFrame *frame,
                                                        LigmaImage      *image,
                                                        LigmaContext    *context);

static void       ligma_color_frame_update_simulation   (LigmaImage        *image,
                                                        LigmaColorFrame   *frame);

G_DEFINE_TYPE (LigmaColorFrame, ligma_color_frame, LIGMA_TYPE_FRAME)

#define parent_class ligma_color_frame_parent_class


static void
ligma_color_frame_class_init (LigmaColorFrameClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose       = ligma_color_frame_dispose;
  object_class->finalize      = ligma_color_frame_finalize;
  object_class->get_property  = ligma_color_frame_get_property;
  object_class->set_property  = ligma_color_frame_set_property;

  widget_class->style_updated = ligma_color_frame_style_updated;
  widget_class->draw          = ligma_color_frame_draw;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_COLOR_PICK_MODE,
                                                      LIGMA_COLOR_PICK_MODE_PIXEL,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_NUMBER,
                                   g_param_spec_boolean ("has-number",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NUMBER,
                                   g_param_spec_int ("number",
                                                     NULL, NULL,
                                                     0, 256, 0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_COLOR_AREA,
                                   g_param_spec_boolean ("has-color-area",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_COORDS,
                                   g_param_spec_boolean ("has-coords",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      LIGMA_PARAM_READWRITE));
}

static void
ligma_color_frame_init (LigmaColorFrame *frame)
{
  GtkListStore *store;
  GtkWidget    *vbox;
  GtkWidget    *vbox2;
  GtkWidget    *label;
  gint          i;

  frame->sample_valid  = FALSE;
  frame->sample_format = babl_format ("R'G'B' u8");

  ligma_rgba_set (&frame->color, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);

  /* create the store manually so the values have a nice order */
  store = ligma_enum_store_new_with_values (LIGMA_TYPE_COLOR_PICK_MODE,
                                           LIGMA_COLOR_PICK_MODE_LAST + 1,
                                           LIGMA_COLOR_PICK_MODE_PIXEL,
                                           LIGMA_COLOR_PICK_MODE_RGB_PERCENT,
                                           LIGMA_COLOR_PICK_MODE_RGB_U8,
                                           LIGMA_COLOR_PICK_MODE_HSV,
                                           LIGMA_COLOR_PICK_MODE_LCH,
                                           LIGMA_COLOR_PICK_MODE_LAB,
                                           LIGMA_COLOR_PICK_MODE_XYY,
                                           LIGMA_COLOR_PICK_MODE_YUV,
                                           LIGMA_COLOR_PICK_MODE_CMYK);
  frame->combo = ligma_enum_combo_box_new_with_model (LIGMA_ENUM_STORE (store));
  g_object_unref (store);

  gtk_frame_set_label_widget (GTK_FRAME (frame), frame->combo);
  gtk_widget_show (frame->combo);

  g_signal_connect (frame->combo, "changed",
                    G_CALLBACK (ligma_color_frame_combo_callback),
                    frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame->color_area =
    g_object_new (LIGMA_TYPE_COLOR_AREA,
                  "color",          &frame->color,
                  "type",           LIGMA_COLOR_AREA_SMALL_CHECKS,
                  "drag-mask",      GDK_BUTTON1_MASK,
                  "draw-border",    TRUE,
                  "height-request", 20,
                  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame->color_area, FALSE, FALSE, 0);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_set_homogeneous (GTK_BOX (vbox2), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  for (i = 0; i < LIGMA_COLOR_FRAME_ROWS; i++)
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
      gtk_label_set_ellipsize (GTK_LABEL (frame->value_labels[i]),
                               PANGO_ELLIPSIZE_END);
      gtk_box_pack_end (GTK_BOX (hbox), frame->value_labels[i],
                        TRUE, TRUE, 0);
      gtk_widget_show (frame->value_labels[i]);
    }

  frame->coords_box_x = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), frame->coords_box_x, FALSE, FALSE, 0);

  /* TRANSLATORS: X for the X coordinate. */
  label = gtk_label_new (C_("Coordinates", "X:"));
  gtk_box_pack_start (GTK_BOX (frame->coords_box_x), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  frame->coords_label_x = gtk_label_new (" ");
  gtk_label_set_selectable (GTK_LABEL (frame->coords_label_x), TRUE);
  gtk_box_pack_end (GTK_BOX (frame->coords_box_x), frame->coords_label_x,
                    FALSE, FALSE, 0);
  gtk_widget_show (frame->coords_label_x);

  frame->coords_box_y = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), frame->coords_box_y, FALSE, FALSE, 0);

  /* TRANSLATORS: Y for the Y coordinate. */
  label = gtk_label_new (C_("Coordinates", "Y:"));
  gtk_box_pack_start (GTK_BOX (frame->coords_box_y), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  frame->coords_label_y = gtk_label_new (" ");
  gtk_label_set_selectable (GTK_LABEL (frame->coords_label_y), TRUE);
  gtk_box_pack_end (GTK_BOX (frame->coords_box_y), frame->coords_label_y,
                    FALSE, FALSE, 0);
  gtk_widget_show (frame->coords_label_y);
}

static void
ligma_color_frame_dispose (GObject *object)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (object);

  if (frame->ligma)
    {
      g_signal_handlers_disconnect_by_func (ligma_get_user_context (frame->ligma),
                                            ligma_color_frame_image_changed,
                                            frame);
      frame->ligma = NULL;
    }

  ligma_color_frame_set_image (frame, NULL);

  ligma_color_frame_set_color_config (frame, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_frame_finalize (GObject *object)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (object);

  g_clear_object (&frame->number_layout);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_frame_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, frame->ligma);
      break;

    case PROP_MODE:
      g_value_set_enum (value, frame->pick_mode);
      break;

    case PROP_ELLIPSIZE:
      g_value_set_enum (value, frame->ellipsize);
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

    case PROP_HAS_COORDS:
      g_value_set_boolean (value, frame->has_coords);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_frame_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (object);
  LigmaContext    *context;
  LigmaImage      *image;

  switch (property_id)
    {
    case PROP_LIGMA:
      frame->ligma = g_value_get_object (value);
      if (frame->ligma)
        {
          context = ligma_get_user_context (frame->ligma);
          image   = ligma_context_get_image (context);

          g_signal_connect_swapped (context, "image-changed",
                                    G_CALLBACK (ligma_color_frame_image_changed),
                                    frame);
          ligma_color_frame_image_changed (frame, image, context);
        }
      break;

    case PROP_MODE:
      ligma_color_frame_set_mode (frame, g_value_get_enum (value));
      break;

    case PROP_ELLIPSIZE:
      ligma_color_frame_set_ellipsize (frame, g_value_get_enum (value));
      break;

    case PROP_HAS_NUMBER:
      ligma_color_frame_set_has_number (frame, g_value_get_boolean (value));
      break;

    case PROP_NUMBER:
      ligma_color_frame_set_number (frame, g_value_get_int (value));
      break;

    case PROP_HAS_COLOR_AREA:
      ligma_color_frame_set_has_color_area (frame, g_value_get_boolean (value));
      break;

    case PROP_HAS_COORDS:
      ligma_color_frame_set_has_coords (frame, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_frame_style_updated (GtkWidget *widget)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&frame->number_layout);
}

static gboolean
ligma_color_frame_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaColorFrame *frame = LIGMA_COLOR_FRAME (widget);

  if (frame->has_number)
    {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      GtkAllocation    allocation;
      GtkAllocation    combo_allocation;
      GtkAllocation    color_area_allocation;
      GtkAllocation    coords_box_x_allocation;
      GtkAllocation    coords_box_y_allocation;
      GdkRGBA          color;
      gchar            buf[8];
      gint             w, h;
      gdouble          scale;

      cairo_save (cr);

      gtk_widget_get_allocation (widget, &allocation);
      gtk_widget_get_allocation (frame->combo, &combo_allocation);
      gtk_widget_get_allocation (frame->color_area, &color_area_allocation);
      gtk_widget_get_allocation (frame->coords_box_x, &coords_box_x_allocation);
      gtk_widget_get_allocation (frame->coords_box_y, &coords_box_y_allocation);

      gtk_style_context_get_color (style,
                                   gtk_style_context_get_state (style),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);

      g_snprintf (buf, sizeof (buf), "%d", frame->number);

      if (! frame->number_layout)
        frame->number_layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (frame->number_layout, buf, -1);
      pango_layout_get_pixel_size (frame->number_layout, &w, &h);

      scale = ((gdouble) (allocation.height -
                          combo_allocation.height -
                          color_area_allocation.height -
                          (coords_box_x_allocation.height +
                           coords_box_y_allocation.height)) /
               (gdouble) h);

      cairo_scale (cr, scale, scale);

      cairo_move_to (cr,
                     (allocation.width / 2.0) / scale - w / 2.0,
                     (allocation.height / 2.0 +
                      combo_allocation.height / 2.0 +
                      color_area_allocation.height / 2.0 +
                      coords_box_x_allocation.height / 2.0 +
                      coords_box_y_allocation.height / 2.0) / scale - h / 2.0);

      cairo_push_group (cr);
      pango_cairo_show_layout (cr, frame->number_layout);
      cairo_pop_group_to_source (cr);
      cairo_paint_with_alpha (cr, 0.2);

      cairo_restore (cr);
    }

  return GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
}


/*  public functions  */

/**
 * ligma_color_frame_new:
 * @ligma: A #Ligma object.
 *
 * Creates a new #LigmaColorFrame widget.
 *
 * Returns: The new #LigmaColorFrame widget.
 **/
GtkWidget *
ligma_color_frame_new (Ligma *ligma)
{
  return g_object_new (LIGMA_TYPE_COLOR_FRAME,
                       "ligma", ligma,
                       NULL);
}


/**
 * ligma_color_frame_set_mode:
 * @frame: The #LigmaColorFrame.
 * @mode:  The new @mode.
 *
 * Sets the #LigmaColorFrame's color pick @mode. Calling this function
 * does the same as selecting the @mode from the frame's #GtkComboBox.
 **/
void
ligma_color_frame_set_mode (LigmaColorFrame    *frame,
                           LigmaColorPickMode  mode)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (frame->combo), mode);
}

void
ligma_color_frame_set_ellipsize (LigmaColorFrame     *frame,
                                PangoEllipsizeMode  ellipsize)
{
  gint i;

  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (ellipsize != frame->ellipsize)
    {
      frame->ellipsize = ellipsize;

      for (i = 0; i < LIGMA_COLOR_FRAME_ROWS; i++)
        {
          if (frame->value_labels[i])
            gtk_label_set_ellipsize (GTK_LABEL (frame->value_labels[i]),
                                     ellipsize);
        }
    }
}

void
ligma_color_frame_set_has_number (LigmaColorFrame *frame,
                                 gboolean        has_number)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (has_number != frame->has_number)
    {
      frame->has_number = has_number ? TRUE : FALSE;

      gtk_widget_queue_draw (GTK_WIDGET (frame));

      g_object_notify (G_OBJECT (frame), "has-number");
    }
}

void
ligma_color_frame_set_number (LigmaColorFrame *frame,
                             gint            number)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (number != frame->number)
    {
      frame->number = number;

      gtk_widget_queue_draw (GTK_WIDGET (frame));

      g_object_notify (G_OBJECT (frame), "number");
    }
}

void
ligma_color_frame_set_has_color_area (LigmaColorFrame *frame,
                                     gboolean        has_color_area)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (has_color_area != frame->has_color_area)
    {
      frame->has_color_area = has_color_area ? TRUE : FALSE;

      g_object_set (frame->color_area, "visible", frame->has_color_area, NULL);

      g_object_notify (G_OBJECT (frame), "has-color-area");
    }
}

void
ligma_color_frame_set_has_coords (LigmaColorFrame *frame,
                                 gboolean        has_coords)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (has_coords != frame->has_coords)
    {
      frame->has_coords = has_coords ? TRUE : FALSE;

      g_object_set (frame->coords_box_x, "visible", frame->has_coords, NULL);
      g_object_set (frame->coords_box_y, "visible", frame->has_coords, NULL);

      g_object_notify (G_OBJECT (frame), "has-coords");
    }
}

/**
 * ligma_color_frame_set_color:
 * @frame:          The #LigmaColorFrame.
 * @sample_average: The set @color is the result of averaging
 * @sample_format:  The format of the #LigmaDrawable or #LigmaImage the @color
 *                  was picked from.
 * @pixel:          The raw pixel in @sample_format.
 * @color:          The @color to set.
 * @x:              X position where the color was picked.
 * @y:              Y position where the color was picked.
 *
 * Sets the color sample to display in the #LigmaColorFrame. if
 * @sample_average is %TRUE, @pixel represents the sample at the
 * center of the average area and will not be displayed.
 **/
void
ligma_color_frame_set_color (LigmaColorFrame *frame,
                            gboolean        sample_average,
                            const Babl     *sample_format,
                            gpointer        pixel,
                            const LigmaRGB  *color,
                            gint            x,
                            gint            y)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));
  g_return_if_fail (color != NULL);

  if (frame->sample_valid                     &&
      frame->sample_average == sample_average &&
      frame->sample_format  == sample_format  &&
      frame->x              == x              &&
      frame->y              == y              &&
      ligma_rgba_distance (&frame->color, color) < RGBA_EPSILON)
    {
      frame->color = *color;
      return;
    }

  frame->sample_valid   = TRUE;
  frame->sample_average = sample_average;
  frame->sample_format  = sample_format;
  frame->color          = *color;
  frame->x              = x;
  frame->y              = y;

  memcpy (frame->pixel, pixel, babl_format_get_bytes_per_pixel (sample_format));

  ligma_color_frame_update (frame);
}

/**
 * ligma_color_frame_set_invalid:
 * @frame: The #LigmaColorFrame.
 *
 * Tells the #LigmaColorFrame that the current sample is invalid. All labels
 * visible for the current color space will show "n/a" (not available).
 *
 * There is no special API for setting the frame to "valid" again because
 * this happens automatically when calling ligma_color_frame_set_color().
 **/
void
ligma_color_frame_set_invalid (LigmaColorFrame *frame)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (! frame->sample_valid)
    return;

  frame->sample_valid = FALSE;

  ligma_color_frame_update (frame);
}

void
ligma_color_frame_set_color_config (LigmaColorFrame  *frame,
                                   LigmaColorConfig *config)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  if (config != frame->config)
    {
      if (frame->config)
        {
          g_object_unref (frame->config);

          ligma_color_frame_update (frame);
        }

      frame->config = config;

      if (frame->config)
          g_object_ref (frame->config);

      ligma_color_area_set_color_config (LIGMA_COLOR_AREA (frame->color_area),
                                        config);
    }
}

void
ligma_color_frame_set_image (LigmaColorFrame *frame,
                            LigmaImage      *image)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  if (image != frame->image)
    {
      if (frame->image)
        {
          g_signal_handlers_disconnect_by_func (frame->image,
                                                ligma_color_frame_update_simulation,
                                                frame);
          g_object_unref (frame->image);
        }
    }

  frame->image = image;

  if (frame->image)
    {
      g_object_ref (frame->image);

      g_signal_connect (frame->image, "simulation-profile-changed",
                        G_CALLBACK (ligma_color_frame_update_simulation),
                        frame);
      g_signal_connect (frame->image, "simulation-intent-changed",
                        G_CALLBACK (ligma_color_frame_update_simulation),
                        frame);

      ligma_color_frame_update_simulation (frame->image, frame);
    }
}

/*  private functions  */

static void
ligma_color_frame_combo_callback (GtkWidget      *widget,
                                 LigmaColorFrame *frame)
{
  gint value;

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &value))
    {
      frame->pick_mode = value;
      ligma_color_frame_update (frame);

      g_object_notify (G_OBJECT (frame), "mode");
    }
}

static void
ligma_color_frame_update (LigmaColorFrame *frame)
{
  const gchar      *names[LIGMA_COLOR_FRAME_ROWS]   = { NULL, };
  gchar           **values                         = NULL;
  const gchar      *tooltip[LIGMA_COLOR_FRAME_ROWS] = { NULL, };
  gboolean          has_alpha;
  LigmaColorProfile *color_profile                  = NULL;
  gint              i;

  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  has_alpha = babl_format_has_alpha (frame->sample_format);

  if (frame->sample_valid)
    {
      gchar str[16];

      ligma_color_area_set_color (LIGMA_COLOR_AREA (frame->color_area),
                                 &frame->color);

      g_snprintf (str, sizeof (str), "%d", frame->x);
      gtk_label_set_text (GTK_LABEL (frame->coords_label_x), str);

      g_snprintf (str, sizeof (str), "%d", frame->y);
      gtk_label_set_text (GTK_LABEL (frame->coords_label_y), str);
    }
  else
    {
      /* TRANSLATORS: n/a for Not Available. */
      gtk_label_set_text (GTK_LABEL (frame->coords_label_x), C_("Coordinates", "n/a"));
      /* TRANSLATORS: n/a for Not Available. */
      gtk_label_set_text (GTK_LABEL (frame->coords_label_y), C_("Coordinates", "n/a"));
    }

  switch (frame->pick_mode)
    {
    case LIGMA_COLOR_PICK_MODE_PIXEL:
      {
        LigmaImageBaseType  base_type;
        LigmaTRCType        trc;
        const Babl        *space;

        base_type = ligma_babl_format_get_base_type (frame->sample_format);
        trc       = ligma_babl_format_get_trc (frame->sample_format);
        space     = babl_format_get_space (frame->sample_format);

        if (frame->sample_valid)
          {
            const Babl *print_format = NULL;
            guchar      print_pixel[32];

            switch (ligma_babl_format_get_precision (frame->sample_format))
              {
              case LIGMA_PRECISION_U8_NON_LINEAR:
                if (babl_format_is_palette (frame->sample_format))
                  {
                    print_format = ligma_babl_format (LIGMA_RGB,
                                                     LIGMA_PRECISION_U8_NON_LINEAR,
                                                     has_alpha,
                                                     space);
                    break;
                  }
                /* else fall thru */

              default:
                print_format = frame->sample_format;
                break;

              case LIGMA_PRECISION_HALF_LINEAR:
              case LIGMA_PRECISION_HALF_NON_LINEAR:
              case LIGMA_PRECISION_HALF_PERCEPTUAL:
                print_format =
                  ligma_babl_format (base_type,
                                    ligma_babl_precision (LIGMA_COMPONENT_TYPE_FLOAT,
                                                         trc),
                                    has_alpha,
                                    space);
                break;
              }

            if (frame->sample_average)
              {
                /* FIXME: this is broken: can't use the averaged sRGB LigmaRGB
                 * value for displaying pixel values when color management
                 * is enabled
                 */
                ligma_rgba_get_pixel (&frame->color, print_format, print_pixel);
              }
            else
              {
                babl_process (babl_fish (frame->sample_format, print_format),
                              frame->pixel, print_pixel, 1);
              }

            values = ligma_babl_print_pixel (print_format, print_pixel);
          }

        if (base_type == LIGMA_GRAY)
          {
            /* TRANSLATORS: V for Value (grayscale) */
            names[0] = C_("Grayscale", "V:");

            if (has_alpha)
              /* TRANSLATORS: A for Alpha (color transparency) */
              names[1] = C_("Alpha channel", "A:");
          }
        else
          {
            /* TRANSLATORS: R for Red (RGB) */
            names[0] = C_("RGB", "R:");
            /* TRANSLATORS: G for Green (RGB) */
            names[1] = C_("RGB", "G:");
            /* TRANSLATORS: B for Blue (RGB) */
            names[2] = C_("RGB", "B:");

            if (has_alpha)
              /* TRANSLATORS: A for Alpha (color transparency) */
              names[3] = C_("Alpha channel", "A:");

            if (babl_format_is_palette (frame->sample_format))
              {
                /* TRANSLATORS: Index of the color in the palette. */
                names[4] = C_("Indexed color", "Index:");

                if (frame->sample_valid)
                  {
                    gchar **v   = g_new0 (gchar *, 6);
                    gchar **tmp = values;

                    memcpy (v, values, 4 * sizeof (gchar *));
                    values = v;

                    g_free (tmp);

                    if (! frame->sample_average)
                      {
                        values[4] = g_strdup_printf (
                          "%d", ((guint8 *) frame->pixel)[0]);
                      }
                  }
              }
          }
      }
      break;

    case LIGMA_COLOR_PICK_MODE_RGB_PERCENT:
    case LIGMA_COLOR_PICK_MODE_RGB_U8:
      /* TRANSLATORS: R for Red (RGB) */
      names[0] = C_("RGB", "R:");
      /* TRANSLATORS: G for Green (RGB) */
      names[1] = C_("RGB", "G:");
      /* TRANSLATORS: B for Blue (RGB) */
      names[2] = C_("RGB", "B:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      /* TRANSLATORS: Hex for Hexadecimal (representation of a color) */
      names[4] = C_("Color representation", "Hex:");

      if (frame->sample_valid)
        {
          guchar r, g, b, a;

          values = g_new0 (gchar *, 6);

          ligma_rgba_get_uchar (&frame->color, &r, &g, &b, &a);

          if (frame->pick_mode == LIGMA_COLOR_PICK_MODE_RGB_PERCENT)
            {
              values[0] = g_strdup_printf ("%.01f %%", frame->color.r * 100.0);
              values[1] = g_strdup_printf ("%.01f %%", frame->color.g * 100.0);
              values[2] = g_strdup_printf ("%.01f %%", frame->color.b * 100.0);
              values[3] = g_strdup_printf ("%.01f %%", frame->color.a * 100.0);
            }
          else
            {
              values[0] = g_strdup_printf ("%d", r);
              values[1] = g_strdup_printf ("%d", g);
              values[2] = g_strdup_printf ("%d", b);
              values[3] = g_strdup_printf ("%d", a);
            }

          values[4] = g_strdup_printf ("%.2x%.2x%.2x", r, g, b);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_HSV:
      /* TRANSLATORS: H for Hue (HSV color space) */
      names[0] = C_("HSV color space", "H:");
      /* TRANSLATORS: S for Saturation (HSV color space) */
      names[1] = C_("HSV color space", "S:");
      /* TRANSLATORS: V for Value (HSV color space) */
      names[2] = C_("HSV color space", "V:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      if (frame->sample_valid)
        {
          LigmaHSV hsv;

          ligma_rgb_to_hsv (&frame->color, &hsv);
          hsv.a = frame->color.a;

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f \302\260", hsv.h * 360.0);
          values[1] = g_strdup_printf ("%.01f %%",       hsv.s * 100.0);
          values[2] = g_strdup_printf ("%.01f %%",       hsv.v * 100.0);
          values[3] = g_strdup_printf ("%.01f %%",       hsv.a * 100.0);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_LCH:
      /* TRANSLATORS: L for Lightness (LCH color space) */
      names[0] = C_("LCH color space", "L*:");
      /* TRANSLATORS: C for Chroma (LCH color space) */
      names[1] = C_("LCH color space", "C*:");
      /* TRANSLATORS: H for Hue angle (LCH color space) */
      names[2] = C_("LCH color space", "h\302\260:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      if (frame->sample_valid)
        {
          static const Babl *fish = NULL;
          gfloat             lch[4];

          if (G_UNLIKELY (! fish))
            fish = babl_fish (babl_format ("R'G'B'A double"),
                              babl_format ("CIE LCH(ab) alpha float"));

          babl_process (fish, &frame->color, lch, 1);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f  ",        lch[0]);
          values[1] = g_strdup_printf ("%.01f  ",        lch[1]);
          values[2] = g_strdup_printf ("%.01f \302\260", lch[2]);
          values[3] = g_strdup_printf ("%.01f %%",       lch[3] * 100.0);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_LAB:
      /* TRANSLATORS: L* for Lightness (Lab color space) */
      names[0] = C_("Lab color space", "L*:");
      /* TRANSLATORS: a* color channel in Lab color space */
      names[1] = C_("Lab color space", "a*:");
      /* TRANSLATORS: b* color channel in Lab color space */
      names[2] = C_("Lab color space", "b*:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      if (frame->sample_valid)
        {
          static const Babl *fish = NULL;
          gfloat             lab[4];

          if (G_UNLIKELY (! fish))
            fish = babl_fish (babl_format ("R'G'B'A double"),
                              babl_format ("CIE Lab alpha float"));

          babl_process (fish, &frame->color, lab, 1);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f  ",  lab[0]);
          values[1] = g_strdup_printf ("%.01f  ",  lab[1]);
          values[2] = g_strdup_printf ("%.01f  ",  lab[2]);
          values[3] = g_strdup_printf ("%.01f %%", lab[3] * 100.0);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_XYY:
      /* TRANSLATORS: x from xyY color space */
      names[0] = C_("xyY color space", "x:");
      /* TRANSLATORS: y from xyY color space */
      names[1] = C_("xyY color space", "y:");
      /* TRANSLATORS: Y from xyY color space */
      names[2] = C_("xyY color space", "Y:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      if (frame->sample_valid)
        {
          static const Babl *fish = NULL;
          gfloat             xyY[4];

          if (G_UNLIKELY (! fish))
            fish = babl_fish (babl_format ("R'G'B'A double"),
                              babl_format ("CIE xyY alpha float"));

          babl_process (fish, &frame->color, xyY, 1);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%1.6f  ",  xyY[0]);
          values[1] = g_strdup_printf ("%1.6f  ",  xyY[1]);
          values[2] = g_strdup_printf ("%1.6f  ",  xyY[2]);
          values[3] = g_strdup_printf ("%.01f %%", xyY[3] * 100.0);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_YUV:
      /* TRANSLATORS: Y from Yu'v' color space */
      names[0] = C_("Yu'v' color space", "Y:");
      /* TRANSLATORS: u' from Yu'v' color space */
      names[1] = C_("Yu'v' color space", "u':");
      /* TRANSLATORS: v' from Yu'v' color space */
      names[2] = C_("Yu'v' color space", "v':");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[3] = C_("Alpha channel", "A:");

      if (frame->sample_valid)
        {
          static const Babl *fish = NULL;
          gfloat             Yuv[4];

          if (G_UNLIKELY (! fish))
            fish = babl_fish (babl_format ("R'G'B'A double"),
                              babl_format ("CIE Yuv alpha float"));

          babl_process (fish, &frame->color, Yuv, 1);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%1.6f  ",  Yuv[0]);
          values[1] = g_strdup_printf ("%1.6f  ",  Yuv[1]);
          values[2] = g_strdup_printf ("%1.6f  ",  Yuv[2]);
          values[3] = g_strdup_printf ("%.01f %%", Yuv[3] * 100.0);
        }
      break;

    case LIGMA_COLOR_PICK_MODE_CMYK:
      {
        const Babl *space = NULL;

        /* Try to load CMYK profile in the following order:
         * 1) Soft-Proof Profile set in the View Menu
         * 2) Preferred CMYK Profile set in Preferences
         * 3) No CMYK Profile (Default Values)
         */
        color_profile = frame->view_profile;

        if (! color_profile && frame->config)
          color_profile = ligma_color_config_get_cmyk_color_profile (frame->config,
                                                                    NULL);
        if (color_profile)
          space = ligma_color_profile_get_space (color_profile,
                                                frame->simulation_intent,
                                                NULL);

        /* TRANSLATORS: C for Cyan (CMYK) */
        names[0] = C_("CMYK", "C:");
        /* TRANSLATORS: M for Magenta (CMYK) */
        names[1] = C_("CMYK", "M:");
        /* TRANSLATORS: Y for Yellow (CMYK) */
        names[2] = C_("CMYK", "Y:");
        /* TRANSLATORS: K for Key/black (CMYK) */
        names[3] = C_("CMYK", "K:");
        if (has_alpha)
          /* TRANSLATORS: A for Alpha (color transparency) */
          names[4] = C_("Alpha channel", "A:");
        if (color_profile)
          names[5] = C_("Color", "Profile:");
        else
          names[5] = C_("Color", "No Profile");

        if (frame->sample_valid)
          {
            const Babl  *fish = NULL;
            gfloat       cmyk[4];
            const gchar *profile_label;

            /* User may swap CMYK color profiles at runtime */
            fish = babl_fish (babl_format ("R'G'B'A double"),
                              babl_format_with_space ("CMYK float", space));

            babl_process (fish, &frame->color, cmyk, 1);

            values = g_new0 (gchar *, 6);

            values[0] = g_strdup_printf ("%.0f %%", cmyk[0] * 100.0);
            values[1] = g_strdup_printf ("%.0f %%", cmyk[1] * 100.0);
            values[2] = g_strdup_printf ("%.0f %%", cmyk[2] * 100.0);
            values[3] = g_strdup_printf ("%.0f %%", cmyk[3] * 100.0);
            if (has_alpha)
              values[4] = g_strdup_printf ("%.01f %%", frame->color.a * 100.0);
            if (color_profile)
              {
                profile_label = ligma_color_profile_get_label (color_profile);
                values[5] = g_strdup_printf (_("%s"), profile_label);
                tooltip[5] = g_strdup_printf (_("%s"), profile_label);
              }
          }
      }
      break;
    }

  for (i = 0; i < LIGMA_COLOR_FRAME_ROWS; i++)
    {
      if (names[i])
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]), names[i]);

          if (frame->sample_valid && values[i])
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), values[i]);
          else
            gtk_label_set_text (GTK_LABEL (frame->value_labels[i]),
                                C_("Color value", "n/a"));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (frame->name_labels[i]),  " ");
          gtk_label_set_text (GTK_LABEL (frame->value_labels[i]), " ");
        }

      gtk_widget_set_tooltip_text (GTK_WIDGET (frame->value_labels[i]),
                                   tooltip[i]);
    }

  g_strfreev (values);
}

static void
ligma_color_frame_image_changed (LigmaColorFrame *frame,
                                LigmaImage      *image,
                                LigmaContext    *context)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (image == frame->image)
    return;

  ligma_color_frame_set_image (frame, image);
}

static void
ligma_color_frame_update_simulation (LigmaImage      *image,
                                    LigmaColorFrame *frame)
{
  g_return_if_fail (LIGMA_IS_COLOR_FRAME (frame));

  if (image && LIGMA_IS_COLOR_FRAME (frame))
    {
      frame->view_profile = ligma_image_get_simulation_profile (image);
      frame->simulation_intent = ligma_image_get_simulation_intent (image);

      ligma_color_frame_update (frame);
    }
}
