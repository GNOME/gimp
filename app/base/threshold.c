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
#include "histogram.h"
#include "image_map.h"
#include "interface.h"
#include "threshold.h"
#include "pixelarea.h"

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
  Histogram   *histogram;

  GimpDrawable *drawable;
  ImageMap     image_map;
  int          color;
  int          low_threshold;
  int          high_threshold;
  int		   bins;

  gint         preview;
};

/*  threshold action functions  */

static void   threshold_button_press   (Tool *, GdkEventButton *, gpointer);
static void   threshold_button_release (Tool *, GdkEventButton *, gpointer);
static void   threshold_motion         (Tool *, GdkEventMotion *, gpointer);
static void   threshold_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   threshold_control        (Tool *, int, gpointer);


static ThresholdDialog *  threshold_new_dialog                 (gint);
static void               threshold_preview                    (ThresholdDialog *);
static void               threshold_ok_callback                (GtkWidget *, gpointer);
static void               threshold_cancel_callback            (GtkWidget *, gpointer);
static gint               threshold_delete_callback            (GtkWidget *, GdkEvent *, gpointer);
static void               threshold_preview_update             (GtkWidget *, gpointer);
static void               threshold_low_threshold_text_update  (GtkWidget *, gpointer);
static void               threshold_high_threshold_text_update (GtkWidget *, gpointer);

static void *threshold_options = NULL;
static ThresholdDialog *threshold_dialog = NULL;

static void       threshold (PixelArea *, PixelArea *, void *);
static void       threshold_histogram_info (PixelArea *, PixelArea *, HistogramValues, void *);
static void       threshold_histogram_range (int, int, int, HistogramValues, void *);
static Argument * threshold_invoker (Argument *);

/*  threshold machinery  */

/*
 *  TBD -WRB need to make work with float data
*/
static void
threshold (PixelArea *src_area,
	   PixelArea *dest_area,
	   void      *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  gint             w, h, b;
  guint8 		  *src, *dest;

  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);

  Tag              dest_tag = pixelarea_tag (dest_area);
  gint             d_num_channels = tag_num_channels (dest_tag);

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  if( tag_precision( src_tag ) != tag_precision( dest_tag ) )
  {
    g_warning( "threshold: src & dest not same bit depth." );
    return;
  }

  src = (guint8*)pixelarea_data (src_area);
  dest = (guint8*)pixelarea_data (dest_area);

  switch( tag_precision( dest_tag ) )
  {
  case PRECISION_U8:
  {
     guint8 *s, *d;
     gint    value;

     while (h--)
       {
         w = pixelarea_width (src_area);
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

	     s += s_num_channels;
	     d += d_num_channels;
	   }

           src += pixelarea_rowstride (src_area);
           dest += pixelarea_rowstride (dest_area);
       }
       break;
  }
  case PRECISION_U16:
  {
     guint16 *s, *d;
     gint     value;

     while (h--)
       {
         w = pixelarea_width (src_area);
         s = (guint16*)src;
         d = (guint16*)dest;
         while (w--)
	   {
	     if (td->color)
	       {
	         value = MAX (s[RED_PIX], s[GREEN_PIX]);
	         value = MAX (value, s[BLUE_PIX]);

	         value = (value >= td->low_threshold && value <= td->high_threshold ) ? 65535 : 0;
	       }
	     else
	       value = (s[GRAY_PIX] >= td->low_threshold && s[GRAY_PIX] <= td->high_threshold) ? 65535 : 0;

	     for (b = 0; b < alpha; b++)
	       d[b] = value;

	     if (has_alpha)
	       d[alpha] = s[alpha];

	     s += s_num_channels;
	     d += d_num_channels;
	   }

         src += pixelarea_rowstride (src_area);
         dest += pixelarea_rowstride (dest_area);
       }
       break;
  }
  case PRECISION_FLOAT:
  {
       g_warning( "threshold_float not implemented yet." );
       break;
  }
  }
}

