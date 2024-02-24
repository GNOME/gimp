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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"

#include "gimpcolorframe.h"

#include "gimp-intl.h"


#define RGBA_EPSILON 1e-6

enum
{
  PROP_0,
  PROP_GIMP,
  PROP_MODE,
  PROP_HAS_NUMBER,
  PROP_NUMBER,
  PROP_HAS_COLOR_AREA,
  PROP_HAS_COORDS,
  PROP_ELLIPSIZE,
};


/*  local function prototypes  */

static void       gimp_color_frame_dispose             (GObject        *object);
static void       gimp_color_frame_finalize            (GObject        *object);
static void       gimp_color_frame_get_property        (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void       gimp_color_frame_set_property        (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);

static void       gimp_color_frame_style_updated       (GtkWidget      *widget);
static gboolean   gimp_color_frame_draw                (GtkWidget      *widget,
                                                        cairo_t        *cr);

static void       gimp_color_frame_combo_callback      (GtkWidget      *widget,
                                                        GimpColorFrame *frame);
static void       gimp_color_frame_update              (GimpColorFrame *frame);

static void       gimp_color_frame_image_changed       (GimpColorFrame *frame,
                                                        GimpImage      *image,
                                                        GimpContext    *context);

static void       gimp_color_frame_update_simulation   (GimpImage        *image,
                                                        GimpColorFrame   *frame);

G_DEFINE_TYPE (GimpColorFrame, gimp_color_frame, GIMP_TYPE_FRAME)

#define parent_class gimp_color_frame_parent_class


static void
gimp_color_frame_class_init (GimpColorFrameClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose       = gimp_color_frame_dispose;
  object_class->finalize      = gimp_color_frame_finalize;
  object_class->get_property  = gimp_color_frame_get_property;
  object_class->set_property  = gimp_color_frame_set_property;

  widget_class->style_updated = gimp_color_frame_style_updated;
  widget_class->draw          = gimp_color_frame_draw;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_COLOR_PICK_MODE,
                                                      GIMP_COLOR_PICK_MODE_PIXEL,
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

  g_object_class_install_property (object_class, PROP_HAS_COORDS,
                                   g_param_spec_boolean ("has-coords",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_color_frame_init (GimpColorFrame *frame)
{
  GtkListStore *store;
  GtkWidget    *vbox;
  GtkWidget    *vbox2;
  GtkWidget    *label;
  gint          i;

  frame->sample_valid = FALSE;

  frame->color = gegl_color_new ("black");

  /* create the store manually so the values have a nice order */
  store = gimp_enum_store_new_with_values (GIMP_TYPE_COLOR_PICK_MODE,
                                           GIMP_COLOR_PICK_MODE_LAST + 1,
                                           GIMP_COLOR_PICK_MODE_PIXEL,
                                           GIMP_COLOR_PICK_MODE_RGB_PERCENT,
                                           GIMP_COLOR_PICK_MODE_RGB_U8,
                                           GIMP_COLOR_PICK_MODE_GRAYSCALE,
                                           GIMP_COLOR_PICK_MODE_HSV,
                                           GIMP_COLOR_PICK_MODE_LCH,
                                           GIMP_COLOR_PICK_MODE_LAB,
                                           GIMP_COLOR_PICK_MODE_XYY,
                                           GIMP_COLOR_PICK_MODE_YUV,
                                           GIMP_COLOR_PICK_MODE_CMYK);
  frame->combo = gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));
  g_object_unref (store);

  gtk_frame_set_label_widget (GTK_FRAME (frame), frame->combo);
  gtk_widget_show (frame->combo);

  g_signal_connect (frame->combo, "changed",
                    G_CALLBACK (gimp_color_frame_combo_callback),
                    frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame->color_area =
    g_object_new (GIMP_TYPE_COLOR_AREA,
                  "color",          frame->color,
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
gimp_color_frame_dispose (GObject *object)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  if (frame->gimp)
    {
      g_signal_handlers_disconnect_by_func (gimp_get_user_context (frame->gimp),
                                            gimp_color_frame_image_changed,
                                            frame);
      frame->gimp = NULL;
    }

  gimp_color_frame_set_image (frame, NULL);

  gimp_color_frame_set_color_config (frame, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_frame_finalize (GObject *object)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);

  g_clear_object (&frame->number_layout);
  g_clear_object (&frame->color);

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
    case PROP_GIMP:
      g_value_set_object (value, frame->gimp);
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
gimp_color_frame_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (object);
  GimpContext    *context;
  GimpImage      *image;

  switch (property_id)
    {
    case PROP_GIMP:
      frame->gimp = g_value_get_object (value);
      if (frame->gimp)
        {
          context = gimp_get_user_context (frame->gimp);
          image   = gimp_context_get_image (context);

          g_signal_connect_swapped (context, "image-changed",
                                    G_CALLBACK (gimp_color_frame_image_changed),
                                    frame);
          gimp_color_frame_image_changed (frame, image, context);
        }
      break;

    case PROP_MODE:
      gimp_color_frame_set_mode (frame, g_value_get_enum (value));
      break;

    case PROP_ELLIPSIZE:
      gimp_color_frame_set_ellipsize (frame, g_value_get_enum (value));
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

    case PROP_HAS_COORDS:
      gimp_color_frame_set_has_coords (frame, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_frame_style_updated (GtkWidget *widget)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&frame->number_layout);
}

static gboolean
gimp_color_frame_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpColorFrame *frame = GIMP_COLOR_FRAME (widget);

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
 * gimp_color_frame_new:
 * @gimp: A #Gimp object.
 *
 * Creates a new #GimpColorFrame widget.
 *
 * Returns: The new #GimpColorFrame widget.
 **/
GtkWidget *
gimp_color_frame_new (Gimp *gimp)
{
  return g_object_new (GIMP_TYPE_COLOR_FRAME,
                       "gimp", gimp,
                       NULL);
}


/**
 * gimp_color_frame_set_mode:
 * @frame: The #GimpColorFrame.
 * @mode:  The new @mode.
 *
 * Sets the #GimpColorFrame's color pick @mode. Calling this function
 * does the same as selecting the @mode from the frame's #GtkComboBox.
 **/
void
gimp_color_frame_set_mode (GimpColorFrame    *frame,
                           GimpColorPickMode  mode)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (frame->combo), mode);
}

void
gimp_color_frame_set_ellipsize (GimpColorFrame     *frame,
                                PangoEllipsizeMode  ellipsize)
{
  gint i;

  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (ellipsize != frame->ellipsize)
    {
      frame->ellipsize = ellipsize;

      for (i = 0; i < GIMP_COLOR_FRAME_ROWS; i++)
        {
          if (frame->value_labels[i])
            gtk_label_set_ellipsize (GTK_LABEL (frame->value_labels[i]),
                                     ellipsize);
        }
    }
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

void
gimp_color_frame_set_has_coords (GimpColorFrame *frame,
                                 gboolean        has_coords)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (has_coords != frame->has_coords)
    {
      frame->has_coords = has_coords ? TRUE : FALSE;

      g_object_set (frame->coords_box_x, "visible", frame->has_coords, NULL);
      g_object_set (frame->coords_box_y, "visible", frame->has_coords, NULL);

      g_object_notify (G_OBJECT (frame), "has-coords");
    }
}

/**
 * gimp_color_frame_set_color:
 * @frame:          The #GimpColorFrame.
 * @sample_average: The set @color is the result of averaging
 * @color:          The [class@Gegl.Color] to set.
 * @x:              X position where the color was picked.
 * @y:              Y position where the color was picked.
 *
 * Sets the color sample to display in the #GimpColorFrame. if
 * @sample_average is %TRUE, @color represents the sample at the
 * center of the average area and will not be displayed.
 **/
void
gimp_color_frame_set_color (GimpColorFrame *frame,
                            gboolean        sample_average,
                            GeglColor      *color,
                            gint            x,
                            gint            y)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));
  g_return_if_fail (GEGL_IS_COLOR (color));

  color = gegl_color_duplicate (color);
  if (frame->sample_valid                     &&
      frame->sample_average == sample_average &&
      frame->x              == x              &&
      frame->y              == y              &&
      gimp_color_is_perceptually_identical (frame->color, color))
    {
      g_clear_object (&frame->color);
      frame->color = color;
      return;
    }

  frame->sample_valid   = TRUE;
  frame->sample_average = sample_average;
  g_clear_object (&frame->color);
  frame->color          = color;
  frame->x              = x;
  frame->y              = y;

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
          g_object_unref (frame->config);

          gimp_color_frame_update (frame);
        }

      frame->config = config;

      if (frame->config)
          g_object_ref (frame->config);

      gimp_color_area_set_color_config (GIMP_COLOR_AREA (frame->color_area),
                                        config);
    }
}

