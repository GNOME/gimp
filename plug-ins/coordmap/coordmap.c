/* Coordinate Map plug-in for The Gimp
 * Copyright (C) 1997 Michael Schubart (schubart@best.com)
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

/*
 * This plug-in for The Gimp uses the algorithm described below to map
 * a source image to a destination image. Two grayscale images X and Y
 * are used to describe the mapping. 
 *
 * For each position in the destination image, get the luminosity of the
 * pixels at the same position in the X and Y images. Interpret this pair
 * of values as a coordinate pair. Copy the pixel at these coordinates in
 * the source image to the destination image. 
 *
 * The latest version and some examples can be found at
 * http://www.best.com/~schubart/cm/
 *
 * This is my first plug-in for The Gimp. You are very welcome to send
 * me any comments you have.
 */

/*
 * Version 0.2 -- Sep 1 1997
 *
 * Cosmetic changes to the source code.
 */

/*
 * Version 0.1 -- Aug 28 1997
 *
 * The first version.
 */

#include <stdlib.h>
#include <stdio.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/*
 * definitions
 */

#undef GARRULOUS 		/* lots of messages to stdout? */

#define MODE_COPY		0
#define MODE_GRAY_TO_COLOR	1
#define MODE_COLOR_TO_GRAY	2

#define ALPHA_NONE		0
#define ALPHA_COPY      	1
#define ALPHA_CLEAR		2

#define LUMINOSITY(X)		((X[0]*30 + X[1]*59 + X[2]*11)/100)

#define SRC_WIDTH		256
#define SRC_HEIGHT		256

/*
 * declare types
 */

typedef struct
{
  gint32 src_drawable_id;	/* the source drawable */
  gint32 x_drawable_id;		/* the x drawable */
  gint32 y_drawable_id;		/* the y drawable */
} coordmap_values_t;

typedef struct
{
  gint run;			/* did the user press "ok"? */
} coordmap_interface_t;

/*
 * declare local functions
 */

static void query(void);
static void run(char *name,
		int nparams, GParam *param,
		int *nreturn_vals, GParam **return_vals);

static gint coordmap(gint32 dst_drawable_id);

static gint coordmap_dialog(gint32 dst_drawable_id);
static gint coordmap_src_drawable_constraint(gint32 image_id,
					     gint32 drawable_id,
					     gpointer data);
static gint coordmap_xy_drawable_constraint(gint32 image_id,
					    gint32 drawable_id,
					    gpointer data);

static void message_box(char *message);

static void coordmap_close_callback(GtkWidget *widget, gpointer data);
static void coordmap_ok_callback(GtkWidget *widget, gpointer dialog);
static void coordmap_src_drawable_callback(gint32 id, gpointer data);
static void coordmap_x_drawable_callback(gint32 id, gpointer data);
static void coordmap_y_drawable_callback(gint32 id, gpointer data);

/*
 * local variables
 */

static coordmap_values_t cvals=
{
  -1,		/* src_drawable_id */
  -1,		/* x_drawable_id */
  -1,		/* y_drawable_id */
};

static coordmap_interface_t cint=
{
  FALSE		/* run */
};

GPlugInInfo PLUG_IN_INFO =
{
  NULL,		/* init_proc */
  NULL,    	/* quit_proc */
  query,   	/* query_proc */
  run,     	/* run_proc */
};

/*
 * functions
 */

MAIN ()