/*
 *  TBD -WRB need to make work with float data
*/
static void
threshold_histogram_info (PixelArea       *src_area,
			  PixelArea       *mask_area,
			  HistogramValues  values,
			  void            *user_data)
{
  ThresholdDialog *td;
  gint             has_alpha;
  int              w, h;
  int              value;
  guint8	  *src, *mask = NULL;

  Tag              src_tag = pixelarea_tag (src_area);
  gint             s_num_channels = tag_num_channels (src_tag);
  gint             alpha = tag_alpha (src_tag);

  Tag              mask_tag = pixelarea_tag (mask_area);
  gint             m_num_channels = tag_num_channels (mask_tag);

  if( mask_area && (tag_precision( src_tag ) != tag_precision( mask_tag )) )
  {
    g_warning( "threshold: src & mask not same bit depth." );
    return;
  }

  td = (ThresholdDialog *) user_data;

  h = pixelarea_height (src_area);

  has_alpha = (alpha == ALPHA_YES) ? TRUE: FALSE;
  alpha = has_alpha ? s_num_channels - 1 : s_num_channels;

  src = (guint8*)pixelarea_data (src_area);

  if (mask_area)
    mask = (guint8*)pixelarea_data (mask_area);

  switch( tag_precision( src_tag ) )
  {
  case PRECISION_U8:
  {
     guint8 *s, *m = NULL;
     gint    value;

     while (h--)
       {
         w = pixelarea_width (src_area);
         s = src;

         if (mask_area)
	   m = mask;

         while (w--)
	   {
	     if (td->color)
	       {
	         value = MAX (s[RED_PIX], s[GREEN_PIX]);
	         value = MAX (value, s[BLUE_PIX]);
	       }
	     else
	       value = s[GRAY_PIX];

	     if (mask_area)
	       values[HISTOGRAM_VALUE][value] += (double) *m / 255.0;
	     else
	       values[HISTOGRAM_VALUE][value] += 1.0;

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
     gint    value;

     while (h--)
       {
         w = pixelarea_width (src_area);
         s = (guint16*)src;

         if (mask_area)
	   m = (guint16*)mask;

         while (w--)
	   {
	     if (td->color)
	       {
	         value = MAX (s[RED_PIX], s[GREEN_PIX]);
	         value = MAX (value, s[BLUE_PIX]);
	       }
	     else
	       value = s[GRAY_PIX];

	     if (mask_area)
	       values[HISTOGRAM_VALUE][value] += (double) *m / 65535.0;
	     else
	       values[HISTOGRAM_VALUE][value] += 1.0;

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
       g_warning( "threshold_float not implemented yet." );
       break;
  }
  }
}

static void
threshold_histogram_range (int              start,
			   int              end,
			   int              bins,
			   HistogramValues  values,
			   void            *user_data)
{
  ThresholdDialog *td;
  char text[12];

  td = (ThresholdDialog *) user_data;

  td->low_threshold = start;
  td->high_threshold = end;
  td->bins = bins;
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
  Tool		* tool;
  Threshold	* private;

  /*  The tool options  */
  if (!threshold_options)
    threshold_options = tools_register_no_options (THRESHOLD, "Threshold Options");

#if 0
  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);
#endif

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
threshold_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  gint	    bins;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("Threshold does not operate on indexed drawables.");
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

  /*  The threshold dialog  */
  if (!threshold_dialog)
    threshold_dialog = threshold_new_dialog ( bins );
  else
    if (!GTK_WIDGET_VISIBLE (threshold_dialog->shell))
      gtk_widget_show (threshold_dialog->shell);

  threshold_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  threshold_dialog->color = drawable_color (threshold_dialog->drawable);
  threshold_dialog->image_map = image_map_create (gdisp_ptr, threshold_dialog->drawable);
  histogram_update (threshold_dialog->histogram,
		    threshold_dialog->drawable,
		    threshold_histogram_info,
		    (void *) threshold_dialog);
  histogram_range (threshold_dialog->histogram,
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
  { "OK", threshold_ok_callback, NULL, NULL },
  { "Cancel", threshold_cancel_callback, NULL, NULL }
};

/*
 *  TBD - WRB -make work with float data
*/
static ThresholdDialog *
threshold_new_dialog (gint bins)
{
  ThresholdDialog *td;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;

  td = g_malloc (sizeof (ThresholdDialog));
  td->preview = TRUE;
  td->high_threshold = bins-1;
  td->low_threshold = td->high_threshold/2;
  td->bins = bins;

  /*  The shell and main vbox  */
  td->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (td->shell), "threshold", "Gimp");
  gtk_window_set_title (GTK_WINDOW (td->shell), "Threshold");

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (td->shell), "delete_event",
		      GTK_SIGNAL_FUNC (threshold_delete_callback),
		      td);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (td->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for threshold text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Threshold Range: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low threshold text  */
  td->low_threshold_text = gtk_entry_new ();
  if( bins == 256 )
  {
    gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "127");
    gtk_widget_set_usize (td->low_threshold_text, TEXT_WIDTH, 25);
  }
  else
  {
    gtk_entry_set_text (GTK_ENTRY (td->low_threshold_text), "32767");
    gtk_widget_set_usize (td->low_threshold_text, TEXT_WIDTH*1.5, 25);
  }
  gtk_box_pack_start (GTK_BOX (hbox), td->low_threshold_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (td->low_threshold_text), "changed",
		      (GtkSignalFunc) threshold_low_threshold_text_update,
		      td);
  gtk_widget_show (td->low_threshold_text);

  /* high threshold text  */
  td->high_threshold_text = gtk_entry_new ();
  if( bins == 256 )
  {
    gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "255");
    gtk_widget_set_usize (td->high_threshold_text, TEXT_WIDTH, 25);
  }
  else
  {
    gtk_entry_set_text (GTK_ENTRY (td->high_threshold_text), "65535");
    gtk_widget_set_usize (td->high_threshold_text, TEXT_WIDTH*1.5, 25);
  }
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

  td->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT, bins,
				    threshold_histogram_range, (void *) td);
  gtk_container_add (GTK_CONTAINER (frame), td->histogram->histogram_widget);
  gtk_widget_show (td->histogram->histogram_widget);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), td->preview);
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
  histogram_range (td->histogram, td->low_threshold, td->high_threshold);
  return td;
}

