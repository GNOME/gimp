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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "canvas.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "curves.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogram.h"
#include "image_map.h"
#include "interface.h"
#include "pixelarea.h"
#include "pixelrow.h"

#define ROUND(x)  ((int) ((x) + 0.5))

#define GRAPH              0x1
#define XRANGE_TOP         0x2
#define XRANGE_BOTTOM      0x4
#define YRANGE             0x8
#define DRAW               0x10
#define ALL                0xFF

#define GRAPH_WIDTH      256
#define GRAPH_HEIGHT     256
#define XRANGE_WIDTH     256
#define XRANGE_HEIGHT    16
#define YRANGE_WIDTH     16
#define YRANGE_HEIGHT    256
#define RADIUS           3
#define MIN_DISTANCE     8

#define SMOOTH       0
#define GFREE        1

#define RANGE_MASK  GDK_EXPOSURE_MASK | \
                    GDK_ENTER_NOTIFY_MASK

#define GRAPH_MASK  GDK_EXPOSURE_MASK | \
		    GDK_POINTER_MOTION_MASK | \
		    GDK_POINTER_MOTION_HINT_MASK | \
                    GDK_ENTER_NOTIFY_MASK | \
		    GDK_BUTTON_PRESS_MASK | \
		    GDK_BUTTON_RELEASE_MASK | \
		    GDK_BUTTON1_MOTION_MASK

typedef struct _Curves Curves;

struct _Curves
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _CurvesDialog CurvesDialog;

struct _CurvesDialog
{
  GtkWidget *    shell;
  GtkWidget *    channel_menu;
  GtkWidget *    xrange;
  GtkWidget *    yrange;
  GtkWidget *    graph;
  GdkPixmap *    pixmap;

  GimpDrawable * drawable;
  ImageMap16     image_map;
  int            color;
  int            channel;
  gint           preview;

  int            grab_point;
  int            last;
  int            leftmost;
  int            rightmost;
  int            curve_type;

  /* points and curves for ui 0-255 based */
  int            points[5][17][2];
  unsigned char  curve[5][256];

};

/* curves for image data */
static PixelRow curve[5];

typedef double CRMatrix[4][4];

/*  curves action functions  */

static void   curves_button_press   (Tool *, GdkEventButton *, gpointer);
static void   curves_button_release (Tool *, GdkEventButton *, gpointer);
static void   curves_motion         (Tool *, GdkEventMotion *, gpointer);
static void   curves_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   curves_control        (Tool *, int, gpointer);

static CurvesDialog *  curves_new_dialog         (void);
static void            curves_update             (CurvesDialog *, int);
static void            curves_calculate_ui_curve (CurvesDialog *);
static void            curves_preview            (CurvesDialog *);
static void            curves_value_callback     (GtkWidget *, gpointer);
static void            curves_red_callback       (GtkWidget *, gpointer);
static void            curves_green_callback     (GtkWidget *, gpointer);
static void            curves_blue_callback      (GtkWidget *, gpointer);
static void            curves_alpha_callback     (GtkWidget *, gpointer);
static void            curves_smooth_callback    (GtkWidget *, gpointer);
static void            curves_free_callback      (GtkWidget *, gpointer);
static void            curves_reset_callback     (GtkWidget *, gpointer);
static void            curves_ok_callback        (GtkWidget *, gpointer);
static void            curves_cancel_callback    (GtkWidget *, gpointer);
static gint            curves_delete_callback    (GtkWidget *, GdkEvent *, gpointer);
static void            curves_preview_update     (GtkWidget *, gpointer);
static gint            curves_xrange_events      (GtkWidget *, GdkEvent *, CurvesDialog *);
static gint            curves_yrange_events      (GtkWidget *, GdkEvent *, CurvesDialog *);
static gint            curves_graph_events       (GtkWidget *, GdkEvent *, CurvesDialog *);
static void            curves_calculate_curve    (PixelRow *, gdouble points[17][2]);
static void            curves_CR_compose         (CRMatrix, CRMatrix, CRMatrix);
static void            curves_free_curves        (void *);
static void 	       curves_plot_boundary_pts_ui (PixelRow *, gdouble points[17][2], gint pts[17], gint);
static void 	       curves_plot_curve_ui      (PixelRow *, gdouble *, gdouble *, gdouble *, gdouble *);
static void            curves_calculate_image_data_curves (CurvesDialog *cd);

static void curves_funcs (Tag dest_tag);

typedef void (*CurvesFunc)(PixelArea *, PixelArea *, void *);
typedef void (*CurvesAllocCurvesFunc)(void *);
typedef void (*CurvesInitCurveFunc)(gint);
typedef void (*CurvesPlotBoundaryPtsFunc)(PixelRow *, 
			gdouble points[17][2], gint pts[17], gint);	
typedef void (*CurvesPlotCurveFunc) (PixelRow *, 
			gdouble *, gdouble *, gdouble *, gdouble *);
typedef gdouble (*CurvesConvertUiValueFunc) (gint);
typedef void (*CurvesCalculateFreeCurveFunc) (guchar *, PixelRow *);

static CurvesFunc curves;
static CurvesAllocCurvesFunc curves_alloc_curves;
static CurvesInitCurveFunc curves_init_curve;
static CurvesPlotBoundaryPtsFunc curves_plot_boundary_pts;
static CurvesPlotCurveFunc curves_plot_curve;
static CurvesConvertUiValueFunc curves_convert_ui_value;
static CurvesCalculateFreeCurveFunc curves_calculate_free_curve;

static void curves_u8 (PixelArea *, PixelArea *, void *);
static void curves_alloc_curves_u8 (void *);
static void curves_init_curve_u8 (gint);
static void curves_plot_boundary_pts_u8 (PixelRow *, gdouble points[17][2], gint pts[17], gint);
static void curves_plot_curve_u8 (PixelRow *, gdouble *, gdouble *, gdouble *, gdouble *);
static gdouble curves_convert_ui_value_u8 (gint);
static void curves_calculate_free_curve_u8(guchar *, PixelRow *);

