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

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "pix_data.h"

#include "Languages.h"
#include "FractalExplorer.h"
#include "Events.h"
#include "Callbacks.h"
#include "Dialogs.h"

/**********************************************************************
 MAIN()  
 *********************************************************************/

MAIN()

/**********************************************************************
 FUNCTION: query  
 *********************************************************************/

void
query()
{
    static GParamDef    args[] =
    {
	{PARAM_INT32, "run_mode", "Interactive, non-interactive"},
	{PARAM_IMAGE, "image", "Input image"},
	{PARAM_DRAWABLE, "drawable", "Input drawable"},
	{PARAM_INT8, "fractaltype", "0: Mandelbrot; 1: Julia; 2: Barnsley 1; 3: Barnsley 2; 4: Barnsley 3; 5: Spider; 6: ManOWar; 7: Lambda; 8: Sierpinski"},
	{PARAM_FLOAT, "xmin", "xmin fractal image delimiter"},
	{PARAM_FLOAT, "xmax", "xmax fractal image delimiter"},
	{PARAM_FLOAT, "ymin", "ymin fractal image delimiter"},
	{PARAM_FLOAT, "ymax", "ymax fractal image delimiter"},
	{PARAM_FLOAT, "iter", "Iteration value"},
	{PARAM_FLOAT, "cx", "cx value ( only Julia)"},
	{PARAM_FLOAT, "cy", "cy value ( only Julia)"},
	{PARAM_INT8, "colormode", "0: Apply colormap as specified by the parameters below; 1: Apply active gradient to final image"},
	{PARAM_FLOAT, "redstretch", "Red stretching factor"},
	{PARAM_FLOAT, "greenstretch", "Green stretching factor"},
	{PARAM_FLOAT, "bluestretch", "Blue stretching factor"},
	{PARAM_INT8, "redmode", "Red application mode (0:SIN;1:COS;2:NONE)"},
        {PARAM_INT8, "greenmode", "Green application mode (0:SIN;1:COS;2:NONE)"},
	{PARAM_INT8, "bluemode", "Blue application mode (0:SIN;1:COS;2:NONE)"},
	{PARAM_INT8, "redinvert", "Red inversion mode (1: enabled; 0: disabled)"},
	{PARAM_INT8, "greeninvert", "Green inversion mode (1: enabled; 0: disabled)"},
	{PARAM_INT8, "blueinvert", "Green inversion mode (1: enabled; 0: disabled)"},
    };
    static GParamDef   *return_vals = NULL;
    static int          nargs = sizeof(args) / sizeof(args[0]);
    static int          nreturn_vals = 0;

    gimp_install_procedure("plug_in_fractalexplorer",
			   "Chaos Fractal Explorer Plug-In",
			   "No help yet.",
	                   "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
			   "Daniel Cotting (cotting@multimania.com, www.multimania.com/cotting)",
			   "December, 1998",
			   "<Image>/Filters/Render/Fractal Explorer",
			   "RGB*",
			   PROC_PLUG_IN,
			   nargs, nreturn_vals,
			   args, return_vals);
}

/**********************************************************************
 FUNCTION: run
 *********************************************************************/

void
run(char *name,
    int nparams,
    GParam * param,
    int *nreturn_vals,
    GParam ** return_vals)
{
    static GParam       values[1];
    gint32              image_ID;
    GRunModeType        run_mode;
    double              xhsiz,
                        yhsiz;
    int                 pwidth,
                        pheight;
    GStatusType         status = STATUS_SUCCESS;
    FILE * fp;
    gchar * filname=NULL;
    gchar load_buf[MAX_LOAD_LINE];

    run_mode = param[0].data.d_int32;

    values[0].type = PARAM_STATUS;
    values[0].data.d_status = status;

    *nreturn_vals = 1;
    *return_vals = values;

  /*  Get the specified drawable  */
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    image_ID = param[1].data.d_image;
    tile_width = gimp_tile_width();
    tile_height = gimp_tile_height();

    img_width = gimp_drawable_width(drawable->id);
    img_height = gimp_drawable_height(drawable->id);
    img_bpp = gimp_drawable_bpp(drawable->id);

    gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

    sel_width = sel_x2 - sel_x1;
    sel_height = sel_y2 - sel_y1;

    cen_x = (double) (sel_x2 - 1 + sel_x1) / 2.0;
    cen_y = (double) (sel_y2 - 1 + sel_y1) / 2.0;

    xhsiz = (double) (sel_width - 1) / 2.0;
    yhsiz = (double) (sel_height - 1) / 2.0;

  /* Calculate preview size */
    if (sel_width > sel_height) {
	pwidth = MIN(sel_width, PREVIEW_SIZE);
	pheight = sel_height * pwidth / sel_width;
    } else {
	pheight = MIN(sel_height, PREVIEW_SIZE);
	pwidth = sel_width * pheight / sel_height;
    }				/* else */

    preview_width = MAX(pwidth, 2);	/* Min size is 2 */
    preview_height = MAX(pheight, 2);

  /* See how we will run */
    switch (run_mode) {
    case RUN_INTERACTIVE:
      /* Possibly retrieve data */

	gimp_get_data("plug_in_fractalexplorer", &wvals);
	
	lng=0;

	filname = g_strconcat (gimp_directory (),
			       G_DIR_SEPARATOR_S,
			       "fractalexplorerrc",
			       NULL);
        fp = fopen (filname, "r");
        if (!fp)
            {
              fp = fopen (filname, "w");
              if (fp) fputs("FX-LANG:En\n",fp);
            }
	    else
	    {
	    	fgets(load_buf, MAX_LOAD_LINE, fp);
                if (strlen(load_buf) > 0) load_buf[strlen(load_buf) - 1] = '\0';
              if(strncmp("FX-LANG:En",load_buf,strlen(load_buf))==0)
               { lng=0; }
              if(strncmp("FX-LANG:Fr",load_buf,strlen(load_buf))==0)
               { lng=1; }
              if(strncmp("FX-LANG:De",load_buf,strlen(load_buf))==0)
               { lng=2; }
	    }   
        fclose(fp);
	
        wvals.language=lng;
	do_english = (wvals.language == 0);
        do_french = (wvals.language == 1);
        do_german = (wvals.language == 2);

      /* Get information from the dialog */

	if (!explorer_dialog())
	    return;

	break;

    case RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */

	if (nparams != 22)
	    status = STATUS_CALLING_ERROR;

	if (status == STATUS_SUCCESS) {
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

    case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */

	gimp_get_data("plug_in_fractalexplorer", &wvals);
	make_color_map();
	break;

    default:
	break;
    }				/* switch */

    xmin = wvals.xmin;
    xmax = wvals.xmax;
    ymin = wvals.ymin;
    ymax = wvals.ymax;
    cx = wvals.cx;
    cy = wvals.cy;

    if (status == STATUS_SUCCESS) {
      /*  Make sure that the drawable is indexed or RGB color  */
	if (gimp_drawable_is_rgb(drawable->id)) {
	    gimp_progress_init("Rendering fractal...");

	  /* Set the tile cache size */
	    gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));
	  /* Run! */

	    explorer(drawable);
	    if (run_mode != RUN_NONINTERACTIVE)
		gimp_displays_flush();

	  /* Store data */

	    if (run_mode == RUN_INTERACTIVE)
		gimp_set_data("plug_in_fractalexplorer", &wvals, sizeof(explorer_vals_t));
	} else {
	    status = STATUS_EXECUTION_ERROR;
	}
    }
    values[0].data.d_status = status;

    gimp_drawable_detach(drawable);

}

