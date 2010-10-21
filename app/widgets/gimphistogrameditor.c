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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimphistogram.h"
#include "core/gimpimage.h"

#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimphistogrambox.h"
#include "gimphistogrameditor.h"
#include "gimphistogramview.h"
#include "gimpsessioninfo-aux.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LINEAR
};


static void     gimp_histogram_editor_docked_iface_init (GimpDockedInterface *iface);

static void     gimp_histogram_editor_set_property  (GObject            *object,
                                                     guint               property_id,
                                                     const GValue       *value,
                                                     GParamSpec         *pspec);
static void     gimp_histogram_editor_get_property  (GObject            *object,
                                                     guint               property_id,
                                                     GValue             *value,
                                                     GParamSpec         *pspec);

static void     gimp_histogram_editor_set_aux_info  (GimpDocked          *docked,
                                                     GList               *aux_info);
static GList  * gimp_histogram_editor_get_aux_info  (GimpDocked          *docked);

static void     gimp_histogram_editor_set_image     (GimpImageEditor     *editor,
                                                     GimpImage           *image);
static void     gimp_histogram_editor_layer_changed (GimpImage           *image,
                                                     GimpHistogramEditor *editor);
static void     gimp_histogram_editor_frozen_update (GimpHistogramEditor *editor,
                                                     const GParamSpec    *pspec);
static void     gimp_histogram_editor_buffer_update (GimpHistogramEditor *editor,
                                                     const GParamSpec    *pspec);
static void     gimp_histogram_editor_update        (GimpHistogramEditor *editor);

static gboolean gimp_histogram_editor_idle_update   (GimpHistogramEditor *editor);
static gboolean gimp_histogram_menu_sensitivity     (gint                 value,
                                                     gpointer             data);
static void     gimp_histogram_editor_menu_update   (GimpHistogramEditor *editor);
static void     gimp_histogram_editor_name_update   (GimpHistogramEditor *editor);
static void     gimp_histogram_editor_info_update   (GimpHistogramEditor *editor);

static gboolean gimp_histogram_view_draw            (GimpHistogramEditor *editor,
                                                     cairo_t             *cr);


G_DEFINE_TYPE_WITH_CODE (GimpHistogramEditor, gimp_histogram_editor,
                         GIMP_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_histogram_editor_docked_iface_init))

#define parent_class gimp_histogram_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_histogram_editor_class_init (GimpHistogramEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  object_class->set_property    = gimp_histogram_editor_set_property;
  object_class->get_property    = gimp_histogram_editor_get_property;

  image_editor_class->set_image = gimp_histogram_editor_set_image;

  g_object_class_install_property (object_class, PROP_LINEAR,
                                   g_param_spec_boolean ("linear",
                                                         _("Linear"), NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_histogram_editor_init (GimpHistogramEditor *editor)
{
  GimpHistogramView *view;
  GtkWidget         *hbox;
  GtkWidget         *label;
  GtkWidget         *menu;
  GtkWidget         *button;
  GtkWidget         *table;
  gint               i;

  const gchar *gimp_histogram_editor_labels[] =
    {
      N_("Mean:"),
      N_("Std dev:"),
      N_("Median:"),
      N_("Pixels:"),
      N_("Count:"),
      N_("Percentile:")
    };

  editor->drawable     = NULL;
  editor->histogram    = NULL;
  editor->bg_histogram = NULL;
  editor->valid        = FALSE;
  editor->idle_id      = 0;
  editor->box          = gimp_histogram_box_new ();

  gimp_editor_set_show_name (GIMP_EDITOR (editor), TRUE);

  view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->menu = menu = gimp_prop_enum_combo_box_new (G_OBJECT (view),
                                                      "histogram-channel",
                                                      0, 0);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (menu),
                                       "gimp-channel");
  gimp_int_combo_box_set_sensitivity (GIMP_INT_COMBO_BOX (editor->menu),
                                      gimp_histogram_menu_sensitivity,
                                      editor, NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (editor->menu),
                                 view->channel);
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  gimp_help_set_help_data (editor->menu,
                           _("Histogram channel"), NULL);

  menu = gimp_prop_enum_icon_box_new (G_OBJECT (view),
                                      "histogram-scale", "gimp-histogram",
                                      0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  button = gimp_prop_check_button_new (G_OBJECT (editor), "linear", NULL);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           "This button switches between a histogram in "
                           "sRGB (gamma corrected) space and a histogram "
                           "in linear space. Also, it's horrible UI and "
                           "needs to be changed.", NULL);

  gtk_box_pack_start (GTK_BOX (editor), editor->box, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->box));

  g_signal_connect_swapped (view, "range-changed",
                            G_CALLBACK (gimp_histogram_editor_info_update),
                            editor);
  g_signal_connect_swapped (view, "notify::histogram-channel",
                            G_CALLBACK (gimp_histogram_editor_info_update),
                            editor);

  g_signal_connect_swapped (view, "draw",
                            G_CALLBACK (gimp_histogram_view_draw),
                            editor);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (i = 0; i < 6; i++)
    {
      gint x = (i / 3) * 2;
      gint y = (i % 3);

      label = gtk_label_new (gettext (gimp_histogram_editor_labels[i]));
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                                 -1);
      gtk_label_set_xalign (GTK_LABEL (label), 1.0);
      gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);
      gtk_widget_show (label);

      editor->labels[i] =
        label = g_object_new (GTK_TYPE_LABEL,
                              "xalign",      0.0,
                              "yalign",      0.5,
                              "width-chars", i > 2 ? 9 : 5,
                              NULL);
      gimp_label_set_attributes (GTK_LABEL (editor->labels[i]),
                                 PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                                 -1);
      gtk_table_attach (GTK_TABLE (table), label, x + 1, x + 2, y, y + 1,
                        GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (label);
    }
}

