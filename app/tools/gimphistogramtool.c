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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/pixel-region.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphistogrambox.h"
#include "widgets/gimphistogramview.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"

#include "gimphistogramtool.h"
#include "tool_manager.h"

#include "app_procs.h"

#include "gimp-intl.h"


typedef struct _HistogramToolDialog HistogramToolDialog;

struct _HistogramToolDialog
{
  GtkWidget         *shell;

  GtkWidget         *info_labels[6];
  GtkWidget         *channel_menu;
  GimpHistogramBox  *histogram_box;
  GimpHistogram     *hist;
  GtkWidget         *gradient;

  gdouble            mean;
  gdouble            std_dev;
  gdouble            median;
  gdouble            pixels;
  gdouble            count;
  gdouble            percentile;

  GimpDrawable      *drawable;
};


/*  local function prototypes  */

static void   gimp_histogram_tool_class_init (GimpHistogramToolClass *klass);
static void   gimp_histogram_tool_init       (GimpHistogramTool      *hist_tool);

static void   gimp_histogram_tool_initialize (GimpTool             *tool,
					      GimpDisplay          *gdisp);
static void   gimp_histogram_tool_control    (GimpTool             *tool,
					      GimpToolAction        action,
					      GimpDisplay          *gdisp);

static HistogramToolDialog *  histogram_tool_dialog_new (GimpToolInfo *tool_info);

static void   histogram_tool_close_callback   (GtkWidget            *widget,
					       gpointer              data);
static gboolean histogram_set_sensitive_callback 
                                              (gpointer              item_data,
                                               HistogramToolDialog  *htd);
static void   histogram_tool_dialog_update    (HistogramToolDialog  *htd,
					       gint                  start,
					       gint                  end);

static void   histogram_tool_histogram_range  (GimpHistogramView    *view,
                                               gint                  start,
                                               gint                  end,
                                               gpointer              data);


static HistogramToolDialog * histogram_dialog = NULL;

static GimpImageMapToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_histogram_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_HISTOGRAM_TOOL,
                G_TYPE_NONE, NULL,
                0,
                "gimp-histogram-tool",
                _("Histogram"),
                _("View image histogram"),
                N_("/Tools/Color Tools/_Histogram..."), NULL,
                NULL, "tools/histogram.html",
                GIMP_STOCK_TOOL_HISTOGRAM,
                data);
}

GType
gimp_histogram_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpHistogramToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_histogram_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpHistogramTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_histogram_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
                                          "GimpHistogramTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_histogram_tool_class_init (GimpHistogramToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->initialize = gimp_histogram_tool_initialize;
  tool_class->control    = gimp_histogram_tool_control;
}

static void
gimp_histogram_tool_init (GimpHistogramTool *hist_tool)
{
}

static void
gimp_histogram_tool_initialize (GimpTool    *tool,
				GimpDisplay *gdisp)
{
  GimpDrawable *drawable;
  PixelRegion   PR;

  if (gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Histogram does not operate on indexed drawables."));
      return;
    }

  /*  The histogram_tool dialog  */
  if (! histogram_dialog)
    histogram_dialog = histogram_tool_dialog_new (tool->tool_info);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (histogram_dialog->shell),
                                     GIMP_VIEWABLE (drawable));

  gtk_widget_show (histogram_dialog->shell);

  histogram_dialog->drawable = drawable;

  gimp_option_menu_set_sensitive (GTK_OPTION_MENU (histogram_dialog->channel_menu),
                                  (GimpOptionMenuSensitivityCallback) histogram_set_sensitive_callback,
                                  histogram_dialog);

  /* calculate the histogram */
  pixel_region_init (&PR, gimp_drawable_data (drawable),
		     0, 0,
		     gimp_item_width  (GIMP_ITEM (drawable)),
		     gimp_item_height (GIMP_ITEM (drawable)),
		     FALSE);
  gimp_histogram_calculate (histogram_dialog->hist, &PR, NULL);

  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (histogram_dialog->histogram_box->histogram),
                                     histogram_dialog->hist);
  gimp_histogram_view_set_range (GIMP_HISTOGRAM_VIEW (histogram_dialog->histogram_box->histogram),
                                 0, 255);
}

static void
gimp_histogram_tool_control (GimpTool       *tool,
			     GimpToolAction  action,
			     GimpDisplay    *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (histogram_dialog)
        histogram_tool_close_callback (NULL, histogram_dialog);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}


/*  histogram_tool machinery  */

static void
histogram_tool_histogram_range (GimpHistogramView *widget,
				gint               start,
				gint               end,
				gpointer           data)
{
  HistogramToolDialog  *htd;
  GimpHistogramChannel  channel;
  gdouble               pixels;
  gdouble               count;

  htd = (HistogramToolDialog *) data;

  if (htd == NULL || htd->hist == NULL ||
      gimp_histogram_nchannels (htd->hist) <= 0)
    return;

  channel = gimp_histogram_view_get_channel (htd->histogram_box->histogram);

  pixels = gimp_histogram_get_count (htd->hist, 0, 255);
  count  = gimp_histogram_get_count (htd->hist, start, end);

  htd->mean       = gimp_histogram_get_mean    (htd->hist, channel, start, end);
  htd->std_dev    = gimp_histogram_get_std_dev (htd->hist, channel, start, end);
  htd->median     = gimp_histogram_get_median  (htd->hist, channel, start, end);
  htd->pixels     = pixels;
  htd->count      = count;
  htd->percentile = count / pixels;

  if (htd->shell)
    histogram_tool_dialog_update (htd, start, end);
}