/**********************************************************************
 FUNCTION: explorer
 *********************************************************************/

void
explorer(GDrawable * drawable)
{
    GPixelRgn           srcPR,
                        destPR;
    gint                width,
                        height,
                    bytes,
                    row,
                    x1,
                        y1,
                        x2,
                        y2;
    guchar             *src_row,
                  *dest_row;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
    gimp_drawable_mask_bounds(drawable->id, &x1, &y1, &x2, &y2);

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
    gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
    gimp_pixel_rgn_init(&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

    xbild = width;
    ybild = height;
    xdiff = (xmax - xmin) / xbild;
    ydiff = (ymax - ymin) / ybild;

    for (row = y1; row < y2; row++) {
	gimp_pixel_rgn_get_row(&srcPR, src_row, x1, row, (x2 - x1));

	explorer_render_row(src_row,
			    dest_row,
			    row,
			    (x2 - x1),
			    bytes);

      /*  store the dest  */
	gimp_pixel_rgn_set_row(&destPR, dest_row, x1, row, (x2 - x1));

	if ((row % 10) == 0)
	    gimp_progress_update((double) row / (double) (y2 - y1));
    }

  /*  update the processed region  */
    gimp_drawable_flush(drawable);
    gimp_drawable_merge_shadow(drawable->id, TRUE);
    gimp_drawable_update(drawable->id, x1, y1, (x2 - x1), (y2 - y1));

    g_free(src_row);
    g_free(dest_row);
}

/**********************************************************************
 FUNCTION: explorer_render_row
 *********************************************************************/

void
explorer_render_row(const guchar * src_row,
		    guchar * dest_row,
		    gint row,
		    gint row_width,
		    gint bytes)
{
    gint                col,
                        bytenum;
    double              a,
                        b,
                        x,
                        y,
			oldx,
			oldy,
			tempsqrx,
			tempsqry,
			tmpx=0,
			tmpy=0,
			foldxinitx,
			foldxinity,
			foldyinitx,
			foldyinity,
                        xx=0,
                        adjust,
                        cx,
                        cy;
    int                 zaehler,
                        color,
                        iteration;
    int                 useloglog;
    cx = wvals.cx;
    cy = wvals.cy;
    useloglog = wvals.useloglog;
    iteration = wvals.iter;
    for (col = 0; col < row_width; col++) {
	a = xmin + (double) col *xdiff;
	b = ymin + (double) row *ydiff;
	if (wvals.fractaltype!=0) {
	    tmpx = x = a;
	    tmpy = y = b;
	} else {
	    x = 0;
	    y = 0;
	}
	for (zaehler = 0; (zaehler < iteration) && ((x * x + y * y) < 4); zaehler++) {
	    oldx=x; oldy=y;
		    if (wvals.fractaltype==1) {
		        /* Julia */
			xx = x * x - y * y + cx;
			y = 2.0 * x * y + cy; 
		    } else if (wvals.fractaltype==0) {
		        /*Mandelbrot*/
		        xx = x * x - y * y + a;
			y = 2.0 * x * y + b;
		    } else if (wvals.fractaltype==2) {
/* Some code taken from X-Fractint */
/* Barnsley M1 */
		    	foldxinitx = oldx * cx;
                        foldyinity = oldy * cy;
                        foldxinity = oldx * cy;
                        foldyinitx = oldy * cx;
                        /* orbit calculation */
                       if(oldx >= 0)
		       {
		          xx = (foldxinitx - cx - foldyinity);
                          y = (foldyinitx - cy + foldxinity);
		       }
		       else
		       {
		          xx = (foldxinitx + cx - foldyinity);
		          y = (foldyinitx + cy + foldxinity);
		       }
		    } else if (wvals.fractaltype==3) {
/* Barnsley Unnamed */
		       
			foldxinitx = oldx * cx;
                        foldyinity = oldy * cy;
                        foldxinity = oldx * cy;
                        foldyinitx = oldy * cx;
                        /* orbit calculation */
                        if(foldxinity + foldyinitx >= 0)
                        {
                           xx = foldxinitx - cx - foldyinity;
                           y = foldyinitx - cy + foldxinity;
                        }
		        else
		        {
		           xx = foldxinitx + cx - foldyinity;
                           y = foldyinitx + cy + foldxinity;
                        }
		    } else if (wvals.fractaltype==4) {
		        /*Barnsley 1*/
                        foldxinitx  = oldx * oldx;
                        foldyinity  = oldy * oldy;
                        foldxinity  = oldx * oldy;
                        /* orbit calculation */
                        if(oldx > 0)
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
		    } else if (wvals.fractaltype==5) {
			 /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
                        xx = x*x - y*y + tmpx + cx;
                        y = 2 * oldx * oldy + tmpy +cy;
                        tmpx = tmpx/2 + xx;
                        tmpy = tmpy/2 + y;
		    } else if (wvals.fractaltype==6) {
/*			ManOWarfpFractal() */
                        xx = x*x - y*y + tmpx + cx;
                        y = 2.0 * x * y + tmpy + cy;
                        tmpx = oldx;
			tmpy = oldy;
		    } else if (wvals.fractaltype==7) {
/* Lambda */
            		tempsqrx=x*x;
	           	tempsqry=y*y;
			tempsqrx = oldx - tempsqrx + tempsqry;
                        tempsqry = -(oldy * oldx);
                        tempsqry += tempsqry + oldy;
                        xx = cx * tempsqrx - cy * tempsqry;
                        y = cx * tempsqry + cy * tempsqrx;
		    } else if (wvals.fractaltype==8) {
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
	if (useloglog) {
	  adjust = log(log(x*x+y*y)/2)/log(2);
	} else {
	  adjust = 0.0;
	}
	color = (int) (((zaehler - adjust) * (wvals.ncolors - 1)) / iteration);
	dest_row[col * bytes] = colormap[color][0];
	dest_row[col * bytes + 1] = colormap[color][1];
	dest_row[col * bytes + 2] = colormap[color][2];

	if (bytes > 3)
	    for (bytenum = 3; bytenum < bytes; bytenum++) {
		dest_row[col * bytes + bytenum] = src_row[col * bytes + bytenum];
	    }
    }
}


/*

gint
rename_button_press(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer data)
{
  GtkWidget *list = (GtkWidget *)data;
  GList * sellist;
  fractalexplorerOBJ * sel_obj;

  sellist = GTK_LIST(list)->selection; 

  sel_obj = (fractalexplorerOBJ *)gtk_object_get_user_data(GTK_OBJECT((GtkWidget *)(sellist->data)));

  fractalexplorer_dialog_edit_list(widget,(gpointer) sel_obj,FALSE);
  return(FALSE);
}
*/

gint
delete_button_press_cancel(GtkWidget *widget,
		  gpointer   data)
{
  gtk_widget_destroy(delete_dialog);
  gtk_widget_set_sensitive(delete_frame_to_freeze,TRUE);
  delete_dialog = NULL;

  return(FALSE);
}

gint
fractalexplorer_delete_fractalexplorer_callback(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer   data)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  char      *str;
  GtkWidget *list = (GtkWidget *)data;
  GList * sellist;
  fractalexplorerOBJ * sel_obj;


  sellist = GTK_LIST(list)->selection; 

  sel_obj = (fractalexplorerOBJ *)gtk_object_get_user_data(GTK_OBJECT((GtkWidget *)(sellist->data)));
  if(delete_dialog)
    return(FALSE);

  delete_dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(delete_dialog), msg[lng][MSG_DELFRAC]);
  gtk_window_position(GTK_WINDOW(delete_dialog), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width(GTK_CONTAINER(delete_dialog), 0);
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(delete_dialog)->vbox), vbox,
		     FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  
  /* Question */
  
  label = gtk_label_new(msg[lng][MSG_DELSURE]);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  
  str = g_strdup_printf(msg[lng][MSG_DELSURE2], sel_obj->draw_name);
  
  label = gtk_label_new(str);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  
  g_free(str);
  
  /* Buttons */
  button = gtk_button_new_with_label (msg[lng][MSG_DEL]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) delete_button_press_ok,
                      data);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete_dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_object_set_user_data(GTK_OBJECT(button),widget);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label (msg[lng][MSG_CANCEL]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) delete_button_press_cancel,
                      data);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (delete_dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_object_set_user_data(GTK_OBJECT(button),widget);
  gtk_widget_show (button);
  
  /* Show! */
  
  gtk_widget_set_sensitive(GTK_WIDGET(delete_frame_to_freeze), FALSE);
  gtk_widget_show(delete_dialog);

  return(FALSE);
} 


