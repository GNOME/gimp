/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Universal plug-in -- universal matrix filter
 * Copyright (C) 1997 Ole Steinfatt
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
 * The plug-in code structure is based on the Whirl plug-in,
 * which is Copyright (C) 1997 Federico Mena Quintero
 * 
 * The basic code is form the solid_noise plug-in
 * Copyright (C) 1997 Marcelo de Gomensoro Malheiros
 *
 * The convolution is copied from the sharpen plug-in
 * Copyright 1997 Michael Sweet
 *
 * Also some stuff from some other plug-ins and from the Gimp has
 * been used. Thanks to all! 
 */


/* Version 0.0:
 *
 *  My first try
 */


#include <math.h>
#include <stdlib.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/*---- Defines ----*/

#define MATRIX_SIZE 3
#define ENTRY_WIDTH 48

/*---- Typedefs ----*/

typedef struct {
  gint matrix[MATRIX_SIZE*MATRIX_SIZE];
  gint automatic;
  gint norm;
  gint bias;
} UniversalValues;

typedef struct {
  gint run;
} UniversalInterface; 

typedef struct {
 GtkWidget *normentry;
 GtkWidget *normlabel;
 GtkWidget *biasentry;
 GtkWidget *biaslabel;  
 GtkWidget *list;
} UniDialog;

typedef struct {
  char *name;
  char *filename;
  int  xsize;
  int  ysize;
  int  *matrix;
  int  automatic;
  int  norm;
  int  bias;
} UniMatrix;

/* Neede for functions copied from the GIMP */
typedef void (*QueryFunc) (GtkWidget *, gpointer, gpointer);


/*---- Prototypes ----*/

static void query (void);
static void run (char *name, int nparams, GParam  *param,
                 int *nreturn_vals, GParam **return_vals);

static void universal (GDrawable *drawable);
static void calc_norm (void);
/* static void uni_save_matrix (UniMatrix *matrix, char *fname); */
/* static void uni_load_matrix (char *fname); */
static UniMatrix *uni_new_matrix (void);
static UniMatrix *uni_new_matrix_with_defaults (gchar *name);
static void uni_add_to_list(UniMatrix *matrix);

static gint universal_dialog (void);
static void dialog_close_callback (GtkWidget *widget, gpointer data);
static void dialog_toggle_update (GtkWidget *widget, gpointer data);
static void dialog_entry_callback (GtkWidget *widget, gpointer data);
static void dialog_matrixentry_callback (GtkWidget *widget, gpointer data);
static void dialog_ok_callback (GtkWidget *widget, gpointer data);
static void dialog_selector_new_callback (GtkWidget *widget, gpointer data);
static void dialog_selector_get_callback (GtkWidget *widget, gpointer data);
static void dialog_selector_save_callback (GtkWidget *widget, gpointer data);
static void dialog_selector_del_callback (GtkWidget *widget, gpointer data);
static void dialog_do_selector_new_callback (GtkWidget *widget, 
            gpointer client_data, gpointer call_data);
static void dialog_selector_select (GtkWidget *widget, gpointer data);

/* Functions copied from the GIMP */
GtkWidget *query_string_box (char *title, char *message, char *initial, QueryFunc callback, gpointer data);
char *prune_filename (char *filename);


/*---- Variables ----*/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

static UniversalValues univals = {
 {0,0,0,
  0,1,0,
  0,0,0},
 1,
 1,
 0,
};

static UniversalInterface uniint = {
     FALSE /* run */
}; 

static UniDialog *unidlg = NULL;
static UniMatrix *current_matrix = NULL; 
  
/*---- Functions ----*/

MAIN()


static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "matrix(0,0)", "Filter Matrix (0,0)" },
    { PARAM_INT32, "matrix(0,1)", "Filter Matrix (0,1)" },
    { PARAM_INT32, "matrix(0,2)", "Filter Matrix (0,2)" },
    { PARAM_INT32, "matrix(1,0)", "Filter Matrix (1,0)" },
    { PARAM_INT32, "matrix(1,1)", "Filter Matrix (1,1)" },
    { PARAM_INT32, "matrix(1,2)", "Filter Matrix (1,2)" },
    { PARAM_INT32, "matrix(2,0)", "Filter Matrix (2,0)" },
    { PARAM_INT32, "matrix(2,1)", "Filter Matrix (2,1)" },
    { PARAM_INT32, "matrix(2,2)", "Filter Matrix (2,2)" },
    { PARAM_INT32, "auto", "Automatic Normalisation (n/y)" },
    { PARAM_INT32, "norm", "Normalisation value" },
    { PARAM_INT32, "bias", "Bias value" },     
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  
  gimp_install_procedure ("plug_in_universal_filter",
			  "Universal matrix filter",
			  "Filter the image by usage of convolution",
			  "O. Steinfatt",
			  "O. Steinfatt",
			  "June 1997, 0.0",
			  "<Image>/Filters/Generic/Universal",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs,
                          nreturn_vals,
			  args,
                          return_vals);
}


