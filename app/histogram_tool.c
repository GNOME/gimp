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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogram.h"
#include "histogram_tool.h"
#include "image_map.h"
#include "interface.h"

#define TEXT_WIDTH 45
#define HISTOGRAM_WIDTH 256
#define HISTOGRAM_HEIGHT 150

typedef struct _HistogramTool HistogramTool;

struct _HistogramTool
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _HistogramToolDialog HistogramToolDialog;

struct _HistogramToolDialog
{
  GtkWidget   *shell;
  GtkWidget   *info_labels[7];
  GtkWidget   *channel_menu;
  Histogram   *histogram;

  double       mean;
  double       std_dev;
  double       median;
  double       pixels;
  double       count;
  double       percentile;

  GimpDrawable *drawable;
  ImageMap     image_map;
  int          channel;
  int          color;
};

/*  histogram_tool action functions  */

static void   histogram_tool_button_press   (Tool *, GdkEventButton *, gpointer);
static void   histogram_tool_button_release (Tool *, GdkEventButton *, gpointer);
static void   histogram_tool_motion         (Tool *, GdkEventMotion *, gpointer);
static void   histogram_tool_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   histogram_tool_control        (Tool *, int, gpointer);

static HistogramToolDialog *  histogram_tool_new_dialog       (gint);
static void                   histogram_tool_close_callback   (GtkWidget *, gpointer);
static gint                   histogram_tool_delete_callback  (GtkWidget *, GdkEvent *, gpointer);
static void                   histogram_tool_value_callback   (GtkWidget *, gpointer);
static void                   histogram_tool_red_callback     (GtkWidget *, gpointer);
static void                   histogram_tool_green_callback   (GtkWidget *, gpointer);
static void                   histogram_tool_blue_callback    (GtkWidget *, gpointer);

static void *histogram_tool_options = NULL;
static HistogramToolDialog *histogram_tool_dialog = NULL;

static void       histogram_tool_histogram_info  (PixelArea *, PixelArea *, HistogramValues, void *);
static void       histogram_tool_histogram_range (int, int, int, HistogramValues, void *);
static void       histogram_tool_dialog_update   (HistogramToolDialog *, int, int);

static Argument * histogram_invoker (Argument *args);


static char * histogram_info_names[7] =
{
  "Mean: ",
  "Std Dev: ",
  "Median: ",
  "Pixels: ",
  "Intensity: ",
  "Count: ",
  "Percentile: "
};

/*  histogram_tool machinery  */

