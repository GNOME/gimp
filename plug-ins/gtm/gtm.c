/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GTM plug-in --- GIMP Table Magic
 * Allows images to be saved as HTML tables with different colored cells.
 * It doesn't  have very much practical use other than being able to
 * easily design a table by "painting" it in GIMP, or to make small HTML
 * table images/icons.
 *
 * Copyright (C) 1997 Daniel Dunbar
 * Email: ddunbar@diads.com
 * WWW:   http://millennium.diads.com/gimp/
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

/* Version 1.0:
 * Once I first found out that it was possible to have pixel level control
 * of HTML tables I instantly realized that it would be possible, however
 * pointless, to save an image as a, albeit huge, HTML table.
 *
 * One night when I was feeling in an adventourously stupid programming mood
 * I decided to write a program to do it.
 *
 * At first I just wrote a really ugly hack to do it, which I then planned
 * on using once just to see how it worked, and then posting a URL and 
 * laughing about it on #gimp.  As it turns out, tigert thought it actually
 * had potential to be a useful plugin, so I started adding features and
 * and making a nice UI.
 *
 * It's still not very usefull, but I did manage to significantly improve my
 * C programming skills in the process, so it was worth it.
 *
 * If you happen to find it usefull I would appreciate any email about it.
 *                                     - Daniel Dunbar
 *                                       ddunbar@diads.com
 */


#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* Typedefs */

typedef struct {
  char captiontxt[256];
  char cellcontent[256];
  char clwidth[256];
  char clheight[256];
  gint fulldoc;
  gint caption;
  gint border;
  gint spantags;
  gint tdcomp;
  gint cellpadding;
  gint cellspacing;
} GTMValues;

typedef struct {
  gint run;
} GTMInterface;

/* Variables */

static GTMInterface bint =
{
  FALSE  /* run */
};

static GTMValues gtmvals =
{
  "Made with GIMP Table Magic",  /* caption text */
  "&nbsp;",  /* cellcontent text */
  "",    /* cell width text */
  "",    /* cell height text */
  1,     /* fulldoc */
  0,     /* caption */
  2,     /* border */
  0,     /* spantags */
  0,     /* tdcomp */
  4,     /* cellpadding */
  0      /* cellspacing */
};

/* Declare some local functions */

static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint   save_image (char   *filename,
			  GDrawable  *drawable);
static gint   save_dialog ();

static gint   color_comp (guchar *buffer, guchar *buf2);
static void   save_close_callback  (GtkWidget *widget, gpointer   data);
static void   gtm_entry_callback  (GtkWidget *widget, gpointer   data);
static void   gtm_toggle_callback  (GtkWidget *widget, gpointer   data);
static void   save_ok_callback     (GtkWidget *widget, gpointer   data);
static void   gtm_caption_callback     (GtkWidget *widget, gpointer   data);
static void   gtm_cellcontent_callback     (GtkWidget *widget, gpointer   data);
static void   gtm_clwidth_callback     (GtkWidget *widget, gpointer   data);
static void   gtm_clheight_callback     (GtkWidget *widget, gpointer   data);
static void   set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
  };

  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_GTM_save",
                          "GIMP Table Magic",
                          "Allows you to draw an HTML table in GIMP. See help for more info.",
                          "Daniel Dunbar",
                          "Daniel Dunbar",
                          "1998",
                          "<Save>/HTML",
			  "RGB*, GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_save_handler ("file_GTM_save", "htm,html", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GDrawable *drawable;

  drawable = gimp_drawable_get (param[2].data.d_int32);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  gimp_get_data ("file_GTM_save", &gtmvals);

  if (save_dialog ()) {
    save_image (param[3].data.d_string, drawable);
    values[0].data.d_status = STATUS_SUCCESS;  
  }

  gimp_set_data ("file_GTM_save", &gtmvals, sizeof (GTMValues));
}