static void curves_u16 (PixelArea *, PixelArea *, void *);
static void curves_alloc_curves_u16 (void *);
static void curves_init_curve_u16 (gint);
static void curves_plot_boundary_pts_u16 (PixelRow *, gdouble points[17][2], gint pts[17], gint);
static void curves_plot_curve_u16 (PixelRow *, gdouble *, gdouble *, gdouble *, gdouble *);
static gdouble curves_convert_ui_value_u16 (gint);
static void curves_calculate_free_curve_u16(guchar *, PixelRow *);

static void *curves_options = NULL;
static CurvesDialog *curves_dialog = NULL;
static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};

static Argument * curves_spline_invoker (Argument *);
static Argument * curves_explicit_invoker (Argument *);

/*  curves machinery  */
static void
curves_funcs (
		 Tag dest_tag
		)
{
  switch (tag_precision (dest_tag))
  {
  case PRECISION_U8:
	curves = curves_u8;
	curves_alloc_curves = curves_alloc_curves_u8;
	curves_init_curve = curves_init_curve_u8;
	curves_plot_boundary_pts = curves_plot_boundary_pts_u8;
	curves_plot_curve = curves_plot_curve_u8;
	curves_convert_ui_value = curves_convert_ui_value_u8;
	curves_calculate_free_curve = curves_calculate_free_curve_u8;
	break;
  case PRECISION_U16:
	curves = curves_u16;
	curves_alloc_curves = curves_alloc_curves_u16;
	curves_init_curve = curves_init_curve_u16;
	curves_plot_boundary_pts = curves_plot_boundary_pts_u16;
	curves_plot_curve = curves_plot_curve_u16;
	curves_convert_ui_value = curves_convert_ui_value_u16;
	curves_calculate_free_curve = curves_calculate_free_curve_u16;
	break;
  default:
	curves = NULL;
	curves_alloc_curves = NULL;
	curves_init_curve = NULL;
	curves_plot_boundary_pts = NULL;
	curves_plot_curve = NULL;
	curves_convert_ui_value = NULL;
	curves_calculate_free_curve = NULL;
	break;
  }
}

static void
curves_free_curves (void *user_data)
{
  gint i;
  for (i = 0; i < 5; i++)
  { 
     guchar* data =  pixelrow_data (&curve[i]);
     g_free (data);
     pixelrow_init (&curve[i], tag_null(), NULL, 0); 
  }
}