static void
histogram_tool_dialog_update (HistogramToolDialog *htd,
			      gint                 start,
			      gint                 end)
{
  gchar text[12];

  /*  mean  */
  g_snprintf (text, sizeof (text), "%3.1f", htd->mean);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[0]), text);

  /*  std dev  */
  g_snprintf (text, sizeof (text), "%3.1f", htd->std_dev);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[1]), text);

  /*  median  */
  g_snprintf (text, sizeof (text), "%3.1f", htd->median);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[2]), text);

  /*  pixels  */
  g_snprintf (text, sizeof (text), "%8.1f", htd->pixels);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[3]), text);

  /*  count  */
  g_snprintf (text, sizeof (text), "%8.1f", htd->count);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[4]), text);

  /*  percentile  */
  g_snprintf (text, sizeof (text), "%2.2f", htd->percentile * 100);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[5]), text);
}

/***************************/
/*  Histogram Tool dialog  */
/***************************/

static HistogramToolDialog *
histogram_tool_dialog_new (GimpToolInfo *tool_info)
{
  HistogramToolDialog *htd;
  GtkWidget   *hbox;
  GtkWidget   *vbox;
  GtkWidget   *table;
  GtkWidget   *label;
  GtkWidget   *menu;
  const gchar *stock_id;
  gint         i;
  gint         x, y;

  static const gchar *histogram_info_names[] =
  {
    N_("Mean:"),
    N_("Std Dev:"),
    N_("Median:"),
    N_("Pixels:"),
    N_("Count:"),
    N_("Percentile:")
  };

  htd = g_new0 (HistogramToolDialog, 1);
  htd->hist = gimp_histogram_new (GIMP_BASE_CONFIG (tool_info->gimp->config));

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

  /*  The shell and main vbox  */
  htd->shell =
    gimp_viewable_dialog_new (NULL,
                              tool_info->blurb,
                              GIMP_OBJECT (tool_info)->name,
                              stock_id,
                              _("View Image Histogram"),
                              gimp_standard_help_func, tool_info->help_data,

                              GTK_STOCK_CLOSE, histogram_tool_close_callback,
                              htd, NULL, NULL, TRUE, TRUE,

                              NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (htd->shell)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Create the histogram view first  */
  htd->histogram_box =
    GIMP_HISTOGRAM_BOX (gimp_histogram_box_new (_("Intensity Range:")));

  /*  The option menu for selecting channels  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Information on Channel:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  htd->channel_menu =
    gimp_prop_enum_option_menu_new (G_OBJECT (htd->histogram_box->histogram),
				    "channel", 0, 0);
  gimp_enum_option_menu_set_stock_prefix (GTK_OPTION_MENU (htd->channel_menu),
                                          "gimp-channel");
  gtk_box_pack_start (GTK_BOX (hbox), htd->channel_menu, FALSE, FALSE, 0);
  gtk_widget_show (htd->channel_menu);

  /*  The histogram tool histogram  */
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (htd->histogram_box),
                      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (htd->histogram_box));

  g_signal_connect (htd->histogram_box->histogram, "range_changed",
                    G_CALLBACK (histogram_tool_histogram_range),
                    htd);

  /*  The option menu for selecting the histogram scale  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Histogram Scale:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  menu =
    gimp_prop_enum_option_menu_new (G_OBJECT (htd->histogram_box->histogram),
				    "scale", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  /*  The table containing histogram information  */
  table = gtk_table_new (3, 4, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the labels for histogram information  */
  for (i = 0; i < 6; i++)
    {
      y = (i % 3);
      x = (i / 3) * 2;

      label = gtk_label_new (gettext (histogram_info_names[i]));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
			GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (label);

      htd->info_labels[i] = gtk_label_new ("0");
      gtk_misc_set_alignment (GTK_MISC (htd->info_labels[i]), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), htd->info_labels[i],
			x + 1, x + 2, y, y + 1,
			GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (htd->info_labels[i]);
    }

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   "gimp-histogram-tool-dialog",
                                   htd->shell);

  return htd;
}

static void
histogram_tool_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  HistogramToolDialog *htd;
  GimpTool            *active_tool;

  htd = (HistogramToolDialog *) data;

  gtk_widget_hide (htd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static gboolean
histogram_set_sensitive_callback (gpointer             item_data,
                                  HistogramToolDialog *htd)
{
  GimpHistogramChannel  channel = GPOINTER_TO_INT (item_data);
  
  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;
    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return gimp_drawable_is_rgb (htd->drawable);
    case GIMP_HISTOGRAM_ALPHA:
      return gimp_drawable_has_alpha (htd->drawable);
    }
  
  return FALSE;
}
