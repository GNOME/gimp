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
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogramwidget.h"
#include "image_map.h"
#include "interface.h"
#include "threshold.h"

#include "libgimp/gimpintl.h"

#define TEXT_WIDTH 45
#define HISTOGRAM_WIDTH 256
#define HISTOGRAM_HEIGHT 150

typedef struct _Threshold Threshold;

struct _Threshold
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _ThresholdDialog ThresholdDialog;

struct _ThresholdDialog
{
  GtkWidget   *shell;
  GtkWidget   *low_threshold_text;
  GtkWidget   *high_threshold_text;
  HistogramWidget *histogram;
  GimpHistogram   *hist;

  GimpDrawable *drawable;
  ImageMap     image_map;
  int          color;
  int          low_threshold;
  int          high_threshold;

  gint         preview;
};

/*  threshold action functions  */

static void   threshold_button_press   (Tool *, GdkEventButton *, gpointer);
static void   threshold_button_release (Tool *, GdkEventButton *, gpointer);
static void   threshold_motion         (Tool *, GdkEventMotion *, gpointer);
static void   threshold_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   threshold_control        (Tool *, int, gpointer);


static ThresholdDialog *  threshold_new_dialog                 (void);
static void               threshold_preview                    (ThresholdDialog *);
static void               threshold_ok_callback                (GtkWidget *, gpointer);
static void               threshold_cancel_callback            (GtkWidget *, gpointer);
static gint               threshold_delete_callback            (GtkWidget *, GdkEvent *, gpointer);
static void               threshold_preview_update             (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update  (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update (GtkWidget *, gpointer);

static void *threshold_options = NULL;
static ThresholdDialog *threshold_dialog = NULL;

static void       threshold (PixelRegion *, PixelRegion *, void *);
static void       threshold_histogram_range (HistogramWidget *, int, int,
					     void*);
static Argument * threshold_invoker (Argument *);

/*  threshold machinery  */

static void
threshold_2 (void        *user_data,
	     PixelRegion *srcPR,
	     PixelRegion *destPR)
{
  /* this function just re-orders the arguments so we can use 
     pixel_regions_process_paralell */
  threshold(srcPR, destPR, user_data);
}

static void
threshold (PixelRegion *srcPR,
	   PixelRegion *destPR,
	   void        *user_data)
{
  ThresholdDialog *td;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int has_alpha, alpha;
  int w, h, b;
  int value;

  td = (ThresholdDialog *) user_data;

  h = srcPR->h;
  src = srcPR->data;
  dest = destPR->data;
  has_alpha = (srcPR->bytes == 2 || srcPR->bytes == 4);
  alpha = has_alpha ? srcPR->bytes - 1 : srcPR->bytes;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;
      while (w--)
	{
	  if (td->color)
	    {
	      value = MAX (s[RED_PIX], s[GREEN_PIX]);
	      value = MAX (value, s[BLUE_PIX]);

	      value = (value >= td->low_threshold && value <= td->high_threshold ) ? 255 : 0;
	    }
	  else
	    value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 255 : 0;

	  for (b = 0; b < alpha; b++)
	    d[b] = value;

	  if (has_alpha)
	    d[alpha] = s[alpha];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
threshold_histogram_range (HistogramWidget *w,
			   int              start,
			   int              end,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[12];

  td = (ThresholdDialog *) user_data;

  td->low_threshold = start;
  td->high_threshold = end;
  sprintf (text, "%d", start);
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), text);
  sprintf (text, "%d", end);
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), text);

  if (td->preview)
    threshold_preview (td);
}

/*  threshold action functions  */

static void
threshold_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
threshold_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
}

static void
threshold_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
}

static void
threshold_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
threshold_control (Tool     *tool,
		   int       action,
		   gpointer  gdisp_ptr)
{
  Threshold * thresh;

  thresh = (Threshold *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (threshold_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (threshold_dialog->image_map);
	  active_tool->preserve = FALSE;
	  threshold_dialog->image_map = NULL;
	  threshold_cancel_callback (NULL, (gpointer) threshold_dialog);
	}
      break;
    }
}

Tool *
tools_new_threshold ()
{
  Tool * tool;
  Threshold * private;

  /*  The tool options  */
  if (!threshold_options)
    threshold_options = tools_register_no_options (THRESHOLD, _("Threshold Options"));

  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Threshold *) g_malloc (sizeof (Threshold));

  tool->type = THRESHOLD;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = threshold_button_press;
  tool->button_release_func = threshold_button_release;
  tool->motion_func = threshold_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = threshold_cursor_update;
  tool->control_func = threshold_control;
  tool->preserve = FALSE;

  return tool;
}

void
tools_free_threshold (Tool *tool)
{
  Threshold * thresh;

  thresh = (Threshold *) tool->private;

  /*  Close the color select dialog  */
  if (threshold_dialog)
    threshold_cancel_callback (NULL, (gpointer) threshold_dialog);

  g_free (thresh);
}

void
threshold_initialize (GDisplay *gdisp)
{
  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Threshold does not operate on indexed drawables."));
      return;
    }

  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);

  threshold_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  threshold_dialog->color = drawable_color (threshold_dialog->drawable);
  threshold_dialog->image_map = image_map_create (gdisp, threshold_dialog->drawable);

  gimp_histogram_calculate_drawable(threshold_dialog->hist,
				    threshold_dialog->drawable);
  
  histogram_widget_update (threshold_dialog->histogram,
			   threshold_dialog->hist);
  histogram_widget_range (threshold_dialog->histogram,
			  threshold_dialog->low_threshold,
			  threshold_dialog->high_threshold);
  if (threshold_dialog->preview)
    threshold_preview (threshold_dialog);
}


