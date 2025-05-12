/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcheckexpander.c
 * Copyright (C) 2025 Jehan
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

#include "widgets-types.h"

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"

#include "gimpcheckexpander.h"
#include "gimpwidgets-utils.h"

#define GIMP_CHECK_EXPANDER_ACTION_KEY "gimp-expander-action"


/**
 * GimpCheckExpander:
 *
 * A checkbox widget which is also an expander in the same time, showing
 * sub-contents when the box is checked.
 *
 * With a side "triangle" button, it is also possible to collapse or
 * expand the contents independently to the box being checked or not.
 */


enum
{
  PROP_0,
  PROP_LABEL,
  PROP_CHECKED,
  PROP_EXPANDED,
  PROP_CHILD,
  N_PROPS
};


struct _GimpCheckExpanderPrivate
{
  GtkWidget  *checkbox;
  GtkWidget  *expander;

  gboolean    expanded;
};


static void     gimp_check_expander_set_property     (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);
static void     gimp_check_expander_get_property     (GObject            *object,
                                                      guint               property_id,
                                                      GValue             *value,
                                                      GParamSpec         *pspec);

static gboolean gimp_check_expander_triangle_draw    (GtkWidget          *triangle,
                                                      cairo_t            *cr,
                                                      GimpCheckExpander  *expander);
static gboolean gimp_check_expander_triangle_clicked (GtkWidget          *triangle,
                                                      GdkEventButton     *event,
                                                      GimpCheckExpander  *expander);

static void     gimp_check_expander_checked           (GtkToggleButton   *checkbox,
                                                       GimpCheckExpander *expander);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCheckExpander, gimp_check_expander, GIMP_TYPE_FRAME)

#define parent_class gimp_check_expander_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

/*  Class functions  */

static void
gimp_check_expander_class_init (GimpCheckExpanderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_check_expander_get_property;
  object_class->set_property = gimp_check_expander_set_property;

  props[PROP_LABEL]    = g_param_spec_string  ("label", NULL, NULL,
                                               NULL,
                                               GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  props[PROP_CHECKED]  = g_param_spec_boolean ("checked",
                                               NULL, NULL,
                                               FALSE,
                                               GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_EXPANDED] = g_param_spec_boolean ("expanded",
                                               NULL, NULL,
                                               FALSE,
                                               GIMP_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_CHILD]    = g_param_spec_object  ("child",
                                               "Child widget",
                                               "The child widget which is shown when this widget is expanded",
                                               GTK_TYPE_WIDGET,
                                               GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_check_expander_init (GimpCheckExpander *expander)
{
  GtkWidget *event_box;
  GtkWidget *title;

  expander->priv = gimp_check_expander_get_instance_private (expander);
  expander->priv->expanded = FALSE;

  title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1.0);
  gtk_frame_set_label_widget (GTK_FRAME (expander), title);
  gtk_widget_set_visible (title, TRUE);

  expander->priv->checkbox = gtk_check_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (expander->priv->checkbox), TRUE);
  gtk_box_pack_start (GTK_BOX (title), expander->priv->checkbox, TRUE, TRUE, 0.0);
  gtk_widget_set_visible (expander->priv->checkbox, TRUE);

  event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (title), event_box, FALSE, FALSE, 0.0);
  gtk_widget_show (event_box);

  expander->priv->expander = gtk_drawing_area_new ();
  gtk_widget_set_size_request (expander->priv->expander, 12, 12);
  gtk_container_add (GTK_CONTAINER (event_box), expander->priv->expander);
  gtk_widget_set_visible (expander->priv->expander, TRUE);

  g_signal_connect (G_OBJECT (expander->priv->expander), "draw",
                    G_CALLBACK (gimp_check_expander_triangle_draw),
                    expander);
  gtk_widget_add_events (event_box, GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (event_box), "button-release-event",
                    G_CALLBACK (gimp_check_expander_triangle_clicked),
                    expander);

  g_signal_connect (expander->priv->checkbox, "toggled",
                    G_CALLBACK (gimp_check_expander_checked),
                    expander);
}

