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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"
#include "base/pixel-region.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpenummenu.h"
#include "gimphelp-ids.h"
#include "gimphistogrambox.h"
#include "gimphistogrameditor.h"
#include "gimphistogramview.h"
#include "gimppropwidgets.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void  gimp_histogram_editor_class_init     (GimpHistogramEditorClass *klass);
static void  gimp_histogram_editor_init           (GimpHistogramEditor *editor);

static void  gimp_histogram_editor_set_image      (GimpImageEditor     *editor,
                                                   GimpImage           *gimage);

static void  gimp_histogram_editor_range_changed  (GimpHistogramView   *view,
                                                   gint                 start,
                                                   gint                 end,
                                                   GimpHistogramEditor *editor);
static void  gimp_histogram_editor_channel_notify (GimpHistogramView   *view,
                                                   GParamSpec          *pspec,
                                                   GimpHistogramEditor *editor);
static void  gimp_histogram_editor_layer_changed  (GimpImage           *gimage,
                                                   GimpHistogramEditor *editor);

static void  gimp_histogram_editor_update         (GimpHistogramEditor *editor);
static gboolean gimp_histogram_editor_idle_update (GimpHistogramEditor *editor);
static void  gimp_histogram_editor_menu_update    (GimpHistogramEditor *editor);
static void  gimp_histogram_editor_info_update    (GimpHistogramEditor *editor);


static GimpImageEditorClass *parent_class = NULL;


GType
gimp_histogram_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpHistogramEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_histogram_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpHistogramEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_histogram_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_IMAGE_EDITOR,
                                            "GimpHistogramEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_histogram_editor_class_init (GimpHistogramEditorClass* klass)
{
  GimpImageEditorClass *image_editor_class;

  image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  image_editor_class->set_image = gimp_histogram_editor_set_image;
}

