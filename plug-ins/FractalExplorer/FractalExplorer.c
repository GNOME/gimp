/**********************************************************************
    Fractal Explorer Plug-in (Version 2.00 Beta 2)
    Daniel Cotting (cotting@multimania.com)
 **********************************************************************
 **********************************************************************
    Official homepages: http://www.multimania.com/cotting
                        http://cotting.citeweb.net
 *********************************************************************/

/**********************************************************************
   The GIMP -- an image manipulation program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *********************************************************************/

/**********************************************************************
   Some code has been 'stolen' from:
        - Peter Kirchgessner (Pkirchg@aol.com)
	- Scott Draves (spot@cs.cmu.edu)
	- Andy Thomas (alt@picnic.demon.co.uk)
	   .
	   .
	   .
 **********************************************************************
   "If you steal from one author it's plagiarism; if you steal from
   many it's research."  --Wilson Mizner
 *********************************************************************/

/* Changes:
 *
 * 2000-01-05  Fixed a problem with strtok and got rid of the selfmade i18n
 *             Sven Neumann <sven@gimp.org>
 */

/**********************************************************************
 Include necessary files  
 *********************************************************************/

#include "config.h"

#include <glib.h>		/* Include early for G_OS_WIN32 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#ifdef G_OS_WIN32
#include <io.h>

#ifndef W_OK
#define W_OK 2
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) ((m) & _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m) & _S_IFREG)
#endif
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "pix_data.h"

#include "FractalExplorer.h"
#include "Events.h"
#include "Dialogs.h"

#include "libgimp/stdplugins-intl.h"

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GimpParam  *param,
		   gint    *nreturn_vals,
		   GimpParam **return_vals);

static void explorer            (GimpDrawable    *drawable);
static void explorer_render_row (const guchar *src_row,
				 guchar       *dest_row,
				 gint          row,
				 gint          row_width,
				 gint          bytes);

/**********************************************************************
 Declare local functions
 *********************************************************************/

/* Functions for dialog widgets */

static gint        list_button_press (GtkWidget *widget,
				      GdkEventButton *event,
				      gpointer   data);
static gint        new_button_press  (GtkWidget *widget,
				      GdkEventButton *bevent,
				      gpointer   data);

static void        delete_dialog_callback               (GtkWidget *widget,
							 gboolean   value,
						         gpointer   data);
static gint        delete_fractal_callback              (GtkWidget *widget,
						         gpointer   data);
static void        fractalexplorer_list_ok_callback     (GtkWidget *widget,
						         gpointer   data);
static void        fractalexplorer_list_cancel_callback (GtkWidget *widget,
							 gpointer   data);
static void        fractalexplorer_dialog_edit_list (GtkWidget          *lwidget,
						     fractalexplorerOBJ *obj,
						     gint                created);
static GtkWidget * new_fractalexplorer_obj     (gchar               *name);
static gint        fractalexplorer_list_pos    (fractalexplorerOBJ  *feOBJ);
static gint        fractalexplorer_list_insert (fractalexplorerOBJ  *feOBJ);
static GtkWidget * fractalexplorer_list_item_new_with_label_and_pixmap
                                               (fractalexplorerOBJ  *obj,
						gchar               *label,
						GtkWidget           *pix_widget);
static GtkWidget * fractalexplorer_new_pixmap  (GtkWidget           *list,
						gchar              **pixdata);
static GtkWidget * fractalexplorer_list_add    (fractalexplorerOBJ  *feOBJ);
static void        list_button_update          (fractalexplorerOBJ  *feOBJ);
static fractalexplorerOBJ *fractalexplorer_new (void);
static void        build_list_items (GtkWidget *list);

static void        fractalexplorer_free              (fractalexplorerOBJ *feOBJ);
static void        fractalexplorer_free_everything   (fractalexplorerOBJ *feOBJ);
static void        fractalexplorer_list_free_all             (void);
static fractalexplorerOBJ * fractalexplorer_load       (gchar     *filename,
							gchar     *name);

static void        fractalexplorer_list_load_all             (GList     *plist);
static void        fractalexplorer_rescan_ok_callback        (GtkWidget *widget,
							      gpointer   data);
static void        fractalexplorer_rescan_list               (void);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/**********************************************************************
 MAIN()  
 *********************************************************************/

MAIN()