static gint
save_image (char   *filename,
	    GDrawable  *drawable)
{
  int row,col, cols, rows, x, y;
  int colcount, colspan, rowspan;
  int palloc[drawable->width][drawable->height];
  guchar *buffer, *buf2;
  guchar *width, *height;
  GPixelRgn pixel_rgn;
  char *name;

  FILE *fp, *fopen();
   
  fp = fopen(filename, "w");
  if (gtmvals.fulldoc) {
    fprintf (fp,"<HTML>\n<HEAD><TITLE>%s</TITLE></HEAD>\n<BODY>\n",filename);
    fprintf (fp,"<H1>%s</H1>\n",filename);
  }
  fprintf (fp,"<TABLE BORDER=%d CELLPADDING=%d CELLSPACING=%d>\n",gtmvals.border,gtmvals.cellpadding,gtmvals.cellspacing);
  if (gtmvals.caption)
    fprintf (fp,"<CAPTION>%s</CAPTION>\n",gtmvals.captiontxt); 

  name = g_malloc (strlen (filename) + 11);
  sprintf (name, "Saving %s:", filename);
  gimp_progress_init (name);
  g_free (name);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  cols = drawable->width;
  rows = drawable->height;
  buffer = g_new(guchar,drawable->bpp);
  buf2 = g_new(guchar,drawable->bpp);

  width = malloc (1);
  height = malloc (1);
  sprintf(width," ");
  sprintf(height," ");
  if (strcmp (gtmvals.clwidth, "") != 0) {
    width = malloc (strlen (gtmvals.clwidth) + 11);
    sprintf(width," WIDTH=\"%s\"",gtmvals.clwidth);
  }
  if (strcmp (gtmvals.clheight, "") != 0) {
    height = malloc (strlen (gtmvals.clheight) + 13);
    sprintf(height," HEIGHT=\"%s\" ",gtmvals.clheight);
  }  
  
  /* Initialize array to hold ROWSPAN and COLSPAN cell allocation table */

  for (row=0; row < rows; row++)
    for (col=0; col < cols; col++)
      palloc[col][row]=1;

  colspan=0;
  rowspan=0;

  for (y = 0; y < rows; y++) {
    fprintf (fp,"   <TR>\n");
    for (x = 0; x < cols; x++) {
      gimp_pixel_rgn_get_pixel(&pixel_rgn, buffer, x, y);

      /* Determine ROWSPAN and COLSPAN */

      if (gtmvals.spantags) { 
	col=x;
	row=y;
	colcount=0;
	colspan=0;
	rowspan=0;
	gimp_pixel_rgn_get_pixel(&pixel_rgn, buf2, col, row);
	
	while (color_comp(buffer,buf2) && palloc[col][row] == 1 && row < drawable->height) {
	  while (color_comp(buffer,buf2) && palloc[col][row] == 1 && col < drawable->width ) {
	    colcount++;
	    col++;
	    gimp_pixel_rgn_get_pixel(&pixel_rgn, buf2, col, row);
	  }
	  
	  if (colcount != 0) {
	    row++;
	    rowspan++;
	  }
	  
	  if (colcount < colspan || colspan == 0)
	    colspan=colcount;
	  
	  col=x;
	  colcount=0;
	  gimp_pixel_rgn_get_pixel(&pixel_rgn, buf2, col, row);
	}
	
	if (colspan > 1 || rowspan > 1) {
	  for (row=0; row < rowspan; row++)
	    for (col=0; col < colspan; col++)
	      palloc[col+x][row+y]=0;
	  palloc[x][y]=2;
	}
      }

      if (palloc[x][y]==1)
	fprintf (fp,"      <TD%s%sBGCOLOR=#%02x%02x%02x>",width,height,buffer[0],buffer[1],buffer[2]);

      if (palloc[x][y]==2)
	fprintf (fp,"      <TD ROWSPAN=\"%d\" COLSPAN=\"%d\"%s%sBGCOLOR=#%02x%02x%02x>",rowspan,colspan,width,height,buffer[0],buffer[1],buffer[2]);

      if (palloc[x][y]!=0) {
	if (gtmvals.tdcomp)
	  fprintf (fp,"%s</TD>\n",gtmvals.cellcontent);
	else 
	  fprintf (fp,"\n      %s\n      </TD>\n",gtmvals.cellcontent);
      }
    }
    fprintf (fp,"   </TR>\n");
    gimp_progress_update ((double) y / (double) rows);
  }

  if (gtmvals.fulldoc)
    fprintf (fp,"</TABLE></BODY></HTML>\n");  
  else fprintf (fp,"</TABLE>\n");
  fclose(fp);
  gimp_drawable_detach (drawable);
  free(width);
  free(height);

  return 1;
}

