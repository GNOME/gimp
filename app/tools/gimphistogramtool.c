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

#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "histogram_tool.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

#define TEXT_WIDTH       45
#define GRADIENT_HEIGHT  15

/*  the histogram structures  */

typedef struct _HistogramTool HistogramTool;

struct _HistogramTool
{
  gint x, y;   /*  coords for last mouse click  */
};

/*  the histogram tool options  */
static ToolOptions * histogram_tool_options = NULL;

/*  the histogram tool dialog  */
static HistogramToolDialog * histogram_tool_dialog = NULL;

/*  histogram_tool action functions  */
static void   histogram_tool_control (Tool *, ToolAction, gpointer);

static HistogramToolDialog *  histogram_tool_dialog_new (void);

static void   histogram_tool_close_callback   (GtkWidget *, gpointer);
static void   histogram_tool_channel_callback (GtkWidget *, gpointer);
static void   histogram_tool_gradient_draw    (GtkWidget *, gint);

static void   histogram_tool_dialog_update    (HistogramToolDialog *,
					       gint, gint);

/*  histogram_tool machinery  */

void
histogram_tool_histogram_range (HistogramWidget *widget,
				gint             start,
				gint             end,
				gpointer         data)
{
  HistogramToolDialog *htd;
  gdouble pixels;
  gdouble count;

  htd = (HistogramToolDialog *) data;

  if (htd == NULL || htd->hist == NULL ||
      gimp_histogram_nchannels(htd->hist) <= 0)
    return;

  pixels = gimp_histogram_get_count(htd->hist, 0, 255);
  count = gimp_histogram_get_count(htd->hist, start, end);

  htd->mean = gimp_histogram_get_mean(htd->hist, htd->channel, start, end);
  htd->std_dev = gimp_histogram_get_std_dev(htd->hist, htd->channel,
					    start, end);
  htd->median = gimp_histogram_get_median(htd->hist, htd->channel, start, end);
  htd->pixels = pixels;
  htd->count = count;
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

  /*  intensity  */
  if (start == end)
    g_snprintf (text, sizeof (text), "%d", start);
  else
    g_snprintf (text, sizeof (text), "%d..%d", start, end);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[4]), text);

  /*  count  */
  g_snprintf (text, sizeof (text), "%8.1f", htd->count);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[5]), text);

  /*  percentile  */
  g_snprintf (text, sizeof (text), "%2.2f", htd->percentile * 100);
  gtk_label_set_text (GTK_LABEL (htd->info_labels[6]), text);
}

/*  histogram_tool action functions  */

static void
histogram_tool_control (Tool       *tool,
			ToolAction  action,
			gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (histogram_tool_dialog)
	histogram_tool_close_callback (NULL, (gpointer) histogram_tool_dialog);
      break;

    default:
      break;
    }
}

Tool *
tools_new_histogram_tool (void)
{
  Tool * tool;
  HistogramTool * private;

  /*  The tool options  */
  if (! histogram_tool_options)
    {
      histogram_tool_options = tool_options_new (_("Histogram"));
      tools_register (HISTOGRAM, histogram_tool_options);
    }

  tool = tools_new_tool (HISTOGRAM);
  private = g_new (HistogramTool, 1);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->control_func = histogram_tool_control;

  return tool;
}

void
tools_free_histogram_tool (Tool *tool)
{
  HistogramTool * hist;

  hist = (HistogramTool *) tool->private;

  /*  Close the histogram dialog  */
  if (histogram_tool_dialog)
    histogram_tool_close_callback (NULL, (gpointer) histogram_tool_dialog);

  g_free (hist);
}

void
histogram_tool_initialize (GDisplay *gdisp)
{
  PixelRegion PR;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Histogram does not operate on indexed drawables."));
      return;
    }

  /*  The histogram_tool dialog  */
  if (!histogram_tool_dialog)
    histogram_tool_dialog = histogram_tool_dialog_new ();
  else if (!GTK_WIDGET_VISIBLE (histogram_tool_dialog->shell))
    gtk_widget_show (histogram_tool_dialog->shell);

  histogram_tool_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  histogram_tool_dialog->color = drawable_color (histogram_tool_dialog->drawable);

  /*  hide or show the channel menu based on image type  */
  if (histogram_tool_dialog->color)
    gtk_widget_show (histogram_tool_dialog->channel_menu);
  else
    gtk_widget_hide (histogram_tool_dialog->channel_menu);

  /* calculate the histogram */
  pixel_region_init (&PR, drawable_data (histogram_tool_dialog->drawable), 0, 0,
		     drawable_width (histogram_tool_dialog->drawable),
		     drawable_height (histogram_tool_dialog->drawable),
		     FALSE);
  gimp_histogram_calculate (histogram_tool_dialog->hist, &PR, NULL);

  histogram_widget_update (histogram_tool_dialog->histogram,
			   histogram_tool_dialog->hist);
  histogram_widget_range (histogram_tool_dialog->histogram, 0, 255);
}