static void query()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "dst_drawable", "Destination drawable" },
    { PARAM_DRAWABLE, "src_drawable",
      "Source drawable, size must be 256x256" },
    { PARAM_DRAWABLE, "x_drawable",
      "X drawable, size must be equal to dst_drawable." },
    { PARAM_DRAWABLE, "y_drawable", 
      "Y drawable, size must be equal to dst_drawable." },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_coordmap",
	  "Maps a source image to a destination image.",
	  "Maps a source image to a destination image. For each position in "
	  "the destination image, the luminosity of of the X map and the Y "
	  "map at the same position are interpreted as coordinates. The pixel "
	  "that is found at these coordinates in the source image is copied "
	  "to the destination image.",			  
			  "Michael Schubart",
			  "Michael Schubart",
			  "Sep 1 1997",
			  "<Image>/Filters/Map/Coordinate Map",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run(char *name,
		int nparams, GParam  *param,
		int *nreturn_vals, GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode=param[0].data.d_int32;
  
  values[0].type=PARAM_STATUS;
  values[0].data.d_status=STATUS_SUCCESS;
  *nreturn_vals = 1;
  *return_vals = values;
  
  switch (run_mode)
  {
    case RUN_INTERACTIVE:
      /* get the parameter values of the last incocation */
      gimp_get_data("plug_in_coordmap",&cvals);
      if (!coordmap_dialog(param[2].data.d_drawable))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /* check number of parameters */
      if (nparams!=6)
	values[0].data.d_status=STATUS_CALLING_ERROR;
      else
      {
	cvals.src_drawable_id=param[3].data.d_drawable;
	cvals.x_drawable_id=param[4].data.d_drawable;
	cvals.y_drawable_id=param[5].data.d_drawable;
      }
      break;

    case RUN_WITH_LAST_VALS:
      /* get the parameter values of the last incocation */
      gimp_get_data("plug_in_coordmap",&cvals);
      break;

    default:
      break;
  }

  if (values[0].data.d_status==STATUS_SUCCESS)
  {
    /*
     * @@ Who can explain to me what this gimp_tile_cache_ntiles() is about,
     * and what's a good value? Thanks!
     */
    gimp_tile_cache_ntiles(48);
      
    if (!coordmap(param[2].data.d_drawable))
      /* bad parameter values */
      values[0].data.d_status=STATUS_EXECUTION_ERROR;
    else
    {
      if (run_mode!=RUN_NONINTERACTIVE)
	gimp_displays_flush();

      if (run_mode==RUN_INTERACTIVE)
	/* store parameter values for next invocation */
	gimp_set_data("plug_in_coordmap",&cvals,sizeof(coordmap_values_t));
    }
  }
}
  