/**********************************************************************
 FUNCTION: query  
 *********************************************************************/

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT8, "fractaltype", "0: Mandelbrot; 1: Julia; 2: Barnsley 1; 3: Barnsley 2; 4: Barnsley 3; 5: Spider; 6: ManOWar; 7: Lambda; 8: Sierpinski" },
    { GIMP_PDB_FLOAT, "xmin", "xmin fractal image delimiter" },
    { GIMP_PDB_FLOAT, "xmax", "xmax fractal image delimiter" },
    { GIMP_PDB_FLOAT, "ymin", "ymin fractal image delimiter" },
    { GIMP_PDB_FLOAT, "ymax", "ymax fractal image delimiter" },
    { GIMP_PDB_FLOAT, "iter", "Iteration value" },
    { GIMP_PDB_FLOAT, "cx", "cx value ( only Julia)" },
    { GIMP_PDB_FLOAT, "cy", "cy value ( only Julia)" },
    { GIMP_PDB_INT8, "colormode", "0: Apply colormap as specified by the parameters below; 1: Apply active gradient to final image" },
    { GIMP_PDB_FLOAT, "redstretch", "Red stretching factor" },
    { GIMP_PDB_FLOAT, "greenstretch", "Green stretching factor" },
    { GIMP_PDB_FLOAT, "bluestretch", "Blue stretching factor" },
    { GIMP_PDB_INT8, "redmode", "Red application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "greenmode", "Green application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "bluemode", "Blue application mode (0:SIN;1:COS;2:NONE)" },
    { GIMP_PDB_INT8, "redinvert", "Red inversion mode (1: enabled; 0: disabled)" },
    { GIMP_PDB_INT8, "greeninvert", "Green inversion mode (1: enabled; 0: disabled)" },
    { GIMP_PDB_INT8, "blueinvert", "Green inversion mode (1: enabled; 0: disabled)" },
  };
  static gint nargs = sizeof(args) / sizeof(args[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_fractalexplorer",
			  "Chaos Fractal Explorer Plug-In",
			  "No help yet.",
			  "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
			  "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
			  "December, 1998",
			  N_("<Image>/Filters/Render/Pattern/Fractal Explorer..."),
			  "RGB*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

/**********************************************************************
 FUNCTION: run
 *********************************************************************/

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  gint32        image_ID;
  GimpRunModeType  run_mode;
  gdouble       xhsiz;
  gdouble       yhsiz;
  gint          pwidth;
  gint          pheight;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N_UI();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;
  tile_width = gimp_tile_width ();
  tile_height = gimp_tile_height ();

  img_width = gimp_drawable_width (drawable->id);
  img_height = gimp_drawable_height (drawable->id);
  img_bpp = gimp_drawable_bpp(drawable->id);

  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  cen_x = (double) (sel_x2 - 1 + sel_x1) / 2.0;
  cen_y = (double) (sel_y2 - 1 + sel_y1) / 2.0;

  xhsiz = (double) (sel_width - 1) / 2.0;
  yhsiz = (double) (sel_height - 1) / 2.0;

  /* Calculate preview size */
  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX (pwidth, 2);
  preview_height = MAX (pheight, 2);

  /* See how we will run */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);

      /* Get information from the dialog */
      if (!explorer_dialog ())
	return;

      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams != 22)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	}
      else
	{
	  wvals.fractaltype = param[3].data.d_int8;
	  wvals.xmin = param[4].data.d_float;
	  wvals.xmax = param[5].data.d_float;
	  wvals.ymin = param[6].data.d_float;
	  wvals.ymax = param[7].data.d_float;
	  wvals.iter = param[8].data.d_float;
	  wvals.cx = param[9].data.d_float;
	  wvals.cy = param[10].data.d_float;
	  wvals.colormode = param[11].data.d_int8;
	  wvals.redstretch = param[12].data.d_float;
	  wvals.greenstretch = param[13].data.d_float;
	  wvals.bluestretch = param[14].data.d_float;
	  wvals.redmode = param[15].data.d_int8;
	  wvals.greenmode = param[16].data.d_int8;
	  wvals.bluemode = param[17].data.d_int8;
	  wvals.redinvert = param[18].data.d_int8;
	  wvals.greeninvert = param[19].data.d_int8;
	  wvals.blueinvert = param[20].data.d_int8;
	  wvals.ncolors = param[21].data.d_int8;
	}
      make_color_map();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data ("plug_in_fractalexplorer", &wvals);
      make_color_map ();
      break;

    default:
      break;
    }

  xmin = wvals.xmin;
  xmax = wvals.xmax;
  ymin = wvals.ymin;
  ymax = wvals.ymax;
  cx = wvals.cx;
  cy = wvals.cy;

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is indexed or RGB color  */
      if (gimp_drawable_is_rgb(drawable->id))
	{
	  gimp_progress_init (_("Rendering Fractal..."));

	  /* Set the tile cache size */
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width() + 1));
	  /* Run! */

	  explorer (drawable);
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /* Store data */
	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_fractalexplorer",
			   &wvals, sizeof (explorer_vals_t));
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/**********************************************************************
 FUNCTION: explorer
 *********************************************************************/