static void
gimp_histogram_editor_init (GimpHistogramEditor *editor)
{
  GimpHistogramView *view;
  GtkWidget         *hbox;
  GtkWidget         *label;
  GtkWidget         *menu;
  GtkWidget         *table;
  gint               i;

  const gchar *gimp_histogram_editor_labels[] =
    {
      N_("Mean:"),
      N_("Std Dev:"),
      N_("Median:"),
      N_("Pixels:"),
      N_("Count:"),
      N_("Percentile:")
    };

  editor->drawable  = NULL;
  editor->histogram = NULL;
  editor->idle_id   = 0;
  editor->box       = gimp_histogram_box_new (_("Intensity Range:"));

  editor->name = label = gtk_label_new (_("(None)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (editor), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  view = GIMP_HISTOGRAM_BOX (editor->box)->view;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Channel:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  editor->menu = menu = gimp_prop_enum_option_menu_new (G_OBJECT (view),
                                                        "histogram-channel",
                                                        0, 0);
  gimp_enum_option_menu_set_stock_prefix (GTK_OPTION_MENU (menu),
                                          "gimp-channel");
  gtk_box_pack_start (GTK_BOX (hbox), menu, TRUE, TRUE, 0);
  gtk_widget_show (menu);

  menu = gimp_prop_enum_stock_box_new (G_OBJECT (view),
                                       "histogram-scale", "gimp-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  gtk_box_pack_start (GTK_BOX (editor), editor->box, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (editor->box));

  g_signal_connect (view, "range_changed",
                    G_CALLBACK (gimp_histogram_editor_range_changed),
                    editor);

  g_signal_connect (view, "notify::histogram-channel",
                    G_CALLBACK (gimp_histogram_editor_channel_notify),
                    editor);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (i = 0; i < 6; i++)
    {
      gint x = (i / 3) * 2;
      gint y = (i % 3);

      label = gtk_label_new (gettext (gimp_histogram_editor_labels[i]));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
			GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (label);

      editor->labels[i] = label = gtk_label_new (NULL);
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, x + 1, x + 2, y, y + 1,
                        GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (label);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
gimp_histogram_editor_set_image (GimpImageEditor *image_editor,
                                 GimpImage       *gimage)
{
  GimpHistogramEditor *editor;
  GimpHistogramView   *view;

  editor = GIMP_HISTOGRAM_EDITOR (image_editor);
  view   = GIMP_HISTOGRAM_BOX (editor->box)->view;

  if (image_editor->gimage)
    {
      if (editor->idle_id)
        {
          g_source_remove (editor->idle_id);
          editor->idle_id = 0;
        }

      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_histogram_editor_layer_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_histogram_editor_menu_update,
                                            editor);

      if (editor->histogram)
        {
          gimp_histogram_free (editor->histogram);
          editor->histogram = NULL;
        }

      gimp_histogram_view_set_histogram (view, NULL);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, gimage);

  if (gimage)
    {
      editor->histogram =
        gimp_histogram_new (GIMP_BASE_CONFIG (gimage->gimp->config));

      gimp_histogram_view_set_histogram (view, editor->histogram);

      g_signal_connect_object (gimage, "mode_changed",
                               G_CALLBACK (gimp_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (gimage, "active_layer_changed",
                               G_CALLBACK (gimp_histogram_editor_layer_changed),
                               editor, 0);
    }

  gimp_histogram_editor_layer_changed (gimage, editor);
}

GtkWidget *
gimp_histogram_editor_new (GimpImage *gimage)
{
  GimpHistogramEditor *editor;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);

  editor = g_object_new (GIMP_TYPE_HISTOGRAM_EDITOR, NULL);

  if (gimage)
    gimp_image_editor_set_image (GIMP_IMAGE_EDITOR (editor), gimage);

  return GTK_WIDGET (editor);
}

static void
gimp_histogram_editor_range_changed (GimpHistogramView   *view,
                                     gint                 start,
                                     gint                 end,
                                     GimpHistogramEditor *editor)
{
  gimp_histogram_editor_info_update (editor);
}

static void
gimp_histogram_editor_channel_notify (GimpHistogramView   *view,
                                      GParamSpec          *pspec,
                                      GimpHistogramEditor *editor)
{
  gimp_histogram_editor_info_update (editor);
}

static void
gimp_histogram_editor_layer_changed (GimpImage           *gimage,
                                     GimpHistogramEditor *editor)
{
  const gchar *name;

  if (editor->drawable)
    {
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_menu_update,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_histogram_editor_update,
                                            editor);
      editor->drawable = NULL;
    }

  if (gimage)
    editor->drawable = (GimpDrawable *) gimp_image_get_active_layer (gimage);

  gimp_histogram_editor_menu_update (editor);

  if (editor->drawable)
    {
      name = gimp_object_get_name (GIMP_OBJECT (editor->drawable));

      g_signal_connect_object (editor->drawable, "invalidate_preview",
                               G_CALLBACK (gimp_histogram_editor_update),
                               editor, G_CONNECT_SWAPPED);
      g_signal_connect_object (editor->drawable, "alpha_changed",
                               G_CALLBACK (gimp_histogram_editor_menu_update),
                               editor, G_CONNECT_SWAPPED);

      gimp_histogram_editor_update (editor);
    }
  else
    {
      name = _("(None)");

      if (editor->histogram)
        {
          gimp_histogram_calculate (editor->histogram, NULL, NULL);
          gtk_widget_queue_draw GTK_WIDGET (editor->box);
          gimp_histogram_editor_info_update (editor);
        }
    }

  gtk_label_set_text (GTK_LABEL (editor->name), name);
}

static void
gimp_histogram_editor_update (GimpHistogramEditor *editor)
{
  if (editor->idle_id)
    g_source_remove (editor->idle_id);

  editor->idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                     (GSourceFunc) gimp_histogram_editor_idle_update,
                                     editor,
                                     (GDestroyNotify) NULL);
}

static gboolean
gimp_histogram_editor_idle_update (GimpHistogramEditor *editor)
{
  editor->idle_id = 0;

  if (editor->drawable && editor->histogram)
    {
      PixelRegion region;

      pixel_region_init (&region, gimp_drawable_data (editor->drawable),
                         0, 0,
                         gimp_item_width  (GIMP_ITEM (editor->drawable)),
                         gimp_item_height (GIMP_ITEM (editor->drawable)),
                         FALSE);

      gimp_histogram_calculate (editor->histogram, &region, NULL);
      gtk_widget_queue_draw GTK_WIDGET (editor->box);
      gimp_histogram_editor_info_update (editor);
    }

  return FALSE;
}

static gboolean
gimp_histogram_editor_item_sensitive (GimpHistogramChannel  channel,
                                      GimpDrawable         *drawable)
{
  if (! drawable)
    return FALSE;

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return gimp_drawable_is_rgb (drawable);

    case GIMP_HISTOGRAM_ALPHA:
      return gimp_drawable_has_alpha (drawable);
    }

    return FALSE;
}

static void
gimp_histogram_editor_menu_update (GimpHistogramEditor *editor)
{
  GimpHistogramView    *view    = GIMP_HISTOGRAM_BOX (editor->box)->view;
  GimpHistogramChannel  channel = gimp_histogram_view_get_channel (view);

  if (! gimp_histogram_editor_item_sensitive (channel, editor->drawable))
    {
      gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
    }

  gimp_int_option_menu_set_sensitive (GTK_OPTION_MENU (editor->menu),
                                      (GimpIntOptionMenuSensitivityCallback) gimp_histogram_editor_item_sensitive,
                                      editor->drawable);
}

static void
gimp_histogram_editor_info_update (GimpHistogramEditor *editor)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_BOX (editor->box)->view;
  GimpHistogram     *hist = editor->histogram;

  if (hist)
    {
      GimpHistogramChannel channel = gimp_histogram_view_get_channel (view);
      gdouble              pixels;
      gdouble              count;
      gchar                text[12];

      /* FIXME: hack */
      if (gimp_histogram_n_channels (hist) == 2)
        channel = (channel > 0) ? 1 : 0;

      pixels = gimp_histogram_get_count (hist, channel, 0, 255);
      count  = gimp_histogram_get_count (hist, channel, view->start, view->end);

      g_snprintf (text, sizeof (text), "%3.1f",
                  gimp_histogram_get_mean (hist, channel,
                                           view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[0]), text);

      g_snprintf (text, sizeof (text), "%3.1f",
                  gimp_histogram_get_std_dev (hist, channel,
                                              view->start, view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[1]), text);

      g_snprintf (text, sizeof (text), "%3.1f",
                  (gdouble) gimp_histogram_get_median  (hist, channel,
                                                        view->start,
                                                        view->end));
      gtk_label_set_text (GTK_LABEL (editor->labels[2]), text);

      g_snprintf (text, sizeof (text), "%8d", (gint) pixels);
      gtk_label_set_text (GTK_LABEL (editor->labels[3]), text);

      g_snprintf (text, sizeof (text), "%8d", (gint) count);
      gtk_label_set_text (GTK_LABEL (editor->labels[4]), text);

      g_snprintf (text, sizeof (text), "%4.1f", (pixels > 0 ?
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