static void
gimp_check_expander_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpCheckExpander *expander = GIMP_CHECK_EXPANDER (object);

  switch (property_id)
    {
    case PROP_LABEL:
      gtk_button_set_label (GTK_BUTTON (expander->priv->checkbox), g_value_get_string (value));
      break;
    case PROP_CHECKED:
      gimp_check_expander_set_checked (expander, g_value_get_boolean (value), FALSE);
      break;
    case PROP_EXPANDED:
      gimp_check_expander_set_expanded (expander, g_value_get_boolean (value));
      break;
    case PROP_CHILD:
      gtk_container_add (GTK_CONTAINER (expander), g_value_get_object (value));
      gtk_widget_set_visible (g_value_get_object (value), expander->priv->expanded);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_check_expander_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpCheckExpander *expander = GIMP_CHECK_EXPANDER (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gtk_button_get_label (GTK_BUTTON (expander->priv->checkbox)));
      break;
    case PROP_CHECKED:
      g_value_set_boolean (value, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (expander->priv->checkbox)));
      break;
    case PROP_EXPANDED:
      g_value_set_boolean (value, expander->priv->expanded);
      break;
    case PROP_CHILD:
      g_value_set_object (value, gtk_bin_get_child (GTK_BIN (expander)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public functions */

GtkWidget *
gimp_check_expander_new (const gchar *label,
                         GtkWidget   *child)
{
  return g_object_new (GIMP_TYPE_CHECK_EXPANDER,
                       "label", label,
                       "child", child,
                       NULL);
}

void
gimp_check_expander_set_label (GimpCheckExpander *expander,
                               const gchar       *label)
{
  g_return_if_fail (GIMP_IS_CHECK_EXPANDER (expander));

  g_object_set (expander, "label", label, NULL);
}

const gchar *
gimp_check_expander_get_label (GimpCheckExpander *expander,
                               const gchar       *label)
{
  g_return_val_if_fail (GIMP_IS_CHECK_EXPANDER (expander), NULL);

  return gtk_button_get_label (GTK_BUTTON (expander->priv->checkbox));
}

void
gimp_check_expander_set_checked (GimpCheckExpander *expander,
                                 gboolean           checked,
                                 gboolean           auto_expand)
{
  g_signal_handlers_block_by_func (expander->priv->checkbox,
                                   G_CALLBACK (gimp_check_expander_checked),
                                   expander);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (expander->priv->checkbox), checked);
  g_signal_handlers_unblock_by_func (expander->priv->checkbox,
                                     G_CALLBACK (gimp_check_expander_checked),
                                     expander);

  if (auto_expand)
    gimp_check_expander_set_expanded (expander, checked);

  g_object_notify_by_pspec (G_OBJECT (expander), props[PROP_CHECKED]);
}

void
gimp_check_expander_set_expanded (GimpCheckExpander *expander,
                                  gboolean           expanded)
{
  g_return_if_fail (GIMP_IS_CHECK_EXPANDER (expander));

  if (expander->priv->expanded != expanded)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (expander));

      expander->priv->expanded = expanded;

      gtk_widget_set_visible (child, expanded);

      g_object_notify_by_pspec (G_OBJECT (expander), props[PROP_EXPANDED]);
      gtk_widget_queue_draw (GTK_WIDGET (expander));
    }
}


/* Private functions */

static void
gimp_check_expander_checked (GtkToggleButton   *checkbox,
                             GimpCheckExpander *expander)
{
  gimp_check_expander_set_expanded (expander, gtk_toggle_button_get_active (checkbox));
  g_object_notify_by_pspec (G_OBJECT (expander), props[PROP_CHECKED]);
}

static gboolean
gimp_check_expander_triangle_draw (GtkWidget         *triangle,
                                   cairo_t           *cr,
                                   GimpCheckExpander *expander)
{
  GtkStyleContext      *context;
  PangoFontDescription *font_desc;
  GdkRGBA              *color;
  gint                  width;
  gint                  height;
  gint                  font_size;
  gdouble               triangle_side;

  context = gtk_widget_get_style_context (triangle);

  width   = gtk_widget_get_allocated_width (triangle);
  height  = gtk_widget_get_allocated_height (triangle);
  gtk_render_background (context, cr, 0, 0, width, height);

  gtk_style_context_get (context,
                         gtk_style_context_get_state (context),
                         GTK_STYLE_PROPERTY_FONT,  &font_desc,
                         GTK_STYLE_PROPERTY_COLOR, &color,
                         NULL);

  /* Get the font size in pixels */
  if (pango_font_description_get_size_is_absolute (font_desc))
    font_size = pango_font_description_get_size (font_desc);
  else
    font_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;
  pango_font_description_free (font_desc);

  triangle_side = MIN (font_size, MIN (width, height));
  if ((gint) triangle_side % 2 == 1)
    triangle_side -= 1;

  gdk_cairo_set_source_rgba (cr, color);

  if (expander->priv->expanded)
    {
      triangle_side = MIN (triangle_side, width);

      cairo_move_to (cr, (width - triangle_side) / 2.0, (height - triangle_side) / 2.0);
      cairo_line_to (cr, (gdouble) width - (width - triangle_side) / 2.0, (height - triangle_side) / 2.0);
      cairo_line_to (cr, width / 2.0, (gdouble) height - (height - triangle_side) / 2.0);
      cairo_line_to (cr, (width - triangle_side) / 2.0, (height - triangle_side) / 2.0);

      cairo_fill (cr);
    }
  else
    {
      cairo_move_to (cr, 1.0, (height - triangle_side) / 2.0);
      cairo_line_to (cr, 1.0, (gdouble) height - (height - triangle_side) / 2.0);
      cairo_line_to (cr, (gdouble) (width - 1.0), height / 2.0);
      cairo_line_to (cr, 1.0, (height - triangle_side) / 2.0);

      cairo_set_line_width (cr, 1.0);
      cairo_stroke (cr);
    }


  gdk_rgba_free (color);

  return FALSE;
}

static
gboolean gimp_check_expander_triangle_clicked (GtkWidget         *event_box,
                                               GdkEventButton    *event,
                                               GimpCheckExpander *expander)
{
  gimp_check_expander_set_expanded (expander, ! expander->priv->expanded);
  return TRUE;
}