static gint coordmap(gint32 dst_drawable_id)
{
  GDrawable *dst_drawable;
  GDrawable *src_drawable;
  GDrawable *x_drawable;
  GDrawable *y_drawable;
  gint src_mode=MODE_COPY;
  gint x_mode=MODE_COPY;
  gint y_mode=MODE_COPY;
  gint src_bpp;
  gint dst_bpp;
  gint copy_bpp;
  gint src_alpha_index;
  gint dst_alpha_index;
  gint alpha_mode=ALPHA_NONE;
  gint x1,y1,x2,y2;
  gint progress,max_progress;
  GPixelRgn dst_rgn;
  GPixelRgn src_rgn;
  GPixelRgn x_rgn;
  GPixelRgn y_rgn;
  guchar *src;
  gpointer pr;

  /* initialize */

  src_alpha_index = 0;
  dst_alpha_index = 0;

  /*
   * some tests to see whether we want to run at all
   */
  
  /* crude way to test if the drawables still exist */
  if (gimp_drawable_bpp(dst_drawable_id)==-1 ||
      gimp_drawable_bpp(cvals.src_drawable_id)==-1 ||
      gimp_drawable_bpp(cvals.x_drawable_id)==-1 ||
      gimp_drawable_bpp(cvals.y_drawable_id)==-1)
  {
#ifdef GARRULOUS
    g_print("coordmap: drawable missing\n"); 
#endif
    return FALSE;
  }

  /* reject indexed drawables */
  if (gimp_drawable_indexed(dst_drawable_id) ||
      gimp_drawable_indexed(cvals.src_drawable_id) ||
      gimp_drawable_indexed(cvals.x_drawable_id) ||
      gimp_drawable_indexed(cvals.y_drawable_id))
  {
#ifdef GARRULOUS
    g_print("coordmap: won't work on indexed images\n");
#endif
    return FALSE;
  }

  /* check source image size */
  if (gimp_drawable_width(cvals.src_drawable_id)!=SRC_WIDTH ||
      gimp_drawable_height(cvals.src_drawable_id)!=SRC_HEIGHT)
  {
#ifdef GARRULOUS
    g_print("coordmap: source image size must be 256x256\n"); 
#endif
    return FALSE;
  }
  
  /* check x and y map size */
  if (gimp_drawable_width(cvals.x_drawable_id)!=
      gimp_drawable_width(dst_drawable_id) ||
      gimp_drawable_height(cvals.x_drawable_id)!=
      gimp_drawable_height(dst_drawable_id) ||
      gimp_drawable_width(cvals.y_drawable_id)!=
      gimp_drawable_width(dst_drawable_id) ||
      gimp_drawable_height(cvals.y_drawable_id)!=
      gimp_drawable_height(dst_drawable_id))
  {
#ifdef GARRULOUS
    g_print("coordmap: x and y map must be of the same size as "
       "the destination image\n"); 
#endif
    return FALSE;
  }

  /*
   * examine the drawables to see what kind of transformations we need
   */

  /* convert gray source pixel to color destination pixel? */
  if (gimp_drawable_gray(cvals.src_drawable_id) &&
      gimp_drawable_color(dst_drawable_id))
    src_mode=MODE_GRAY_TO_COLOR;

  /* convert color source pixel to gray destination pixel? */
  if (gimp_drawable_color(cvals.src_drawable_id) &&
      gimp_drawable_gray(dst_drawable_id))
    src_mode=MODE_COLOR_TO_GRAY;

  /* is the x map gray, or do we have to computer its luminosity? */
  if (gimp_drawable_color(cvals.x_drawable_id))
    x_mode=MODE_COLOR_TO_GRAY;

  /* is the y map gray, or do we have to computer its luminosity? */
  if (gimp_drawable_color(cvals.y_drawable_id))
    y_mode=MODE_COLOR_TO_GRAY;

  /* how may bytes of color information in the destination image? */
  dst_bpp=gimp_drawable_bpp(dst_drawable_id);
  if (gimp_drawable_has_alpha(dst_drawable_id))
  {
    dst_alpha_index=--dst_bpp;
    alpha_mode=ALPHA_CLEAR;
  }
  
  /* how may bytes of color information in the source image? */
  src_bpp=gimp_drawable_bpp(cvals.src_drawable_id);
  if (gimp_drawable_has_alpha(cvals.src_drawable_id))
    src_alpha_index=--src_bpp;

  /* how many bytes of of color information are we going to copy? */
  copy_bpp=MIN(src_bpp,dst_bpp);

  /* copy alpha together with color if both drawables have alpha */ 
  if (gimp_drawable_has_alpha(dst_drawable_id) &&
      gimp_drawable_has_alpha(cvals.src_drawable_id))
  {
    if (src_mode==MODE_COPY)
      copy_bpp++;
    else
      alpha_mode=ALPHA_COPY;
  }

#ifdef GARRULOUS

  /* some debug messages */
  if (src_mode==MODE_COPY)
  {
    g_print("src_mode: MODE_COPY\n");
    g_print("copy_bpp: %d\n",copy_bpp);
  }
  else if (src_mode==MODE_GRAY_TO_COLOR)
    g_print("src_mode: MODE_GRAY_TO_COLOR\n");
  else
    g_print("src_mode: MODE_COLOR_TO_GRAY\n");

  if (alpha_mode==ALPHA_NONE)
    g_print("alpha_mode: ALPHA_NONE\n");
  else if (alpha_mode==ALPHA_COPY)
  {
    g_print("alpha_mode: ALPHA_COPY\n");
    g_print("src_alpha_index: %d\n",src_alpha_index);
    g_print("dst_alpha_index: %d\n",dst_alpha_index);
  }  
  else if (alpha_mode==ALPHA_CLEAR)
  {
    g_print("alpha_mode: ALPHA_CLEAR\n");
    g_print("dst_alpha_index: %d\n",dst_alpha_index);
  }  
  g_print("\n");

#endif

  /* get the drawables form the drawable ids */
  dst_drawable=gimp_drawable_get(dst_drawable_id);
  src_drawable=gimp_drawable_get(cvals.src_drawable_id);
  x_drawable=gimp_drawable_get(cvals.x_drawable_id);
  y_drawable=gimp_drawable_get(cvals.y_drawable_id);

  /* copy the source drawable to one big array */
  gimp_pixel_rgn_init(&src_rgn,src_drawable,0,0,
		      SRC_WIDTH,SRC_HEIGHT,FALSE,FALSE);
  src=g_new(guchar,SRC_WIDTH*SRC_HEIGHT*src_drawable->bpp);
  gimp_pixel_rgn_get_rect(&src_rgn,src,0,0,SRC_WIDTH,SRC_HEIGHT);

  /* get the area that is going to be processed */
  gimp_drawable_mask_bounds(dst_drawable->id,&x1,&y1,&x2,&y2);
  progress=0;
  max_progress=(x2-x1)*(y2-y1);
  gimp_progress_init("Coordinate Map...");

  /* initialize the pixel regions */
  gimp_pixel_rgn_init(&dst_rgn,dst_drawable,x1,y1,x2-x1,y2-y1,TRUE,TRUE);
  gimp_pixel_rgn_init(&x_rgn,x_drawable,x1,y1,x2-x1,y2-y1,FALSE,FALSE);
  gimp_pixel_rgn_init(&y_rgn,y_drawable,x1,y1,x2-x1,y2-y1,FALSE,FALSE);

  /* iterate over the areas the Gimp chooses for us */
  for (pr=gimp_pixel_rgns_register(3,&dst_rgn,&x_rgn,&y_rgn);
       pr!=NULL;pr=gimp_pixel_rgns_process(pr))
  {
    gint x,y;
    guchar *dst_ptr=dst_rgn.data;
    guchar *x_ptr=x_rgn.data;
    guchar *y_ptr=y_rgn.data;
    guchar *src_ptr;
    guchar x_value;
    guchar y_value;
    gint byte;

    /* iterate over all pixels in the current area */
    for (y=0;y<dst_rgn.h;y++)
      for (x=0;x<dst_rgn.w;x++)
      {
	/* get the source address */
	x_value=(x_mode==MODE_COPY)?x_ptr[0]:LUMINOSITY(x_ptr);
	y_value=(y_mode==MODE_COPY)?y_ptr[0]:LUMINOSITY(y_ptr);
	src_ptr=src+(y_value*SRC_WIDTH+x_value)*src_rgn.bpp;

	/* simply copy the color information? */
	if (src_mode==MODE_COPY)
	  for (byte=0;byte<copy_bpp;byte++)
	    dst_ptr[byte]=src_ptr[byte];

	/* or copy a gray value to red, green and blue? */
	else if (src_mode==MODE_GRAY_TO_COLOR)
	  for (byte=0;byte<dst_bpp;byte++)
	    dst_ptr[byte]=src_ptr[0];

	/* or convert from color to gray? (MODE_COLOR_TO_GRAY) */
	else 		
	  dst_ptr[0]=LUMINOSITY(src_ptr);

	/* copy alpha information? */
	if (alpha_mode==ALPHA_COPY)
	  dst_ptr[dst_alpha_index]=src_ptr[src_alpha_index];

	/* or clear the alpha chennel? */
	else if (alpha_mode==ALPHA_CLEAR)
	  dst_ptr[dst_alpha_index]=0xff;

	/* next pixel */
	dst_ptr+=dst_rgn.bpp;
	x_ptr+=x_rgn.bpp;
	y_ptr+=y_rgn.bpp;
      }

    /* update progress indicator */
    progress+=dst_rgn.w*dst_rgn.h;
    gimp_progress_update((double)progress/(double)max_progress);
  }

  /* update the destination drawable */
  gimp_drawable_flush(dst_drawable);
  gimp_drawable_merge_shadow(dst_drawable->id,TRUE);
  gimp_drawable_update(dst_drawable->id,x1,y1,(x2-x1),(y2-y1));

  /* free the memory used for the contents of the source drawable */
  g_free(src);

  /* free the drawable structs */
  gimp_drawable_detach(dst_drawable);
  gimp_drawable_detach(src_drawable);
  gimp_drawable_detach(x_drawable);
  gimp_drawable_detach(y_drawable);

  /* success! */
  return TRUE;
}

