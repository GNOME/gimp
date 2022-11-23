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
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmaasync.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-histogram.h"
#include "core/ligmahistogram.h"
#include "core/ligmaimage.h"

#include "ligmadocked.h"
#include "ligmahelp-ids.h"
#include "ligmahistogrambox.h"
#include "ligmahistogrameditor.h"
#include "ligmahistogramview.h"
#include "ligmapropwidgets.h"
#include "ligmasessioninfo-aux.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_TRC
};


static void     ligma_histogram_editor_docked_iface_init (LigmaDockedInterface *iface);

static void     ligma_histogram_editor_set_property  (GObject            *object,
                                                     guint               property_id,
                                                     const GValue       *value,
                                                     GParamSpec         *pspec);
static void     ligma_histogram_editor_get_property  (GObject            *object,
                                                     guint               property_id,
                                                     GValue             *value,
                                                     GParamSpec         *pspec);

static void     ligma_histogram_editor_set_aux_info  (LigmaDocked          *docked,
                                                     GList               *aux_info);
static GList  * ligma_histogram_editor_get_aux_info  (LigmaDocked          *docked);

static void     ligma_histogram_editor_set_image     (LigmaImageEditor     *editor,
                                                     LigmaImage           *image);
static void     ligma_histogram_editor_layer_changed (LigmaImage           *image,
                                                     LigmaHistogramEditor *editor);
static void     ligma_histogram_editor_frozen_update (LigmaHistogramEditor *editor,
                                                     const GParamSpec    *pspec);
static void     ligma_histogram_editor_buffer_update (LigmaHistogramEditor *editor,
                                                     const GParamSpec    *pspec);
static void     ligma_histogram_editor_update        (LigmaHistogramEditor *editor);

static gboolean ligma_histogram_editor_idle_update   (LigmaHistogramEditor *editor);
static gboolean ligma_histogram_menu_sensitivity     (gint                 value,
                                                     gpointer             data);
static void     ligma_histogram_editor_menu_update   (LigmaHistogramEditor *editor);
static void     ligma_histogram_editor_name_update   (LigmaHistogramEditor *editor);
static void     ligma_histogram_editor_info_update   (LigmaHistogramEditor *editor);

static gboolean ligma_histogram_editor_view_draw     (LigmaHistogramEditor *editor,
                                                     cairo_t             *cr);


G_DEFINE_TYPE_WITH_CODE (LigmaHistogramEditor, ligma_histogram_editor,
                         LIGMA_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_histogram_editor_docked_iface_init))

#define parent_class ligma_histogram_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_histogram_editor_class_init (LigmaHistogramEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  LigmaImageEditorClass *image_editor_class = LIGMA_IMAGE_EDITOR_CLASS (klass);

  object_class->set_property    = ligma_histogram_editor_set_property;
  object_class->get_property    = ligma_histogram_editor_get_property;

  image_editor_class->set_image = ligma_histogram_editor_set_image;

  g_object_class_install_property (object_class, PROP_TRC,
                                   g_param_spec_enum ("trc",
                                                      _("Linear/Perceptual"),
                                                      NULL,
                                                      LIGMA_TYPE_TRC_TYPE,
                                                      LIGMA_TRC_LINEAR,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
ligma_histogram_editor_init (LigmaHistogramEditor *editor)
{
  LigmaHistogramView *view;
  GtkWidget         *hbox;
  GtkWidget         *label;
  GtkWidget         *menu;
  GtkWidget         *grid;
  gint               i;

  const gchar *ligma_histogram_editor_labels[] =
    {
      N_("Mean:"),
      N_("Std dev:"),
      N_("Median:"),
      N_("Pixels:"),
      N_("Count:"),
      N_("Percentile:")
    };

  editor->box = ligma_histogram_box_new ();

  ligma_editor_set_show_name (LIGMA_EDITOR (editor), TRUE);

  view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->menu = menu = ligma_prop_enum_combo_box_new (G_OBJECT (view),
                                                      "histogram-channel",
                                                      0, 0);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (menu),
                                       "ligma-channel");
  ligma_int_combo_box_set_sensitivity (LIGMA_INT_COMBO_BOX (editor->menu),
                                      ligma_histogram_menu_sensitivity,
                                      editor, NULL);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (editor->menu),
                                 view->channel);
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  ligma_help_set_help_data (editor->menu,
                           _("Histogram channel"), NULL);

  menu = ligma_prop_enum_icon_box_new (G_OBJECT (view),
                                      "histogram-scale", "ligma-histogram",
                                      0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  menu = ligma_prop_enum_icon_box_new (G_OBJECT (editor), "trc",
                                      "ligma-color-space",
                                      -1, -1);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (editor), editor->box, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->box));

  g_signal_connect_swapped (view, "range-changed",
                            G_CALLBACK (ligma_histogram_editor_info_update),
                            editor);
  g_signal_connect_swapped (view, "notify::histogram-channel",
                            G_CALLBACK (ligma_histogram_editor_info_update),
                            editor);

  g_signal_connect_swapped (view, "draw",
                            G_CALLBACK (ligma_histogram_editor_view_draw),
                            editor);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  for (i = 0; i < 6; i++)
    {
      gint x = (i / 3) * 2;
      gint y = (i % 3);

      label = gtk_label_new (gettext (ligma_histogram_editor_labels[i]));
      ligma_label_set_attributes (GTK_LABEL (label),
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
      ligma_label_set_attributes (GTK_LABEL (editor->labels[i]),
                                 PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                                 -1);
      gtk_grid_attach (GTK_GRID (grid), label, x + 1, y, 1, 1);
      gtk_widget_show (label);
    }
}