static void
threshold_preview (ThresholdDialog *td)
{
  if (!td->image_map)
    g_message ("threshold_preview(): No image map");
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
      histogram_range (td->histogram,
		       td->low_threshold,
		       td->high_threshold);
      if (td->preview)
	threshold_preview (td);
    }
}

/*
 *  TBD - WRB -make work with float data
*/
static void
threshold_high_threshold_text_update (GtkWidget *w,
				      gpointer   data)
{
  ThresholdDialog *td;
  char *str;
  int value;

  td = (ThresholdDialog *) data;
  str = gtk_entry_get_text (GTK_ENTRY (w));
  value = BOUNDS (((int) atof (str)), td->low_threshold, td->bins-1);

  if (value != td->high_threshold)
    {
      td->high_threshold = value;
      histogram_range (td->histogram,
		       td->low_threshold,
		       td->high_threshold);
      if (td->preview)
	threshold_preview (td);
    }
}


/*  The threshold procedure definition  */
ProcArg threshold_args[] =
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
    "low_threshold",
    "the low threshold value: (0 <= low_threshold <= maxbright)"
  },
  { PDB_INT32,
    "high_threshold",
    "the high threshold value: (0 <= high_threshold <= maxbright)"
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
  4,
  threshold_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { threshold_invoker } },
};


/*
 *  TBD - WRB -make work with float data
*/
static Argument *
threshold_invoker (args)
     Argument *args;
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  ThresholdDialog td;
  GImage *gimage;
  GimpDrawable *drawable;
  int low_threshold;
  int high_threshold;
  int int_value;
  int max, bins;
  int x1, y1, x2, y2;
  void *pr;

  drawable    = NULL;
  low_threshold  = 0;
  high_threshold = 0;

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
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

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

  /*  low threhsold  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 0 && int_value <= max)
	low_threshold = int_value;
      else
	success = FALSE;
    }
  /*  high threhsold  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value <= max)
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

      pixelarea_init (&src_area, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); pr != NULL; pr = pixelarea_process (pr))
	threshold (&src_area, &dest_area, (void *) &td);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }
  return procedural_db_return_args (&threshold_proc, success);
}