void
fractalexplorer_save(void)
{
  /* Save the current object */
  if(!current_obj->filename)
   {
     create_file_selection(current_obj,NULL);
     return;
    }
  save_callback();
}


gint
gradient_list_button_press(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer   data)
{

  gradientOBJ * sel_obj;
  FILE * fp;
  gchar * filename;
  gchar load_buf[MAX_LOAD_LINE];
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if(event->button == 3)
	{
	}
      break;
    case GDK_2BUTTON_PRESS:
        sel_obj = (gradientOBJ *)data;
        if(sel_obj) {
	if (sel_obj->obj_status && gradient_GRADIENTEDITOR)
	{
	  wvals.colormode=1;
	  gimp_gradients_set_active(sel_obj->name);
          dialog_change_scale();  
          set_cmap_preview();
          dialog_update_preview(); 
	}
	else
	{
          filename=sel_obj->filename;  
          g_assert (filename != NULL);
          fp = fopen (filename, "r");
          if (!fp)
            {
              g_warning (msg[lng][MSG_OPENERROR], filename);
              return 0;
            }
          get_line(load_buf,MAX_LOAD_LINE,fp,1);
          if(strncmp(fractalexplorer_HEADER,load_buf,strlen(load_buf)))
          {
             gchar err[MAXSTRLEN];
             sprintf(err,msg[lng][MSG_WRONGFILETYPE],sel_obj->filename);
             create_warn_dialog(err);
             return(0);
          }

          if(gradient_load_options(sel_obj,fp))
            {
               gchar err[MAXSTRLEN];
               sprintf(err,msg[lng][MSG_CORRUPTFILE],
	      filename,
	      line_no);
              create_warn_dialog(err);
              return(0);
            }
          fclose(fp);
          dialog_change_scale();  
          set_cmap_preview();
          dialog_update_preview(); 
	}
	}
        else
          g_warning(msg[lng][MSG_NULLLIST]);
        break;
    default:
      printf(msg[lng][MSG_UNKNOWN_EVENT]);
      break;
    }

  return(FALSE);
}


void
fractalexplorer_list_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  fractalexplorerListOptions *options;
  GtkWidget *list;
  gint pos;

  options = (fractalexplorerListOptions *) client_data;
  list = options->list_entry;

  /*  Set the new layer name  */
  if (options->obj->draw_name)
    {
      g_free(options->obj->draw_name);
    }
  options->obj->draw_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  /* Need to reorder the list */
  /* gtk_label_set (GTK_LABEL (options->layer_widget->label), layer->name);*/

  pos = gtk_list_child_position(GTK_LIST(fractalexplorer_gtk_list),list);

  gtk_list_clear_items(GTK_LIST (fractalexplorer_gtk_list),pos,pos+1);

  /* remove/Add again */
  fractalexplorer_list = g_list_remove(fractalexplorer_list,options->obj);
  fractalexplorer_list_add(options->obj);

  options->obj->obj_status |= fractalexplorer_MODIFIED;

  gtk_widget_destroy (options->query_box);
  g_free (options);

}

void
fractalexplorer_list_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  fractalexplorerListOptions *options;

  options = (fractalexplorerListOptions *) client_data;
  if(options->created)
    {
      /* We are creating an entry so if cancelled must del the list item as well */
      delete_button_press_ok(w,fractalexplorer_gtk_list);
    }

  gtk_widget_destroy (options->query_box);
  g_free (options);
}


void
fractalexplorer_dialog_edit_list (GtkWidget *lwidget,fractalexplorerOBJ *obj,gint created)
{
  fractalexplorerListOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;

  /*  the new options structure  */
  options = (fractalexplorerListOptions *) g_malloc (sizeof (fractalexplorerListOptions));
  options->list_entry = lwidget;
  options->obj = obj;
  options->created = created;

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (options->query_box),msg[lng][MSG_EDIT_FRACNAME]);
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (msg[lng][MSG_FRACNAME]);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),obj->draw_name);
		      
  gtk_widget_show (options->name_entry);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (msg[lng][MSG_OK]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_list_ok_callback,
                      options);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (msg[lng][MSG_CANCEL]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_list_cancel_callback,
                      options);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