static void
gimp_histogram_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  parent_docked_iface = g_type_interface_peek_parent (docked_iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  docked_iface->set_aux_info = gimp_histogram_editor_set_aux_info;
  docked_iface->get_aux_info = gimp_histogram_editor_get_aux_info;
}

static void
gimp_histogram_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpHistogramEditor *editor = GIMP_HISTOGRAM_EDITOR (object);
  GimpHistogramView   *view   = GIMP_HISTOGRAM_BOX (editor->box)->view;

  switch (property_id)
    {
    case PROP_LINEAR:
      editor->linear = g_value_get_boolean (value);

      if (editor->histogram)
        {
          g_object_unref (editor->histogram);
          editor->histogram = NULL;

          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_object_unref (editor->bg_histogram);
          editor->bg_histogram = NULL;

          gimp_histogram_view_set_background (view, NULL);
        }

      gimp_histogram_editor_update (editor);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpHistogramEditor *editor = GIMP_HISTOGRAM_EDITOR (object);

  switch (property_id)
    {
    case PROP_LINEAR:
      g_value_set_boolean (value, editor->linear);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_editor_set_aux_info (GimpDocked *docked,
                                    GList      *aux_info)
{
  GimpHistogramEditor *editor = GIMP_HISTOGRAM_EDITOR (docked);
  GimpHistogramView   *view   = GIMP_HISTOGRAM_BOX (editor->box)->view;

  parent_docked_iface->set_aux_info (docked, aux_info);

  gimp_session_info_aux_set_props (G_OBJECT (view), aux_info,
                                   "histogram-channel",
                                   "histogram-scale",
                                   NULL);
}

static GList *
gimp_histogram_editor_get_aux_info (GimpDocked *docked)
{
  GimpHistogramEditor *editor = GIMP_HISTOGRAM_EDITOR (docked);
  GimpHistogramView   *view   = GIMP_HISTOGRAM_BOX (editor->box)->view;
  GList               *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  return g_list_concat (aux_info,
                        gimp_session_info_aux_new_from_props (G_OBJECT (view),
                                                              "histogram-channel",
                                                              "histogram-scale",
                                                              NULL));
}

static void
gimp_histogram_editor_set_image (GimpImageEditor *image_editor,
                                 GimpImage       *image)
{
  GimpHistogramEditor *editor = GIMP_HISTOGRAM_EDITOR (image_editor);
  GimpHistogramView   *view   = GIMP_HISTOGRAM_BOX (editor->box)->view;

  if (image_editor->image)
    {
      if (editor->idle_id)
        {
          g_source_remove (editor->idle_id);
          editor->idle_id = 0;
        }

      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_histogram_editor_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_histogram_editor_layer_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_histogram_editor_menu_update,
                                            editor);

      if (editor->histogram)
        {
          g_object_unref (editor->histogram);
          editor->histogram = NULL;

          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_object_unref (editor->bg_histogram);
          editor->bg_histogram = NULL;

          gimp_histogram_view_set_background (view, NULL);
        }
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect_object (image, "mode-changed",
                               G_CALLBACK (gimp_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (image, "active-layer-changed",
                               G_CALLBACK (gimp_histogram_editor_layer_changed),
                               editor, 0);
      g_signal_connect_object (image, "mask-changed",
                               G_CALLBACK (gimp_histogram_editor_update),
                               editor, G_CONNECT_SWAPPED);
    }

  gimp_histogram_editor_layer_changed (image, editor);
}

GtkWidget *
gimp_histogram_editor_new (void)
{
  return g_object_new (GIMP_TYPE_HISTOGRAM_EDITOR, NULL);
}

static void
gimp_histogram_editor_layer_changed (GimpImage           *image,
                                     GimpHistogramEditor *editor)
{
  if (editor->drawable)
    {
      GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

      if (editor->histogram)
        {
          g_object_unref (editor->histogram);
          editor->histogram = NULL;

          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_object_unref (editor->bg_histogram);
          editor->bg_histogram = NULL;

          gimp_histogram_view_set_background (view, NULL);
        }

      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_name_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_menu_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_buffer_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_frozen_update,
                                            editor);
      editor->drawable = NULL;
    }

  if (image)
    editor->drawable = (GimpDrawable *) gimp_image_get_active_layer (image);

  gimp_histogram_editor_menu_update (editor);

  if (editor->drawable)
    {
      g_signal_connect_object (editor->drawable, "notify::frozen",
                               G_CALLBACK (gimp_histogram_editor_frozen_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "notify::buffer",
                               G_CALLBACK (gimp_histogram_editor_buffer_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "update",
                               G_CALLBACK (gimp_histogram_editor_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "alpha-changed",
                               G_CALLBACK (gimp_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "name-changed",
                               G_CALLBACK (gimp_histogram_editor_name_update),
                               editor, G_CONNECT_SWAPPED);

      gimp_histogram_editor_buffer_update (editor, NULL);
    }
  else if (editor->histogram)
    {
      editor->valid = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (editor->box));
    }

  gimp_histogram_editor_info_update (editor);
  gimp_histogram_editor_name_update (editor);
}

static gboolean
gimp_histogram_editor_validate (GimpHistogramEditor *editor)
{
  if (! editor->valid)
    {
      if (editor->drawable)
        {
          if (! editor->histogram)
            {
              GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

              editor->histogram = gimp_histogram_new (editor->linear);

              gimp_histogram_view_set_histogram (view, editor->histogram);
            }

          gimp_drawable_calculate_histogram (editor->drawable,
                                             editor->histogram);
        }
      else
        {
          if (editor->histogram)
            gimp_histogram_clear_values (editor->histogram);
        }

      gimp_histogram_editor_info_update (editor);

      if (editor->histogram)
        editor->valid = TRUE;
    }

  return editor->valid;
}

static void
gimp_histogram_editor_frozen_update (GimpHistogramEditor *editor,
                                     const GParamSpec    *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  if (gimp_viewable_preview_is_frozen (GIMP_VIEWABLE (editor->drawable)))
    {
      /* Only do the background histogram if the histogram is visible.
       * This is a workaround for the fact that recalculating the
       * histogram is expensive and that it is only validated when it
       * is shown. So don't slow down painting by doing something that
       * is not even seen by the user.
       */
      if (! editor->bg_histogram &&
          gtk_widget_is_drawable (GTK_WIDGET (editor)))
        {
          if (gimp_histogram_editor_validate (editor))
            editor->bg_histogram = gimp_histogram_duplicate (editor->histogram);

          gimp_histogram_view_set_background (view, editor->bg_histogram);
        }
    }
  else if (editor->bg_histogram)
    {
      g_object_unref (editor->bg_histogram);
      editor->bg_histogram = NULL;

      gimp_histogram_view_set_background (view, NULL);
    }
}

static void
gimp_histogram_editor_buffer_update (GimpHistogramEditor *editor,
                                     const GParamSpec    *pspec)
{
  g_object_set (editor,
                "linear", gimp_drawable_get_linear (editor->drawable),
                NULL);
}

static void
gimp_histogram_editor_update (GimpHistogramEditor *editor)
{
  if (editor->idle_id)
    g_source_remove (editor->idle_id);

  editor->idle_id =
    g_timeout_add_full (G_PRIORITY_LOW,
                        200,
                        (GSourceFunc) gimp_histogram_editor_idle_update,
                        editor,
                        NULL);
}

static gboolean
gimp_histogram_editor_idle_update (GimpHistogramEditor *editor)
{
  editor->idle_id = 0;

  /* Mark the histogram as invalid and queue a redraw.
   * We will then recalculate the histogram when the view is exposed.
   */

  editor->valid = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (editor->box));

  return FALSE;
}

static gboolean
gimp_histogram_editor_channel_valid (GimpHistogramEditor  *editor,
                                     GimpHistogramChannel  channel)
{
  if (editor->drawable)
    {
      switch (channel)
        {
        case GIMP_HISTOGRAM_VALUE:
          return TRUE;

        case GIMP_HISTOGRAM_RED:
        case GIMP_HISTOGRAM_GREEN:
        case GIMP_HISTOGRAM_BLUE:
        case GIMP_HISTOGRAM_LUMINANCE:
        case GIMP_HISTOGRAM_RGB:
          return gimp_drawable_is_rgb (editor->drawable);

        case GIMP_HISTOGRAM_ALPHA:
          return gimp_drawable_has_alpha (editor->drawable);
        }
    }

  return TRUE;
}

static gboolean
gimp_histogram_menu_sensitivity (gint      value,
                                 gpointer  data)
{
  GimpHistogramEditor  *editor  = GIMP_HISTOGRAM_EDITOR (data);
  GimpHistogramChannel  channel = value;

  if (editor->drawable)
    return gimp_histogram_editor_channel_valid (editor, channel);

  return FALSE;
}

static void
gimp_histogram_editor_menu_update (GimpHistogramEditor *editor)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  gtk_widget_queue_draw (editor->menu);

  if (! gimp_histogram_editor_channel_valid (editor, view->channel))
    {
      gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
    }
}

static void
gimp_histogram_editor_name_update (GimpHistogramEditor *editor)
{
  const gchar *name = NULL;

  if (editor->drawable)
    name = gimp_object_get_name (editor->drawable);

  gimp_editor_set_name (GIMP_EDITOR (editor), name);
}

static void
gimp_histogram_editor_info_update (GimpHistogramEditor *editor)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;
  GimpHistogram     *hist = editor->histogram;

  if (hist)
    {
      gint    n_bins;
      gdouble pixels;
      gdouble count;
      gchar   text[12];

      n_bins = gimp_histogram_n_bins (hist);

      pixels = gimp_histogram_get_count (hist, view->channel, 0, n_bins - 1);
      count  = gimp_histogram_get_count (hist, view->channel,
                                         view->start, view->end);

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_mean (hist, view->channel,
                                           view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[0]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_std_dev (hist, view->channel,
                                              view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[1]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_median  (hist, view->channel,
                                              view->start,
                                              view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[2]), text);

      g_snprintf (text, sizeof (text), "%d", (gint) pixels);
      gtk_label_set_text (GTK_LABEL (editor->labels[3]), text);

      g_snprintf (text, sizeof (text), "%d", (gint) count);
      gtk_label_set_text (GTK_LABEL (editor->labels[4]), text);

      g_snprintf (text, sizeof (text), "%.1f", (pixels > 0 ?
                                                 (100.0 * count / pixels) :
                                                 0.0));
      gtk_label_set_text (GTK_LABEL (editor->labels[5]), text);
    }
  else
    {
      gint i;

      for (i = 0; i < 6; i++)
        gtk_label_set_text (GTK_LABEL (editor->labels[i]), NULL);
    }
}

static gboolean
gimp_histogram_view_draw (GimpHistogramEditor *editor,
                          cairo_t             *cr)
{
  gimp_histogram_editor_validate (editor);

  return FALSE;
}