/*
 *  TBD -WRB need to make work with float data
*/
static void
histogram_tool_histogram_info (PixelArea       *src_area,
			       PixelArea       *mask_area,
			       HistogramValues  values,
			       void            *user_data)
{
  HistogramToolDialog *htd;
  int                  w, h;
  guint8	      *src, *mask = NULL;

  Tag                  src_tag = pixelarea_tag (src_area);
  gint                 s_num_channels = tag_num_channels (src_tag);
  gint                 alpha = tag_alpha (src_tag);

  Tag                  mask_tag = pixelarea_tag (mask_area);
  gint                 m_num_channels = tag_num_channels (mask_tag);

  htd = (HistogramToolDialog *) user_data;

  h = pixelarea_height (src_area);

  src = (guint8*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guint8*)pixelarea_data (mask_area);

  switch( tag_precision( src_tag ) )
  {
  case PRECISION_U8:
  {
     guint8 *s, *m = NULL;
     gint    value, red, green, blue;
     gfloat  scale = 1./255.;

     while (h--)
       {
         w = pixelarea_width (src_area);
         s = src;

         if (mask_area)
	   m = mask;

         while (w--)
	   {
	     if (htd->color)
	       {
	         value = MAX (s[RED_PIX], s[GREEN_PIX]);
	         value = MAX (value, s[BLUE_PIX]);
	         red   = s[RED_PIX];
	         green = s[GREEN_PIX];
	         blue  = s[BLUE_PIX];

	         if (mask_area)
		   {
		     values[HISTOGRAM_VALUE][value] += (double) *m * scale;
		     values[HISTOGRAM_RED][red]     += (double) *m * scale;
		     values[HISTOGRAM_GREEN][green] += (double) *m * scale;
		     values[HISTOGRAM_BLUE][blue]   += (double) *m * scale;
		   }
	         else
		   {
		     values[HISTOGRAM_VALUE][value] += 1.0;
		     values[HISTOGRAM_RED][red]     += 1.0;
		     values[HISTOGRAM_GREEN][green] += 1.0;
		     values[HISTOGRAM_BLUE][blue]   += 1.0;
		   }
	       }
	     else
	       {
	         value = s[GRAY_PIX];
	         if (mask_area)
		   values[HISTOGRAM_VALUE][value] += (double) *m * scale;
	         else
		   values[HISTOGRAM_VALUE][value] += 1.0;
	       }

	     s += s_num_channels;

	     if (mask_area)
	       m += m_num_channels;
	   }

           src += pixelarea_rowstride (src_area);

           if (mask_area)
	     mask += pixelarea_rowstride (mask_area);
       }
       break;
  }
  case PRECISION_U16:
  {
     guint16 *s, *m = NULL;
     gint    value, red, green, blue;
     gfloat  scale = 1./65535.;

     while (h--)
       {
         w = pixelarea_width (src_area);
         s = (guint16*)src;

         if (mask_area)
	   m = (guint16*)mask;

         while (w--)
	   {
	     if (htd->color)
	       {
	         value = MAX (s[RED_PIX], s[GREEN_PIX]);
	         value = MAX (value, s[BLUE_PIX]);
	         red   = s[RED_PIX];
	         green = s[GREEN_PIX];
	         blue  = s[BLUE_PIX];

	         if (mask_area)
		   {
		     values[HISTOGRAM_VALUE][value] += (double) *m * scale;
		     values[HISTOGRAM_RED][red]     += (double) *m * scale;
		     values[HISTOGRAM_GREEN][green] += (double) *m * scale;
		     values[HISTOGRAM_BLUE][blue]   += (double) *m * scale;
		   }
	         else
		   {
		     values[HISTOGRAM_VALUE][value] += 1.0;
		     values[HISTOGRAM_RED][red]     += 1.0;
		     values[HISTOGRAM_GREEN][green] += 1.0;
		     values[HISTOGRAM_BLUE][blue]   += 1.0;
		   }
	       }
	     else
	       {
	         value = s[GRAY_PIX];
	         if (mask_area)
		   values[HISTOGRAM_VALUE][value] += (double) *m * scale;
	         else
		   values[HISTOGRAM_VALUE][value] += 1.0;
	       }

	     s += s_num_channels;

	     if (mask_area)
	       m += m_num_channels;
	   }

           src += pixelarea_rowstride (src_area);

           if (mask_area)
	     mask += pixelarea_rowstride (mask_area);
       }
       break;
  }
  case PRECISION_FLOAT:
  {
       g_warning( "histogram_float not implemented yet." );
       break;
  }
  }
}

/*
 *  TBD-WRB - need to make to work with F.P. image data (median calculation)
*/
static void
histogram_tool_histogram_range (int              start,
				int              end,
				int		 bins,
				HistogramValues  values,
				void            *user_data)
{
  HistogramToolDialog	*htd;
  double		 pixels;
  double 		 mean;
  double 		 std_dev;
  int 			 median;
  double 		 count;
  double 		 percentile;
  double 		 tmp;
  int 			 i;

  htd = (HistogramToolDialog *) user_data;

  count = 0.0;
  pixels = 0.0;
  for (i = 0; i < bins; i++)
    {
      pixels += values[htd->channel][i];

      if (i >= start && i <= end)
	count += values[htd->channel][i];
    }

  mean = 0.0;
  tmp = 0.0;
  median = -1;
  for (i = start; i <= end; i++)
    {
      mean += i * values[htd->channel][i];
      tmp += values[htd->channel][i];
      if (median == -1 && tmp > count / 2)
	median = i;
    }

  if (count)
    mean /= count;

  std_dev = 0.0;
  for (i = start; i <= end; i++)
    std_dev += values[htd->channel][i] * (i - mean) * (i - mean);

  if (count)
    std_dev = sqrt (std_dev / count);

  percentile = count / pixels;

  htd->mean = mean;
  htd->std_dev = std_dev;
  htd->median = median;
  htd->pixels = pixels;
  htd->count = count;
  htd->percentile = percentile;

  if (htd->shell)
    histogram_tool_dialog_update (htd, start, end);
}