static void
run (char *name, int nparams, GParam *param, int *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];

  int    i;  

  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status;
    
  status = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;
  
  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  See how we will run  */
  switch (run_mode) {
  case RUN_INTERACTIVE:
    /*  Possibly retrieve data  */
    gimp_get_data("plug_in_universal", &univals);

    /*  Get information from the dialog  */
    if (!universal_dialog ())
      return;

    break;

  case RUN_NONINTERACTIVE:
    /*  Make sure all the arguments are present  */
    if (nparams != 15)
      status = STATUS_CALLING_ERROR;
    if (status == STATUS_SUCCESS) {
      for (i=0; i<(MATRIX_SIZE*MATRIX_SIZE); i++) 
        univals.matrix[i] = param[3+i].data.d_int32;
      univals.automatic=param[12].data.d_int32;
      univals.norm=param[13].data.d_int32;
      univals.bias=param[14].data.d_int32;
    }
    break;

  case RUN_WITH_LAST_VALS:
    /*  Possibly retrieve data  */
    gimp_get_data("plug_in_universal", &univals);
    break;
    
  default:
    break;
  }
  
  /*  Create texture  */
  if ((status == STATUS_SUCCESS) && (gimp_drawable_color (drawable->id) ||
      gimp_drawable_gray (drawable->id)))
    {
      /*  Set the tile cache size  */
      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

      /*  Run!  */
      universal (drawable);

      /*  If run mode is interactive, flush displays  */
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_displays_flush();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data("plug_in_universal", &univals,
                      sizeof(UniversalValues));
    }
  else
    {
      /* gimp_message ("Universal Filter: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }  
  
  values[0].data.d_status = status;
  
  gimp_drawable_detach(drawable);
}

static void calc_norm (void)
{
  int i;

  if (univals.automatic) {
    univals.norm=0;
    univals.bias=0;
    for (i=0; i<MATRIX_SIZE*MATRIX_SIZE; i++)
      univals.norm += univals.matrix[i];
    if (univals.norm==0) {
      univals.norm=1;
      univals.bias=128;
     }
    if (univals.norm<0) { 
      univals.norm=-univals.norm;
      univals.bias=255;
     } 
   }
}

static void
universal (GDrawable *drawable) 
{
  GPixelRgn	src_rgn,	/* Source image region */
		dst_rgn;	/* Destination image region */
  guchar	*src_rows[4],	/* Source pixel rows */
		*dst_row,	/* Destination pixel row */
		*dst_ptr,	/* Current destination pixel */
		*src_filters[3];/* Source pixels, ordered for filter */
  int		i,              /* Looping vars */
		x, y,		/* Current location in image */
		xmax,		/* Maximum filtered X coordinate */
		row,		/* Current row in src_rows */
		trow,		/* Looping var */
		count,		/* Current number of filled src_rows */
		width;		/* Byte width of the image */
 
  gint          sel_x1, sel_y1, sel_x2, sel_y2;   
  gint          sel_width, sel_height; 
  gint          img_bpp;
 
 /*
  * Calculate normalisation value
  */

  calc_norm();  

 /*
  * Find boundaries
  */
 
  gimp_drawable_mask_bounds (drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2); 
  sel_width = sel_x2 - sel_x1; 
  sel_height = sel_y2 - sel_y1; 
  img_bpp       = gimp_drawable_bpp(drawable->id);

 /*
  * Let the user know what we're doing...
  */

  gimp_progress_init("Filtering...");

 /*
  * Setup for filter...
  */

  gimp_pixel_rgn_init(&src_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init(&dst_rgn, drawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  width = sel_width * img_bpp;
  xmax  = width - img_bpp;

  for (row = 0; row < 4; row ++)
    src_rows[row] = g_malloc(width * sizeof(guchar));

  dst_row = g_malloc(width * sizeof(guchar));

 /*
  * Pre-load the first row for the filter...
  */

  gimp_pixel_rgn_get_row(&src_rgn, src_rows[0], sel_x1, sel_y1, sel_width);
  row   = 1;
  count = 1;

 /*
  * Filter...
  */

  for (y = sel_y1; y < sel_y2; y ++)
  {
   /*
    * Load the next pixel row...
    */

    if ((y + 1) < sel_y2)
    {
     /*
      * Check to see if our src_rows[] array is overflowing yet...
      */

      if (count >= 3)
        count --;

     /*
      * Grab the next row...
      */

      gimp_pixel_rgn_get_row(&src_rgn, src_rows[row], sel_x1, y + 1, sel_width);

      count ++;
      row = (row + 1) & 3;
    }
    else
    {
     /*
      * No more pixels at the bottom...  Drop the oldest samples...
      */
   
      count --;
    };

   /*
    * Now filter pixels and save the results...
    */

    if (count == 3)
    {
      for (i = 0, trow = (row + 1) & 3;
           i < 3;
           i ++, trow = (trow + 1) & 3)
	src_filters[i] = src_rows[trow];

      for (x = img_bpp, dst_ptr = dst_row + img_bpp;
           x < xmax;
           x ++, dst_ptr ++)
      {
	i = ((univals.matrix[4] * src_filters[1][x] +
              univals.matrix[0] * src_filters[0][x - img_bpp] + 
              univals.matrix[1] * src_filters[0][x] +
              univals.matrix[2] * src_filters[0][x + img_bpp] + 
              univals.matrix[3] * src_filters[1][x - img_bpp] +
              univals.matrix[5] * src_filters[1][x + img_bpp] + 
              univals.matrix[6] * src_filters[2][x - img_bpp] +
              univals.matrix[7] * src_filters[2][x] +
              univals.matrix[8] * src_filters[2][x + img_bpp])/univals.norm)
             +univals.bias;

	if (i < 0)
          *dst_ptr = 0;
	else if (i > 255)
          *dst_ptr = 255;
	else
          *dst_ptr = i;
      };

      gimp_pixel_rgn_set_row(&dst_rgn, dst_row + img_bpp, sel_x1 + 1, y, sel_width - 2);
    };

    if ((y & 15) == 0)
      gimp_progress_update((double)(y - sel_y1) / (double)sel_height);
  };

 /*
  * OK, we're done.  Free all memory used...
  */

  for (row = 0; row < 4; row ++)
    g_free(src_rows[row]);

  g_free(dst_row);

 /*
  * Update the screen...
  */

  gimp_drawable_flush(drawable);
  gimp_drawable_merge_shadow(drawable->id, TRUE);
  gimp_drawable_update(drawable->id, sel_x1, sel_y1, sel_width, sel_height);
}

static gint
universal_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *gtable;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *listbox;
  GtkWidget *vbox;
  GtkWidget *hbox;

  gchar **argv;
  gint  argc;
  guchar buffer[32];

  static struct {
    char		*label;
    GtkSignalFunc	callback;
  } buttons[] = {
    { "New",	(GtkSignalFunc) &dialog_selector_new_callback},
    { "Get",	(GtkSignalFunc) &dialog_selector_get_callback},
    { "Save",	(GtkSignalFunc) &dialog_selector_save_callback},
    { "Delete", (GtkSignalFunc) &dialog_selector_del_callback}
  };

  int i,j;

  /*  Set args  */
  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("universal");
  gtk_init (&argc, &argv);

  /*  Init global stucture */
  unidlg = g_new (UniDialog,1);
  /*  Dialog initialization  */
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Universal Filter");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) dialog_close_callback,
		      NULL);
  /* General table */
  gtable = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (gtable), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), gtable, TRUE, TRUE, 0);
  
  /*  Frame  */
  frame = gtk_frame_new ("Matrix");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_table_attach (GTK_TABLE (gtable), frame, 0, 1, 0, 1, GTK_FILL,
                      GTK_FILL, 0, 0 );
  
  /*  Matrix Table  */
  table = gtk_table_new (MATRIX_SIZE, MATRIX_SIZE, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  for (i=0; i<MATRIX_SIZE; i++) {
    for (j=0; j<MATRIX_SIZE; j++) {
      /*  Entry fields */  
      entry = gtk_entry_new ();
      gtk_table_attach (GTK_TABLE (table), entry, j, j+1, i, i+1, GTK_FILL,
                        GTK_FILL, 0, 0 );
      gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
      sprintf(buffer, "%d", univals.matrix[i*MATRIX_SIZE+j]);
      gtk_entry_set_text (GTK_ENTRY (entry), buffer);
      gtk_signal_connect (GTK_OBJECT (entry), "changed",
                         (GtkSignalFunc) dialog_matrixentry_callback
                          , &univals.matrix[i*MATRIX_SIZE+j]);
      gtk_widget_show (entry);
     }
   }
   
  gtk_widget_show (frame);
  gtk_widget_show (table); 

  /* Normalisation Frame */
  frame = gtk_frame_new ("Normalisation");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_table_attach (GTK_TABLE (gtable), frame, 0, 1, 1, 2, GTK_FILL,
                      GTK_FILL, 0, 0 );
  
  /* N. Table */
  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /*  Check button #1  */
  toggle = gtk_check_button_new_with_label ("Automatic");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 2, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), univals.automatic);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) dialog_toggle_update, &univals.automatic);
  gtk_widget_show (toggle);

  /* Entry 1 */
  
  unidlg->normlabel = gtk_label_new ("Divider");
  gtk_misc_set_alignment (GTK_MISC (unidlg->normlabel), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), unidlg->normlabel, 0, 1, 1, 2, GTK_FILL,
                    0, 5, 0);
  gtk_widget_set_sensitive (unidlg->normlabel,!univals.automatic);
  gtk_widget_show (unidlg->normlabel);
  
  unidlg->normentry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), unidlg->normentry, 1, 2, 1, 2, GTK_FILL,
                    GTK_FILL, 0, 0 );
  gtk_widget_set_usize (unidlg->normentry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%d", univals.norm); 
  gtk_entry_set_text (GTK_ENTRY (unidlg->normentry), buffer);
  gtk_signal_connect (GTK_OBJECT (unidlg->normentry), "changed",
                     (GtkSignalFunc) dialog_entry_callback
                      , &univals.norm);
  gtk_widget_set_sensitive (unidlg->normentry,!univals.automatic);
  gtk_widget_show (unidlg->normentry);

  /* Entry 2 */
  
  unidlg->biaslabel = gtk_label_new ("Bias");
  gtk_misc_set_alignment (GTK_MISC (unidlg->biaslabel), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), unidlg->biaslabel, 0, 1, 2, 3, GTK_FILL,
                    0, 5, 0);
  gtk_widget_set_sensitive (unidlg->biaslabel,!univals.automatic);
  gtk_widget_show (unidlg->biaslabel);
  
  unidlg->biasentry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), unidlg->biasentry, 1, 2, 2, 3, GTK_FILL,
                    GTK_FILL, 0, 0 );
  gtk_widget_set_usize (unidlg->biasentry, ENTRY_WIDTH, 0);
  sprintf(buffer, "%d", univals.bias); 
  gtk_entry_set_text (GTK_ENTRY (unidlg->biasentry), buffer);
  gtk_signal_connect (GTK_OBJECT (unidlg->biasentry), "changed",
                     (GtkSignalFunc) dialog_entry_callback
                      , &univals.bias);
  gtk_widget_set_sensitive (unidlg->biasentry,!univals.automatic);
  gtk_widget_show (unidlg->biasentry);

  /*  Button #1  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) dialog_ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /*  Button #2  */
  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (frame);
  gtk_widget_show (table);

  /* Selection Frame */
  frame = gtk_frame_new ("Defined");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_table_attach (GTK_TABLE (gtable), frame, 1, 2, 0, 2, GTK_FILL,
                      GTK_FILL, 0, 0 );
 
  /* Selection List */
  vbox = gtk_vbox_new (FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  
  /* gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
	  			  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS); */
  gtk_widget_set_usize (listbox, 100 /* DLG_LISTBOX_WIDTH */, 100 /* DLG_LISTBOX_HEIGHT */ );
  
  gtk_widget_show (listbox);

  unidlg->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (unidlg->list), GTK_SELECTION_BROWSE);
  gtk_container_add (GTK_CONTAINER (listbox), unidlg->list);
  gtk_widget_show (unidlg->list);

  uni_new_matrix_with_defaults("default"); 
  /*
   *	Buttons
   */
  hbox = gtk_hbox_new (FALSE, 0);
  for (i = 0; i < sizeof (buttons) / sizeof (buttons[0]); i++)
    {
      button = gtk_button_new_with_label (buttons[i].label);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  buttons[i].callback, dlg);
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
    }
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox); 
 
  /*  additional features diabled for the moment */
  /*  gtk_widget_show (frame); */

  /* Display all */

  gtk_widget_show (gtable);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return uniint.run;
}