static void
curves_u8 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  CurvesDialog *cd;
  guchar *src, *dest;
  guint8 *s, *d;
  guint8 *curve_red, *curve_green, *curve_blue, *curve_value, *curve_alpha;
  int has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  int alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  int w, h;

  cd = (CurvesDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);

  curve_red = (guint8*)pixelrow_data (&curve[HISTOGRAM_RED]);
  curve_green = (guint8*)pixelrow_data (&curve[HISTOGRAM_GREEN]);
  curve_blue = (guint8*)pixelrow_data (&curve[HISTOGRAM_BLUE]);
  curve_value = (guint8*)pixelrow_data (&curve[HISTOGRAM_VALUE]);
  if (has_alpha)
    curve_alpha = (guint8*)pixelrow_data (&curve[HISTOGRAM_ALPHA]);

  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint8*)src;
      d = (guint8*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (cd->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = curve_red[s[RED_PIX]];
	      d[GREEN_PIX] = curve_green[s[GREEN_PIX]];
	      d[BLUE_PIX] = curve_blue[s[BLUE_PIX]];

	      /*  The overall changes  */
	      d[RED_PIX] = curve_value[d[RED_PIX]];
	      d[GREEN_PIX] = curve_value[d[GREEN_PIX]];
	      d[BLUE_PIX] = curve_value[d[BLUE_PIX]];
	    }
	  else
	    d[GRAY_PIX] = curve_value[s[GRAY_PIX]];

	  if (has_alpha) 
	    d[alpha] = curve_alpha[s[alpha]];

	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
curves_alloc_curves_u8 (void * user_data)
{
  gint i;
  Tag tag = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
  
  for (i = 0; i < 5; i++)
  { 
       guint8* data = (guint8*) g_malloc (sizeof(guint8) * 256 );
       pixelrow_init (&curve[i], tag, (guchar*)data, 256);
  }
}

static void
curves_init_curve_u8 (gint channel)
{
  gint j;
  guint8* data = (guint8*)pixelrow_data (&curve[channel]);

  for (j= 0; j < 256; j++)
   *data++ = j;
}



static void
curves_u16 (PixelArea *src_area,
	PixelArea *dest_area,
	void        *user_data)
{
  Tag src_tag = pixelarea_tag (src_area);
  Tag dest_tag = pixelarea_tag (dest_area);
  gint src_num_channels = tag_num_channels (src_tag);
  gint dest_num_channels = tag_num_channels (dest_tag);
  CurvesDialog *cd;
  guchar *src, *dest;
  guint16 *s, *d;
  guint16 *curve_red, *curve_green, *curve_blue, *curve_value, *curve_alpha;
  int has_alpha = tag_alpha (src_tag) == ALPHA_YES ? TRUE: FALSE;
  int alpha = has_alpha ? src_num_channels - 1 : src_num_channels;
  int w, h;

  cd = (CurvesDialog *) user_data;

  src = pixelarea_data (src_area);
  dest = pixelarea_data (dest_area);

  curve_red = (guint16*)pixelrow_data (&curve[HISTOGRAM_RED]);
  curve_green = (guint16*)pixelrow_data (&curve[HISTOGRAM_GREEN]);
  curve_blue = (guint16*)pixelrow_data (&curve[HISTOGRAM_BLUE]);
  curve_value = (guint16*)pixelrow_data (&curve[HISTOGRAM_VALUE]);
  if (has_alpha)
    curve_alpha = (guint16*)pixelrow_data (&curve[HISTOGRAM_ALPHA]);

  h = pixelarea_height (src_area);
  while (h--)
    {
      s = (guint16*)src;
      d = (guint16*)dest;
      w = pixelarea_width (src_area);
      while (w--)
	{
	  if (cd->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = curve_red[s[RED_PIX]];
	      d[GREEN_PIX] = curve_green[s[GREEN_PIX]];
	      d[BLUE_PIX] = curve_blue[s[BLUE_PIX]];

	      /*  The overall changes  */
	      d[RED_PIX] = curve_value[d[RED_PIX]];
	      d[GREEN_PIX] = curve_value[d[GREEN_PIX]];
	      d[BLUE_PIX] = curve_value[d[BLUE_PIX]];
	    }
	  else
	    d[GRAY_PIX] = curve_value[s[GRAY_PIX]];

	  if (has_alpha) 
	    d[alpha] = curve_alpha[s[alpha]];

	  s += src_num_channels;
	  d += dest_num_channels;
	}

      src += pixelarea_rowstride (src_area);
      dest += pixelarea_rowstride (dest_area);
    }
}

static void
curves_alloc_curves_u16 (void * user_data)
{
  gint i;
  Tag tag = tag_new (PRECISION_U16, FORMAT_GRAY, ALPHA_NO);
  
  for (i = 0; i < 5; i++)
  { 
       guint16* data = (guint16*) g_malloc (sizeof(guint16) * 65536 );
       pixelrow_init (&curve[i], tag, (guchar*)data, 65536);
  }
}

static void
curves_init_curve_u16 (gint channel)
{
  gint j;
  guint16* data = (guint16*)pixelrow_data (&curve[channel]);

  for (j= 0; j < 65536; j++)
   *data++ = j;
}

/*  curves action functions  */

static void
curves_button_press (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
}

static void
curves_button_release (Tool           *tool,
		       GdkEventButton *bevent,
		       gpointer        gdisp_ptr)
{
}

static void
curves_motion (Tool           *tool,
	       GdkEventMotion *mevent,
	       gpointer        gdisp_ptr)
{
}

static void
curves_cursor_update (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
curves_control (Tool     *tool,
		int       action,
		gpointer  gdisp_ptr)
{
  Curves * _curves;

  _curves = (Curves *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (curves_dialog)
	{
	  image_map_abort_16 (curves_dialog->image_map);
	  curves_free_curves(curves_dialog);
	  curves_dialog->image_map = NULL;
	  curves_cancel_callback (NULL, (gpointer) curves_dialog);
	}
      break;
    }
}

Tool *
tools_new_curves ()
{
  Tool * tool;
  Curves * private;

  /*  The tool options  */
  if (!curves_options)
    curves_options = tools_register_no_options (CURVES, "Curves Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Curves *) g_malloc (sizeof (Curves));

  tool->type = CURVES;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = curves_button_press;
  tool->button_release_func = curves_button_release;
  tool->motion_func = curves_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = curves_cursor_update;
  tool->control_func = curves_control;

  return tool;
}

void
tools_free_curves (Tool *tool)
{
  Curves * _curves;

  _curves = (Curves *) tool->private;

  /*  Close the color select dialog  */
  if (curves_dialog)
    curves_ok_callback (NULL, (gpointer) curves_dialog);

  g_free (_curves);
}

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Reset", curves_reset_callback, NULL, NULL },
  { "OK", curves_ok_callback, NULL, NULL },
  { "Cancel", curves_cancel_callback, NULL, NULL }
};

static MenuItem channel_items[] =
{
  { "Value", 0, 0, curves_value_callback, NULL, NULL, NULL },
  { "Red", 0, 0, curves_red_callback, NULL, NULL, NULL },
  { "Green", 0, 0, curves_green_callback, NULL, NULL, NULL },
  { "Blue", 0, 0, curves_blue_callback, NULL, NULL, NULL },
  { "Alpha", 0, 0, curves_alpha_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

static MenuItem curve_type_items[] =
{
  { "Smooth", 0, 0, curves_smooth_callback, NULL, NULL, NULL },
  { "Free", 0, 0, curves_free_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

void
curves_initialize (void *gdisp_ptr)
{
  GDisplay *gdisp;
  int i, j;

  gdisp = (GDisplay *) gdisp_ptr;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      message_box ("Curves for indexed drawables cannot be adjusted.", NULL, NULL);
      return;
    }

  /*  The curves dialog  */
  if (!curves_dialog)
    curves_dialog = curves_new_dialog ();
  else if (!GTK_WIDGET_VISIBLE (curves_dialog->shell))
    gtk_widget_show (curves_dialog->shell);
  
  /* Set up the function pointers for this tag */
  curves_funcs( drawable_tag (gimage_active_drawable (gdisp->gimage)));
  
  /* allocate all curves data */
  (*curves_alloc_curves) (curves_dialog);
  
  /* Initialize all the curves for image data */ 
  for (i = 0; i < 5; i++)
    (*curves_init_curve) (i);

  /* Initialize all the curves for the ui  */
  curves_dialog->channel = HISTOGRAM_VALUE;
  for (i = 0; i < 5; i++)
    for (j = 0; j < 256; j++)
      curves_dialog->curve[i][j] = j;

  curves_dialog->grab_point = -1;
  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 17; j++)
	{
	  curves_dialog->points[i][j][0] = -1;
	  curves_dialog->points[i][j][1] = -1;
	}
      curves_dialog->points[i][0][0] = 0;
      curves_dialog->points[i][0][1] = 0;
      curves_dialog->points[i][16][0] = 255;
      curves_dialog->points[i][16][1] = 255;
    }

  curves_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  curves_dialog->color = drawable_color ( (curves_dialog->drawable));
  curves_dialog->image_map = image_map_create_16 (gdisp_ptr, curves_dialog->drawable);

  /* check for alpha channel */
  if (drawable_has_alpha ( (curves_dialog->drawable)))
    gtk_widget_set_sensitive( channel_items[4].widget, TRUE);
  else 
    gtk_widget_set_sensitive( channel_items[4].widget, FALSE);
  
  /*  hide or show the channel menu based on image type  */
  if (curves_dialog->color)
    for (i = 0; i < 4; i++) 
       gtk_widget_set_sensitive( channel_items[i].widget, TRUE);
  else 
    for (i = 1; i < 4; i++) 
       gtk_widget_set_sensitive( channel_items[i].widget, FALSE);

  /* set the current selection */
  gtk_option_menu_set_history ( GTK_OPTION_MENU (curves_dialog->channel_menu), 0);



  curves_update (curves_dialog, GRAPH | DRAW);
}

void
curves_free ()
{
  if (curves_dialog)
    {
      if (curves_dialog->image_map)
	{
	  image_map_abort_16 (curves_dialog->image_map);
	  curves_free_curves(curves_dialog);
	  curves_dialog->image_map = NULL;
	}
      if (curves_dialog->pixmap)
	gdk_pixmap_unref (curves_dialog->pixmap);
      gtk_widget_destroy (curves_dialog->shell);
    }
}

/**************************/
/*  Select Curves dialog  */
/**************************/

CurvesDialog *
curves_new_dialog ()
{
  CurvesDialog *cd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *option_menu;
  GtkWidget *channel_hbox;
  GtkWidget *menu;
  GtkWidget *table;
  int i;

  cd = g_malloc (sizeof (CurvesDialog));
  cd->preview = TRUE;
  cd->curve_type = SMOOTH;
  cd->pixmap = NULL;

  for (i = 0; i < 5; i++)
    channel_items [i].user_data = (gpointer) cd;
  for (i = 0; i < 2; i++)
    curve_type_items [i].user_data = (gpointer) cd;

  /*  The shell and main vbox  */
  cd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (cd->shell), "curves", "Gimp");
  gtk_window_set_title (GTK_WINDOW (cd->shell), "Curves");

  gtk_signal_connect (GTK_OBJECT (cd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (curves_delete_callback),
		      cd);
  
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Modify Curves for Channel: ");
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);

  menu = build_menu (channel_items, NULL);
  cd->channel_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (channel_hbox), cd->channel_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (cd->channel_menu);
  gtk_widget_show (channel_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cd->channel_menu), menu);

  /*  The table for the yrange and the graph  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  cd->yrange = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (cd->yrange), YRANGE_WIDTH, YRANGE_HEIGHT);
  gtk_widget_set_events (cd->yrange, RANGE_MASK);
  gtk_signal_connect (GTK_OBJECT (cd->yrange), "event",
		      (GtkSignalFunc) curves_yrange_events,
		      cd);
  gtk_container_add (GTK_CONTAINER (frame), cd->yrange);
  gtk_widget_show (cd->yrange);
  gtk_widget_show (frame);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_FILL, 0, 0);

  cd->graph = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (cd->graph),
			 GRAPH_WIDTH + RADIUS * 2,
			 GRAPH_HEIGHT + RADIUS * 2);
  gtk_widget_set_events (cd->graph, GRAPH_MASK);
  gtk_signal_connect (GTK_OBJECT (cd->graph), "event",
		      (GtkSignalFunc) curves_graph_events,
		      cd);
  gtk_container_add (GTK_CONTAINER (frame), cd->graph);
  gtk_widget_show (cd->graph);
  gtk_widget_show (frame);

  /*  The range drawing area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  cd->xrange = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (cd->xrange), XRANGE_WIDTH, XRANGE_HEIGHT);
  gtk_widget_set_events (cd->xrange, RANGE_MASK);
  gtk_signal_connect (GTK_OBJECT (cd->xrange), "event",
		      (GtkSignalFunc) curves_xrange_events,
		      cd);
  gtk_container_add (GTK_CONTAINER (frame), cd->xrange);
  gtk_widget_show (cd->xrange);
  gtk_widget_show (frame);
  gtk_widget_show (table);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The option menu for selecting the drawing method  */
  label = gtk_label_new ("Curve Type: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  menu = build_menu (curve_type_items, NULL);
  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (option_menu);
  gtk_widget_show (hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), cd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) curves_preview_update,
		      cd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = cd;
  action_items[1].user_data = cd;
  action_items[2].user_data = cd;
  build_action_area (GTK_DIALOG (cd->shell), action_items, 3, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (cd->shell);

  return cd;
}

static void
curves_update (CurvesDialog *cd,
	       int           update)
{
  GdkRectangle area;
  int i, j;

  if (update & XRANGE_TOP)
    {
      for (i = 0; i < XRANGE_HEIGHT / 2; i++)
	gtk_preview_draw_row (GTK_PREVIEW (cd->xrange),
			      cd->curve[cd->channel],
			      0, i, XRANGE_WIDTH);

      if (update & DRAW)
	{
	  area.x = 0;
	  area.y = 0;
	  area.width = XRANGE_WIDTH;
	  area.height = XRANGE_HEIGHT / 2;
	  gtk_widget_draw (cd->xrange, &area);
	}
    }
  if (update & XRANGE_BOTTOM)
    {
      unsigned char buf[XRANGE_WIDTH];

      for (i = 0; i < XRANGE_WIDTH; i++)
	buf[i] = i;

      for (i = XRANGE_HEIGHT / 2; i < XRANGE_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (cd->xrange), buf, 0, i, XRANGE_WIDTH);

      if (update & DRAW)
	{
	  area.x = 0;
	  area.y = XRANGE_HEIGHT / 2;
	  area.width = XRANGE_WIDTH;
	  area.height = XRANGE_HEIGHT / 2;
	  gtk_widget_draw (cd->xrange, &area);
	}
    }
  if (update & YRANGE)
    {
      unsigned char buf[YRANGE_WIDTH];

      for (i = 0; i < YRANGE_HEIGHT; i++)
	{
	  for (j = 0; j < YRANGE_WIDTH; j++)
	    buf[j] = (255 - i);

	  gtk_preview_draw_row (GTK_PREVIEW (cd->yrange), buf, 0, i, YRANGE_WIDTH);

	}

      if (update & DRAW)
	gtk_widget_draw (cd->yrange, NULL);
    }
  if ((update & GRAPH) && (update & DRAW) && cd->pixmap != NULL)
    {
      GdkPoint points[256];

      /*  Clear the pixmap  */
      gdk_draw_rectangle (cd->pixmap, cd->graph->style->bg_gc[GTK_STATE_NORMAL],
			  TRUE, 0, 0, GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);

      /*  Draw the grid lines  */
      for (i = 0; i < 5; i++)
	{
	  gdk_draw_line (cd->pixmap, cd->graph->style->dark_gc[GTK_STATE_NORMAL],
			 RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS,
			 GRAPH_WIDTH + RADIUS, i * (GRAPH_HEIGHT / 4) + RADIUS);
	  gdk_draw_line (cd->pixmap, cd->graph->style->dark_gc[GTK_STATE_NORMAL],
			 i * (GRAPH_WIDTH / 4) + RADIUS, RADIUS,
			 i * (GRAPH_WIDTH / 4) + RADIUS, GRAPH_HEIGHT + RADIUS);
	}

      /*  Draw the curve the 8 bit ui curve  */
      for (i = 0; i < 256; i++)
	{
	  points[i].x = i + RADIUS;
	  points[i].y = 255 - cd->curve[cd->channel][i] + RADIUS;
	}
      gdk_draw_points (cd->pixmap, cd->graph->style->black_gc, points, 256);

      /*  Draw the points  */
      if (cd->curve_type == SMOOTH)
	for (i = 0; i < 17; i++)
	  {
	    if (cd->points[cd->channel][i][0] != -1)
	      gdk_draw_arc (cd->pixmap, cd->graph->style->black_gc, TRUE,
			    cd->points[cd->channel][i][0],
			    255 - cd->points[cd->channel][i][1],
			    RADIUS * 2, RADIUS * 2, 0, 23040);
	  }

      gdk_draw_pixmap (cd->graph->window, cd->graph->style->black_gc, cd->pixmap,
		       0, 0, 0, 0, GRAPH_WIDTH + RADIUS * 2, GRAPH_HEIGHT + RADIUS * 2);
    }
}


static void
curves_calculate_image_data_curves (CurvesDialog *cd)
{
  /* copy the ui points to points */
  gdouble points[17][2];
  gint i,j,k;
  
  /* reset all the function pointers to this tag type */
  curves_funcs (drawable_tag (cd->drawable));

  for (k = 0; k < 5; k++)  
  {
    if (cd->curve_type == GFREE) 
      (*curves_calculate_free_curve)(cd->curve[k], &curve[k]); 
    else if (cd->curve_type == SMOOTH)
    {
      for (i = 0; i < 17; i++)
	for(j= 0; j < 2; j++)
	{
	  if (cd->points[k][i][j] != -1)
	    points[i][j] = (*curves_convert_ui_value) (cd->points[k][i][j]);
	  else
	    points[i][j] = -1.0;
	}
	curves_calculate_curve (&curve[k], points); 
    }
  }
}

static gdouble 
curves_convert_ui_value_u8( gint ui_value )
{
  return (gdouble)ui_value;
}

static gdouble 
curves_convert_ui_value_u16( gint ui_value )
{
  return ((gdouble)ui_value/255.0)*65535.0;
}

static void
curves_calculate_curve (
			PixelRow *curve,
			gdouble points[17][2]
			)
{
  int i;
  int pts[17];
  int num_pts;
  int p1, p2, p3, p4;

  /*  cycle through the curves  */
  num_pts = 0;
  for (i = 0; i < 17; i++)
    if (points[i][0] != -1)
      pts[num_pts++] = i;

  /*  Initialize boundary points */
  if (num_pts != 0)
      (*curves_plot_boundary_pts) (curve, points, pts, num_pts);   

  for (i = 0; i < num_pts - 1; i++)
    {
      p1 = (i == 0) ? pts[i] : pts[(i - 1)];
      p2 = pts[i];
      p3 = pts[(i + 1)];
      p4 = (i == (num_pts - 2)) ? pts[(num_pts - 1)] : pts[(i + 2)];
   
      /* plot this section of the curve */
      (*curves_plot_curve) (curve, points[p1], points[p2], points[p3], points[p4]);
    }
}

static void 
curves_calculate_free_curve_u8 (guchar *ui_curve, PixelRow *curve)
{
  gint i;
  guint8 *curve_data = (guint8*)pixelrow_data (curve);
  for (i = 0; i < 256; i++)
    curve_data[i] = ui_curve[i];
}

static void 
curves_calculate_free_curve_u16(guchar *ui_curve, PixelRow *curve)
{
  gint i, index, frac, y1, y2;
  gfloat m;
  guint16 *curve_data = (guint16*)pixelrow_data (curve);
  for ( i = 0; i < 65536; i++)
  {
    frac = i % 257;
    index = i / 257;
    y1 = ui_curve[index] * 257;
    if (index + 1 < 256)
    {
      y2 = ui_curve[index+1] * 257;
      m = ui_curve[index+1] - ui_curve[index];
      curve_data[i] = (guint16)(y1 + m*(frac)); 
    }
    else
      curve_data[i] = (guint16)y1;
  }
}

static void
curves_plot_boundary_pts_u8(
			        PixelRow *curve,
				gdouble points[17][2],
				gint pts[17],
				gint num_pts
				)	
{
  /* can just call the corresponding ui code since it is 0-255 based*/ 
  curves_plot_boundary_pts_ui (curve, points, pts, num_pts);
}					


static void
curves_plot_curve_u8(
		   PixelRow      *curve,
		   gdouble       *pt1,             
		   gdouble       *pt2,
		   gdouble       *pt3, 
		   gdouble       *pt4
			)
{
  /* can just call the corresponding ui code since it is 0-255 based*/ 
  curves_plot_curve_ui (curve, pt1, pt2, pt3, pt4);
}

static void
curves_plot_boundary_pts_u16(
			        PixelRow *curve,
				gdouble points[17][2],
				gint pts[17],
				gint num_pts
				)	
{
  gint i;
  guint16 *curve_data = (guint16*)pixelrow_data (curve);
	
  for (i = 0; i < points[pts[0]][0]; i++)
    curve_data[i] = (guint16)points[pts[0]][1];
  for (i = points[pts[num_pts - 1]][0]; i < 65536; i++)
    curve_data[i] = (guint16)points[pts[num_pts - 1]][1];
}					


static void
curves_plot_curve_u16(
		   PixelRow      *curve,
		   gdouble       *pt1,             
		   gdouble       *pt2,
		   gdouble       *pt3, 
		   gdouble       *pt4
			)
{
  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  int newx, newy;
  int i;
  guint16* curve_data = (guint16*) pixelrow_data (curve);

  
  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  for (i = 0; i < 2; i++)
    {
      geometry[0][i] = pt1[i];
      geometry[1][i] = pt2[i];
      geometry[2][i] = pt3[i];
      geometry[3][i] = pt4[i];
    }

  /* subdivide the curve 70000 times */
  /* n can be adjusted to give a finer or coarser curve */
  d = 1.0 / 70000;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward differencing deltas */
  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  curves_CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  curves_CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = BOUNDS (x, 0, 65535);
  lasty = BOUNDS (y, 0, 65535);

  curve_data[lastx] = lasty;

  /* loop over the curve */
  for (i = 0; i < 70000; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = BOUNDS ((ROUND (x)), 0, 65535);
      newy = BOUNDS ((ROUND (y)), 0, 65535);

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	curve_data[newx] = newy;

      lastx = newx;
      lasty = newy;
    }
}

static void
curves_plot_boundary_pts_ui(
			        PixelRow *curve,
				gdouble points[17][2],
				gint pts[17],
				gint num_pts
				)	
{
  gint i;
  guint8 *curve_data = (guint8*)pixelrow_data (curve);
	
  for (i = 0; i < points[pts[0]][0]; i++)
    curve_data[i] = (guint8)points[pts[0]][1];
  for (i = points[pts[num_pts - 1]][0]; i < 256; i++)
    curve_data[i] = (guint8)points[pts[num_pts - 1]][1];
}					


static void
curves_plot_curve_ui(
		   PixelRow      *curve,
		   gdouble       *pt1,             
		   gdouble       *pt2,
		   gdouble       *pt3, 
		   gdouble       *pt4
			)
{
  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  int newx, newy;
  int i;
  guint8* curve_data = (guint8*) pixelrow_data (curve);

  
  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  for (i = 0; i < 2; i++)
    {
      geometry[0][i] = pt1[i];
      geometry[1][i] = pt2[i];
      geometry[2][i] = pt3[i];
      geometry[3][i] = pt4[i];
    }

  /* subdivide the curve 1000 times */
  /* n can be adjusted to give a finer or coarser curve */
  d = 1.0 / 1000;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward differencing deltas */
  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  curves_CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  curves_CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = BOUNDS (x, 0, 255);
  lasty = BOUNDS (y, 0, 255);

  curve_data[lastx] = lasty;

  /* loop over the curve */
  for (i = 0; i < 1000; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = BOUNDS ((ROUND (x)), 0, 255);
      newy = BOUNDS ((ROUND (y)), 0, 255);

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	curve_data[newx] = newy;

      lastx = newx;
      lasty = newy;
    }
}
					

static void
curves_preview (CurvesDialog *cd)
{
  if (!cd->image_map)
    g_warning ("No image map");

  curves_calculate_image_data_curves(cd); 
  image_map_apply_16 (cd->image_map, curves, (void *) cd);
}

static void
curves_value_callback (GtkWidget *w,
		       gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->channel != HISTOGRAM_VALUE)
    {
      cd->channel = HISTOGRAM_VALUE;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_red_callback (GtkWidget *w,
		     gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->channel != HISTOGRAM_RED)
    {
      cd->channel = HISTOGRAM_RED;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_green_callback (GtkWidget *w,
		       gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->channel != HISTOGRAM_GREEN)
    {
      cd->channel = HISTOGRAM_GREEN;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_blue_callback (GtkWidget *w,
		      gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->channel != HISTOGRAM_BLUE)
    {
      cd->channel = HISTOGRAM_BLUE;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_alpha_callback (GtkWidget *w,
		      gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->channel != HISTOGRAM_ALPHA)
    {
      cd->channel = HISTOGRAM_ALPHA;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_smooth_callback (GtkWidget *w,
			gpointer   client_data)
{
  CurvesDialog *cd;
  int i, index;

  cd = (CurvesDialog *) client_data;

  if (cd->curve_type != SMOOTH)
    {
      cd->curve_type = SMOOTH;

      /*  pick representative points from the curve and make them control points  */
      for (i = 0; i <= 8; i++)
	{
	  index = BOUNDS ((i * 32), 0, 255);
	  cd->points[cd->channel][i * 2][0] = index;
	  cd->points[cd->channel][i * 2][1] = cd->curve[cd->channel][index];
	}
	
      /* compute the ui curve from curve dialog */
      curves_calculate_ui_curve (cd);
      
      curves_update (cd, GRAPH | DRAW);

      if (cd->preview)
	curves_preview (cd);
    }
}

static void
curves_calculate_ui_curve (CurvesDialog *cd)
{
  gdouble points_ui[17][2];
  PixelRow curve;
  Tag tag = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NO);
  gint i,j;
  
  if (cd->curve_type == GFREE)
    return;

  /* get double versions of the ui points */
  for (i = 0; i < 17; i++)
    for(j= 0; j < 2; j++)
	  points_ui[i][j] = cd->points[cd->channel][i][j];
  
  /* make the curve data point to the ui curve from the dialog  */
  pixelrow_init (&curve, tag, (guchar*)(cd->curve[cd->channel]), 256);	
 
  /* set the function pointers for ui routines since we are 
	computing a 0-255 based ui curve*/ 
  
  curves_plot_boundary_pts = curves_plot_boundary_pts_ui;
  curves_plot_curve = curves_plot_curve_ui;

  curves_calculate_curve (&curve, points_ui); 
}


static void
curves_free_callback (GtkWidget *w,
		      gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (cd->curve_type != GFREE)
    {
      cd->curve_type = GFREE;
      curves_update (cd, GRAPH | DRAW);
    }
}

static void
curves_reset_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  CurvesDialog *cd;
  int i;

  cd = (CurvesDialog *) client_data;

  /* Initialize the curve for this channel  */
  (*curves_init_curve)(cd->channel);
  
  /* Initialize the ui curve for this channel  */
  for (i = 0; i < 256; i++)
    cd->curve[cd->channel][i] = i;

  cd->grab_point = -1;
  for (i = 0; i < 17; i++)
    {
      cd->points[cd->channel][i][0] = -1;
      cd->points[cd->channel][i][1] = -1;
    }
  cd->points[cd->channel][0][0] = 0;
  cd->points[cd->channel][0][1] = 0;
  cd->points[cd->channel][16][0] = 255;
  cd->points[cd->channel][16][1] = 255;

  curves_update (cd, GRAPH | XRANGE_TOP | DRAW);
  if (cd->preview)
    curves_preview (cd);
}

static void
curves_ok_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (cd->shell))
    gtk_widget_hide (cd->shell);

  if (!cd->preview)
  {
    curves_calculate_image_data_curves (cd);
    image_map_apply_16 (cd->image_map, curves, (void *) cd);
  }

  if (cd->image_map)
  {
    image_map_commit_16 (cd->image_map);
    curves_free_curves(cd);
  }

  cd->image_map = NULL;
}

static void
curves_cancel_callback (GtkWidget *widget,
			gpointer   client_data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (cd->shell))
    gtk_widget_hide (cd->shell);

  if (cd->image_map)
    {
      image_map_abort_16 (cd->image_map);
      curves_free_curves(cd);
      gdisplays_flush ();
    }

  cd->image_map = NULL;
}

static gint 
curves_delete_callback (GtkWidget *w,
			GdkEvent *e,
			gpointer data) 
{
  curves_cancel_callback (w, data);

  return FALSE;
}
static void
curves_preview_update (GtkWidget *w,
		       gpointer   data)
{
  CurvesDialog *cd;

  cd = (CurvesDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      cd->preview = TRUE;
      curves_preview (cd);
    }
  else
    cd->preview = FALSE;
}

static gint
curves_graph_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     CurvesDialog *cd)
{
  static GdkCursorType cursor_type = GDK_TOP_LEFT_ARROW;
  GdkCursorType new_type;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int i;
  int tx, ty;
  int x, y;
  int closest_point;
  int distance;
  int x1, x2, y1, y2;

  new_type      = GDK_X_CURSOR;
  closest_point = 0;

  /*  get the pointer position  */
  gdk_window_get_pointer (cd->graph->window, &tx, &ty, NULL);
  x = BOUNDS ((tx - RADIUS), 0, 255);
  y = BOUNDS ((ty - RADIUS), 0, 255);

  distance = G_MAXINT;
  for (i = 0; i < 17; i++)
    {
      if (cd->points[cd->channel][i][0] != -1)
	if (abs (x - cd->points[cd->channel][i][0]) < distance)
	  {
	    distance = abs (x - cd->points[cd->channel][i][0]);
	    closest_point = i;
	  }
    }
  if (distance > MIN_DISTANCE)
    closest_point = (x + 8) / 16;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (cd->pixmap == NULL)
	cd->pixmap = gdk_pixmap_new (cd->graph->window,
				     GRAPH_WIDTH + RADIUS * 2,
				     GRAPH_HEIGHT + RADIUS * 2, -1);

      curves_update (cd, GRAPH | DRAW);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      new_type = GDK_TCROSS;

      switch (cd->curve_type)
	{
	case SMOOTH:
	  /*  determine the leftmost and rightmost points  */
	  cd->leftmost = -1;
	  for (i = closest_point - 1; i >= 0; i--)
	    if (cd->points[cd->channel][i][0] != -1)
	      {
		cd->leftmost = cd->points[cd->channel][i][0];
		break;
	      }
	  cd->rightmost = 256;
	  for (i = closest_point + 1; i < 17; i++)
	    if (cd->points[cd->channel][i][0] != -1)
	      {
		cd->rightmost = cd->points[cd->channel][i][0];
		break;
	      }

	  cd->grab_point = closest_point;
	  cd->points[cd->channel][cd->grab_point][0] = x;
	  cd->points[cd->channel][cd->grab_point][1] = 255 - y;
	  
          /* calculate a ui curve */
	  curves_calculate_ui_curve (cd);
	  break;

	case GFREE:
	  cd->curve[cd->channel][x] = 255 - y;
	  cd->grab_point = x;
	  cd->last = y;
	  break;
	}

      curves_update (cd, GRAPH | XRANGE_TOP | DRAW);
      break;

    case GDK_BUTTON_RELEASE:
      new_type = GDK_FLEUR;
      cd->grab_point = -1;

      if (cd->preview)
	curves_preview (cd);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if (mevent->is_hint)
	{
	  mevent->x = tx;
	  mevent->y = ty;
	}

      switch (cd->curve_type)
	{
	case SMOOTH:
	  /*  If no point is grabbed...  */
	  if (cd->grab_point == -1)
	    {
	      if (cd->points[cd->channel][closest_point][0] != -1)
		new_type = GDK_FLEUR;
	      else
		new_type = GDK_TCROSS;
	    }
	  /*  Else, drag the grabbed point  */
	  else
	    {
	      new_type = GDK_TCROSS;

	      cd->points[cd->channel][cd->grab_point][0] = -1;

	      if (x > cd->leftmost && x < cd->rightmost)
		{
		  closest_point = (x + 8) / 16;
		  if (cd->points[cd->channel][closest_point][0] == -1)
		    cd->grab_point = closest_point;
		  cd->points[cd->channel][cd->grab_point][0] = x;
		  cd->points[cd->channel][cd->grab_point][1] = 255 - y;
		}
	      /* calculate a ui curve */
	      curves_calculate_ui_curve (cd);
	      curves_update (cd, GRAPH | XRANGE_TOP | DRAW);
	    }
	  break;

	case GFREE:
	  if (cd->grab_point != -1)
	    {
	      if (cd->grab_point > x)
		{
		  x1 = x;
		  x2 = cd->grab_point;
		  y1 = y;
		  y2 = cd->last;
		}
	      else
		{
		  x1 = cd->grab_point;
		  x2 = x;
		  y1 = cd->last;
		  y2 = y;
		}

	      if (x2 != x1)
		for (i = x1; i <= x2; i++)
		  cd->curve[cd->channel][i] = 255 - (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
	      else
		cd->curve[cd->channel][x] = 255 - y;

	      cd->grab_point = x;
	      cd->last = y;

	      curves_update (cd, GRAPH | XRANGE_TOP | DRAW);
	    }

	  if (mevent->state & GDK_BUTTON1_MASK)
	    new_type = GDK_TCROSS;
	  else
	    new_type = GDK_PENCIL;
	  break;
	}

      if (new_type != cursor_type)
	{
	  cursor_type = new_type;
	  change_win_cursor (cd->graph->window, cursor_type);
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
curves_xrange_events (GtkWidget    *widget,
		      GdkEvent     *event,
		      CurvesDialog *cd)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      curves_update (cd, XRANGE_TOP | XRANGE_BOTTOM);
      break;

    case GDK_DELETE:
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
curves_yrange_events (GtkWidget    *widget,
		      GdkEvent     *event,
		      CurvesDialog *cd)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      curves_update (cd, YRANGE);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
curves_CR_compose (CRMatrix a,
		   CRMatrix b,
		   CRMatrix ab)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          ab[i][j] = (a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j]);
        }
    }
}