static void
ligma_histogram_editor_docked_iface_init (LigmaDockedInterface *docked_iface)
{
  parent_docked_iface = g_type_interface_peek_parent (docked_iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  docked_iface->set_aux_info = ligma_histogram_editor_set_aux_info;
  docked_iface->get_aux_info = ligma_histogram_editor_get_aux_info;
}

static void
ligma_histogram_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaHistogramEditor *editor = LIGMA_HISTOGRAM_EDITOR (object);
  LigmaHistogramView   *view   = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  switch (property_id)
    {
    case PROP_TRC:
      editor->trc = g_value_get_enum (value);

      if (editor->histogram)
        {
          g_clear_object (&editor->histogram);
          ligma_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
          ligma_histogram_view_set_background (view, NULL);
        }

      ligma_histogram_editor_update (editor);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_histogram_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaHistogramEditor *editor = LIGMA_HISTOGRAM_EDITOR (object);

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
ligma_histogram_editor_set_aux_info (LigmaDocked *docked,
                                    GList      *aux_info)
{
  LigmaHistogramEditor *editor = LIGMA_HISTOGRAM_EDITOR (docked);
  LigmaHistogramView   *view   = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  parent_docked_iface->set_aux_info (docked, aux_info);

  ligma_session_info_aux_set_props (G_OBJECT (view), aux_info,
                                   "histogram-channel",
                                   "histogram-scale",
                                   NULL);
}

static GList *
ligma_histogram_editor_get_aux_info (LigmaDocked *docked)
{
  LigmaHistogramEditor *editor = LIGMA_HISTOGRAM_EDITOR (docked);
  LigmaHistogramView   *view   = LIGMA_HISTOGRAM_BOX (editor->box)->view;
  GList               *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  return g_list_concat (aux_info,
                        ligma_session_info_aux_new_from_props (G_OBJECT (view),
                                                              "histogram-channel",
                                                              "histogram-scale",
                                                              NULL));
}

static void
ligma_histogram_editor_set_image (LigmaImageEditor *image_editor,
                                 LigmaImage       *image)
{
  LigmaHistogramEditor *editor = LIGMA_HISTOGRAM_EDITOR (image_editor);
  LigmaHistogramView   *view   = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  if (image_editor->image)
    {
      if (editor->idle_id)
        {
          g_source_remove (editor->idle_id);
          editor->idle_id = 0;
        }

      editor->update_pending = FALSE;

      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_histogram_editor_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_histogram_editor_layer_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_histogram_editor_menu_update,
                                            editor);

      if (editor->histogram)
        {
          g_clear_object (&editor->histogram);
          ligma_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
          ligma_histogram_view_set_background (view, NULL);
        }
    }

  LIGMA_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect_object (image, "mode-changed",
                               G_CALLBACK (ligma_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (image, "selected-layers-changed",
                               G_CALLBACK (ligma_histogram_editor_layer_changed),
                               editor, 0);
      g_signal_connect_object (image, "mask-changed",
                               G_CALLBACK (ligma_histogram_editor_update),
                               editor, G_CONNECT_SWAPPED);
    }

  ligma_histogram_editor_layer_changed (image, editor);
}

GtkWidget *
ligma_histogram_editor_new (void)
{
  return g_object_new (LIGMA_TYPE_HISTOGRAM_EDITOR, NULL);
}

static void
ligma_histogram_editor_layer_changed (LigmaImage           *image,
                                     LigmaHistogramEditor *editor)
{
  if (editor->drawable)
    {
      LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

      if (editor->histogram)
        {
          g_clear_object (&editor->histogram);
          ligma_histogram_view_set_histogram (view, NULL);
        }

      if (editor->bg_histogram)
        {
          g_clear_object (&editor->bg_histogram);
          ligma_histogram_view_set_background (view, NULL);
        }

      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            ligma_histogram_editor_name_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            ligma_histogram_editor_menu_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            ligma_histogram_editor_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            ligma_histogram_editor_buffer_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            ligma_histogram_editor_frozen_update,
                                            editor);
      editor->drawable = NULL;
    }

  if (image)
    {
      GList *layers;

      layers = ligma_image_get_selected_layers (image);
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

  ligma_histogram_editor_menu_update (editor);

  if (editor->drawable)
    {
      g_signal_connect_object (editor->drawable, "notify::frozen",
                               G_CALLBACK (ligma_histogram_editor_frozen_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "notify::buffer",
                               G_CALLBACK (ligma_histogram_editor_buffer_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "update",
                               G_CALLBACK (ligma_histogram_editor_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "alpha-changed",
                               G_CALLBACK (ligma_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "name-changed",
                               G_CALLBACK (ligma_histogram_editor_name_update),
                               editor, G_CONNECT_SWAPPED);

      ligma_histogram_editor_buffer_update (editor, NULL);
    }
  else if (editor->histogram)
    {
      editor->recompute = TRUE;
      gtk_widget_queue_draw (GTK_WIDGET (editor->box));
    }

  ligma_histogram_editor_info_update (editor);
  ligma_histogram_editor_name_update (editor);
}

static void
ligma_histogram_editor_calculate_async_callback (LigmaAsync           *async,
                                                LigmaHistogramEditor *editor)
{
  editor->calculate_async = NULL;

  if (ligma_async_is_finished (async) && editor->histogram)
    {
      if (editor->bg_pending)
        {
          LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

          editor->bg_histogram = ligma_histogram_duplicate (editor->histogram);

          ligma_histogram_view_set_background (view, editor->bg_histogram);
        }

      ligma_histogram_editor_info_update (editor);
    }

  editor->bg_pending = FALSE;

  if (editor->update_pending)
    ligma_histogram_editor_update (editor);
}

static gboolean
ligma_histogram_editor_validate (LigmaHistogramEditor *editor)
{
  if (editor->recompute || ! editor->histogram)
    {
      if (editor->drawable &&
          /* avoid calculating the histogram of a detached layer.  this can
           * happen during ligma_image_remove_layer(), as a result of a pending
           * "expose-event" signal (handled in
           * ligma_histogram_editor_view_expose()) executed through
           * gtk_tree_view_clamp_node_visible(), as a result of the
           * LigmaLayerTreeView in the Layers dialog receiving the image's
           * "selected-layers-changed" signal before us.  See bug #795716,
           * comment 6.
           */
          ligma_item_is_attached (LIGMA_ITEM (editor->drawable)))
        {
          LigmaAsync *async;

          if (! editor->histogram)
            {
              LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

              editor->histogram = ligma_histogram_new (editor->trc);

              ligma_histogram_clear_values (
                editor->histogram,
                babl_format_get_n_components (
                  ligma_drawable_get_format (editor->drawable)));

              ligma_histogram_view_set_histogram (view, editor->histogram);
            }

          async = ligma_drawable_calculate_histogram_async (editor->drawable,
                                                           editor->histogram,
                                                           TRUE);

          editor->calculate_async = async;

          ligma_async_add_callback (
            async,
            (LigmaAsyncCallback) ligma_histogram_editor_calculate_async_callback,
            editor);

          g_object_unref (async);
        }
      else if (editor->histogram)
        {
          ligma_histogram_clear_values (editor->histogram, 0);

          ligma_histogram_editor_info_update (editor);
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
ligma_histogram_editor_frozen_update (LigmaHistogramEditor *editor,
                                     const GParamSpec    *pspec)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  if (ligma_viewable_preview_is_frozen (LIGMA_VIEWABLE (editor->drawable)))
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

              ligma_histogram_editor_idle_update (editor);
            }

          if (ligma_histogram_editor_validate (editor))
            {
              if (editor->calculate_async)
                {
                  editor->bg_pending = TRUE;
                }
              else
                {
                  editor->bg_histogram = ligma_histogram_duplicate (
                    editor->histogram);

                  ligma_histogram_view_set_background (view,
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
          ligma_histogram_view_set_background (view, NULL);
        }

      editor->bg_pending = FALSE;

      if (editor->update_pending)
        ligma_async_cancel_and_wait (editor->calculate_async);
    }
}

static void
ligma_histogram_editor_buffer_update (LigmaHistogramEditor *editor,
                                     const GParamSpec    *pspec)
{
  g_object_set (editor,
                "trc", ligma_drawable_get_trc (editor->drawable),
                NULL);
}

static void
ligma_histogram_editor_update (LigmaHistogramEditor *editor)
{
  if (editor->bg_pending)
    {
      editor->update_pending = TRUE;

      return;
    }

  editor->update_pending = FALSE;

  if (editor->calculate_async)
    ligma_async_cancel_and_wait (editor->calculate_async);

  if (editor->idle_id)
    g_source_remove (editor->idle_id);

  editor->idle_id =
    g_timeout_add_full (G_PRIORITY_LOW,
                        200,
                        (GSourceFunc) ligma_histogram_editor_idle_update,
                        editor,
                        NULL);
}

static gboolean
ligma_histogram_editor_idle_update (LigmaHistogramEditor *editor)
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
ligma_histogram_menu_sensitivity (gint      value,
                                 gpointer  data)
{
  LigmaHistogramEditor  *editor  = LIGMA_HISTOGRAM_EDITOR (data);
  LigmaHistogramChannel  channel = value;

  if (editor->histogram)
    return ligma_histogram_has_channel (editor->histogram, channel);

  return FALSE;
}

static void
ligma_histogram_editor_menu_update (LigmaHistogramEditor *editor)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;

  gtk_widget_queue_draw (editor->menu);

  if (editor->histogram &&
      ! ligma_histogram_has_channel (editor->histogram, view->channel))
    {
      ligma_histogram_view_set_channel (view, LIGMA_HISTOGRAM_VALUE);
    }
}

static void
ligma_histogram_editor_name_update (LigmaHistogramEditor *editor)
{
  const gchar *name = NULL;

  if (editor->drawable)
    name = ligma_object_get_name (editor->drawable);

  ligma_editor_set_name (LIGMA_EDITOR (editor), name);
}

static void
ligma_histogram_editor_info_update (LigmaHistogramEditor *editor)
{
  LigmaHistogramView *view = LIGMA_HISTOGRAM_BOX (editor->box)->view;
  LigmaHistogram     *hist = editor->histogram;

  if (hist)
    {
      gint    n_bins;
      gdouble pixels;
      gdouble count;
      gchar   text[12];

      n_bins = ligma_histogram_n_bins (hist);

      pixels = ligma_histogram_get_count (hist, view->channel, 0, n_bins - 1);
      count  = ligma_histogram_get_count (hist, view->channel,
                                         view->start, view->end);

      g_snprintf (text, sizeof (text), "%.3f",
                  ligma_histogram_get_mean (hist, view->channel,
                                           view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[0]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  ligma_histogram_get_std_dev (hist, view->channel,
                                              view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[1]), text);

      g_snprintf (text, sizeof (text), "%.3f",
                  ligma_histogram_get_median  (hist, view->channel,
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
ligma_histogram_editor_view_draw (LigmaHistogramEditor *editor,
                                 cairo_t             *cr)
{
  ligma_histogram_editor_validate (editor);

  return FALSE;
}
