/* Plug-in to save .gpb (GIMP Pixmap Brush) files.
 * The format for pixmap brushes might well change, and this will have to
 * be updated.
 *
 * Copyright (C) 1999 Tor Lillqvist
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

/* set to the level of debugging output you want, 0 for none */
#define GPB_DEBUG 0

#define IFDBG(level) if (GPB_DEBUG >= level)

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "app/brush_header.h"
#include "app/pattern_header.h"

#define DUMMY_PATTERN_NAME "x"

#define MAXDESCLEN 256
#define MAXDIM 4

/* Parameters applicable each time we save a gpb or gih, saved
 * in the main gimp application between invocations of this plug-in.
 */
static struct {
  /* Use by both gpb and gih: */
  guint spacing;
  guchar description[MAXDESCLEN+1];
} info =
/* Initialize to this, change if non-interactive later */
{  
  10,
  "GIMP Pixmap Brush"
};

static gint run_flag = 0;
static gint num_layers_with_alpha;

/* Parameters related to one single gih file, collected in a struct
 * just for clarity.
 */
static struct {
  gint step;
  gint ncells;
  gint dim;
  gint cols;
  gint rows;
  gchar *placement;
  gint rank[MAXDIM];
  gchar *selection[MAXDIM];
} gihparms;

/* Declare some local functions.
 */
static void   query (void);
static void   run   (char    *name,
		     int      nparams,
		     GParam  *param,
		     int     *nreturn_vals,
		     GParam **return_vals);

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
  static GParamDef gpb_save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the brush in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the brush in" },
    { PARAM_INT32, "spacing", "Spacing of the brush" },
    { PARAM_STRING, "description", "Short description of the brush" },
  };
  static int ngpb_save_args = sizeof (gpb_save_args)
    / sizeof (gpb_save_args[0]);

  static GParamDef gih_save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the brush pipe in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the brush pipe in" },
    { PARAM_INT32, "spacing", "Spacing of the brush" },
    { PARAM_STRING, "description", "Short description of the brush pipe" },
  };
  static int ngih_save_args = sizeof (gih_save_args)
    / sizeof (gih_save_args[0]);

  gimp_install_procedure ("file_gpb_save",
			  "saves images in GIMP pixmap brush format", 
                          "This plug-in saves a layer of an image in "
			  "the GIMP pixmap brush format. "
                          "The image must have an alpha channel.",
                          "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          "<Save>/GPB",
			  "RGBA",
                          PROC_PLUG_IN,
                          ngpb_save_args, 0,
                          gpb_save_args, NULL);

  gimp_register_save_handler ("file_gpb_save", "gpb", "");

  gimp_install_procedure ("file_gih_save",
			  "saves images in GIMP pixmap brush pipe format", 
                          "This plug-in saves an image in "
			  "the GIMP pixmap brush pipe format. "
                          "The image must have an alpha channel."
			  "The image can be multi-layered, "
			  "and additionally the layers can be divided into "
			  "a rectangular array of brushes.",
                          "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          "<Save>/GIH",
			  "RGBA",
                          PROC_PLUG_IN,
                          ngih_save_args, 0,
                          gih_save_args, NULL);

  gimp_register_save_handler ("file_gih_save", "gih", "");
}

static void
adjustment_callback (GtkWidget *widget,
		     gpointer   data)
{
  *((gint *) data) = GTK_ADJUSTMENT (widget)->value;
}

static void
entry_callback (GtkWidget *widget,
		gpointer   data)
{
  if (data == info.description)
    {
      strncpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)), MAXDESCLEN);
      info.description[MAXDESCLEN]  = 0;
    }
}

static void
cb_callback (GtkWidget *widget,
	     gpointer   data)
{
  *((char **) data) = gtk_entry_get_text (GTK_ENTRY (widget));
}

static void
close_callback (GtkWidget *widget,
		gpointer   data)
{
  gtk_main_quit ();
}