GtkWidget *
new_fractalexplorer_obj(gchar * name)
{
  fractalexplorerOBJ * fractalexplorer;
  GtkWidget * new_list_item;
  /* Create a new entry */

  fractalexplorer = fractalexplorer_new();

  if(!name)
    name = msg[lng][MSG_NEWFRAC];

  fractalexplorer->draw_name = g_strdup(name);

  /* Leave options as before */
  pic_obj = current_obj = fractalexplorer;
  
  new_list_item = fractalexplorer_list_add(fractalexplorer);

/*  obj_creating = tmp_line = NULL; */

  /* Redraw areas */
  /*  update_draw_area(fractalexplorer_preview,NULL); */
  list_button_update(fractalexplorer);
  return(new_list_item);
}


gint
new_button_press(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer   data)
{
  GtkWidget * new_list_item;
  
  new_list_item = new_fractalexplorer_obj((gchar*)data);
  fractalexplorer_dialog_edit_list(new_list_item,current_obj,TRUE);

  return(FALSE);
}

void
fractalexplorer_rescan_cancel_callback (GtkWidget *w,
				  gpointer   client_data)
{
  gtk_widget_destroy (GTK_WIDGET (client_data));
}

void 
clear_list_items(GtkList *list)
{
  gtk_list_clear_items(list,0,-1);
}

/*
 * Load all fractalexplorer, which are founded in fractalexplorer-path-list, into fractalexplorer_list.
 * fractalexplorer-path-list must be initialized first. (plug_in_parse_fractalexplorer_path ())
 * based on code from Gflare.
 */

gint
fractalexplorer_list_pos(fractalexplorerOBJ *fractalexplorer)
{
  fractalexplorerOBJ *g;
  int n;
  GList *tmp;

  n = 0;
  
  tmp = fractalexplorer_list;
  
  while (tmp) 
    {
      g = tmp->data;
      
      if (strcmp (fractalexplorer->draw_name, g->draw_name) <= 0)
	break;
      n++;
      tmp = tmp->next;
    }

  return(n);
}

gint
gradient_list_pos(gradientOBJ *gradi)
{
  gradientOBJ *g;
  int n;
  GList *tmp;

  n = 0;
  
  tmp = gradient_list;
  
  while (tmp) 
    {
      g = tmp->data;
      
      if (strcmp (gradi->draw_name, g->draw_name) <= 0)
	break;
      n++;
      tmp = tmp->next;
    }

  return(n);
}


gint
fractalexplorer_list_insert (fractalexplorerOBJ *fractalexplorer)
{
  int		n;

  /*
   *	Insert fractalexplorers in alphabetical order
   */

  n = fractalexplorer_list_pos(fractalexplorer);

  fractalexplorer_list = g_list_insert (fractalexplorer_list, fractalexplorer, n);

  return n;
}

gint
gradient_list_insert (gradientOBJ *gradi)
{
  int		n;

  /*
   *	Insert gradis in alphabetical order
   */

  n = gradient_list_pos(gradi);

  gradient_list = g_list_insert (gradient_list, gradi, n);

  return n;
}

GtkWidget*
fractalexplorer_list_item_new_with_label_and_pixmap (fractalexplorerOBJ *obj, gchar *label, GtkWidget *pix_widget)
{
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *alignment;
  GtkWidget *hbox;

  hbox = gtk_hbox_new(FALSE,1);
  gtk_widget_show(hbox);

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

GtkWidget*
gradient_list_item_new_with_label_and_pixmap (gradientOBJ *obj, gchar *label, GtkWidget *pix_widget)
{
  GtkWidget *list_item;
  GtkWidget *label_widget;
  GtkWidget *alignment;
  GtkWidget *hbox;

  hbox = gtk_hbox_new(FALSE,1);
  gtk_widget_show(hbox);

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


GtkWidget*
fractalexplorer_new_pixmap(GtkWidget *list, char **pixdata)
{
  GtkWidget *pixmap_widget;
  GdkPixmap *pixmap;
  GdkColor transparent;
  GdkBitmap *mask=NULL;
  GdkColormap *colormap;

  colormap = gtk_widget_get_default_colormap ();

  pixmap = gdk_pixmap_colormap_create_from_xpm_d(list->window,
						 colormap,
						 &mask,
						 &transparent,
						 pixdata);

  /* pixmap = gdk_pixmap_create_from_xpm_d(list->window,&mask,&transparent,pixdata); */
  pixmap_widget = gtk_pixmap_new(pixmap,mask);
  gtk_widget_show(pixmap_widget); 
  return(pixmap_widget);
}


GtkWidget *
fractalexplorer_list_add(fractalexplorerOBJ *obj)
{
  GList *list;
  gint pos;
  GtkWidget *list_item;
  GtkWidget *list_pix;

  list_pix = fractalexplorer_new_pixmap(fractalexplorer_gtk_list,Floppy6_xpm);
  list_item = fractalexplorer_list_item_new_with_label_and_pixmap(obj,obj->draw_name,list_pix);      

  gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer)obj);

  pos = fractalexplorer_list_insert(obj);

  list = g_list_append(NULL, list_item);
  gtk_list_insert_items(GTK_LIST(fractalexplorer_gtk_list), list, pos);
  gtk_widget_show (list_item);
  gtk_list_select_item(GTK_LIST(fractalexplorer_gtk_list), pos);  
  
  gtk_signal_connect(GTK_OBJECT(list_item), "button_press_event",
		     (GtkSignalFunc) list_button_press,
		     (gpointer)obj);

  return(list_item);
}




void
list_button_update(fractalexplorerOBJ *obj)
{
  g_return_if_fail (obj != NULL);
  pic_obj = (fractalexplorerOBJ *)obj;
  /* Draw xxxxxxxxxxxxxxxxxxxx */
}






fractalexplorerOBJ *
fractalexplorer_new(void)
{
  fractalexplorerOBJ * new;

  new = g_new0(fractalexplorerOBJ,1);
  return(new);
}

gradientOBJ *
gradient_new(void)
{
  gradientOBJ * new;

  new = g_new0(gradientOBJ,1);
  return(new);
}