static void
explorer (GimpDrawable * drawable)
{
  GimpPixelRgn   srcPR;
  GimpPixelRgn   destPR;
  gint        width;
  gint        height;
  gint        bytes;
  gint        row;
  gint        x1;
  gint        y1;
  gint        x2;
  gint        y2;
  guchar     *src_row;
  guchar     *dest_row;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  /*  allocate row buffers  */
  src_row = (guchar *) g_malloc((x2 - x1) * bytes);
  dest_row = (guchar *) g_malloc((x2 - x1) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  xbild = width;
  ybild = height;
  xdiff = (xmax - xmin) / xbild;
  ydiff = (ymax - ymin) / ybild;

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, (x2 - x1));

      explorer_render_row (src_row,
			   dest_row,
			   row,
			   (x2 - x1),
			   bytes);

      /*  store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, row, (x2 - x1));

      if ((row % 10) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  /*  update the processed region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  g_free (src_row);
  g_free (dest_row);
}

/**********************************************************************
 FUNCTION: explorer_render_row
 *********************************************************************/

static void
explorer_render_row (const guchar *src_row,
		     guchar       *dest_row,
		     gint          row,
		     gint          row_width,
		     gint          bytes)
{
  gint    col;
  gint    bytenum;
  gdouble a;
  gdouble b;
  gdouble x;
  gdouble y;
  gdouble oldx;
  gdouble oldy;
  gdouble tempsqrx;
  gdouble tempsqry;
  gdouble tmpx = 0;
  gdouble tmpy = 0;
  gdouble foldxinitx;
  gdouble foldxinity;
  gdouble foldyinitx;
  gdouble foldyinity;
  gdouble xx = 0;
  gdouble adjust;
  gdouble cx;
  gdouble cy;
  gint    zaehler;
  gint    color;
  gint    iteration;
  gint    useloglog;

  cx = wvals.cx;
  cy = wvals.cy;
  useloglog = wvals.useloglog;
  iteration = wvals.iter;

  for (col = 0; col < row_width; col++)
    {
      a = xmin + (double) col *xdiff;
      b = ymin + (double) row *ydiff;
      if (wvals.fractaltype!=0)
	{
	  tmpx = x = a;
	  tmpy = y = b;
	}
      else
	{
	  x = 0;
	  y = 0;
	}
      for (zaehler = 0;
	   (zaehler < iteration) && ((x * x + y * y) < 4);
	   zaehler++)
	{
	  oldx=x;
	  oldy=y;
	  if (wvals.fractaltype == 0)
	    {
	      /*Mandelbrot*/
	      xx = x * x - y * y + a;
	      y = 2.0 * x * y + b;
	    }
	  else if (wvals.fractaltype == 1)
	    {
	      /* Julia */
	      xx = x * x - y * y + cx;
	      y = 2.0 * x * y + cy; 
	    }
	  else if (wvals.fractaltype == 2)
	    {
	      /* Some code taken from X-Fractint */
	      /* Barnsley M1 */
	      foldxinitx = oldx * cx;
	      foldyinity = oldy * cy;
	      foldxinity = oldx * cy;
	      foldyinitx = oldy * cx;
	      /* orbit calculation */
	      if (oldx >= 0)
		{
		  xx = (foldxinitx - cx - foldyinity);
		  y = (foldyinitx - cy + foldxinity);
		}
	      else
		{
		  xx = (foldxinitx + cx - foldyinity);
		  y = (foldyinitx + cy + foldxinity);
		}
	    }
	  else if (wvals.fractaltype == 3)
	    {
	      /* Barnsley Unnamed */
	      foldxinitx = oldx * cx;
	      foldyinity = oldy * cy;
	      foldxinity = oldx * cy;
	      foldyinitx = oldy * cx;
	      /* orbit calculation */
	      if (foldxinity + foldyinitx >= 0)
		{
		  xx = foldxinitx - cx - foldyinity;
		  y = foldyinitx - cy + foldxinity;
		}
	      else
		{
		  xx = foldxinitx + cx - foldyinity;
		  y = foldyinitx + cy + foldxinity;
		}
	    }
	  else if (wvals.fractaltype == 4)
	    {
	      /*Barnsley 1*/
	      foldxinitx  = oldx * oldx;
	      foldyinity  = oldy * oldy;
	      foldxinity  = oldx * oldy;
	      /* orbit calculation */
	      if (oldx > 0)
		{
		  xx = foldxinitx - foldyinity - 1.0;
		  y = foldxinity * 2;
		}
	      else
		{
		  xx = foldxinitx - foldyinity -1.0 + cx * oldx;
		  y = foldxinity * 2;
		  y += cy * oldx;
		}
	    }
	  else if (wvals.fractaltype == 5)
	    {
	      /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
	      xx = x*x - y*y + tmpx + cx;
	      y = 2 * oldx * oldy + tmpy +cy;
	      tmpx = tmpx/2 + xx;
	      tmpy = tmpy/2 + y;
	    }
	  else if (wvals.fractaltype == 6)
	    {
	      /* ManOWarfpFractal() */
	      xx = x*x - y*y + tmpx + cx;
	      y = 2.0 * x * y + tmpy + cy;
	      tmpx = oldx;
	      tmpy = oldy;
	    }
	  else if (wvals.fractaltype == 7)
	    {
	      /* Lambda */
	      tempsqrx=x*x;
	      tempsqry=y*y;
	      tempsqrx = oldx - tempsqrx + tempsqry;
	      tempsqry = -(oldy * oldx);
	      tempsqry += tempsqry + oldy;
	      xx = cx * tempsqrx - cy * tempsqry;
	      y = cx * tempsqry + cy * tempsqrx;
	    }
	  else if (wvals.fractaltype == 8)
	    {
	      /* Sierpinski */
	      xx = oldx + oldx;
	      y = oldy + oldy;
	      if(oldy > .5)
		y = y - 1;
	      else if (oldx > .5)
		xx = xx - 1;
	    }
	  x = xx;
	}

      if (useloglog)
	{
	  adjust = log (log (x * x + y * y) / 2) / log (2);
	}
      else
	{
	  adjust = 0.0;
	}

      color = (int) (((zaehler - adjust) * (wvals.ncolors - 1)) / iteration);
      dest_row[col * bytes] = colormap[color][0];
      dest_row[col * bytes + 1] = colormap[color][1];
      dest_row[col * bytes + 2] = colormap[color][2];

      if (bytes > 3)
	for (bytenum = 3; bytenum < bytes; bytenum++)
	  {
	    dest_row[col * bytes + bytenum] = src_row[col * bytes + bytenum];
	  }
    }
}

static void
delete_dialog_callback (GtkWidget *widget,
			gboolean   delete,
			gpointer   data)
{
  gint       pos;
  GList     *sellist;
  fractalexplorerOBJ *sel_obj;
  GtkWidget *list = (GtkWidget *) data;

  if (delete)
    {
      /* Must update which object we are editing */
      /* Get the list and which item is selected */
      /* Only allow single selections */

      sellist = GTK_LIST(list)->selection; 
      
      /* g_print ("list: %i\n", g_list_length (sellist)); */
      
      sel_obj = (fractalexplorerOBJ *)
	gtk_object_get_user_data (GTK_OBJECT(sellist->data));

      pos = gtk_list_child_position (GTK_LIST (fractalexplorer_gtk_list),
				     sellist->data);

      /* Delete the current  item + asssociated file */
      gtk_list_clear_items (GTK_LIST (fractalexplorer_gtk_list), pos, pos + 1);
      /* Shadow copy for ordering info */
      fractalexplorer_list = g_list_remove (fractalexplorer_list, sel_obj);
      /*
	if(sel_obj == current_obj)
	{
	clear_undo();
	}
      */  
      /* Free current obj */
      fractalexplorer_free_everything (sel_obj);

      /* Select previous one */
      if (pos > 0)
	pos--;
      
      if ((pos == 0) && (g_list_length (fractalexplorer_list) == 0))
	{
	  /*gtk_widget_sed_sensitive ();*/
	  /* Warning - we have a problem here
	   * since we are not really "creating an entry"
	   * why call fractalexplorer_new?
	   */
	  new_button_press(NULL,NULL,NULL);
	}

      gtk_widget_set_sensitive (delete_frame_to_freeze, TRUE);

      gtk_list_select_item (GTK_LIST (fractalexplorer_gtk_list), pos);

      current_obj = g_list_nth_data (fractalexplorer_list, pos);

      list_button_update(current_obj);
    }
  else
    {
      gtk_widget_set_sensitive (delete_frame_to_freeze, TRUE);
    }

  delete_dialog = NULL;
  
  return;
}

static gint
delete_fractal_callback (GtkWidget *widget,
			 gpointer   data)
{
  gchar     *str;
  GtkWidget *list = (GtkWidget *) data;
  GList     *sellist;
  fractalexplorerOBJ * sel_obj;

  if (delete_dialog)
    return FALSE;

  sellist = GTK_LIST(list)->selection; 

  sel_obj = (fractalexplorerOBJ *)
    gtk_object_get_user_data (GTK_OBJECT ((GtkWidget *) (sellist->data)));

  str = g_strdup_printf (_("Are you sure you want to delete\n"
			   "\"%s\" from the list and from disk?"), 
			 sel_obj->draw_name);
  
  delete_dialog = gimp_query_boolean_box (_("Delete Fractal"),
					  gimp_standard_help_func, 
					  "filters/fractalexplorer.html", 
					  FALSE,
					  str, 
					  _("Delete"), _("Cancel"),
					  GTK_OBJECT (widget), "destroy",
					  delete_dialog_callback,
					  data);
  g_free (str);

  gtk_widget_set_sensitive (GTK_WIDGET (delete_frame_to_freeze), FALSE);
  gtk_widget_show (delete_dialog);

  return FALSE;
} 

static void
fractalexplorer_list_ok_callback (GtkWidget *widget,
				  gpointer   data)
{
  fractalexplorerListOptions *options;
  GtkWidget *list;
  gint pos;

  options = (fractalexplorerListOptions *) data;
  list = options->list_entry;

  /*  Set the new layer name  */
  if (options->obj->draw_name)
    {
      g_free(options->obj->draw_name);
    }
  options->obj->draw_name =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  /* Need to reorder the list */
  pos = gtk_list_child_position (GTK_LIST (fractalexplorer_gtk_list), list);

  gtk_list_clear_items (GTK_LIST (fractalexplorer_gtk_list), pos, pos + 1);

  /* remove/Add again */
  fractalexplorer_list = g_list_remove (fractalexplorer_list, options->obj);
  fractalexplorer_list_add (options->obj);

  options->obj->obj_status |= fractalexplorer_MODIFIED;

  gtk_widget_destroy (options->query_box);
  g_free (options);

}

static void
fractalexplorer_list_cancel_callback (GtkWidget *widget,
				      gpointer   data)
{
  fractalexplorerListOptions *options;

  options = (fractalexplorerListOptions *) data;
  if(options->created)
    {
      /* We are creating an entry so if cancelled
       * must del the list item as well
       */
      delete_dialog_callback (widget, TRUE, fractalexplorer_gtk_list);
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
fractalexplorer_dialog_edit_list (GtkWidget          *lwidget,
				  fractalexplorerOBJ *obj,
				  gint                created)
{
  fractalexplorerListOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;

  /*  the new options structure  */
  options = g_new (fractalexplorerListOptions, 1);
  options->list_entry = lwidget;
  options->obj = obj;
  options->created = created;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (options->query_box), _("Edit fractal name"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Fractal name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),obj->draw_name);
		      
  gtk_widget_show (options->name_entry);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("OK"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_list_ok_callback,
                      options);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_list_cancel_callback,
                      options);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

static GtkWidget *
new_fractalexplorer_obj (gchar *name)
{
  fractalexplorerOBJ *fractalexplorer;
  GtkWidget *new_list_item;

  /* Create a new entry */
  fractalexplorer = fractalexplorer_new ();

  if (!name)
    name = _("New Fractal");

  fractalexplorer->draw_name = g_strdup(name);

  /* Leave options as before */
  pic_obj = current_obj = fractalexplorer;
  
  new_list_item = fractalexplorer_list_add(fractalexplorer);

  /*  obj_creating = tmp_line = NULL; */

  /* Redraw areas */
  /*  update_draw_area(fractalexplorer_preview,NULL); */
  list_button_update (fractalexplorer);
  return new_list_item;
}

static gint
new_button_press (GtkWidget      *widget,
		  GdkEventButton *event,
		  gpointer        data)
{
  GtkWidget * new_list_item;
  
  new_list_item = new_fractalexplorer_obj((gchar*)data);
  fractalexplorer_dialog_edit_list(new_list_item,current_obj,TRUE);

  return(FALSE);
}

/*
 * Load all fractalexplorer, which are founded in fractalexplorer-path-list, into fractalexplorer_list.
 * fractalexplorer-path-list must be initialized first. (plug_in_parse_fractalexplorer_path ())
 * based on code from Gflare.
 */

static gint
fractalexplorer_list_pos (fractalexplorerOBJ *fractalexplorer)
{
  fractalexplorerOBJ *g;
  gint n;
  GList *tmp;

  n = 0;

  for (tmp = fractalexplorer_list; tmp; tmp = g_list_next (tmp)) 
    {
      g = tmp->data;
      
      if (strcmp (fractalexplorer->draw_name, g->draw_name) <= 0)
	break;

      n++;
    }

  return n;
}

static gint
fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer)
{
  gint n;

  /*
   *	Insert fractalexplorers in alphabetical order
   */
  n = fractalexplorer_list_pos (fractalexplorer);

  fractalexplorer_list = g_list_insert (fractalexplorer_list,
					fractalexplorer, n);

  return n;
}

GtkWidget *
fractalexplorer_list_item_new_with_label_and_pixmap (fractalexplorerOBJ *obj,
						     gchar *label,
						     GtkWidget *pix_widget)
{
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *alignment;
  GtkWidget *hbox;

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (hbox);

  list_item = gtk_list_item_new ();
  label_widget = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.5);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 0);
  gtk_widget_show(alignment);

  gtk_box_pack_start(GTK_BOX(hbox),pix_widget,FALSE,FALSE,0);
  gtk_container_add (GTK_CONTAINER (hbox), label_widget);
  gtk_container_add (GTK_CONTAINER (list_item), hbox);

  gtk_widget_show (obj->label_widget = label_widget);
  gtk_widget_show (obj->pixmap_widget = pix_widget);
  gtk_widget_show (obj->list_item = list_item);

  return list_item;
}