static gint coordmap_dialog(gint32 dst_drawable_id)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *menu;
  gint argc=1;
  gchar **argv=g_new(gchar *,1);
  gint src_drawable_count=0;

  /* initialize gtk */
  argv[0]=g_strdup("Coordinate Map");
  gtk_init(&argc,&argv);
  gtk_rc_parse(gimp_gtkrc());
  g_free(argv[0]);
  g_free(argv);

  /* the dialog box */
  dlg=gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg),"Coordinate Map");
  gtk_window_position(GTK_WINDOW(dlg),GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg),"destroy",
		     (GtkSignalFunc)coordmap_close_callback,
		     NULL);

  /* the "OK" button */
  button=gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     (GtkSignalFunc)coordmap_ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button,TRUE,TRUE,0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  /* the "Cancel" button */
  button=gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
			    (GtkSignalFunc)gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button,TRUE,TRUE,0);
  gtk_widget_show (button);

  /* a frame for that extra sexy look */
  frame=gtk_frame_new("Coordinate Map Options");
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame),10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox),frame,TRUE,TRUE,0);

  /* table to arrange the following labels and option menus*/
  table=gtk_table_new(3,2,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),10);
  gtk_container_add(GTK_CONTAINER(frame),table);

  /* the "Source Image" label */
  label=gtk_label_new("Source Image");
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0,1,0,1,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2,2);
  gtk_widget_show(label);

  /* option menu with a list of the possible source drawables */
  option_menu=gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table),option_menu, 1,2,0,1,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2,2);
  menu=gimp_drawable_menu_new(coordmap_src_drawable_constraint,
			      coordmap_src_drawable_callback,
			      &src_drawable_count,cvals.src_drawable_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
  gtk_widget_show(option_menu);
  
  /* the "X Map" label */
  label=gtk_label_new("X Map");
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0,1,1,2,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2,2);
  gtk_widget_show(label);

  /* option menu with a list of the possible x drawables */
  option_menu=gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table),option_menu, 1,2,1,2,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2,2);
  menu=gimp_drawable_menu_new(coordmap_xy_drawable_constraint,
			      coordmap_x_drawable_callback,
			      &dst_drawable_id,cvals.x_drawable_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
  gtk_widget_show(option_menu);
  
  /* the "Y Map" label */
  label=gtk_label_new("Y Map");
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0,1,2,3,
		   GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2,2);
  gtk_widget_show(label);

  /* option menu with a list of the possible y drawables */
  option_menu=gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table),option_menu,1,2,2,3,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 2,2);
  menu=gimp_drawable_menu_new(coordmap_xy_drawable_constraint,
			      coordmap_y_drawable_callback,
			      &dst_drawable_id,cvals.y_drawable_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
  gtk_widget_show(option_menu);
  
  gtk_widget_show(table);    
  gtk_widget_show(frame);

  /* display error message if no appropriate source drawables were found */
  if (src_drawable_count)
    gtk_widget_show(dlg);
  else
  {
    gtk_widget_destroy(dlg);
    message_box("No source image was found."
		"(The source image size must be 256x256)");
  }
  
  gtk_main();
  gdk_flush();

  return cint.run;
}