void
build_list_items(GtkWidget *list)
{
  GList *tmp = fractalexplorer_list;
  GtkWidget *list_item;
  GtkWidget *list_pix;
  fractalexplorerOBJ *g;

  while(tmp)
    {
      g = tmp->data;

      if(g->obj_status & fractalexplorer_READONLY)
	list_pix = fractalexplorer_new_pixmap(list,mini_cross_xpm);
      else
	list_pix = fractalexplorer_new_pixmap(list,bluedot_xpm);

      list_item = fractalexplorer_list_item_new_with_label_and_pixmap(g,g->draw_name,list_pix);      
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer)g);
      gtk_list_append_items (GTK_LIST (list), g_list_append(NULL,list_item));

      gtk_signal_connect(GTK_OBJECT(list_item), "button_press_event",
			     (GtkSignalFunc) list_button_press,
			     (gpointer)g);
      gtk_widget_show (list_item);

      tmp = tmp->next;
    }
}

void
gradient_build_list_items(GtkWidget *list)
{
  GList *tmp = gradient_list;
  GtkWidget *list_item;
  GtkWidget *list_pix;
  gradientOBJ *g;

  while(tmp)
    {
      g = tmp->data;

      if(g->obj_status & gradient_GRADIENTEDITOR)
	list_pix = fractalexplorer_new_pixmap(list,greendot_xpm);
      else 
	list_pix = fractalexplorer_new_pixmap(list,bluedot_xpm);

      list_item = gradient_list_item_new_with_label_and_pixmap(g,g->draw_name,list_pix);      
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer)g);
      gtk_list_append_items (GTK_LIST (list), g_list_append(NULL,list_item));

      gtk_signal_connect(GTK_OBJECT(list_item), "button_press_event",
			     (GtkSignalFunc) gradient_list_button_press,
			     (gpointer)g);
      gtk_widget_show (list_item);

      tmp = tmp->next;
    }
}

/*

void
fractalexplorer_save_menu_callback(GtkWidget *widget, gpointer data)
{
  fractalexplorerOBJ * real_current = current_obj;

  current_obj = fractalexplorer_obj_for_menu;

  fractalexplorer_save(); 

  current_obj = real_current;
}

void
fractalexplorer_load_menu_callback(GtkWidget *widget, gpointer data)
{
      dialog_change_scale();
      set_cmap_preview();
      dialog_update_preview();
}

void
fractalexplorer_rename_menu_callback(GtkWidget *widget, gpointer data)
{
  create_file_selection(fractalexplorer_obj_for_menu,fractalexplorer_obj_for_menu->filename);
}

void
fractalexplorer_copy_menu_callback(GtkWidget *widget, gpointer data)
{
  gchar *new_name = g_strup_printf(msg[lng][MSG_COPYNAME],fractalexplorer_obj_for_menu->draw_name);
  new_fractalexplorer_obj(new_name);
  g_free(new_name);

  current_obj->opts = fractalexplorer_obj_for_menu->opts; 

  update_draw_area(fractalexplorer_preview,NULL);
  list_button_update(current_obj);
}

*/


/*

void
fractalexplorer_op_menu_create(GtkWidget *window)
{
  GtkWidget *menu_item;
  GtkAcceleratorTable *accelerator_table;

  fractalexplorer_op_menu = gtk_menu_new();

  accelerator_table = gtk_accelerator_table_new();
  gtk_menu_set_accelerator_table(GTK_MENU(fractalexplorer_op_menu),
				 accelerator_table);
  gtk_window_add_accelerator_table(GTK_WINDOW(window),accelerator_table);

  save_menu_item = menu_item = gtk_menu_item_new_with_label(msg[lng][MSG_SAVE]);
  gtk_menu_append(GTK_MENU(fractalexplorer_op_menu),menu_item);
  gtk_widget_show(menu_item);

  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)fractalexplorer_save_menu_callback,
		     NULL);

  gtk_widget_install_accelerator(menu_item,
				 accelerator_table,
				"activate",'S',0);

  menu_item = gtk_menu_item_new_with_label(msg[lng][MSG_SAVEAS]);
  gtk_menu_append(GTK_MENU(fractalexplorer_op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)fractalexplorer_rename_menu_callback,
		     NULL);
  gtk_widget_install_accelerator(menu_item,
				 accelerator_table,
				"activate",'A',0);

  menu_item = gtk_menu_item_new_with_label(msg[lng][MSG_COPY]);
  gtk_menu_append(GTK_MENU(fractalexplorer_op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)fractalexplorer_copy_menu_callback,
		     NULL);
  gtk_widget_install_accelerator(menu_item,
				 accelerator_table,
				"activate",'C',0);

  menu_item = gtk_menu_item_new_with_label(msg[lng][MSG_LOAD]);
  gtk_menu_append(GTK_MENU(fractalexplorer_op_menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     (GtkSignalFunc)fractalexplorer_load_menu_callback,
		     NULL);
  gtk_widget_install_accelerator(menu_item,
				 accelerator_table,
				"activate",'L',0);

}


void
fractalexplorer_op_menu_popup(gint button, guint32 activate_time,fractalexplorerOBJ *obj)
{
  fractalexplorer_obj_for_menu = obj; 

  if(obj->obj_status & fractalexplorer_READONLY)
    {
      gtk_widget_set_sensitive(save_menu_item,FALSE);
    }
  else
    {
      gtk_widget_set_sensitive(save_menu_item,TRUE);
    }

  gtk_menu_popup(GTK_MENU(fractalexplorer_op_menu),NULL,NULL,NULL,NULL,button,activate_time);
}

*/

gint
list_button_press(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer   data)
{

  fractalexplorerOBJ * sel_obj;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    /*
      if(event->button == 3)
	{
	  fractalexplorer_op_menu_popup(event->button,event->time,(fractalexplorerOBJ *)data);
	  return(FALSE);
	}
*/
      list_button_update((fractalexplorerOBJ *)data);
      break;
    case GDK_2BUTTON_PRESS:

  sel_obj = (fractalexplorerOBJ *)data;
  
  if(sel_obj) {
      current_obj=sel_obj;
      wvals=current_obj->opts;
      dialog_change_scale();
      set_cmap_preview();
      dialog_update_preview(); }
/*    new_obj_2edit(sel_obj); */
  else
    g_warning(msg[lng][MSG_NULLLIST]);

      break;
    default:
      printf(msg[lng][MSG_UNKNOWN_EVENT]);
      break;
    }

  return(FALSE);
}



/*
 *	Query gimprc for fractalexplorer-path, and parse it.
 *	This code is based on script_fu_find_scripts ()
 *      and the Gflare plugin.
 */