GtkWidget *
fractalexplorer_new_pixmap (GtkWidget  *list,
			    gchar     **pixdata)
{
  GtkWidget *pixmap_widget;
  GdkPixmap *pixmap;
  GdkColor transparent;
  GdkBitmap *mask=NULL;
  GdkColormap *colormap;

  colormap = gtk_widget_get_default_colormap ();

  pixmap = gdk_pixmap_colormap_create_from_xpm_d (list->window,
						  colormap,
						  &mask,
						  &transparent,
						  pixdata);

  pixmap_widget = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (pixmap_widget); 

  return pixmap_widget;
}

GtkWidget *
fractalexplorer_list_add (fractalexplorerOBJ *obj)
{
  GList *list;
  gint pos;
  GtkWidget *list_item;
  GtkWidget *list_pix;

  list_pix = fractalexplorer_new_pixmap (fractalexplorer_gtk_list, Floppy6_xpm);
  list_item =
    fractalexplorer_list_item_new_with_label_and_pixmap (obj,
							 obj->draw_name,
							 list_pix);      

  gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer)obj);

  pos = fractalexplorer_list_insert (obj);

  list = g_list_append (NULL, list_item);
  gtk_list_insert_items (GTK_LIST (fractalexplorer_gtk_list), list, pos);
  gtk_widget_show (list_item);
  gtk_list_select_item (GTK_LIST (fractalexplorer_gtk_list), pos);  

  gtk_signal_connect (GTK_OBJECT (list_item), "button_press_event",
		      GTK_SIGNAL_FUNC (list_button_press),
		      obj);

  return list_item;
}