static void
ok_callback (GtkWidget *widget,
	     gpointer    data)
{
  run_flag = 1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
common_save_dialog (GtkWidget *dlg,
		    GtkWidget *table)
{
  GtkWidget *button;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *box;
  GtkWidget *spinbutton;
  GtkWidget *entry;
  gchar buffer[12];

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);


  /*
   * Spacing: __
   */
  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Spacing (percent):");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (info.spacing, 1, 1000, 1, 10, 10);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, TRUE, 0);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2, 
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) adjustment_callback, &info.spacing);
  gtk_widget_show (spinbutton);
  gtk_widget_show (box);

  /*
   * Description: ___________
   */
  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Description:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, info.description);
  gtk_widget_show (entry);
}

static gint
gpb_save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *entry;
  gchar **argv;
  gint argc;
  gchar buffer[100];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gpb_save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save As Pixmap Brush");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /* The main table */
  table = gtk_table_new (2, 0, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  common_save_dialog (dlg, table);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static gint
gih_save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *table, *dimtable;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *entry;
  GtkWidget *box;
  GtkWidget *cb;
  GList *cbitems = NULL;
  gint i;
  gchar **argv;
  gint argc;
  gchar buffer[100];

  /* Setup default values */
  if (gihparms.rows >= 1 && gihparms.cols >= 1)
    gihparms.ncells = num_layers_with_alpha * gihparms.rows * gihparms.cols;
  else
    gihparms.ncells = 1;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gpb_save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save As Pixmap Brush Pipe");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /* The main table */
  table = gtk_table_new (2, 0, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  common_save_dialog (dlg, table);

  /*
   * Number of cells: ___
   */

  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Number of cells:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (gihparms.ncells, 1, 1000, 1, 10, 10);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, TRUE, 0);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) adjustment_callback, &gihparms.ncells);
  gtk_widget_show (spinbutton);
  gtk_widget_show (box);

  /*
   * Display as: __ rows x __ cols
   */
  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Display as:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  box = gtk_hbox_new (FALSE, 0);
  
  adjustment = gtk_adjustment_new (gihparms.rows, 1, 10, 1, 1, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				    GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) adjustment_callback, &gihparms.rows);
  gtk_widget_show (spinbutton);

  label = gtk_label_new (" rows of ");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  adjustment = gtk_adjustment_new (gihparms.cols, 1, 10, 1, 1, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				    GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) adjustment_callback, &gihparms.cols);
  gtk_widget_show (spinbutton);

  label = gtk_label_new (" columns");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  gtk_table_attach (GTK_TABLE (table), box, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (box);
  
  /*
   * Dimension: ___
   */

  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Dimension:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (gihparms.dim, 1, 5, 1, 1, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				    GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) adjustment_callback, &gihparms.dim);
  gtk_widget_show (spinbutton);
  gtk_widget_show (box);

  /*
   * Ranks: __ __ __ __ __
   */

  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Ranks:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  dimtable = gtk_table_new (MAXDIM, 1, FALSE);
  for (i = 0; i < MAXDIM; i++)
    {
      adjustment = gtk_adjustment_new (gihparms.rank[i], 0, 100, 1, 1, 1);
      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      box = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, TRUE, 0);
      gtk_table_attach (GTK_TABLE (dimtable), box, i, i + 1, 0, 1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 3, 0);
      gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			  (GtkSignalFunc) adjustment_callback, &gihparms.rank[i]);
      gtk_widget_show (spinbutton);
      gtk_widget_show (box);
    }
  gtk_table_attach (GTK_TABLE (table), dimtable, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (dimtable);

  /*
   * Selection: ______ ______ ______ ______ ______
   */
  
  gtk_table_resize (GTK_TABLE (table),
		    GTK_TABLE (table)->nrows + 1, GTK_TABLE (table)->ncols);

  label = gtk_label_new ("Selection:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  cbitems = g_list_append (cbitems, "incremental");
  cbitems = g_list_append (cbitems, "angular");
  cbitems = g_list_append (cbitems, "random");
  cbitems = g_list_append (cbitems, "velocity");
  cbitems = g_list_append (cbitems, "pressure");
  cbitems = g_list_append (cbitems, "xtilt");
  cbitems = g_list_append (cbitems, "ytilt");

  box = gtk_hbox_new (FALSE, 0);
  for (i = 0; i < MAXDIM; i++)
    {
      cb = gtk_combo_new ();
      gtk_combo_set_popdown_strings (GTK_COMBO (cb), cbitems);
      if (gihparms.selection[i])
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cb)->entry), gihparms.selection[i]);
      gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (cb)->entry), FALSE);

      gtk_box_pack_start (GTK_BOX (box), cb, FALSE, TRUE, 2);
      gtk_signal_connect (GTK_OBJECT (GTK_COMBO (cb)->entry), "changed",
			  (GtkSignalFunc) cb_callback, &gihparms.selection[i]);
      gtk_widget_show (cb);
    }

  g_list_free (cbitems);

  gtk_table_attach (GTK_TABLE (table), box, 1, 2,
		    GTK_TABLE (table)->nrows - 1, GTK_TABLE (table)->nrows,
		    GTK_SHRINK, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (box);
  
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  if (run_flag)
    {
      /* Fix up bogus values */
      gihparms.ncells =
	MIN (gihparms.ncells,
	     num_layers_with_alpha * gihparms.rows * gihparms.cols);
    }
  return run_flag;
}