/*
 *  The curves procedure definitions
 */


/*  Procedure for defining the curve with a spline  */
ProcArg curves_spline_args[] =
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
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), ALPHA (4), GRAY (0) }"
  },
  { PDB_INT32,
    "num_points",
    "the number of values in the control point array ( 3 < num_points <= 32 )"
  },
  { PDB_INT8ARRAY,
    "control_pts",
    "the spline control points: { cp1.x, cp1.y, cp2.x, cp2.y, ... }"
  }
};

ProcRecord curves_spline_proc =
{
  "gimp_curves_spline",
  "Modifies the intensity curve(s) for specified drawable",
  "Modifies the intensity mapping for one channel in the specified drawable.  The drawable must be either grayscale or RGB, and the channel can be either an intensity component, or the value.  The 'control_pts' parameter is an array of integers which define a set of control points which describe a Catmull Rom spline which yields the final intensity curve.  Use the 'gimp_curves_explicit' function to explicitly modify intensity levels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  curves_spline_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { curves_spline_invoker } },
};


static Argument *
curves_spline_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  int int_value;
  CurvesDialog cd;
  GImage *gimage;
  int channel;
  int num_cp;
  unsigned char *control_pts;
  int x1, y1, x2, y2;
  int i, j;
  void *pr;
  GimpDrawable *drawable;

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
      drawable =  drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);
  
    
  /*  channel  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    if (int_value != 0)
	      success = FALSE;
	  else if (drawable_color (drawable))
	    if (int_value < 0 || int_value > 3)
	      success = FALSE;
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  if (success)
    {
      num_cp = args[3].value.pdb_int;
      if (num_cp < 4 || num_cp > 32 || (num_cp & 0x1))
	success = FALSE;
    }
  /*  control points  */
  if (success)
    {
      control_pts = (unsigned char *) args[4].value.pdb_pointer;
    }

  /*  arrange to modify the curves  */
  if (success)
    {
      for (i = 0; i < 5; i++)
	for (j = 0; j < 256; j++)
	  cd.curve[i][j] = j;

      for (i = 0; i < 5; i++)
	for (j = 0; j < 17; j++)
	  {
	    cd.points[i][j][0] = -1;
	    cd.points[i][j][1] = -1;
	  }

      cd.channel = channel;
      cd.color = drawable_color (drawable);
      cd.curve_type = SMOOTH;

      for (j = 0; j < num_cp / 2; j++)
	{
	  cd.points[cd.channel][j][0] = control_pts[j * 2];
	  cd.points[cd.channel][j][1] = control_pts[j * 2 + 1];
	}

      /* Set up the function pointers for this tag */
      curves_funcs (drawable_tag (drawable));
      
      /* allocate all curves data */
      (*curves_alloc_curves) (&cd);
      
      /* Initialize all the curves for image data */ 
      for (i = 0; i < 5; i++)
	(*curves_init_curve) (i);

      /* calculate curves for the image data */
      curves_calculate_image_data_curves (&cd);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data_canvas (drawable), NULL,
			x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow_canvas (drawable), NULL, 
			x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	(*curves) (&src_area, &dest_area, (void *) &cd);

      curves_free_curves (&cd);
      drawable_merge_shadow_canvas (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&curves_spline_proc, success);
}