static void
list_button_update (fractalexplorerOBJ *obj)
{
  g_return_if_fail (obj != NULL);
  pic_obj = (fractalexplorerOBJ *)obj;
}

fractalexplorerOBJ *
fractalexplorer_new (void)
{
  fractalexplorerOBJ * new;

  new = g_new0 (fractalexplorerOBJ, 1);

  return new;
}

void
build_list_items (GtkWidget *list)
{
  GList *tmp = fractalexplorer_list;
  GtkWidget *list_item;
  GtkWidget *list_pix;
  fractalexplorerOBJ *g;

  while (tmp)
    {
      g = tmp->data;

      if (g->obj_status & fractalexplorer_READONLY)
	list_pix = fractalexplorer_new_pixmap(list,mini_cross_xpm);
      else
	list_pix = fractalexplorer_new_pixmap(list,bluedot_xpm);

      list_item =
	fractalexplorer_list_item_new_with_label_and_pixmap
	(g, g->draw_name,list_pix);      
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) g);
      gtk_list_append_items (GTK_LIST (list), g_list_append(NULL,list_item));

      gtk_signal_connect (GTK_OBJECT (list_item), "button_press_event",
			  GTK_SIGNAL_FUNC (list_button_press),
			  (gpointer) g);
      gtk_widget_show (list_item);

      tmp = tmp->next;
    }
}