static gboolean
try_fwrite (gpointer buffer,
	    guint   size,
	    guint   nitems,
	    FILE   *file)
{
  if (fwrite (buffer, size, nitems, file) < nitems)
    {
      g_message ("GPB: write error");
      fclose (file);
      return FALSE;
    }
  return TRUE;
}

static gboolean
save_one_gpb (FILE      *file,
	      GPixelRgn *pixel_rgn,
	      int	 index,
	      int        total)
{
  BrushHeader brushheader;
  PatternHeader patternheader;
  guint y, x;
  guchar *buffer;
  guint width, height;

  width = pixel_rgn->w;
  height = pixel_rgn->h;

  brushheader.header_size = g_htonl (sizeof (brushheader) + strlen (info.description) + 1);
  brushheader.version = g_htonl (2);
  brushheader.width = g_htonl (width);
  brushheader.height = g_htonl (height);
  brushheader.bytes = g_htonl (1);
  brushheader.magic_number = g_htonl (GBRUSH_MAGIC);
  brushheader.spacing = g_htonl (info.spacing);

  if (!try_fwrite (&brushheader, sizeof (brushheader), 1, file))
    return FALSE;
  if (!try_fwrite (info.description, strlen (info.description) + 1, 1, file))
    return FALSE;

  IFDBG(3) g_message ("saving gpb %d of %d: %dx%d, offset %d,%d stride %d drawable %d ",
		      index+1, total,
		      width, height, pixel_rgn->x, pixel_rgn->y,
		      pixel_rgn->rowstride, pixel_rgn->drawable->id);
  
  buffer = g_malloc (width * pixel_rgn->bpp);
  for (y = 0; y < height; y++)
    {
#if 0
      /* No; the brushes don't have to be the same size, of course. */
      if (y >= pixel_rgn->h)
	{
	  if (y == pixel_rgn->h)
	    memset (buffer, 0, width);
	  if (!try_fwrite (buffer, width, 1, file))
	    {
	      g_free (buffer);
	      return FALSE;
	    }
	}
      else
#endif
	{
	  gimp_pixel_rgn_get_row (pixel_rgn, buffer,
				  0 + pixel_rgn->x, y + pixel_rgn->y,
				  pixel_rgn->w);
	  for (x = 0; x < pixel_rgn->w; x++)
	    if (!try_fwrite (buffer + x*pixel_rgn->bpp + 3, 1, 1, file))
	      {
		g_free (buffer);
		return FALSE;
	      }
#if 0
	  if (width > pixel_rgn->w)
	    {
	      memset (buffer, 0, width - pixel_rgn->w);
	      if (!try_fwrite (buffer, width - pixel_rgn->w, 1, file))
		{
		  g_free (buffer);
		  return FALSE;
		}
	    }
#endif
	}
      if (y % 10 == 0)
	gimp_progress_update
	  (((double) index + 0.5 * y / pixel_rgn->h) / total);
    }

  gimp_progress_update (((double) index + 0.5) / total);

  patternheader.header_size = g_htonl (sizeof (patternheader) + strlen (DUMMY_PATTERN_NAME) + 1 );
  patternheader.version = g_htonl (1);
  patternheader.width = g_htonl (width);
  patternheader.height = g_htonl (height);
  patternheader.bytes = g_htonl (3);
  patternheader.magic_number = g_htonl (GPATTERN_MAGIC);

  if (!try_fwrite (&patternheader, sizeof (patternheader), 1, file))
    return FALSE;

  /* Pattern name is irrelevant */
  if (!try_fwrite (DUMMY_PATTERN_NAME, strlen (DUMMY_PATTERN_NAME) + 1, 1, file))
    return FALSE;

  for (y = 0; y < height; y++)
    {
#if 0
      if (y >= pixel_rgn->h)
	{
	  if (y == pixel_rgn->h)
	    memset (buffer, 0, width * 3);
	  if (!try_fwrite (buffer, width*3, 1, file))
	    {
	      g_free (buffer);
	      return FALSE;
	    }
	}
      else
#endif
	{
	  gimp_pixel_rgn_get_row (pixel_rgn, buffer,
				  0 + pixel_rgn->x, y + pixel_rgn->y, pixel_rgn->w);
	  for (x = 0; x < pixel_rgn->w; x++)
	    if (!try_fwrite (buffer + x*pixel_rgn->bpp, 3, 1, file))
	      return FALSE;
#if 0
	  if (width > pixel_rgn->w)
	    {
	      memset (buffer, 0, (width - pixel_rgn->w) * 3);
	      if (!try_fwrite (buffer, (width - pixel_rgn->w) * 3, 1, file))
		{
		  g_free (buffer);
		  return FALSE;
		}
	    }
#endif
	}
      
      if (y % 10 == 0)
	gimp_progress_update
	  (((double) index + 0.5 + 0.5 * y / pixel_rgn->h) / total);
    }
  g_free (buffer);

  gimp_progress_update (((double) index + 1.0) / total);

  return TRUE;
}