/*  Procedure for explicitly defining the curve  */
ProcArg curves_explicit_args[] =
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
    "num_bytes",
    "the number of bytes in the new curve (always 256)"
  },
  { PDB_INT8ARRAY,
    "curve",
    "the explicit curve"
  }
};

ProcRecord curves_explicit_proc =
{
  "gimp_curves_explicit",
  "Modifies the intensity curve(s) for specified drawable",
  "Modifies the intensity mapping for one channel in the specified drawable.  The drawable must be either grayscale or RGB, and the channel can be either an intensity component, or the value.  The 'curve' parameter is an array of bytes which explicitly defines how each pixel value in the drawable will be modified.  Use the 'gimp_curves_spline' function to modify intensity levels with Catmull Rom splines.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  curves_explicit_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { curves_explicit_invoker } },
};


static Argument *
curves_explicit_invoker (Argument *args)
{
  PixelArea src_area, dest_area;
  int success = TRUE;
  int int_value;
  CurvesDialog cd;
  GImage *gimage;
  int channel;
  unsigned char *curve;
  int x1, y1, x2, y2;
  int i, j;
  void *pr;
  GimpDrawable *drawable;

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
      drawable =  drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);
  
  /*  channel  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    if (int_value != 0)
	      success = FALSE;
	  else if (drawable_color (drawable))
	    if (int_value < 0 || int_value > 3)
	      success = FALSE;
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  the number of bytes  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value != 256)
	success = FALSE;
    }
  /*  the curve  */
  if (success)
    {
      curve = (unsigned char *) args[4].value.pdb_pointer;
    }

  /*  arrange to modify the curves  */
  if (success)
    {
      for (i = 0; i < 5; i++)
	for (j = 0; j < 256; j++)
	  cd.curve[i][j] = j;

      cd.channel = channel;
      cd.color = drawable_color (drawable);

      for (j = 0; j < 256; j++)
	cd.curve[cd.channel][j] = curve[j];
      
      /* Set up the function pointers for this tag */
      curves_funcs (drawable_tag (drawable));
      
      /* allocate all curves data */
      (*curves_alloc_curves) (&cd);
      
      /* Initialize all the curves for image data */ 
      for (i = 0; i < 5; i++)
	(*curves_init_curve) (i);

      /* calculate curves for the image data */
      curves_calculate_image_data_curves (&cd);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixelarea_init (&src_area, drawable_data_canvas (drawable),NULL, 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&dest_area, drawable_shadow_canvas (drawable),NULL, 
		x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixelarea_register (2, &src_area, &dest_area); 
		pr != NULL; 
		pr = pixelarea_process (pr))
	(*curves) (&src_area, &dest_area, (void *) &cd);
      
      curves_free_curves (&cd);
      drawable_merge_shadow_canvas (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&curves_explicit_proc, success);
}