static gint
list_button_press (GtkWidget      *widget,
		   GdkEventButton *event,
		   gpointer        data)
{

  fractalexplorerOBJ * sel_obj;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      list_button_update ((fractalexplorerOBJ *) data);
      break;

    case GDK_2BUTTON_PRESS:
      sel_obj = (fractalexplorerOBJ *)data;

      if (sel_obj)
	{
	  current_obj = sel_obj;
	  wvals = current_obj->opts;
	  dialog_change_scale ();
	  set_cmap_preview ();
	  dialog_update_preview ();
	}
      else
	{
	  g_warning ("Internal error - list item has null object!");
	}
      break;

    default:
      break;
    }

  return FALSE;
}

/*
 *	Query gimprc for fractalexplorer-path, and parse it.
 *	This code is based on script_fu_find_scripts ()
 *      and the Gflare plugin.
 */
void
plug_in_parse_fractalexplorer_path (void)
{
  GList *fail_list = NULL;
  GList *list;
  gchar *fractalexplorer_path;

  gimp_path_free (fractalexplorer_path_list);
  fractalexplorer_path_list = NULL;

  fractalexplorer_path = gimp_gimprc_query ("fractalexplorer-path");
  
  if (!fractalexplorer_path)
    {
      gchar *gimprc = gimp_personal_rc_file ("gimprc");
      gchar *path = gimp_strescape
	("${gimp_dir}" G_DIR_SEPARATOR_S "fractalexplorer"
	 G_SEARCHPATH_SEPARATOR_S
	 "${gimp_data_dir}" G_DIR_SEPARATOR_S "fractalexplorer",
	 NULL);
      g_message (_("No fractalexplorer-path in gimprc:\n"
		   "You need to add an entry like\n"
		   "(fractalexplorer-path \"%s\")\n"
		   "to your %s file."), path, gimprc);
      g_free (gimprc);
      g_free (path);
      return;
    }

  fractalexplorer_path_list = gimp_path_parse (fractalexplorer_path,
					       16, TRUE, &fail_list);

  g_free (fractalexplorer_path);

  if (fail_list)
    {
      GString *err =
        g_string_new (_("fractalexplorer-path misconfigured - "
                        "the following directories were not found"));

      for (list = fail_list; list; list = g_list_next (list))
        {
          g_string_append_c (err, '\n');
          g_string_append (err, (gchar *) list->data);
        }

      g_message (err->str);

      g_string_free (err, TRUE);
      gimp_path_free (fail_list);
    }
}