void
plug_in_parse_fractalexplorer_path()
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *path_string;
  gchar *home;
  gchar *path;
  gchar *token;
  struct stat filestat;
  gint	err;
  gchar buf[MAXSTRLEN];
  
  if(fractalexplorer_path_list)
    g_list_free(fractalexplorer_path_list);
  
  fractalexplorer_path_list = NULL;
  
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "fractalexplorer-path",
				    PARAM_END);
  
  if (return_vals[0].data.d_status != STATUS_SUCCESS || return_vals[1].data.d_string == NULL)
    {
      g_warning(msg[lng][MSG_MISSING_GIMPRC]);
      create_warn_dialog(msg[lng][MSG_MISSING_GIMPRC]);
       
      gimp_destroy_params (return_vals, nreturn_vals);
      return;
    }
  
  path_string = g_strdup (return_vals[1].data.d_string);
  gimp_destroy_params (return_vals, nreturn_vals);
  
  /* Set local path to contain temp_path, where (supposedly)
   * there may be working files.
   */
  home = g_get_home_dir ();

  /* Search through all directories in the  path */

  token = strtok (path_string, G_SEARCHPATH_SEPARATOR_S);

  while (token)
    {
      if (*token == '\0')
	{
	  token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
	  continue;
	}

      if (*token == '~')
	{
	  path = g_strdup_printf ("%s%s", home, token + 1);
	}
      else
	{
	  path = g_strdup (token);
	} /* else */

      /* Check if directory exists */
      err = stat (path, &filestat);

      if (!err && S_ISDIR (filestat.st_mode))
	{
	  if (path[strlen (path) - 1] != G_DIR_SEPARATOR)
	    strcat (path, G_DIR_SEPARATOR_S);

	  fractalexplorer_path_list = g_list_append (fractalexplorer_path_list, path);
	}
      else
	{
	  sprintf(buf,msg[lng][MSG_WRONGPATH], path);
	  g_warning(buf);
	  create_warn_dialog(buf);
	  g_free (path);
	}
      token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
    }
  g_free (path_string);
}


void
fractalexplorer_free(fractalexplorerOBJ * fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  if(fractalexplorer->name)
    g_free(fractalexplorer->name);
  if(fractalexplorer->filename)
    g_free(fractalexplorer->filename);
  if(fractalexplorer->draw_name)
    g_free(fractalexplorer->draw_name);
  g_free (fractalexplorer);
}

void
gradient_free(gradientOBJ * gradi)
{
  g_assert (gradi != NULL);

  if(gradi->name)
    g_free(gradi->name);
  if(gradi->filename)
    g_free(gradi->filename);
  if(gradi->draw_name)
    g_free(gradi->draw_name);
  g_free (gradi);
}

void
fractalexplorer_free_everything(fractalexplorerOBJ * fractalexplorer)
{
  g_assert (fractalexplorer != NULL);

  if(fractalexplorer->filename)
    {
      remove(fractalexplorer->filename);
    }
  fractalexplorer_free(fractalexplorer);
}

void
gradient_free_everything(gradientOBJ * gradi)
{
  g_assert (gradi != NULL);

  if(gradi->filename)
    {
      remove(gradi->filename);
    }
  gradient_free(gradi);
}

void
fractalexplorer_list_free_all ()
{
  GList * list;
  fractalexplorerOBJ * fractalexplorer;

  list = fractalexplorer_list;
  while (list)
    {
      fractalexplorer = (fractalexplorerOBJ *) list->data;
      fractalexplorer_free (fractalexplorer);
      list = list->next;
    }

  g_list_free (fractalexplorer_list);
  fractalexplorer_list = NULL;
}

void
gradient_list_free_all ()
{
  GList * list;
  gradientOBJ * gradi;

  list = gradient_list;
  while (list)
    {
      gradi = (gradientOBJ *) list->data;
      gradient_free (gradi);
      list = list->next;
    }

  g_list_free (gradient_list);
  gradient_list = NULL;
}

fractalexplorerOBJ *
fractalexplorer_load (gchar *filename, gchar *name)
{
  fractalexplorerOBJ * fractalexplorer;
  FILE * fp;
  gchar load_buf[MAX_LOAD_LINE];
  
  g_assert (filename != NULL);
  fp = fopen (filename, "r");
  if (!fp)
    {
      g_warning (msg[lng][MSG_OPENERROR], filename);
      return NULL;
    }

  fractalexplorer = fractalexplorer_new();

  fractalexplorer->name = g_strdup(name);
  fractalexplorer->draw_name = g_strdup(name);
  fractalexplorer->filename = g_strdup(filename);


  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line(load_buf,MAX_LOAD_LINE,fp,1);

  if(strncmp(fractalexplorer_HEADER,load_buf,strlen(load_buf)))
    {
      gchar err[MAXSTRLEN];
      sprintf(err,msg[lng][MSG_WRONGFILETYPE],fractalexplorer->filename);
      create_warn_dialog(err);
      fclose(fp);
      return(NULL);
    }
  
  if(load_options(fractalexplorer,fp))
    {
      /* waste some mem */
      gchar err[MAXSTRLEN];
      sprintf(err,
	      msg[lng][MSG_CORRUPTFILE],
	      filename,
	      line_no);
      create_warn_dialog(err);
      fclose(fp);
      return(NULL);
    }

  fclose(fp);

  if(!pic_obj)
    pic_obj = fractalexplorer;

  fractalexplorer->obj_status = fractalexplorer_OK;

  return(fractalexplorer);
}


gradientOBJ *
gradient_load (gchar *filename, gchar *name)
{
  gradientOBJ * gradi;
  FILE * fp;
  gchar load_buf[MAX_LOAD_LINE];
  
  g_assert (filename != NULL);

  fp = fopen (filename, "r");
  if (!fp)
    {
      g_warning (msg[lng][MSG_OPENERROR], filename);
      return NULL;
    }

  gradi = gradient_new();

  gradi->name = g_strdup(name);
  gradi->draw_name = g_strdup(name);
  gradi->filename = g_strdup(filename);


  /* HEADER
   * draw_name
   * version
   * obj_list
   */

  get_line(load_buf,MAX_LOAD_LINE,fp,1);

  if(strncmp(fractalexplorer_HEADER,load_buf,strlen(load_buf)))
    {
      gchar err[MAXSTRLEN];
      sprintf(err,msg[lng][MSG_WRONGFILETYPE],gradi->filename);
      create_warn_dialog(err);
      return(NULL);
    }

/*      
  if(gradient_load_options(gradient,fp))
    {
      gchar err[MAXSTRLEN];
      sprintf(err,
	      msg[lng][MSG_CORRUPTFILE],
	      filename,
	      line_no);
      create_warn_dialog(err);
      return(NULL);
    }
*/
  fclose(fp);
 /*
  if(!pic_obj)
    pic_obj = fractalexplorer;

  fractalexplorer->obj_status = fractalexplorer_OK;
*/
  return(gradi);
}

