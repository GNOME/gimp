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
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpasync.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimphistogram.h"
#include "core/gimpimage.h"

#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimphistogrambox.h"
#include "gimphistogrameditor.h"
#include "gimphistogramview.h"
#include "gimppropwidgets.h"
#include "gimpsessioninfo-aux.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TRC
};

enum
{
  LABEL_MEAN,
  LABEL_STD_DEV,
  LABEL_MEDIAN,
  LABEL_PIXELS,
  LABEL_COUNT,
  LABEL_PERCENTILE,
  LABEL_N_COLORS,
  N_LABELS
};


static void     gimp_histogram_editor_docked_iface_init (GimpDockedInterface *iface);

static void     gimp_histogram_editor_finalize      (GObject            *object);
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

static gboolean gimp_histogram_editor_view_draw     (GimpHistogramEditor *editor,
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

  object_class->finalize        = gimp_histogram_editor_finalize;
  object_class->set_property    = gimp_histogram_editor_set_property;
  object_class->get_property    = gimp_histogram_editor_get_property;

  image_editor_class->set_image = gimp_histogram_editor_set_image;

  g_object_class_install_property (object_class, PROP_TRC,
                                   g_param_spec_enum ("trc",
                                                      _("Linear/Perceptual"),
                                                      NULL,
                                                      GIMP_TYPE_TRC_TYPE,
                                                      GIMP_TRC_LINEAR,
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
  GtkWidget         *grid;
  gint               i;

  const gchar *gimp_histogram_editor_labels[] =
    {
      N_("Mean: "),
      N_("Std dev: "),
      N_("Median: "),
      N_("Pixels: "),
      N_("Count: "),
      N_("Percentile: ")
    };

  editor->box = gimp_histogram_box_new ();

  gimp_editor_set_show_name (GIMP_EDITOR (editor), TRUE);

  view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
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

  gimp_help_set_help_data (editor->menu,
                           _("Histogram channel"), NULL);

  menu = gimp_prop_enum_icon_box_new (G_OBJECT (view),
                                      "histogram-scale", "gimp-histogram",
                                      0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  menu = gimp_prop_enum_icon_box_new (G_OBJECT (editor), "trc",
                                      "gimp-color-space",
                                      -1, -1);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (editor), editor->box, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->box));

  g_signal_connect_swapped (view, "range-changed",
                            G_CALLBACK (gimp_histogram_editor_info_update),
                            editor);
  g_signal_connect_swapped (view, "notify::histogram-channel",
                            G_CALLBACK (gimp_histogram_editor_info_update),
                            editor);

  g_signal_connect_swapped (view, "draw",
                            G_CALLBACK (gimp_histogram_editor_view_draw),
                            editor);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  for (i = 0; i <= LABEL_PERCENTILE; i++)
    {
      gint x = (i / 3) * 2;
      gint y = (i % 3);

      label = gtk_label_new (gettext (gimp_histogram_editor_labels[i]));
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                                 -1);
      gtk_label_set_xalign (GTK_LABEL (label), 1.0);
      gtk_widget_set_hexpand (label, TRUE);
      gtk_grid_attach (GTK_GRID (grid), label, x, y, 1, 1);
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
      gtk_grid_attach (GTK_GRID (grid), label, x + 1, y, 1, 1);
      gtk_widget_show (label);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->toggle = gtk_check_button_new_with_label (_("Compute unique colors:"));
  gimp_widget_set_identifier (editor->toggle, "toggle-compute-unique-colors");
  gimp_label_set_attributes (GTK_LABEL (gtk_bin_get_child (GTK_BIN (editor->toggle))),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->toggle), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), editor->toggle, FALSE, FALSE, 0);
  gtk_widget_show (editor->toggle);

  g_signal_connect_swapped (editor->toggle, "toggled",
                            G_CALLBACK (gimp_histogram_editor_info_update),
                            editor);

  editor->labels[6] = label = g_object_new (GTK_TYPE_LABEL,
                                            "xalign",      0.0,
                                            "yalign",      0.5,
                                            "width-chars", 9,
                                            NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
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
gimp_histogram_editor_finalize (GObject *object)
{
  if (GIMP_HISTOGRAM_EDITOR (object)->idle_id)
    g_source_remove (GIMP_HISTOGRAM_EDITOR (object)->idle_id);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case PROP_TRC:
      editor->trc = g_value_get_enum (value);

      if (editor->histogram)
        {
          g_clear_object (&editor->histogram);
          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
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
    case PROP_TRC:
      g_value_set_enum (value, editor->trc);
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

      editor->update_pending = FALSE;

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
          g_clear_object (&editor->histogram);
          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
          gimp_histogram_view_set_background (view, NULL);
        }
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect_object (image, "mode-changed",
                               G_CALLBACK (gimp_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (image, "selected-layers-changed",
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
          g_clear_object (&editor->histogram);
          gimp_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
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
    {
      GList *layers;

      layers = gimp_image_get_selected_layers (image);
      /*
       * TODO: right now, we only support making an histogram for a single
       * layer. In future, it would be nice to have the ability to make the
       * histogram for:
       * - all individual pixels of selected layers;
       * - the merged composition of selected layers;
       * - the visible image.
       */
      editor->drawable = (g_list_length (layers) == 1 ? layers->data : NULL);
    }

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
      editor->recompute = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (editor->box));
    }

  gimp_histogram_editor_info_update (editor);
  gimp_histogram_editor_name_update (editor);
}

static void
gimp_histogram_editor_calculate_async_callback (GimpAsync           *async,
                                                GimpHistogramEditor *editor)
{
  editor->calculate_async = NULL;

  if (gimp_async_is_finished (async) && editor->histogram)
    {
      if (editor->bg_pending)
        {
          GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

          editor->bg_histogram = gimp_histogram_duplicate (editor->histogram);

          gimp_histogram_view_set_background (view, editor->bg_histogram);
        }

      gimp_histogram_editor_info_update (editor);
    }

  editor->bg_pending = FALSE;

  if (editor->update_pending)
    gimp_histogram_editor_update (editor);
}

static gboolean
gimp_histogram_editor_validate (GimpHistogramEditor *editor)
{
  if (editor->recompute || ! editor->histogram)
    {
      if (editor->drawable &&
          /* avoid calculating the histogram of a detached layer.
           * this can happen during gimp_image_remove_layer(), as a
           * result of a pending "expose-event" signal (handled in
           * gimp_histogram_editor_view_expose()) executed through
           * gtk_tree_view_clamp_node_visible(), as a result of the
           * GimpLayerTreeView in the Layers dialog receiving the
           * image's "selected-layers-changed" signal before us.  See
           * bug #795716, comment 6.
           */
          gimp_item_is_attached (GIMP_ITEM (editor->drawable)))
        {
          GimpAsync *async;

          if (! editor->histogram)
            {
              GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

              editor->histogram = gimp_histogram_new (editor->trc);

              gimp_histogram_clear_values (
                editor->histogram,
                babl_format_get_n_components (
                  gimp_drawable_get_format (editor->drawable)));

              gimp_histogram_view_set_histogram (view, editor->histogram);
            }

          async = gimp_drawable_calculate_histogram_async (editor->drawable,
                                                           editor->histogram,
                                                           TRUE);

          editor->calculate_async = async;

          gimp_async_add_callback (
            async,
            (GimpAsyncCallback) gimp_histogram_editor_calculate_async_callback,
            editor);

          g_object_unref (async);
        }
      else if (editor->histogram)
        {
          gimp_histogram_clear_values (editor->histogram, 0);

          gimp_histogram_editor_info_update (editor);
        }

      editor->recompute = FALSE;

      if (editor->idle_id)
        {
          g_source_remove (editor->idle_id);
          editor->idle_id = 0;
        }
    }

  return (editor->histogram != NULL);
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
          if (editor->idle_id)
            {
              g_source_remove (editor->idle_id);

              gimp_histogram_editor_idle_update (editor);
            }

          if (gimp_histogram_editor_validate (editor))
            {
              if (editor->calculate_async)
                {
                  editor->bg_pending = TRUE;
                }
              else
                {
                  editor->bg_histogram = gimp_histogram_duplicate (
                    editor->histogram);

                  gimp_histogram_view_set_background (view,
                                                      editor->bg_histogram);
                }
            }
        }
    }
  else
    {
      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
          gimp_histogram_view_set_background (view, NULL);
        }

      editor->bg_pending = FALSE;

      if (editor->update_pending)
        gimp_async_cancel_and_wait (editor->calculate_async);
    }
}