void
gimp_color_frame_set_image (GimpColorFrame *frame,
                            GimpImage      *image)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  if (image != frame->image)
    {
      if (frame->image)
        {
          g_signal_handlers_disconnect_by_func (frame->image,
                                                gimp_color_frame_update_simulation,
                                                frame);
          g_object_unref (frame->image);
        }
    }

  frame->image = image;

  if (frame->image)
    {
      g_object_ref (frame->image);

      g_signal_connect (frame->image, "simulation-profile-changed",
                        G_CALLBACK (gimp_color_frame_update_simulation),
                        frame);
      g_signal_connect (frame->image, "simulation-intent-changed",
                        G_CALLBACK (gimp_color_frame_update_simulation),
                        frame);

      gimp_color_frame_update_simulation (frame->image, frame);
    }
}

/*  private functions  */

static void
gimp_color_frame_combo_callback (GtkWidget      *widget,
                                 GimpColorFrame *frame)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      frame->pick_mode = value;
      gimp_color_frame_update (frame);

      g_object_notify (G_OBJECT (frame), "mode");
    }
}

static void
gimp_color_frame_update (GimpColorFrame *frame)
{
  const Babl       *format;
  const Babl       *space;
  GimpTRCType       trc;
  const gchar      *names[GIMP_COLOR_FRAME_ROWS]   = { NULL, };
  gchar           **values                         = NULL;
  const gchar      *tooltip[GIMP_COLOR_FRAME_ROWS] = { NULL, };
  gboolean          has_alpha;
  GimpColorProfile *color_profile                  = NULL;
  gint              i;

  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  format    = gegl_color_get_format (frame->color);
  space     = babl_format_get_space (format);
  trc       = gimp_babl_format_get_trc (format);
  has_alpha = babl_format_has_alpha (format);

  if (frame->sample_valid)
    {
      gchar str[16];

      gimp_color_area_set_color (GIMP_COLOR_AREA (frame->color_area), frame->color);

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
    case GIMP_COLOR_PICK_MODE_PIXEL:
      {
        GimpImageBaseType  base_type;
        gint               values_len = 0;

        base_type     = gimp_babl_format_get_base_type (format);
        color_profile = gimp_babl_format_get_color_profile (format);

        if (frame->sample_valid)
          values = gimp_babl_print_color (frame->color);

        if (base_type == GIMP_GRAY)
          {
            /* TRANSLATORS: V for Value (grayscale) */
            names[values_len++] = C_("Grayscale", "V:");

            if (has_alpha)
              /* TRANSLATORS: A for Alpha (color transparency) */
              names[values_len++] = C_("Alpha channel", "A:");
          }
        else if (base_type == GIMP_RGB || base_type == GIMP_INDEXED)
          {
            /* TRANSLATORS: R for Red (RGB) */
            names[values_len++] = C_("RGB", "R:");
            /* TRANSLATORS: G for Green (RGB) */
            names[values_len++] = C_("RGB", "G:");
            /* TRANSLATORS: B for Blue (RGB) */
            names[values_len++] = C_("RGB", "B:");

            if (has_alpha)
              /* TRANSLATORS: A for Alpha (color transparency) */
              names[values_len++] = C_("Alpha channel", "A:");

            if (babl_format_is_palette (format))
              {
                /* TRANSLATORS: Index of the color in the palette. */
                names[values_len] = C_("Indexed color", "Index:");

                if (frame->sample_valid)
                  {
                    gchar **v = g_new0 (gchar *, values_len + 2);

                    memcpy (v, values, values_len * sizeof (gchar *));
                    g_free (values);
                    values = v;

                    if (! frame->sample_average)
                      {
                        guint8 index;

                        gegl_color_get_pixel (frame->color, format, &index);

                        values[values_len++] = g_strdup_printf ("%d", index);
                      }
                  }
              }
          }

        if (frame->sample_valid && color_profile)
          {
            const gchar  *profile_label;
            gchar       **v;

            v = g_new0 (gchar *, values_len + 2);
            memcpy (v, values, values_len * sizeof (gchar *));
            g_free (values);
            values = v;

            names[values_len] = C_("Color", "Profile:");

            if (space == babl_space ("sRGB"))
              profile_label = "sRGB";
            else
              profile_label = gimp_color_profile_get_label (color_profile);

            values[values_len]  = g_strdup_printf (_("%s"), profile_label);
            tooltip[values_len] = g_strdup_printf (_("%s"), profile_label);
          }
      }
      break;

    case GIMP_COLOR_PICK_MODE_RGB_PERCENT:
    case GIMP_COLOR_PICK_MODE_RGB_U8:
      if (babl_space_is_gray (space) || babl_space_is_rgb (space))
        color_profile = gimp_babl_format_get_color_profile (format);

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

      if (color_profile)
        names[5] = C_("Color", "Profile:");
      else
        names[5] = C_("Color", "No Profile");

      if (frame->sample_valid)
        {
          const Babl *print_format;
          guint8      rgba[4];

          values = g_new0 (gchar *, 7);

          print_format = gimp_babl_format (GIMP_RGB,
                                           gimp_babl_precision (GIMP_COMPONENT_TYPE_U8, trc),
                                           TRUE, space);
          gegl_color_get_pixel (frame->color, print_format, rgba);

          if (frame->pick_mode == GIMP_COLOR_PICK_MODE_RGB_PERCENT)
            {
              gfloat d[4];

              print_format = gimp_babl_format (GIMP_RGB,
                                               gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT, trc),
                                               TRUE, space);
              gegl_color_get_pixel (frame->color, print_format, d);
              values[0] = g_strdup_printf ("%.01f %%", d[0] * 100.0);
              values[1] = g_strdup_printf ("%.01f %%", d[1] * 100.0);
              values[2] = g_strdup_printf ("%.01f %%", d[2] * 100.0);
              values[3] = g_strdup_printf ("%.01f %%", d[3] * 100.0);
            }
          else
            {
              values[0] = g_strdup_printf ("%d", rgba[0]);
              values[1] = g_strdup_printf ("%d", rgba[1]);
              values[2] = g_strdup_printf ("%d", rgba[2]);
              values[3] = g_strdup_printf ("%d", rgba[3]);
            }

          values[4] = g_strdup_printf ("%.2x%.2x%.2x", rgba[0], rgba[1], rgba[2]);

          if (color_profile)
            {
              const gchar *profile_label;

              if (space == babl_space ("sRGB"))
                profile_label = "sRGB";
              else
                profile_label = gimp_color_profile_get_label (color_profile);

              values[5]  = g_strdup_printf (_("%s"), profile_label);
              tooltip[5] = g_strdup_printf (_("%s"), profile_label);
            }
        }
      break;

    case GIMP_COLOR_PICK_MODE_GRAYSCALE:
      if (babl_space_is_gray (space) || babl_space_is_rgb (space))
        color_profile = gimp_babl_format_get_color_profile (format);

      /* TRANSLATORS: V for Value (grayscale) */
      names[0] = C_("Grayscale", "V:");

      if (has_alpha)
        /* TRANSLATORS: A for Alpha (color transparency) */
        names[1] = C_("Alpha channel", "A:");

      if (color_profile)
        names[2] = C_("Color", "Profile:");
      else
        names[2] = C_("Color", "No Profile");

      if (frame->sample_valid)
        {
          const Babl *print_format;
          gfloat      grayscale[2];
          GeglColor  *grayscale_color;

          print_format = gimp_babl_format (GIMP_GRAY,
                                           gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT, trc),
                                           TRUE, space);
          gegl_color_get_pixel (frame->color, print_format, grayscale);

          /* Change color area color to grayscale if image is not
           * in Grayscale Mode */
          grayscale_color = gegl_color_duplicate (frame->color);
          gegl_color_set_pixel (grayscale_color, print_format, grayscale);
          gimp_color_set_alpha (grayscale_color, GIMP_OPACITY_OPAQUE);
          gimp_color_area_set_color (GIMP_COLOR_AREA (frame->color_area), grayscale_color);
          g_object_unref (grayscale_color);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f %%", grayscale[0] * 100.0);
          if (has_alpha)
            values[1] = g_strdup_printf ("%.01f %%", grayscale[1] * 100.0);

          if (color_profile)
            {
              const gchar *profile_label;

              profile_label = gimp_color_profile_get_label (color_profile);
              values[2]  = g_strdup_printf (_("%s"), profile_label);
              tooltip[2] = g_strdup_printf (_("%s"), profile_label);
            }
        }
      break;

    case GIMP_COLOR_PICK_MODE_HSV:
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
          gfloat hsva[4];

          gegl_color_get_pixel (frame->color, babl_format ("HSVA float"), hsva);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f \302\260", hsva[0] * 360.0);
          values[1] = g_strdup_printf ("%.01f %%",       hsva[1] * 100.0);
          values[2] = g_strdup_printf ("%.01f %%",       hsva[2] * 100.0);
          values[3] = g_strdup_printf ("%.01f %%",       hsva[3] * 100.0);
        }
      break;

    case GIMP_COLOR_PICK_MODE_LCH:
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
          gfloat lch[4];

          gegl_color_get_pixel (frame->color, babl_format ("CIE LCH(ab) alpha float"), lch);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f  ",        lch[0]);
          values[1] = g_strdup_printf ("%.01f  ",        lch[1]);
          values[2] = g_strdup_printf ("%.01f \302\260", lch[2]);
          if (has_alpha)
            values[3] = g_strdup_printf ("%.01f %%",       lch[3] * 100.0);
        }
      break;

    case GIMP_COLOR_PICK_MODE_LAB:
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
          gfloat lab[4];

          gegl_color_get_pixel (frame->color, babl_format ("CIE Lab alpha float"), lab);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%.01f  ",  lab[0]);
          values[1] = g_strdup_printf ("%.01f  ",  lab[1]);
          values[2] = g_strdup_printf ("%.01f  ",  lab[2]);
          if (has_alpha)
            values[3] = g_strdup_printf ("%.01f %%", lab[3] * 100.0);
        }
      break;

    case GIMP_COLOR_PICK_MODE_XYY:
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
          gfloat xyY[4];

          gegl_color_get_pixel (frame->color, babl_format ("CIE xyY alpha float"), xyY);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%1.6f  ",  xyY[0]);
          values[1] = g_strdup_printf ("%1.6f  ",  xyY[1]);
          values[2] = g_strdup_printf ("%1.6f  ",  xyY[2]);
          values[3] = g_strdup_printf ("%.01f %%", xyY[3] * 100.0);
        }
      break;

    case GIMP_COLOR_PICK_MODE_YUV:
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
          gfloat Yuv[4];

          gegl_color_get_pixel (frame->color, babl_format ("CIE Yuv alpha float"), Yuv);

          values = g_new0 (gchar *, 5);

          values[0] = g_strdup_printf ("%1.6f  ",  Yuv[0]);
          values[1] = g_strdup_printf ("%1.6f  ",  Yuv[1]);
          values[2] = g_strdup_printf ("%1.6f  ",  Yuv[2]);
          if (has_alpha)
            values[3] = g_strdup_printf ("%.01f %%", Yuv[3] * 100.0);
        }
      break;

    case GIMP_COLOR_PICK_MODE_CMYK:
      {
        const Babl *space = NULL;

        /* Try to load CMYK profile in the following order:
         * 1) Soft-Proof Profile set in the View Menu
         * 2) Preferred CMYK Profile set in Preferences
         * 3) No CMYK Profile (Default Values)
         */
        color_profile = frame->view_profile ? g_object_ref (frame->view_profile) : NULL;

        if (! color_profile && frame->config)
          color_profile = gimp_color_config_get_cmyk_color_profile (frame->config,
                                                                    NULL);
        if (color_profile)
          space = gimp_color_profile_get_space (color_profile,
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
            gfloat       cmyk[5];

            /* User may swap CMYK color profiles at runtime */
            gegl_color_get_pixel (frame->color, babl_format_with_space ("CMYKA float", space), cmyk);

            values = g_new0 (gchar *, 7);

            values[0] = g_strdup_printf ("%.0f %%", cmyk[0] * 100.0);
            values[1] = g_strdup_printf ("%.0f %%", cmyk[1] * 100.0);
            values[2] = g_strdup_printf ("%.0f %%", cmyk[2] * 100.0);
            values[3] = g_strdup_printf ("%.0f %%", cmyk[3] * 100.0);
            if (has_alpha)
              values[4] = g_strdup_printf ("%.01f %%", cmyk[4] * 100.0);

            if (color_profile)
              {
                const gchar *profile_label;

                profile_label = gimp_color_profile_get_label (color_profile);
                values[5]  = g_strdup_printf (_("%s"), profile_label);
                tooltip[5] = g_strdup_printf (_("%s"), profile_label);
              }
          }
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

  g_clear_object (&color_profile);

  g_strfreev (values);
}

static void
gimp_color_frame_image_changed (GimpColorFrame *frame,
                                GimpImage      *image,
                                GimpContext    *context)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (image == frame->image)
    return;

  gimp_color_frame_set_image (frame, image);
}

static void
gimp_color_frame_update_simulation (GimpImage      *image,
                                    GimpColorFrame *frame)
{
  g_return_if_fail (GIMP_IS_COLOR_FRAME (frame));

  if (image && GIMP_IS_COLOR_FRAME (frame))
    {
      frame->view_profile = gimp_image_get_simulation_profile (image);
      frame->simulation_intent = gimp_image_get_simulation_intent (image);

      gimp_color_frame_update (frame);
    }
}