static void
fractalexplorer_free (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  g_free (fractalexplorer->name);
  g_free (fractalexplorer->filename);
  g_free (fractalexplorer->draw_name);
  g_free (fractalexplorer);
}

static void
fractalexplorer_free_everything (fractalexplorerOBJ *fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  if(fractalexplorer->filename)
    {
      remove (fractalexplorer->filename);
    }
  fractalexplorer_free (fractalexplorer);
}

static void
fractalexplorer_list_free_all (void)
{
  GList * list;
  fractalexplorerOBJ * fractalexplorer;

  for (list = fractalexplorer_list; list; list = g_list_next (list))
    {
      fractalexplorer = (fractalexplorerOBJ *) list->data;
      fractalexplorer_free (fractalexplorer);
    }

  g_list_free (fractalexplorer_list);
  fractalexplorer_list = NULL;
}

fractalexplorerOBJ *
fractalexplorer_load (gchar *filename,
		      gchar *name)
{
  fractalexplorerOBJ * fractalexplorer;
  FILE * fp;
  gchar load_buf[MAX_LOAD_LINE];

  g_assert (filename != NULL);
  fp = fopen (filename, "rt");
  if (!fp)
    {
      g_warning ("Error opening: %s", filename);
      return NULL;
    }

  fractalexplorer = fractalexplorer_new ();

  fractalexplorer->name = g_strdup (name);
  fractalexplorer->draw_name = g_strdup (name);
  fractalexplorer->filename = g_strdup (filename);

  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (fractalexplorer_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("File '%s' is not a FractalExplorer file"), filename);
      fclose (fp);

      return NULL;
    }

  if (load_options (fractalexplorer, fp))
    {
      g_message (_("File '%s' is corrupt.\nLine %d Option section incorrect"), filename, line_no);
      fclose (fp);

      return NULL;
    }

  fclose (fp);

  if (!pic_obj)
    pic_obj = fractalexplorer;

  fractalexplorer->obj_status = fractalexplorer_OK;

  return fractalexplorer;
}