static void
histogram_tool_dialog_update (HistogramToolDialog *htd,
			      int                  start,
			      int                  end)
{
  char text[12];

  /*  mean  */
  sprintf (text, "%3.1f", htd->mean);
  gtk_label_set (GTK_LABEL (htd->info_labels[0]), text);

  /*  std dev  */
  sprintf (text, "%3.1f", htd->std_dev);
  gtk_label_set (GTK_LABEL (htd->info_labels[1]), text);

  /*  median  */
  sprintf (text, "%3.1f", htd->median);
  gtk_label_set (GTK_LABEL (htd->info_labels[2]), text);

  /*  pixels  */
  sprintf (text, "%8.1f", htd->pixels);
  gtk_label_set (GTK_LABEL (htd->info_labels[3]), text);

  /*  intensity  */
  if (start == end)
    sprintf (text, "%d", start);
  else
    sprintf (text, "%d..%d", start, end);
  gtk_label_set (GTK_LABEL (htd->info_labels[4]), text);

  /*  count  */
  sprintf (text, "%8.1f", htd->count);
  gtk_label_set (GTK_LABEL (htd->info_labels[5]), text);

  /*  percentile  */
  sprintf (text, "%2.2f", htd->percentile * 100);
  gtk_label_set (GTK_LABEL (htd->info_labels[6]), text);
}

/*  histogram_tool action functions  */

static void
histogram_tool_button_press (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
histogram_tool_button_release (Tool           *tool,
			       GdkEventButton *bevent,
			       gpointer        gdisp_ptr)
{
}

static void
histogram_tool_motion (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
}

static void
histogram_tool_cursor_update (Tool           *tool,
			      GdkEventMotion *mevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
histogram_tool_control (Tool     *tool,
			int       action,
			gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (histogram_tool_dialog)
	histogram_tool_close_callback (NULL, (gpointer) histogram_tool_dialog);
      break;
    }
}

Tool *
tools_new_histogram_tool ()
{
  Tool * tool;
  HistogramTool * private;

  /*  The tool options  */
  if (!histogram_tool_options)
    histogram_tool_options = tools_register_no_options (HISTOGRAM, "Histogram Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (HistogramTool *) g_malloc (sizeof (HistogramTool));

  tool->type = HISTOGRAM;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = histogram_tool_button_press;
  tool->button_release_func = histogram_tool_button_release;
  tool->motion_func = histogram_tool_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = histogram_tool_cursor_update;
  tool->control_func = histogram_tool_control;
  tool->preserve = FALSE;

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

/*
 *  TBD - WRB -make work with float data
*/
void
histogram_tool_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  gint	    bins;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("Histogram does not operate on indexed drawables.");
      return;
    }

  switch( tag_precision( gimage_tag( gdisp->gimage ) ) )
  {
  case PRECISION_U8:
      bins = 256;
      break;

  case PRECISION_U16:
      bins = 65536;
      break;

  case PRECISION_FLOAT:
      g_warning( "histogram_float not implemented yet." );
      return;
  }


  /*  The histogram_tool dialog  */
  if (!histogram_tool_dialog)
    histogram_tool_dialog = histogram_tool_new_dialog ( bins );
  else if (!GTK_WIDGET_VISIBLE (histogram_tool_dialog->shell))
    gtk_widget_show (histogram_tool_dialog->shell);

  histogram_tool_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  histogram_tool_dialog->color = drawable_color (histogram_tool_dialog->drawable);

  /*  hide or show the channel menu based on image type  */
  if (histogram_tool_dialog->color)
    gtk_widget_show (histogram_tool_dialog->channel_menu);
  else
    gtk_widget_hide (histogram_tool_dialog->channel_menu);

  histogram_update (histogram_tool_dialog->histogram,
		    histogram_tool_dialog->drawable,
		    histogram_tool_histogram_info,
		    (void *) histogram_tool_dialog);
  histogram_range (histogram_tool_dialog->histogram, 0, 255);
}


/***************************/
/*  Histogram Tool dialog  */
/***************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", histogram_tool_close_callback, NULL, NULL }
};

static MenuItem color_option_items[] =
{
  { "Value", 0, 0, histogram_tool_value_callback, NULL, NULL, NULL },
  { "Red", 0, 0, histogram_tool_red_callback, NULL, NULL, NULL },
  { "Green", 0, 0, histogram_tool_green_callback, NULL, NULL, NULL },
  { "Blue", 0, 0, histogram_tool_blue_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};


static HistogramToolDialog *
histogram_tool_new_dialog ( gint bins )
{
  HistogramToolDialog *htd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *menu;
  int i;
  int x, y;

  htd = (HistogramToolDialog *) g_malloc (sizeof (HistogramToolDialog));
  htd->channel = HISTOGRAM_VALUE;

  for (i = 0; i < 4; i++)
    color_option_items [i].user_data = (gpointer) htd;

  /*  The shell and main vbox  */
  htd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (htd->shell), "histogram", "Gimp");
  gtk_window_set_title (GTK_WINDOW (htd->shell), "Histogram");

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (htd->shell), "delete_event",
		      (GtkSignalFunc) histogram_tool_delete_callback,
		      htd);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (htd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The option menu for selecting channels  */
  htd->channel_menu = gtk_hbox_new (TRUE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), htd->channel_menu, FALSE, FALSE, 0);

  label = gtk_label_new ("Information on Channel: ");
  gtk_box_pack_start (GTK_BOX (htd->channel_menu), label, FALSE, FALSE, 0);

  menu = build_menu (color_option_items, NULL);
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (htd->channel_menu), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (htd->channel_menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  /*  The histogram tool histogram  */
  hbox = gtk_hbox_new (TRUE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  htd->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT, bins,
				     histogram_tool_histogram_range, (void *) htd);
  gtk_container_add (GTK_CONTAINER (frame), htd->histogram->histogram_widget);
  gtk_widget_show (htd->histogram->histogram_widget);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  The table containing histogram information  */
  table = gtk_table_new (4, 4, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  the labels for histogram information  */
  for (i = 0; i < 7; i++)
    {
      y = (i % 4);
      x = (i / 4) * 2;

      label = gtk_label_new (histogram_info_names[i]);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, x, x + 1, y, y + 1,
			GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (label);

      htd->info_labels[i] = gtk_label_new ("0");
      gtk_misc_set_alignment (GTK_MISC (htd->info_labels[i]), 0.5, 0.5);
      gtk_table_attach (GTK_TABLE (table), htd->info_labels[i], x + 1, x + 2, y, y + 1,
			GTK_FILL, GTK_FILL, 2, 2);
      gtk_widget_show (htd->info_labels[i]);
    }

  /*  The action area  */
  action_items[0].user_data = htd;
  build_action_area (GTK_DIALOG (htd->shell), action_items, 1, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (htd->shell);

  return htd;
}

static void
histogram_tool_close_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (htd->shell))
    gtk_widget_hide (htd->shell);
}