/****************************/
/*  Select by Color dialog  */
/****************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { N_("OK"), threshold_ok_callback, NULL, NULL },
  { N_("Cancel"), threshold_cancel_callback, NULL, NULL }
};

static ThresholdDialog *
threshold_new_dialog ()
{
  ThresholdDialog *td;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;

  td = g_malloc (sizeof (ThresholdDialog));
  td->preview = TRUE;
  td->low_threshold = 127;
  td->high_threshold = 255;
  td->hist = gimp_histogram_new();

  /*  The shell and main vbox  */
  td->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (td->shell), "threshold", "Gimp");
  gtk_window_set_title (GTK_WINDOW (td->shell), _("Threshold"));

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (td->shell), "delete_event",
		      GTK_SIGNAL_FUNC (threshold_delete_callback),
		      td);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (td->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for threshold text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold Range: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low threshold text  */
  td->low_threshold_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "127");
  gtk_widget_set_usize (td->low_threshold_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), td->low_threshold_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (td->low_threshold_text), "changed",
		      (GtkSignalFunc) threshold_low_threshold_text_update,
		      td);
  gtk_widget_show (td->low_threshold_text);

  /* high threshold text  */
  td->high_threshold_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "255");
  gtk_widget_set_usize (td->high_threshold_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), td->high_threshold_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (td->high_threshold_text), "changed",
		      (GtkSignalFunc) threshold_high_threshold_text_update,
		      td);
  gtk_widget_show (td->high_threshold_text);
  gtk_widget_show (hbox);

  /*  The threshold histogram  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  td->histogram = histogram_widget_new (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);

  gtk_signal_connect (GTK_OBJECT (td->histogram), "rangechanged",
		      (GtkSignalFunc) threshold_histogram_range,
		      (void*)td);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(td->histogram));
  gtk_widget_show (GTK_WIDGET(td->histogram));
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), td->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) threshold_preview_update,
		      td);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = td;
  action_items[1].user_data = td;
  build_action_area (GTK_DIALOG (td->shell), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (td->shell);

  /* This code is so far removed from the histogram creation because the
     function histogram_range requires a non-NULL drawable, and that
     doesn't happen until after the top-level dialog is shown. */
  histogram_widget_range (td->histogram, td->low_threshold, td->high_threshold);
  return td;
}

static void
threshold_preview (ThresholdDialog *td)
{
  if (!td->image_map)
    g_message (_("threshold_preview(): No image map"));
  image_map_apply (td->image_map, threshold, (void *) td);
}

static void
threshold_ok_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (td->shell))
    gtk_widget_hide (td->shell);

  active_tool->preserve = TRUE;

  if (!td->preview)
    image_map_apply (td->image_map, threshold, (void *) td);
  if (td->image_map)
    image_map_commit (td->image_map);

  active_tool->preserve = FALSE;

  td->image_map = NULL;
}

static gint
threshold_delete_callback (GtkWidget *w,
			   GdkEvent *e,
			   gpointer client_data) 
{
  threshold_cancel_callback (w, client_data);

  return TRUE;
}

static void
threshold_cancel_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (td->shell))
    gtk_widget_hide (td->shell);

  if (td->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (td->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  td->image_map = NULL;
}

static void
threshold_preview_update (GtkWidget *w,
			  gpointer   data)
{
  ThresholdDialog *td;

  td = (ThresholdDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      td->preview = TRUE;
      threshold_preview (td);
    }
  else
    td->preview = FALSE;
}

static void
threshold_low_threshold_text_update (GtkWidget *w,
				     gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), 0, td->high_threshold);

  if (value != td->low_threshold)
    {
      td->low_threshold = value;
      histogram_widget_range (td->histogram,
			      td->low_threshold,
			      td->high_threshold);
      if (td->preview)
	threshold_preview (td);
    }
}

static void
threshold_high_threshold_text_update (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), td->low_threshold, 255);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      histogram_widget_range (td->histogram,
			      td->low_threshold,
			      td->high_threshold);
      if (td->preview)
	threshold_preview (td);
    }
}


/*  The threshold procedure definition  */
ProcArg threshold_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "low_threshold",
    "the low threshold value: (0 <= low_threshold <= 255)"
  },
  { PDB_INT32,
    "high_threshold",
    "the high threshold value: (0 <= high_threshold <= 255)"
  }
};

ProcRecord threshold_proc =
{
  "gimp_threshold",
  "Threshold the specified drawable",
  "This procedures generates a threshold map of the specified drawable.  All pixels between the values of 'low_threshold' and 'high_threshold' are replaced with white, and all other pixels with black.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  threshold_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { threshold_invoker } },
};


static Argument *
threshold_invoker (args)
     Argument *args;
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  ThresholdDialog td;
  GImage *gimage;
  GimpDrawable *drawable;
  int low_threshold;
  int high_threshold;
  int int_value;
  int x1, y1, x2, y2;
  void *pr;

  drawable    = NULL;
  low_threshold  = 0;
  high_threshold = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);
  
  /*  low threhsold  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_threshold = int_value;
      else
	success = FALSE;
    }
  /*  high threhsold  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_threshold = int_value;
      else
	success = FALSE;
    }
  if (success)
    success =  (low_threshold < high_threshold);

  /*  arrange to modify the levels  */
  if (success)
    {
      td.color = drawable_color (drawable);
      td.low_threshold = low_threshold;
      td.high_threshold = high_threshold;

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      pixel_regions_process_parallel((p_func)threshold_2, (void*) &td,
				     2, &srcPR, &destPR);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&threshold_proc, success);
}
