#include "rcm.h"
#include "rcm_hsv.h"

RcmParams Current=
{
  SELECTION,           /* SELECTION ONLY */
  TRUE,                   /* REAL TIME */
  FALSE,                   /* SUCCESS */
  RADIANS_OVER_PI,     /* START IN RADIANS OVER PI */
  GRAY_TO
};

void      query  (void);
void      run    (char      *name,
		  int        nparams,
		  GParam    *param,
		  int       *nreturn_vals,
		  GParam   **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};



MAIN();


void
query ()
{
  GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (used for indexed images)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
   GParamDef *return_vals = NULL;
   int nargs = sizeof (args) / sizeof (args[0]);
   int nreturn_vals = 0;

  gimp_install_procedure ("Colormap Rotation Plug-in",
			  "Does that xv rotation thingy",
			  "Then something else here",
			  "Pavel Grinfeld (pavel@ml.com)",
			  "Pavel Grinfeld (pavel@ml.com)",
			  "27th March 1997",
			  "<Image>/Filters/Darkroom/Colormap Rotation",
			  "RGB*,INDEXED*,GRAY",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

/********************************STANDARD RUN*************************/
void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  
  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  Current.drawable = gimp_drawable_get (param[2].data.d_drawable);
  Current.mask = gimp_drawable_get(gimp_image_get_selection(param[1].data.d_image));

  if (gimp_drawable_indexed (Current.drawable->id) ||
      gimp_drawable_gray (Current.drawable->id) ) {
    ErrorMessage("Convert the image to RGB first!");
    status = STATUS_EXECUTION_ERROR;
  }
  
  else if (gimp_drawable_color (Current.drawable->id) && rcm_dialog())
    {
      gimp_progress_init ("Rotating the Colormap..."); 
      gimp_tile_cache_ntiles (2 * (Current.drawable->width / 
				   gimp_tile_width () + 1));
      rcm (Current.drawable);
      gimp_displays_flush ();
    }
  else status = STATUS_EXECUTION_ERROR;
  
  
  values[0].data.d_status = status;
  if (status==STATUS_SUCCESS)
    gimp_drawable_detach (Current.drawable);
}


void
rcm_row (const guchar *src_row,
	 guchar       *dest_row,
	 gint         row,
	 gint         row_width,
	 gint         bytes)
{
  gint col, bytenum, skip;
  hsv H,S,V,R,G,B;
  
  for (col = 0; col < row_width ; col++)
    {
      skip=0;
      
      R = (float)src_row[col*bytes + 0]/255.0;
      G = (float)src_row[col*bytes + 1]/255.0;
      B = (float)src_row[col*bytes + 2]/255.0;
      
      rgb_to_hsv(R,G,B,&H,&S,&V);
      
      if (rcm_is_gray(S)) 
	if (Current.Gray_to_from==GRAY_FROM)
	  if (angle_inside_slice(Current.Gray->hue,Current.From->angle)<=1) {
	    H = Current.Gray->hue/TP;
	    S = Current.Gray->satur;
	  }
	  else 
	    skip=1;
	else {
	  skip=1;
	  hsv_to_rgb(Current.Gray->hue/TP,
		     Current.Gray->satur,
		     V,
		     &R,&G,&B);
	}
      
      
      if (!skip) {
	H=rcm_linear(rcm_left_end(Current.From->angle),
		     rcm_right_end(Current.From->angle),
		     rcm_left_end(Current.To->angle),
		     rcm_right_end(Current.To->angle),
		     H*TP);    
	H=angle_mod_2PI(H)/TP;
	hsv_to_rgb(H,S,V,&R,&G,&B);
      }
      
      dest_row[col*bytes +0] = R*255;
      dest_row[col*bytes +1] = G*255;
      dest_row[col*bytes +2] = B*255;
      
      if (bytes>3)
	for (bytenum = 3; bytenum<bytes; bytenum++)
	  dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
      
    }
}


void rcm (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *src_row, *dest_row;
  gint row;
  gint x1, y1, x2, y2;
  
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  src_row = (guchar *) malloc ((x2 - x1) * bytes);
  dest_row = (guchar *) malloc ((x2 - x1) * bytes);

  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, row, (x2 - x1));

      rcm_row (src_row,
	       dest_row,
	       row,
	       (x2 - x1),
	       bytes
	       );
      
      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, row, (x2 - x1));
      
      if ((row % 10) == 0)
	gimp_progress_update ((double) row / (double) (y2 - y1));
    }


  /*  update the processed region  */

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
  
  free (src_row);
  free (dest_row);
 
}


void 
ErrorMessage(guchar *message)
{
  GtkWidget *window, *label, *button,*table;
  gchar **argv=g_new (gchar *, 1);
  gint argc=1;
  argv[0] = g_strdup ("rcm");
  gtk_init (&argc, &argv);
  
  window=gtk_dialog_new();
  gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_window_set_title(GTK_WINDOW(window),"Filter Pack Simulation Message");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc) rcm_close_callback,
		      NULL);
  
  button = gtk_button_new_with_label ("Got It!");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) rcm_ok_callback,
                      window);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button, TRUE, TRUE, 0);
 
  table=gtk_table_new(2,2,FALSE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox),table,TRUE,TRUE,0);
  gtk_widget_show(table);
  
  label=gtk_label_new("");
  gtk_label_set(GTK_LABEL(label),message);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table),label,0,1,0,1,
		   GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,15,15);
  
  gtk_widget_show(window);
  gtk_main ();

}