static gint
histogram_tool_delete_callback (GtkWidget *widget,
				GdkEvent *event,
				gpointer client_data)
{
  histogram_tool_close_callback (widget, client_data);

  return TRUE;
}

static void
histogram_tool_value_callback (GtkWidget *w,
			       gpointer   client_data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) client_data;

  if (htd->channel != HISTOGRAM_VALUE)
    {
      htd->channel = HISTOGRAM_VALUE;
      histogram_channel (htd->histogram, htd->channel);
    }
}

static void
histogram_tool_red_callback (GtkWidget *w,
			     gpointer   client_data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) client_data;

  if (htd->channel != HISTOGRAM_RED)
    {
      htd->channel = HISTOGRAM_RED;
      histogram_channel (htd->histogram, htd->channel);
    }
}

static void
histogram_tool_green_callback (GtkWidget *w,
			       gpointer   client_data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) client_data;

  if (htd->channel != HISTOGRAM_GREEN)
    {
      htd->channel = HISTOGRAM_GREEN;
      histogram_channel (htd->histogram, htd->channel);
    }
}

static void
histogram_tool_blue_callback (GtkWidget *w,
			      gpointer   client_data)
{
  HistogramToolDialog *htd;

  htd = (HistogramToolDialog *) client_data;

  if (htd->channel != HISTOGRAM_BLUE)
    {
      htd->channel = HISTOGRAM_BLUE;
      histogram_channel (htd->histogram, htd->channel);
    }
}

/*  The histogram procedure definition  */
ProcArg histogram_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "channel",
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), GRAY (0) }"
  },
  { PDB_INT32,
    "start_range",
    "start of the intensity measurement range"
  },
  { PDB_INT32,
    "end_range",
    "end of the intensity measurement range"
  }
};

ProcArg histogram_out_args[] =
{
  { PDB_FLOAT,
    "mean",
    "mean intensity value"
  },
  { PDB_FLOAT,
    "std_dev",
    "standard deviation of intensity values"
  },
  { PDB_FLOAT,
    "median",
    "median intensity value"
  },
  { PDB_FLOAT,
    "pixels",
    "alpha-weighted pixel count for entire image"
  },
  { PDB_FLOAT,
    "count",
    "alpha-weighted pixel count for range"
  },
  { PDB_FLOAT,
    "percentile",
    "percentile that range falls under"
  }
};