void
fractalexplorer_rescan_file_selection_ok(GtkWidget *w,
		   GtkFileSelection *fs,
		   gpointer data)
{
  GtkWidget *list_item;
  GtkWidget *lw = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(fs));
  gchar * filenamebuf;
  struct stat	filestat;
  gint		err;

  filenamebuf = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  err = stat(filenamebuf, &filestat);

  if (!S_ISDIR(filestat.st_mode))
    {
     g_warning(msg[lng][MSG_NOTDIR],filenamebuf);
    }  
  else
    {

      list_item = gtk_list_item_new_with_label(filenamebuf);
      gtk_widget_show(list_item);
      
      gtk_list_prepend_items(GTK_LIST(lw),g_list_append(NULL, list_item));
      
      rescan_list = g_list_prepend(rescan_list,g_strdup(filenamebuf));
    }

  gtk_widget_destroy(GTK_WIDGET(fs));
}

void
fractalexplorer_list_load_all(GList *plist)
{
  fractalexplorerOBJ  * fractalexplorer;
  GList    * list;
  gchar	   * path;
  gchar	   * filename;
  DIR	   * dir;
  struct dirent *dir_ent;
  struct stat	filestat;
  gint		err;
  /*  Make sure to clear any existing fractalexplorers  */
  current_obj = pic_obj = NULL;
  fractalexplorer_list_free_all ();
  list = plist;
  while (list)
    {
      path = list->data;
      list = list->next;

      /* Open directory */
      dir = opendir (path);

      if (!dir)
	g_warning(msg[lng][MSG_DIRREADERROR], path);
      else
	{
	  while ((dir_ent = readdir (dir)))
	    {
	      filename = g_strdup_printf ("%s%s", path, dir_ent->d_name);

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
      fractalexplorer->draw_name = g_strdup(msg[lng][MSG_FIRSTFRACTAL]);
      fractalexplorer_list_insert(fractalexplorer);
    }
  pic_obj = current_obj = fractalexplorer_list->data;  /* set to first entry */

}

void
gradient_list_load_all(GList *plist)
{
  gradientOBJ * gradi;
  GList    * list;
  gchar	   * path;
  gchar	   * filename;
  char     **gradients=NULL;
  DIR	   * dir;
  struct dirent *dir_ent;
  struct stat	filestat;
  gint		err;
  gint 		gradnumber=200,i;
  /*  Make sure to clear any existing gradients  */
  gradient_list_free_all ();
  list = plist;
  while (list)
    {
      path = list->data;
      list = list->next;

      /* Open directory */
      dir = opendir (path);

      if (!dir)
	g_warning(msg[lng][MSG_DIRREADERROR], path);
      else
	{
	  while ((dir_ent = readdir (dir)))
	    {
	      filename = g_strdup_printf ("%s%s", path, dir_ent->d_name);

	      /* Check the file and see that it is not a sub-directory */
	      err = stat (filename, &filestat);

	      if (!err && S_ISREG (filestat.st_mode))
		{

		  gradi = gradient_load (filename, dir_ent->d_name);
		  
		  if (gradi)
		    {
		      gradient_list_insert (gradi);
		    }
		}

	      g_free (filename);
	    } /* while */
	  closedir (dir);
	} /* else */
    }
    
    gradients=gimp_gradients_get_list(&gradnumber);
    for (i=0; i< gradnumber; i++)
    {
	  gradi = gradient_new();
          gradi->name = gradi->draw_name = gradi->filename=gradients[i];
		  if (gradi)
		    {
		      gradi->obj_status=gradient_GRADIENTEDITOR;
		      gradient_list_insert (gradi);
		    }
    }
    
  if(!gradient_list)
    {
      /* lets have at least one! */
      gradi = gradient_new();
      gradi->draw_name = g_strdup("Gradient");
      gradient_list_insert(gradi);
    }
/*  pic_obj = current_obj = gradient_list->data;  set to first entry */
}


GtkWidget *
add_objects_list ()
{
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *list_frame;
  GtkWidget *scrolled_win;
  GtkWidget *list;
  GtkWidget *button;

  frame = gtk_frame_new(msg[lng][MSG_CHOOSE_FRACTAL]);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
  gtk_widget_show(frame);

  table = gtk_table_new (6, 4, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 10);
  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
/*  gtk_table_set_col_spacings(GTK_TABLE(table), 10); */
  gtk_widget_show(table);

  delete_frame_to_freeze = list_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (list_frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show(list_frame);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (list_frame), scrolled_win);
  gtk_widget_show (scrolled_win);

  fractalexplorer_gtk_list = list = gtk_list_new ();
  /* gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_MULTIPLE); */
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 list);
  gtk_widget_show (list);
  /* Load saved objects */
  fractalexplorer_list_load_all(fractalexplorer_path_list);
  /* Put list in */
  build_list_items(list);

  /* Put buttons in */
  button = gtk_button_new_with_label (msg[lng][MSG_RESCAN]);
  gtk_widget_show(button);
  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) rescan_button_press,
		      NULL);
  gtk_tooltips_set_tip(tips,button,msg[lng][MSG_RESCAN_COMMENT], NULL); 
  gtk_table_attach(GTK_TABLE(table), button, 0, 1, 3, 4, GTK_FILL, GTK_FILL,  0, 0);

/*  
  button = gtk_button_new_with_label (msg[lng][MSG_NEW]);
  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) new_button_press,
		      "My New Fractal");
  gtk_widget_show(button);
  gtk_tooltips_set_tip(tips,button,msg[lng][MSG_NEW_COMMENT], NULL); 
  gtk_table_attach(GTK_TABLE(table), button, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

  button = gtk_button_new_with_label (msg[lng][MSG_RENAME]);
  gtk_widget_show(button);
  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) rename_button_press,
		      (gpointer) list);
  gtk_tooltips_set_tip(tips,button,msg[lng][MSG_RENAME_COMMENT], NULL); 
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 2, 3, GTK_FILL, GTK_FILL,  0, 0);
*/
 
  button = gtk_button_new_with_label (msg[lng][MSG_DEL]);
  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) fractalexplorer_delete_fractalexplorer_callback,
		      (gpointer)list);
  gtk_widget_show(button);
  gtk_tooltips_set_tip(tips,button,msg[lng][MSG_DELETE_COMMENT], NULL); 
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);

  /* Attach the frame for the list Show the widgets */

  gtk_table_attach(GTK_TABLE(table), list_frame, 0, 2, 0, 3, GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 0, 0);

  cmap_preview_long = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(cmap_preview_long), GR_WIDTH, 32);
  gtk_table_attach(GTK_TABLE(table), cmap_preview_long, 0, 2, 4, 5, GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(cmap_preview_long);

  gtk_container_add (GTK_CONTAINER (frame), table);
  return (frame);
}