static gboolean
gpb_save_image (char   *filename,
		gint32  image_ID,
		gint32  drawable_ID)
{
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  FILE *file;
  gchar *temp;

  g_assert (gimp_drawable_has_alpha (drawable_ID));

  drawable = gimp_drawable_get (drawable_ID);
  gimp_tile_cache_size (gimp_tile_height () * drawable->width * 4);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  temp = g_strdup_printf ("Saving %s:", filename);
  gimp_progress_init (temp);
  g_free (temp);

  file = fopen (filename, "wb");

  if (file == NULL)
    {
      g_message ("GPB: can't open \"%s\"", filename);
      return FALSE;
    }

  if (!save_one_gpb (file, &pixel_rgn, 0, 1))
    return FALSE;

  fclose (file);

  return TRUE;
}

static void
init_pipe_parameters ()
{
  int i;

  gihparms.step = 100;
  gihparms.ncells = 1;
  gihparms.dim = 1;
  gihparms.cols = 1;
  gihparms.rows = 1;
  gihparms.placement = "constant";
  for (i = 0; i < MAXDIM; i++)
    gihparms.selection[i] = "random";
  gihparms.rank[0] = 1;
  for (i = 1; i < MAXDIM; i++)
    gihparms.rank[i] = 0;
}

static void
parse_brush_pipe_parameters (gchar *parameters)
{
  guchar *p, *q, *r, *s;	/* Don't you love single-char identifiers?  */
  gint i;

  IFDBG(2) g_message ("parsing parasite: %s", parameters);

  q = parameters;
  while ((p = strtok (q, " ")) != NULL)
    {
      q = NULL;
      r = strchr (p, ':');
      if (r)
	*r = 0;

      if (strcmp (p, "ncells") == 0)
	{
	  if (r)
	    gihparms.ncells = atoi (r + 1);
	}
      else if (strcmp (p, "step") == 0)
	{
	  if (r)
	    gihparms.step = atoi (r + 1);
	}
      else if (strcmp (p, "dim") == 0)
	{
	  if (r)
	    gihparms.dim = atoi (r + 1);
	}
      else if (strcmp (p, "cols") == 0)
	{
	  if (r)
	    gihparms.cols = atoi (r + 1);
	}
      else if (strcmp (p, "rows") == 0)
	{
	  if (r)
	    gihparms.rows = atoi (r + 1);
	}
      else if (strcmp (p, "placement") == 0)
	{
	  if (r)
	    gihparms.placement = g_strdup (r + 1);
	}
      else if (strncmp (p, "rank", strlen ("rank")) == 0 && r)
	{
	  if (r)
	    {
	      i = atoi (p + strlen ("rank"));
	      if (i >= 0 && i < gihparms.dim)
		gihparms.rank[i] = atoi (r + 1);
	    }
	}
      else if (strncmp (p, "sel", strlen ("sel")) == 0 && r)
	{
	  if (r)
	    {
	      i = atoi (p + strlen ("sel"));
	      if (i >= 0 && i < gihparms.dim)
		gihparms.selection[i] = g_strdup (r + 1);
	    }
	}
      if (r)
	*r = ':';
    }

  IFDBG(2) g_message ("parsed parasite: "
		      "ncells:%d step:%d dim:%d cols:%d rows:%d "
		      "placement:%s "
		      "rank0:%d rank1:%d rank2:%d rank3:%d"
		      "sel%d:%s sel%d:%s sel%d:%s ",
		      gihparms.ncells, gihparms.step,
		      gihparms.dim, gihparms.cols, gihparms.rows,
		      gihparms.placement,
		      gihparms.rank[0], gihparms.rank[1],
		      gihparms.rank[2], gihparms.rank[3],
		      gihparms.selection[0], gihparms.selection[1],
		      gihparms.selection[2], gihparms.selection[3]);
}

