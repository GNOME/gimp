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
#include "colormaps.h"
#include "cursorutil.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogram.h"
#include "image_map.h"
#include "interface.h"
#include "curves.h"

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
  ImageMap       image_map;
  int            color;
  int            channel;
  gint           preview;

  int            grab_point;
  int            last;
  int            leftmost;
  int            rightmost;
  int            curve_type;
  int            points[5][17][2];
  unsigned char  curve[5][256];
};

typedef double CRMatrix[4][4];

/*  curves action functions  */

static void   curves_button_press   (Tool *, GdkEventButton *, gpointer);
static void   curves_button_release (Tool *, GdkEventButton *, gpointer);
static void   curves_motion         (Tool *, GdkEventMotion *, gpointer);
static void   curves_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   curves_control        (Tool *, int, gpointer);

static CurvesDialog *  curves_new_dialog              (void);
static void            curves_update                  (CurvesDialog *, int);
static void            curves_plot_curve              (CurvesDialog *, int, int, int, int);
static void            curves_calculate_curve         (CurvesDialog *);
static void            curves_preview                 (CurvesDialog *);
static void            curves_value_callback          (GtkWidget *, gpointer);
static void            curves_red_callback            (GtkWidget *, gpointer);
static void            curves_green_callback          (GtkWidget *, gpointer);
static void            curves_blue_callback           (GtkWidget *, gpointer);
static void            curves_alpha_callback           (GtkWidget *, gpointer);
static void            curves_smooth_callback         (GtkWidget *, gpointer);
static void            curves_free_callback           (GtkWidget *, gpointer);
static void            curves_reset_callback          (GtkWidget *, gpointer);
static void            curves_ok_callback             (GtkWidget *, gpointer);
static void            curves_cancel_callback         (GtkWidget *, gpointer);
static gint            curves_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void            curves_preview_update          (GtkWidget *, gpointer);
static gint            curves_xrange_events           (GtkWidget *, GdkEvent *, CurvesDialog *);
static gint            curves_yrange_events           (GtkWidget *, GdkEvent *, CurvesDialog *);
static gint            curves_graph_events            (GtkWidget *, GdkEvent *, CurvesDialog *);
static void            curves_CR_compose              (CRMatrix, CRMatrix, CRMatrix);

static void *curves_options = NULL;
static CurvesDialog *curves_dialog = NULL;
static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};


static void       curves (PixelRegion *, PixelRegion *, void *);
static Argument * curves_spline_invoker (Argument *);
static Argument * curves_explicit_invoker (Argument *);

/*  curves machinery  */

static void
curves (PixelRegion *srcPR,
	PixelRegion *destPR,
	void        *user_data)
{
  CurvesDialog *cd;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int has_alpha, alpha;
  int w, h;

  cd = (CurvesDialog *) user_data;

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
	  if (cd->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = cd->curve[HISTOGRAM_RED][s[RED_PIX]];
	      d[GREEN_PIX] = cd->curve[HISTOGRAM_GREEN][s[GREEN_PIX]];
	      d[BLUE_PIX] = cd->curve[HISTOGRAM_BLUE][s[BLUE_PIX]];

	      /*  The overall changes  */
	      d[RED_PIX] = cd->curve[HISTOGRAM_VALUE][d[RED_PIX]];
	      d[GREEN_PIX] = cd->curve[HISTOGRAM_VALUE][d[GREEN_PIX]];
	      d[BLUE_PIX] = cd->curve[HISTOGRAM_VALUE][d[BLUE_PIX]];
	    }
	  else
	    d[GRAY_PIX] = cd->curve[HISTOGRAM_VALUE][s[GRAY_PIX]];

	  if (has_alpha) {
	    d[alpha] = cd->curve[HISTOGRAM_ALPHA][s[alpha]];
	    /* d[alpha] = s[alpha]; */
	  }

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
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
	  image_map_abort (curves_dialog->image_map);
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

  /*  Initialize the values  */
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
  curves_dialog->image_map = image_map_create (gdisp_ptr, curves_dialog->drawable);

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
	  image_map_abort (curves_dialog->image_map);
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

static CurvesDialog *
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

      /*  Draw the curve  */
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
curves_plot_curve (CurvesDialog *cd,
		   int           p1,
		   int           p2,
		   int           p3,
		   int           p4)
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

  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  for (i = 0; i < 2; i++)
    {
      geometry[0][i] = cd->points[cd->channel][p1][i];
      geometry[1][i] = cd->points[cd->channel][p2][i];
      geometry[2][i] = cd->points[cd->channel][p3][i];
      geometry[3][i] = cd->points[cd->channel][p4][i];
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

  cd->curve[cd->channel][lastx] = lasty;

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
	cd->curve[cd->channel][newx] = newy;

      lastx = newx;
      lasty = newy;
    }
}

static void
curves_calculate_curve (CurvesDialog *cd)
{
  int i;
  int points[17];
  int num_pts;
  int p1, p2, p3, p4;

  switch (cd->curve_type)
    {
    case GFREE:
      break;
    case SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < 17; i++)
	if (cd->points[cd->channel][i][0] != -1)
	  points[num_pts++] = i;

      /*  Initialize boundary curve points */
      if (num_pts != 0)
	{
	  for (i = 0; i < cd->points[cd->channel][points[0]][0]; i++)
	    cd->curve[cd->channel][i] = cd->points[cd->channel][points[0]][1];
	  for (i = cd->points[cd->channel][points[num_pts - 1]][0]; i < 256; i++)
	    cd->curve[cd->channel][i] = cd->points[cd->channel][points[num_pts - 1]][1];
	}

      for (i = 0; i < num_pts - 1; i++)
	{
	  p1 = (i == 0) ? points[i] : points[(i - 1)];
	  p2 = points[i];
	  p3 = points[(i + 1)];
	  p4 = (i == (num_pts - 2)) ? points[(num_pts - 1)] : points[(i + 2)];

	  curves_plot_curve (cd, p1, p2, p3, p4);
	}
      break;
    }
}

static void
curves_preview (CurvesDialog *cd)
{
  if (!cd->image_map)
    g_warning ("No image map");
  image_map_apply (cd->image_map, curves, (void *) cd);
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

      curves_calculate_curve (cd);
      curves_update (cd, GRAPH | DRAW);

      if (cd->preview)
	curves_preview (cd);
    }
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

  /*  Initialize the values  */
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
    image_map_apply (cd->image_map, curves, (void *) cd);

  if (cd->image_map)
    image_map_commit (cd->image_map);

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
      image_map_abort (cd->image_map);
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

  return TRUE;
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

	  curves_calculate_curve (cd);
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

	      curves_calculate_curve (cd);
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
  PixelRegion srcPR, destPR;
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
      curves_calculate_curve (&cd);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
	curves (&srcPR, &destPR, (void *) &cd);

      drawable_merge_shadow (drawable, TRUE);
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
  PixelRegion srcPR, destPR;
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

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
	curves (&srcPR, &destPR, (void *) &cd);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&curves_explicit_proc, success);
}