GtkWidget *
add_gradients_list ()
{
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *list_frame;
  GtkWidget *scrolled_win;
  GtkWidget *list;

  frame = gtk_frame_new(msg[lng][MSG_CHOOSE_GRADIENT]);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
  gtk_widget_show(frame);

  table = gtk_table_new (6, 4, FALSE);
 
  gtk_container_set_border_width(GTK_CONTAINER(table), 10);
  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
/*  gtk_table_set_col_spacings(GTK_TABLE(table), 10); */
  gtk_widget_show(table);

  list_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (list_frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show(list_frame);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (list_frame), scrolled_win);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 list);
  gtk_widget_show (list);
  /* Load saved objects */
  
  gradient_list_load_all(fractalexplorer_path_list);
  /* Put list in */
  gradient_build_list_items(list);

  /* Put buttons in 
  button = gtk_button_new_with_label (msg[lng][MSG_RESCAN]);
  gtk_widget_show(button);
  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) rescan_button_press,
		      NULL);
  gtk_tooltips_set_tips(tips,button,msg[lng][MSG_RESCAN_COMMENT]); 
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 0, 1, GTK_FILL, GTK_FILL,  0, 0);
  */  

  /* Attach the frame for the list Show the widgets */

  gtk_table_attach(GTK_TABLE(table), list_frame, 0, 3, 0, 4, GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 1, 1);
  
  cmap_preview_long2 = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(cmap_preview_long2), GR_WIDTH, 32);
  gtk_table_attach(GTK_TABLE(table), cmap_preview_long2, 0, 3, 4, 5, GTK_FILL|GTK_EXPAND , GTK_FILL|GTK_EXPAND, 1, 1);
  gtk_widget_show(cmap_preview_long2);

  gtk_container_add (GTK_CONTAINER (frame), table);
  return (frame);
}



void
fractalexplorer_rescan_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  GList *list;

  list = rescan_list;
  while (list) 
    {
      list = list->next;
    }
  list = fractalexplorer_path_list;
  while (list) 
    {
      rescan_list = g_list_append(rescan_list,g_strdup(list->data));
      list = list->next;
    }
  clear_list_items(GTK_LIST(fractalexplorer_gtk_list));
  fractalexplorer_list_load_all(rescan_list);
  build_list_items(fractalexplorer_gtk_list);
  list_button_update(current_obj);
  gtk_widget_destroy (GTK_WIDGET (client_data));
}


void
fractalexplorer_rescan_add_entry_callback (GtkWidget *w,
		       gpointer   client_data)
{
  static GtkWidget *window = NULL;

  /* Call up the file sel dialouge */
  window = gtk_file_selection_new (msg[lng][MSG_ADDPATH]);
  gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_object_set_user_data(GTK_OBJECT(window),(gpointer)client_data);


  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc) destroy_window,
		      &window);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked", (GtkSignalFunc) fractalexplorer_rescan_file_selection_ok,
		      (gpointer)window);

  gtk_signal_connect_object(GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			    "clicked", (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(window));
  gtk_widget_show(window);
}


void
fractalexplorer_rescan_list (void)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *dlg;
  GtkWidget *list_frame;
  GtkWidget *scrolled_win;
  GtkWidget *list_widget;
  GList *list;

  /*  the dialog  */
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), msg[lng][MSG_RESCANTITLE1]);
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);

  /* path list */
  list_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (list_frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show(list_frame);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (list_frame), scrolled_win);
  gtk_widget_show (scrolled_win);

  list_widget = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list_widget), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 list_widget);
  gtk_widget_show (list_widget);
  gtk_box_pack_start (GTK_BOX (vbox), list_frame, TRUE, TRUE, 0);

  list = fractalexplorer_path_list;
  while (list)
    {
      GtkWidget *list_item;
      list_item = gtk_list_item_new_with_label(list->data);
      gtk_widget_show(list_item);
      gtk_container_add (GTK_CONTAINER (list_widget), list_item);
      list = list->next;
    }

  button = gtk_button_new_with_label (msg[lng][MSG_OK]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_rescan_ok_callback,
                      (gpointer)dlg);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /* Clear the old list out */
  if((list = rescan_list))
    {
      while (list) 
	{
	  g_free(list->data);
	  list = list->next;
	}

      g_list_free(rescan_list);
      rescan_list = NULL;
    }

  button = gtk_button_new_with_label (msg[lng][MSG_ADDDIR]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_rescan_add_entry_callback,
                      (gpointer)list_widget);

  gtk_object_set_user_data(GTK_OBJECT(dlg),(gpointer)list_widget);


  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (msg[lng][MSG_CANCEL]);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)fractalexplorer_rescan_cancel_callback,
                      (gpointer)dlg);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (vbox);
  gtk_widget_show (dlg);
}


gint
rescan_button_press(GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer   data)
{
  fractalexplorer_rescan_list();
  return(FALSE);
}


gint
delete_button_press_ok(GtkWidget *widget,
		  gpointer   data)
{
  gint pos;
  GList * sellist;
  fractalexplorerOBJ * sel_obj;
  GtkWidget *list = (GtkWidget *)data;

  /* Must update which object we are editing */
  /* Get the list and which item is selected */
  /* Only allow single selections */

  sellist = GTK_LIST(list)->selection; 

  g_print ("list: %i\n", g_list_length (sellist));

  sel_obj = (fractalexplorerOBJ *)gtk_object_get_user_data(GTK_OBJECT((GtkWidget *)(sellist->data)));

  pos = gtk_list_child_position(GTK_LIST(fractalexplorer_gtk_list),sellist->data);

  /* Delete the current  item + asssociated file */
  gtk_list_clear_items(GTK_LIST (fractalexplorer_gtk_list),pos,pos+1);
  /* Shadow copy for ordering info */
  fractalexplorer_list = g_list_remove(fractalexplorer_list,sel_obj);
/*
  if(sel_obj == current_obj)
    {
      clear_undo();
    }
*/  
  /* Free current obj */
  fractalexplorer_free_everything(sel_obj);

  /* Select previous one */
  if (pos > 0)
    pos--;

  if((pos == 0) && (g_list_length(fractalexplorer_list) == 0))
    {      
      /* Warning - we have a problem here
       * since we are not really "creating an entry"
       * why call fractalexplorer_new?
       */
      new_button_press(NULL,NULL,NULL);
    }

  gtk_widget_destroy(delete_dialog);
  gtk_widget_set_sensitive(delete_frame_to_freeze,TRUE);

  delete_dialog = NULL;

  gtk_list_select_item(GTK_LIST(fractalexplorer_gtk_list), pos);

  current_obj = g_list_nth_data(fractalexplorer_list,pos);

  /*
  draw xxxxxxxxxxxxxxxx 
  update_draw_area(fractalexplorer_preview,NULL);
  */

  list_button_update(current_obj);


  return(FALSE);
}