static gchar *
build_brush_pipe_parameters ()
{
  GString *s = g_string_new (NULL);
  gchar *str;
  
  int i;

  g_string_sprintf (s, "ncells:%d step:%d dim:%d cols:%d rows:%d placement:%s",
		    gihparms.ncells, gihparms.step,
		    gihparms.dim,
		    gihparms.cols, gihparms.rows,
		    gihparms.placement);

  for (i = 0; i < gihparms.dim; i++)
    {
      g_string_sprintfa (s, " rank%d:%d", i, gihparms.rank[i]);
      g_string_sprintfa (s, " sel%d:%s", i, gihparms.selection[i]);
    }

  str = s->str;
  g_string_free (s, FALSE);

  IFDBG(2) g_message ("built parasite string: %s", str);
  return str;
}

static gboolean
gih_save_image (char   *filename,
		gint32  image_ID,
		gint32  drawable_ID)
{
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  FILE *file;
  Parasite *pipe_parasite;
  gchar *msg, *pars, *ncells;
  gint32 *layer_ID;
  gint nlayers, layer, row, col;
  gint imagew, imageh, offsetx, offsety;
  gint k, x, y, thisx, thisy, xnext, ynext, thisw, thish;

  imagew = gimp_image_width (image_ID);
  imageh = gimp_image_height (image_ID);
  gimp_tile_cache_size (gimp_tile_height () * imagew * 4);

  msg = g_strdup_printf ("Saving %s:", filename);
  gimp_progress_init (msg);
  g_free (msg);

  file = fopen (filename, "wb");

  if (file == NULL)
    {
      g_message ("GPB: can't open \"%s\"", filename);
      return FALSE;
    }

  pars = build_brush_pipe_parameters ();
  ncells = g_strdup_printf ("%d ", gihparms.ncells);
  if (!(try_fwrite (info.description, strlen (info.description), 1, file)
	&& try_fwrite ("\n", 1, 1, file)
	&& try_fwrite (ncells, strlen (ncells), 1, file)
	&& try_fwrite (pars, strlen (pars), 1, file)
	&& try_fwrite ("\n", 1, 1, file)))
    {
      g_free (pars);
      g_free (ncells);
      return FALSE;
    }

  pipe_parasite = parasite_new ("gimp-brush-pipe-parameters",
				PARASITE_PERSISTENT,
				strlen (pars) + 1, pars);
  gimp_image_attach_parasite (image_ID, pipe_parasite);
  parasite_free (pipe_parasite);

  g_free (pars);
  g_free (ncells);

  layer_ID = gimp_image_get_layers (image_ID, &nlayers);
  
  k = 0;
  for (layer = 0; layer < nlayers; layer++)
    {
      if (!gimp_drawable_has_alpha (layer_ID[layer]))
	continue;
      drawable = gimp_drawable_get (layer_ID[layer]);
      gimp_drawable_offsets(layer_ID[layer], &offsetx, &offsety);

      for (row = 0; row < gihparms.rows; row++)
	{
	  y = (row * imageh) / gihparms.rows ;
	  ynext = ((row + 1) * imageh / gihparms.rows);
	  /* Assume layer is offset to positive direction in x and y.
	   * That's reasonable, as otherwise all of the layer
	   * won't be visible.
	   * thisy and thisx are in the drawable's coordinate space.
	   */
	  thisy = MAX (0, y - offsety);
	  thish = (ynext - offsety) - thisy;
	  thish = MIN (thish, drawable->height - thisy);
	  for (col = 0; col < gihparms.cols; col++)
	    {
	      x = (col * imagew / gihparms.cols);
	      xnext = ((col + 1) * imagew / gihparms.cols);
	      thisx = MAX (0, x - offsetx);
	      thisw = (xnext - offsetx) - thisx;
	      thisw = MIN (thisw, drawable->width - thisx);
	      IFDBG(3) g_message ("gimp_pixel_rgn_init %d %d %d %d",
				  thisx, thisy, thisw, thish);
	      gimp_pixel_rgn_init (&pixel_rgn, drawable, thisx, thisy, 
				   thisw, thish, FALSE, FALSE);
	      if (!save_one_gpb (file, &pixel_rgn, k, gihparms.ncells))
		return FALSE;
	      k++;
	    }
	}

    }
  
  gimp_progress_update (1.0);

  fclose (file);

  return TRUE;
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  Parasite *pipe_parasite;
  gint32 image_ID, *layer_ID;
  gint nlayers, layer;
  gchar *layer_name;

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  *nreturn_vals = 1;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_gpb_save") == 0)
    {
      switch (run_mode) {
      case RUN_INTERACTIVE:
	/*  Possibly retrieve data  */
	gimp_get_data ("file_gpb_save", &info);
	if (!gpb_save_dialog ())
	  return;
	break;

      case RUN_NONINTERACTIVE:  /* FIXME - need a real RUN_NONINTERACTIVE */
	if (nparams != 7)
	  status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS)
	  {
	    info.spacing = param[5].data.d_int32;
	    strncpy (info.description, param[6].data.d_string, MAXDESCLEN);
	    info.description[MAXDESCLEN] = 0;
	  }
	break;

      case RUN_WITH_LAST_VALS:
	gimp_get_data ("file_gpb_save", &info);
	break;
      }

      if (gpb_save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  gimp_set_data ("file_gpb_save", &info, sizeof (info));
	  status = STATUS_SUCCESS;
	}
      else
	status = STATUS_EXECUTION_ERROR;
    }
  else if (strcmp (name, "file_gih_save") == 0)
    {
      image_ID = param[1].data.d_int32;
      layer_ID = gimp_image_get_layers (image_ID, &nlayers);
      num_layers_with_alpha = 0;
      for (layer = 0; layer < nlayers; layer++)
	{
	  if (!gimp_drawable_has_alpha (layer_ID[layer]))
	    {
	      layer_name = gimp_layer_get_name (layer_ID[layer]);
	      g_message ("Layer %s doesn't have an alpha channel, skipped",
			 layer_name);
	      g_free (layer_name);
	    }
	  num_layers_with_alpha++;
	}
      
      IFDBG(2) g_message ("nlayers:%d num_layers_with_alpha:%d",
			  nlayers, num_layers_with_alpha);
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gih_save", &info);
	  pipe_parasite = gimp_image_find_parasite (image_ID, "gimp-brush-pipe-parameters");
	  init_pipe_parameters ();
	  if (pipe_parasite)
	    {
	      parse_brush_pipe_parameters (pipe_parasite->data);
	    }

	  if (!gih_save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:  /* FIXME - need a real RUN_NONINTERACTIVE */
	  if (nparams != 7)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      info.spacing = param[5].data.d_int32;
	      strncpy (info.description, param[6].data.d_string, MAXDESCLEN);
	      info.description[MAXDESCLEN] = 0;
	    }
	  break;
	  
	case RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_gih_save", &info);
	  break;
	}
      
      if (gih_save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  gimp_set_data ("file_gih_save", &info, sizeof (info));
	  status = STATUS_SUCCESS;
	}
      else
	status = STATUS_EXECUTION_ERROR;
    }
  values[0].data.d_status = status;
}