/***************************/
/*  Histogram Tool dialog  */
/***************************/

static HistogramToolDialog *
histogram_tool_dialog_new (void)
{
  HistogramToolDialog *htd;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *grad_hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *option_menu;
  gint i;
  gint x, y;

  static gchar * histogram_info_names[] =
  {
    N_("Mean:"),
    N_("Std Dev:"),
    N_("Median:"),
    N_("Pixels:"),
    N_("Intensity:"),
    N_("Count:"),
    N_("Percentile:")
  };

  htd = g_new (HistogramToolDialog, 1);
  htd->channel = HISTOGRAM_VALUE;
  htd->hist    = gimp_histogram_new ();

  /*  The shell and main vbox  */
  htd->shell = gimp_dialog_new (_("Histogram"), "histogram",
				tools_help_func,
				tool_info[HISTOGRAM].private_tip,
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Close"), histogram_tool_close_callback,
				htd, NULL, NULL, TRUE, TRUE,

				NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (htd->shell)->vbox), main_vbox);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  /*  The option menu for selecting channels  */
  htd->channel_menu = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), htd->channel_menu, FALSE, FALSE, 0);

  label = gtk_label_new (_("Information on Channel:"));
  gtk_box_pack_start (GTK_BOX (htd->channel_menu), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  option_menu = gimp_option_menu_new2
    (FALSE, histogram_tool_channel_callback,
     htd, (gpointer) htd->channel,

     _("Value"), (gpointer) HISTOGRAM_VALUE, NULL,
     _("Red"),   (gpointer) HISTOGRAM_RED, NULL,
     _("Green"), (gpointer) HISTOGRAM_GREEN, NULL,
     _("Blue"),  (gpointer) HISTOGRAM_BLUE, NULL,

     NULL);
  gtk_box_pack_start (GTK_BOX (htd->channel_menu), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  gtk_widget_show (htd->channel_menu);

  /*  The histogram tool histogram  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  htd->histogram = histogram_widget_new (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(htd->histogram));

  gtk_signal_connect (GTK_OBJECT (htd->histogram), "rangechanged",
		      GTK_SIGNAL_FUNC (histogram_tool_histogram_range),
		      htd);

  gtk_widget_show (GTK_WIDGET (htd->histogram));
  gtk_widget_show (frame);

  /*  The gradient below the histogram */
  grad_hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), grad_hbox, FALSE, FALSE, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (grad_hbox), frame, FALSE, FALSE, 0);

  htd->gradient = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (htd->gradient), 
		    HISTOGRAM_WIDTH, GRADIENT_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(htd->gradient));
  gtk_widget_show (htd->gradient);
  gtk_widget_show (frame);
  gtk_widget_show (grad_hbox);
  histogram_tool_gradient_draw (htd->gradient, HISTOGRAM_VALUE);

  gtk_widget_show (vbox);
  gtk_widget_show (hbox);

  /*  The table containing histogram information  */
  table = gtk_table_new (4, 4, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  the labels for histogram information  */
  for (i = 0; i < 7; i++)
    {
      y = (i % 4);
      x = (i / 4) * 2;

      label = gtk_label_new (gettext(histogram_info_names[i]));
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

  gtk_widget_show (table);

  gtk_widget_show (main_vbox);
  gtk_widget_show (htd->shell);

  return htd;
}

static void
histogram_tool_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) data;

  gimp_dialog_hide (htd->shell);
       
  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
histogram_tool_channel_callback (GtkWidget *widget,
				 gpointer   data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) data;

  gimp_menu_item_update (widget, &htd->channel);

  histogram_widget_channel (htd->histogram, htd->channel);
  histogram_tool_gradient_draw (htd->gradient, htd->channel);
}

static void
histogram_tool_gradient_draw (GtkWidget *gradient,
			      gint       channel)
{
  guchar buf[HISTOGRAM_WIDTH * 3];
  guchar r, g, b;
  gint i;

  r = g = b = 0;
  switch (channel) 
    {
    case HISTOGRAM_VALUE:
    case HISTOGRAM_ALPHA:  r = g = b = 1; 
      break;
    case HISTOGRAM_RED:    r = 1;         
      break;
    case HISTOGRAM_GREEN:  g = 1;         
      break;
    case HISTOGRAM_BLUE:   b = 1;        
      break;
    default:
      g_warning ("unknown channel type, can't happen\n");
      break;
    }
  
  for (i = 0; i < HISTOGRAM_WIDTH; i++)
    {
      buf[3*i+0] = i*r;
      buf[3*i+1] = i*g;
      buf[3*i+2] = i*b;
    }

  for (i = 0; i < GRADIENT_HEIGHT; i++)
    gtk_preview_draw_row (GTK_PREVIEW (gradient),
			  buf, 0, i, HISTOGRAM_WIDTH);
  
  gtk_widget_queue_draw (gradient);
}

 
