#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "fp.h"
#include "fp_hsv.h"

FP_Params Current =
{
  1,
  .25,                /* Initial Roughness */
  NULL,
  .6,                 /* Initial Degree of Aliasing */
  NULL,
  60,
  NULL,
  MIDTONES,           /* Initial Range */
  BY_VAL,             /* Initial God knows what */
  TRUE,               /* Selection Only */
  TRUE,               /* Real Time */
  0,                  /* Offset */
  HUE|VALUE,
  {32,224,255},
  {0,0,0}
};

GDrawable *drawable, *mask;

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



MAIN()


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

  gimp_install_procedure ("plug_in_filter_pack",
			  "Allows the user to change H, S, or C with many previews",
			  "Then something else here",
			  "Pavel Grinfeld (pavel@ml.com)",
			  "Pavel Grinfeld (pavel@ml.com)",
			  "27th March 1997",
			  "<Image>/Filters/Colors/Filter Pack",
			  "RGB*,INDEXED*,GRAY*",
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

  initializeFilterPacks();

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  mask=gimp_drawable_get(gimp_image_get_selection(param[1].data.d_image));

  if (gimp_drawable_indexed (drawable->id) ||gimp_drawable_gray (drawable->id) ) {
    ErrorMessage("Convert the image to RGB first!");
    status = STATUS_EXECUTION_ERROR;
  }

  else if (gimp_drawable_color (drawable->id) && fp_dialog())
    {
      gimp_progress_init ("Applying the Filter Pack...");
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      fp (drawable);
      gimp_displays_flush ();
    }
  else status = STATUS_EXECUTION_ERROR;


  values[0].data.d_status = status;
  if (status==STATUS_SUCCESS)
    gimp_drawable_detach (drawable);
}


void
fp_row (const guchar *src_row,
	guchar       *dest_row,
	gint         row,
	gint         row_width,
	gint         bytes)
{
  gint col, bytenum, k;
  int JudgeBy, Intensity, P[3], backupP[3];
  hsv H,S,V;
  gint M, m, middle;

  /* initialize */

  Intensity = 0;

  for (col = 0; col < row_width ; col++)
    {

      backupP[0] = P[0] = src_row[col*bytes+0];
      backupP[0] = P[1] = src_row[col*bytes+1];
      backupP[0] = P[2] = src_row[col*bytes+2];


      rgb_to_hsv(P[0]/255.0, P[1]/255.0, P[2]/255.0, &H, &S, &V);

      for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++) {
	if (!Current.Touched[JudgeBy]) continue;

	switch (JudgeBy) {
	case BY_HUE:
	  Intensity=255*H;
	  break;
	case BY_SAT:
	  Intensity=255*S;
	  break;
	case BY_VAL:
	  Intensity=255*V;
	  break;
	}


	/* It's important to take care of Saturation first!!! */

      m = MIN(MIN(P[0],P[1]),P[2]);
      M = MAX(MAX(P[0],P[1]),P[2]);
      middle=(M+m)/2;

      for (k=0; k<3; k++)
	if (P[k]!=m && P[k]!=M) middle=P[k];

	for (k=0; k<3; k++)
	  if (M!=m)
	    if (P[k] == M)
	      P[k] = MAX(P[k]+Current.satAdj[JudgeBy][Intensity],middle);
	    else if (P[k] == m)
	      P[k] = MIN(P[k]-Current.satAdj[JudgeBy][Intensity],middle);



	P[0] += Current.redAdj[JudgeBy][Intensity];
	P[1] += Current.greenAdj[JudgeBy][Intensity];
	P[2] += Current.blueAdj[JudgeBy][Intensity];

	P[0]  =  MAX(0,MIN(255, P[0]));
	P[1]  =  MAX(0,MIN(255, P[1]));
	P[2]  =  MAX(0,MIN(255, P[2]));
      }
      dest_row[col*bytes + 0] = P[0];
      dest_row[col*bytes + 1] = P[1];
      dest_row[col*bytes + 2] = P[2];

      if (bytes>3)
	for (bytenum = 3; bytenum<bytes; bytenum++)
	  dest_row[col*bytes+bytenum] = src_row[col*bytes+bytenum];
    }
}


void fp (GDrawable *drawable)
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

      fp_row (src_row,
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
  argv[0] = g_strdup ("fp");
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  window=gtk_dialog_new();
  gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  gtk_window_set_title(GTK_WINDOW(window),"Filter Pack Simulation Message");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc) fp_close_callback,
		      NULL);

  button = gtk_button_new_with_label ("Got It!");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) fp_ok_callback,
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