static gint coordmap_src_drawable_constraint(gint32 image_id,
					     gint32 drawable_id,
					     gpointer data)
/*
 * only appropriate drawables appear in the the "Source image" option menu:
 * size must be SRC_WIDTH x SRC_HEIGHT.
 * drawable must not be indexed.
 */
{
  gint *src_drawable_count=(gint *)data;
  
  if (drawable_id==-1)
    return TRUE;
  
  if (gimp_drawable_width(drawable_id)==SRC_WIDTH &&
      gimp_drawable_height(drawable_id)==SRC_HEIGHT &&
      !gimp_drawable_indexed(drawable_id))
  {
    /* count how many valid drawables there are */
    (*src_drawable_count)++;
    return TRUE;
  }

  return FALSE;
}

static gint coordmap_xy_drawable_constraint(gint32 image_id,
					    gint32 drawable_id,
					    gpointer data)
/*
 * only appropriate drawables appear in the the "X/Y MAP" option menus:
 * size must be equal to destination drawable.
 * drawable must not be indexed.
 */
{
  gint32 dst_drawable_id=*(gint32 *)data;
  
  return (drawable_id==-1 ||
	  (gimp_drawable_width(drawable_id)==
	   gimp_drawable_width(dst_drawable_id) &&
	   gimp_drawable_height(drawable_id)==
	   gimp_drawable_height(dst_drawable_id) &&
	   !gimp_drawable_indexed(drawable_id)));
}

static void message_box(char *message)
/*
 * display a small dialog box for error messages
 */
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;

  /* the dialog box */
  dlg=gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg),"Coordinate Map Error");
  gtk_window_position(GTK_WINDOW(dlg),GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg),"destroy",
		     (GtkSignalFunc)coordmap_close_callback,
		     NULL);

  /* the "OK" button */
  button=gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
			    (GtkSignalFunc)coordmap_close_callback,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area),
		     button,TRUE,TRUE,0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  /* the label showing the message */
  label=gtk_label_new(message);
  gtk_misc_set_padding(GTK_MISC(label),10,10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox),label,TRUE,TRUE,0);
  gtk_widget_show(label);
  
  gtk_widget_show(dlg);
}

static void coordmap_close_callback(GtkWidget *widget, gpointer data)
/*
 * gets called when the dialog window is closed via window manager
 */
{
  gtk_main_quit();
}  

static void coordmap_ok_callback(GtkWidget *widget, gpointer dialog)
/*
 * gets called when "OK" is pressed
 */
{
  /* yes, we're going to run the plug-in */
  cint.run=TRUE;

  /* close dialog window */
  gtk_widget_destroy(GTK_WIDGET(dialog));
}  

static void coordmap_src_drawable_callback(gint32 id, gpointer data)
/*
 * gets called when the user selects a different source drawable
 */
{
  cvals.src_drawable_id=id;
}  

static void coordmap_x_drawable_callback(gint32 id, gpointer data)
/*
 * gets called when the user selects a different x drawable
 */
{
  cvals.x_drawable_id=id;
}  

static void coordmap_y_drawable_callback(gint32 id, gpointer data)
/*
 * gets called when the user selects a different y drawable
 */
{
  cvals.y_drawable_id=id;
}  