static void
gimp_histogram_editor_buffer_update (GimpHistogramEditor *editor,
                                     const GParamSpec    *pspec)
{
  g_object_set (editor,
                "trc", gimp_drawable_get_trc (editor->drawable),
                NULL);
}

static void
gimp_histogram_editor_update (GimpHistogramEditor *editor)
{
  if (editor->bg_pending)
    {
      editor->update_pending = TRUE;

      return;
    }

  editor->update_pending = FALSE;

  if (editor->calculate_async)
    gimp_async_cancel_and_wait (editor->calculate_async);

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

  /* Mark the histogram for recomputation and queue a redraw.
   * We will then recalculate the histogram when the view is exposed.
   */
  editor->recompute = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (editor->box));

  return FALSE;
}

static gboolean
gimp_histogram_menu_sensitivity (gint      value,
                                 gpointer  data)
{
  GimpHistogramEditor  *editor  = GIMP_HISTOGRAM_EDITOR (data);
  GimpHistogramChannel  channel = value;

  if (editor->histogram)
    return gimp_histogram_has_channel (editor->histogram, channel);

  return FALSE;
}

static void
gimp_histogram_editor_menu_update (GimpHistogramEditor *editor)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  gtk_widget_queue_draw (editor->menu);

  if (editor->histogram &&
      ! gimp_histogram_has_channel (editor->histogram, view->channel))
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

      /* For the RGB histogram, we need to divide by three
       * since it combines three histograms in one */
      if (view->channel == GIMP_HISTOGRAM_RGB)
        {
          pixels /= 3;
          count /= 3;
        }

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_mean (hist, view->channel,
                                           view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_MEAN]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_std_dev (hist, view->channel,
                                              view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_STD_DEV]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  gimp_histogram_get_median  (hist, view->channel,
                                              view->start,
                                              view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_MEDIAN]), text);

      g_snprintf (text, sizeof (text), "%d", (gint) pixels);
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_PIXELS]), text);

      g_snprintf (text, sizeof (text), "%d", (gint) count);
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_COUNT]), text);

      g_snprintf (text, sizeof (text), "%.1f", (pixels > 0 ?
                                                 (100.0 * count / pixels) :
                                                 0.0));
      gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_PERCENTILE]), text);

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->toggle)))
        {
          g_snprintf (text, sizeof (text), "%d",
                      gimp_histogram_unique_colors (editor->drawable));
          gtk_label_set_text (GTK_LABEL (editor->labels[LABEL_N_COLORS]), text);
        }
      else
        {
          gchar *markup = g_strdup_printf ("<i>%s</i>", _("n/a"));

          gtk_label_set_markup (GTK_LABEL (editor->labels[LABEL_N_COLORS]),
                                markup);
          g_free (markup);
        }
    }
  else
    {
      gint i;

      for (i = 0; i < N_LABELS; i++)
        gtk_label_set_text (GTK_LABEL (editor->labels[i]), NULL);
    }
}

static gboolean
gimp_histogram_editor_view_draw (GimpHistogramEditor *editor,
                                 cairo_t             *cr)
{
  gimp_histogram_editor_validate (editor);

  return FALSE;
}