static void
dialog_close_callback (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}


static void
dialog_toggle_update (GtkWidget *widget, gpointer data)
{
  int *toggle_val;
  char buffer[10];

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
   
  gtk_widget_set_sensitive (unidlg->normentry,!(*toggle_val));
  gtk_widget_set_sensitive (unidlg->normlabel,!(*toggle_val));
  gtk_widget_set_sensitive (unidlg->biasentry,!(*toggle_val));
  gtk_widget_set_sensitive (unidlg->biaslabel,!(*toggle_val));
  if (*toggle_val) {
    calc_norm();
    sprintf(buffer, "%d", univals.norm); 
    gtk_entry_set_text (GTK_ENTRY (unidlg->normentry), buffer);
    sprintf(buffer, "%d", univals.bias); 
    gtk_entry_set_text (GTK_ENTRY (unidlg->biasentry), buffer);
  }
}


static void
dialog_entry_callback (GtkWidget *widget, gpointer data)
{
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
dialog_matrixentry_callback (GtkWidget *widget, gpointer data)
{
  gint *text_val;
  char buffer[10];
  
  text_val = (gint *) data;

  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (univals.automatic) {
    calc_norm();
    sprintf(buffer, "%d", univals.norm); 
    gtk_entry_set_text (GTK_ENTRY (unidlg->normentry), buffer);
    sprintf(buffer, "%d", univals.bias); 
    gtk_entry_set_text (GTK_ENTRY (unidlg->biasentry), buffer);
  }
}

static void
dialog_ok_callback (GtkWidget *widget, gpointer data)
{
  uniint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
dialog_selector_new_callback (GtkWidget *widget, gpointer data)
{
  query_string_box ( "New Filtermatrix",
		     "Enter a name for the new Matrix",
		     "untitled",
		     dialog_do_selector_new_callback, unidlg);
}

static void
dialog_do_selector_new_callback (GtkWidget *widget, gpointer client_data,
                                 gpointer call_data)
{
  char		*new_name = call_data;
  UniMatrix	*matrix;

  g_assert (new_name != NULL);

  matrix = uni_new_matrix_with_defaults (new_name); 
}

static void
dialog_selector_get_callback (GtkWidget *widget, gpointer data)
{
  if (current_matrix==NULL) return;

  univals.automatic=current_matrix->automatic;
  univals.norm=current_matrix->norm;
  univals.bias=current_matrix->bias;
}

static void
dialog_selector_save_callback (GtkWidget *widget, gpointer data)
{ 
}

static void
dialog_selector_del_callback (GtkWidget *widget, gpointer data)
{
}

static void
dialog_selector_select (GtkWidget *widget, gpointer data)
{
  current_matrix=data;
}

/*
static void
uni_load_matrix (char *filename)
{
  FILE            *file;
  char            line[1024];
  UniMatrix       *matrix;
  int             i;
 
  g_assert(filename != NULL);

  file = fopen(filename, "r");
  if (!file) return;

  fgets(line, 1024, file);
  if (strcmp(line, "GIMP Matrix\n") != 0) return;
   
  matrix = uni_new_matrix();
  
  matrix->filename = g_strdup(filename);
  matrix->name     = g_strdup(prune_filename(filename));

  fgets(line, 1024, file);
  matrix->xsize =atoi(line);
  
  fgets(line, 1024, file);
  matrix->ysize =atoi(line);

  if ((matrix->xsize < 1) || (matrix->ysize < 1)) {
    g_warning("uni_load_matrix(): invalid number of rows/colums in \"%s\"",
                 filename);
    g_free(matrix);
    return;    
  }

  matrix->matrix = g_malloc(sizeof(int) * matrix->xsize * matrix->ysize); 

  for (i=0; i<(matrix->ysize * matrix->xsize); i++) {
    fgets(line, 1024, file);
    matrix->matrix[i]=atoi(line);
  }

  fgets(line, 1024, file);
  matrix->automatic =atoi(line);
  fgets(line, 1024, file);
  matrix->norm =atoi(line);
  fgets(line, 1024, file);
  matrix->bias =atoi(line);
     
  fclose(file);    
}
*/

/*
static void
uni_save_matrix (UniMatrix *matrix, char *filename)
{
  FILE            *file;
  int             i;

  g_assert(matrix != NULL);

  if (!filename) {
    g_warning("uni_save_matrix(): can not save matrix with NULL filename");
    return;
  }*/ /* if */

  /*
  file = fopen(filename, "w");
  if (!file) {
    g_warning("uni_save_matrix(): can't open \"%s\"", filename);
    return;
  } */ /* if */

     /* File format is:
      * 
      * GIMP Matrix
      * columns
      * rows
      * value(0,0) (column,row)
      * value(1,0)
      * ...
      * automatic
      * divider
      * bias
      */ 

  /*
  fprintf (file,"GIMP Matrix\n");
  fprintf (file,"%i\n",matrix->xsize);
  fprintf (file,"%i\n",matrix->ysize);
  for (i=0; i< (matrix->xsize * matrix->ysize); i++) 
    fprintf (file,"%i",matrix->matrix[i]);
  fprintf (file,"%i\n",matrix->automatic);
  fprintf (file,"%i\n",matrix->norm);
  fprintf (file,"%i\n",matrix->bias);
      
  fclose(file);    
}
*/

static UniMatrix *uni_new_matrix (void)
{
  UniMatrix *matrix;

  matrix = g_malloc(sizeof(UniMatrix));
 
  matrix->name = NULL;
  matrix->filename = NULL;
  matrix->xsize = 0;
  matrix->ysize = 0;
  matrix->matrix = NULL;
  matrix->automatic = 1;
  matrix->norm = 1;
  matrix->bias = 0;
  
  return matrix; 
}


static UniMatrix *uni_new_matrix_with_defaults (gchar *name)
{
  UniMatrix *matrix;
  int i;

  matrix = uni_new_matrix ();
 
  matrix->name = name;
  matrix->filename = name;
  matrix->xsize = 3;
  matrix->ysize = 3;
  matrix->matrix = g_malloc(sizeof(int)*9);
  for (i=0; i<9; i++)
    matrix->matrix[i]=0;
  matrix->matrix[4]= 1; 
  matrix->automatic = 1;
  matrix->norm = 1;
  matrix->bias = 0;
  
  uni_add_to_list (matrix);

  return matrix; 
}

static void uni_add_to_list(UniMatrix *matrix)
{
  GList     *list;
  GtkWidget *list_item;

  list_item = gtk_list_item_new_with_label(matrix->name);
  gtk_signal_connect(GTK_OBJECT(list_item), "select",
		     (GtkSignalFunc) dialog_selector_select,
		     (gpointer) matrix);
  gtk_widget_show (list_item);  
  list = g_list_append(NULL,list_item);
  gtk_list_insert_items(GTK_LIST(unidlg->list), list, 0);
}

/*************************************************************************/
/**									**/
/**			+++ Miscellaneous				**/
/**									**/
/*************************************************************************/

/*
  These query box functions are yanked from app/interface.c.	taka
 */

/*
 *  A text string query box
 */

typedef struct _QueryBox QueryBox;

struct _QueryBox
{
  GtkWidget *qbox;
  GtkWidget *entry;
  QueryFunc callback;
  gpointer data;
};

static void query_box_cancel_callback (GtkWidget *, gpointer);
static void query_box_ok_callback (GtkWidget *, gpointer);

GtkWidget *
query_string_box (char	      *title,
		  char	      *message,
		  char	      *initial,
		  QueryFunc    callback,
		  gpointer     data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;

  query_box = (QueryBox *) g_malloc (sizeof (QueryBox));

  qbox = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (qbox), title);
  gtk_window_position (GTK_WINDOW (qbox), GTK_WIN_POS_MOUSE);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (qbox)->action_area), 2);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) query_box_ok_callback,
		      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) query_box_cancel_callback,
		      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);
  gtk_widget_show (entry);

  query_box->qbox = qbox;
  query_box->entry = entry;
  query_box->callback = callback;
  query_box->data = data;

  gtk_widget_show (qbox);

  return qbox;
}

static void
query_box_cancel_callback (GtkWidget *w,
			   gpointer   client_data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) client_data;

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
query_box_ok_callback (GtkWidget *w,
		       gpointer	  client_data)
{
  QueryBox *query_box;
  char *string;

  query_box = (QueryBox *) client_data;

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) string);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

/* Function copied from general.c */
/* prune filename removes all of the leading path information to a filename */

char *
prune_filename (char *filename)
{
  char *last_slash = filename;

  while (*filename)
    if (*filename++ == '/')
      last_slash = filename;

  return last_slash;
}