static void
fractalexplorer_list_load_all (GList *plist)
{
  fractalexplorerOBJ  * fractalexplorer;
  GList    * list;
  gchar	   * path;
  gchar	   * filename;
  DIR	   * dir;
  struct dirent *dir_ent;
  struct stat	filestat;
  gint		err;
  gchar       pathlast;

  /*  Make sure to clear any existing fractalexplorers  */
  current_obj = pic_obj = NULL;
  fractalexplorer_list_free_all ();
  list = plist;
  while (list)
    {
      path = list->data;
      list = list->next;
      pathlast = path[strlen (path) - 1];

      /* Open directory */
      dir = opendir (path);

      if (!dir)
	g_warning ("error reading fractalexplorer directory \"%s\"", path);
      else
	{
	  while ((dir_ent = readdir (dir)))
	    {
	      filename = g_strdup_printf
		("%s%s%s", path,
		 (pathlast == G_DIR_SEPARATOR ? "" : G_DIR_SEPARATOR_S),
		 dir_ent->d_name);

	      /* Check the file and see that it is not a sub-directory */
	      err = stat (filename, &filestat);

	      if (!err && S_ISREG (filestat.st_mode))
		{

		  fractalexplorer = fractalexplorer_load (filename, dir_ent->d_name);
		  
		  if (fractalexplorer)
		    {
		      /* Read only ?*/
		      if(access(filename,W_OK))
			fractalexplorer->obj_status |= fractalexplorer_READONLY;

		      fractalexplorer_list_insert (fractalexplorer);
		    }
		}

	      g_free (filename);
	    } /* while */
	  closedir (dir);
	} /* else */
    }

  if(!fractalexplorer_list)
    {
      /* lets have at least one! */
      fractalexplorer = fractalexplorer_new();
      fractalexplorer->draw_name = g_strdup(_("My first fractal"));
      fractalexplorer_list_insert(fractalexplorer);
    }
  pic_obj = current_obj = fractalexplorer_list->data;  /* set to first entry */

}

GtkWidget *
add_objects_list (void)
{
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *list_frame;
  GtkWidget *scrolled_win;
  GtkWidget *list;
  GtkWidget *button;

  frame = gtk_frame_new (_("Choose Fractal by double-clicking on it"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  delete_frame_to_freeze = list_frame = gtk_frame_new (NULL);
  gtk_table_attach (GTK_TABLE (table), list_frame, 0, 2, 0, 1,
		    GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (list_frame);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (list_frame), scrolled_win);
  gtk_widget_show (scrolled_win);

  fractalexplorer_gtk_list = list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 list);
  gtk_widget_show (list);

  fractalexplorer_list_load_all (fractalexplorer_path_list);
  build_list_items (list);

  /* Put buttons in */
  button = gtk_button_new_with_label (_("Rescan"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (fractalexplorer_rescan_list),
		      NULL);
  gimp_help_set_help_data (button,
			   _("Select directory and rescan collection"), NULL); 
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Delete"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (delete_fractal_callback),
		      (gpointer) list);
  gtk_widget_show (button);
  gimp_help_set_help_data (button,
			   _("Delete currently selected fractal"), NULL); 

  return frame;
}

static void
fractalexplorer_rescan_ok_callback (GtkWidget *widget,
				    gpointer   data)
{
  GtkWidget *patheditor;
  gchar     *raw_path;

  gtk_widget_set_sensitive (GTK_WIDGET (data), FALSE);

  gimp_path_free (fractalexplorer_path_list);
  fractalexplorer_path_list = NULL;

  patheditor = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (data),
                                                "patheditor"));

  raw_path = gimp_path_editor_get_path (GIMP_PATH_EDITOR (patheditor));

  fractalexplorer_path_list = gimp_path_parse (raw_path, 16, FALSE, NULL);

  g_free (raw_path);

  if (fractalexplorer_path_list)
    {
      gtk_list_clear_items (GTK_LIST (fractalexplorer_gtk_list), 0, -1);
      fractalexplorer_list_load_all (fractalexplorer_path_list);
      build_list_items (fractalexplorer_gtk_list);
      list_button_update (current_obj);
    }

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
fractalexplorer_rescan_list (void)
{
  static GtkWidget *dlg = NULL;

  GtkWidget *patheditor;
  gchar     *path;

  if (dlg)
    {
      gdk_window_raise (dlg->window);
      return;
    }

  /*  the dialog  */
  dlg = gimp_dialog_new (_("Rescan for Fractals"), "fractalexplorer",
                         gimp_standard_help_func, "filters/fractalexplorer.html",
                         GTK_WIN_POS_MOUSE,
                         FALSE, TRUE, FALSE,

                         _("OK"), fractalexplorer_rescan_ok_callback,
                         NULL, NULL, NULL, TRUE, FALSE,
                         _("Cancel"), gtk_widget_destroy,
                         NULL, 1, NULL, FALSE, TRUE,

                         NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
                      &dlg);

  path = gimp_path_to_str (fractalexplorer_path_list);

  patheditor = gimp_path_editor_new (_("Add FractalExplorer Path"), path);
  gtk_container_set_border_width (GTK_CONTAINER (patheditor), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), patheditor,
                      TRUE, TRUE, 0);
  gtk_widget_show (patheditor);

  g_free (path);

  gtk_object_set_data (GTK_OBJECT (dlg), "patheditor", patheditor);

  gtk_widget_show (dlg);
}