static gint save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *toggle;
  GtkTooltips *tips;
  GdkColor tips_fg, tips_bg;
  gchar **argv;
  guchar buffer[32];
  gint argc;

  bint.run=FALSE;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "GIMP HTML Magic");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) save_close_callback,
                      NULL);

  /* Initialize Tooltips */

  /* use black as foreground: */
  tips = gtk_tooltips_new ();
  tips_fg.red   = 0;
  tips_fg.green = 0;
  tips_fg.blue  = 0;
  /* postit yellow (khaki) as background: */
  gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_fg);
  tips_bg.red   = 61669;
  tips_bg.green = 59113;
  tips_bg.blue  = 35979;
  gdk_color_alloc (gtk_widget_get_colormap (dlg), &tips_bg);
  gtk_tooltips_set_colors (tips,&tips_bg,&tips_fg);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE,
 TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE,
 TRUE, 0);
  gtk_widget_show (button);

  /* HTML Page Options */

  frame = gtk_frame_new ("HTML Page Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg) -> vbox), frame, TRUE, TRUE, 0);

  toggle = gtk_check_button_new_with_label ("Generate Full HTML Document");
  gtk_container_add (GTK_CONTAINER(frame), toggle);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gtm_toggle_callback, 
		      &gtmvals.fulldoc);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gtmvals.fulldoc);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If checked GTM will output a full HTML document with <HTML>, <BODY>, etc. tags instead of just the table html.");

  gtk_widget_show (frame);

  /* HTML Table Creation Options */

  frame = gtk_frame_new ("Table Creation Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg) -> vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  toggle = gtk_check_button_new_with_label ("Use Cellspan");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		     (GtkSignalFunc) gtm_toggle_callback, 
		     &gtmvals.spantags);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gtmvals.spantags);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"If checked GTM will replace any rectangular sections of identically colored blocks with one large cell with ROWSPAN and COLSPAN values.");

  toggle = gtk_check_button_new_with_label ("Compress TD tags");
  gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		     (GtkSignalFunc) gtm_toggle_callback, 
		     &gtmvals.tdcomp);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gtmvals.tdcomp);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"Checking this tag will cause GTM to leave no whitespace between the TD tags and the cellcontent.  This is only necessary for pixel level positioning control.");

  toggle = gtk_check_button_new_with_label ("Caption");
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		     (GtkSignalFunc) gtm_toggle_callback, 
		     &gtmvals.caption);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), gtmvals.caption);
  gtk_widget_show (toggle);
  set_tooltip(tips,toggle,"Check if you would like to have the table captioned.");

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_set_usize (entry, 100, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                    (GtkSignalFunc) gtm_caption_callback,
                    NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.captiontxt);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The text for the table caption.");

  label = gtk_label_new ("Cell Content");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3, GTK_FILL, 0, 5, 0);
  gtk_widget_set_usize (entry, 100, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                    (GtkSignalFunc) gtm_cellcontent_callback,
                    NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.cellcontent);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The text to go into each cell.");

  gtk_widget_show (table);
  gtk_widget_show (frame);
 
  /* HTML Table Options */

  frame = gtk_frame_new ("Table Options");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg) -> vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (5, 4, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ("Border");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 35, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) gtm_entry_callback,
                      &gtmvals.border);
  sprintf(buffer, "%d", gtmvals.border);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The number of pixels in the table border.  Can only be a number.");

  label = gtk_label_new ("Width");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);
    
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 35, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) gtm_clwidth_callback,
                      NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.clwidth);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The width for each table cell.  Can be a number or a percent.");

  label = gtk_label_new ("Height");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 35, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) gtm_clheight_callback,
                      NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), gtmvals.clheight);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The height for each table cell.  Can be a number or a percent.");

  label = gtk_label_new ("Cell-Padding");
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 35, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) gtm_entry_callback,
                      &gtmvals.cellpadding);
  sprintf(buffer, "%d", gtmvals.cellpadding);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The amount of cellpadding.  Can only be a number.");


  label = gtk_label_new ("Cell-Spacing");
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2, GTK_FILL, 0, 5, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 35, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
                      (GtkSignalFunc) gtm_entry_callback,
                      &gtmvals.cellspacing);
  sprintf(buffer, "%d", gtmvals.cellspacing);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_widget_show (entry);
  set_tooltip(tips,entry,"The amount of cellspacing.  Can only be a number.");


  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}

static gint color_comp (guchar *buffer, guchar *buf2) {
  if (buffer[0] == buf2[0] && buffer[1] == buf2[1] && buffer[2] == buf2[2])
    return 1;
  else
    return 0;
}  


/*  Save interface functions  */

static void gtm_toggle_callback (GtkWidget *widget, gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void gtm_entry_callback (GtkWidget *widget, gpointer data)
{
  gint *text_val;

  text_val = (gint*)data;
  *text_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void save_close_callback (GtkWidget *widget, gpointer   data)
{
  gtk_main_quit ();
}

static void save_ok_callback (GtkWidget *widget, gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void gtm_caption_callback (GtkWidget *widget, gpointer   data)
{
  strcpy(gtmvals.captiontxt, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void gtm_cellcontent_callback (GtkWidget *widget, gpointer   data)
{
  strcpy(gtmvals.cellcontent, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void gtm_clwidth_callback (GtkWidget *widget, gpointer   data)
{
  strcpy(gtmvals.clwidth, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void gtm_clheight_callback (GtkWidget *widget, gpointer   data)
{
  strcpy(gtmvals.clheight, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
set_tooltip (GtkTooltips *tooltips, GtkWidget *widget, const char *desc)
{
  if (desc && desc[0])
    gtk_tooltips_set_tip (tooltips, widget, (char *) desc, NULL);
}