ProcRecord histogram_proc =
{
  "gimp_histogram",
  "Returns information on the intensity histogram for the specified drawable",
  "This tool makes it possible to gather information about the intensity histogram of a drawable.  A channel to examine is first specified.  This can be either value, red, green, or blue, depending on whether the drawable is of type color or grayscale.  The drawable may not be indexed.  Second, a range of intensities are specified.  The gimp_histogram function returns statistics based on the pixels in the drawable that fall under this range of values.  Mean, standard deviation, median, number of pixels, and percentile are all returned.  Additionally, the total count of pixels in the image is returned.  Counts of pixels are weighted by any associated alpha values and by the current selection mask.  That is, pixels that lie outside an active selection mask will not be counted.  Similarly, pixels with transparent alpha values will not be counted.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  histogram_args,

  /*  Output arguments  */
  6,
  histogram_out_args,

  /*  Exec method  */
  { { histogram_invoker } },
};


/*
 *  TBD - WRB -make work with float data
*/
static Argument *
histogram_invoker (Argument *args)
{
  Argument *return_args;
  PixelArea src_area, mask_area;
  int success = TRUE;
  HistogramToolDialog htd;
  GImage *gimage;
  Channel *mask;
  GimpDrawable *drawable;
  int channel;
  int low_range;
  int high_range;
  int int_value;
  int no_mask;
  int max, bins;
  int x1, y1, x2, y2;
  int off_x, off_y;
  void *pr;

  drawable = NULL;
  low_range   = 0;
  high_range  = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  channel  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    {
	      if (int_value != 0)
		success = FALSE;
	    }
	  else if (drawable_color (drawable))
	    {
	      if (int_value < 0 || int_value > 3)
		success = FALSE;
	    }
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  low range  */

  if( success )
  {
      switch( tag_precision( gimage_tag( gimage ) ) )
      {
      case PRECISION_U8:
	  bins = 256;
	  max = bins-1;
	  break;

      case PRECISION_U16:
	  bins = 65536;
	  max = bins-1;
	  break;

      case PRECISION_FLOAT:
	  g_warning( "histogram_float not implemented yet." );
	  success = FALSE;
      }
  } 

  if (success)
    {
      int_value = args[3].value.pdb_int;
	  /*printf( "low range int: %d\n", int_value );*/
      if (int_value >= 0 && int_value <= max)
	low_range = int_value;
      else
	success = FALSE;
    }
  /*  high range  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
	  /*printf( "hight range int: %d\n", int_value );*/
      if (int_value >= 0 && int_value <= max)
	high_range = int_value;
      else
	success = FALSE;
    }

  /*  arrange to calculate the histogram  */
  if (success)
    {
      htd.shell = NULL;
      htd.channel = channel;
      htd.drawable = drawable;
      htd.color = drawable_color (drawable);
      htd.histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT, bins,
					histogram_tool_histogram_range, (void *) &htd);

      /*  The information collection should occur only within selection bounds  */
      no_mask = (drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2) == FALSE);
      drawable_offsets (drawable, &off_x, &off_y);

      /*  Configure the src from the drawable data  */
      pixelarea_init (&src_area, drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);

      /*  Configure the mask from the gimage's selection mask  */
      mask = gimage_get_mask (gimage);
      pixelarea_init (&mask_area, drawable_data (GIMP_DRAWABLE(mask)),
			 x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);

      /*  Apply the image transformation to the pixels  */
      if (no_mask)
	for (pr = pixelarea_register (1, &src_area); pr != NULL; pr = pixelarea_process (pr))
	  histogram_tool_histogram_info (&src_area, NULL, *(histogram_values (htd.histogram)), &htd);
      else
	for (pr = pixelarea_register (2, &src_area, &mask_area); pr != NULL; pr = pixelarea_process (pr))
	  histogram_tool_histogram_info (&src_area, &mask_area, *(histogram_values (htd.histogram)), &htd);

      /*  calculate the statistics  */
      histogram_tool_histogram_range (low_range, high_range, bins,
				      *(histogram_values (htd.histogram)), &htd);

      return_args = procedural_db_return_args (&histogram_proc, success);

      return_args[1].value.pdb_float = htd.mean;
      return_args[2].value.pdb_float = htd.std_dev;
      return_args[3].value.pdb_float = htd.median;
      return_args[4].value.pdb_float = htd.pixels;
      return_args[5].value.pdb_float = htd.count;
      return_args[6].value.pdb_float = htd.percentile;
    }
  else
    return_args = procedural_db_return_args (&histogram_proc, success);

  return return_args;
}
