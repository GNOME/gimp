/* Plug-in to save .gih (GIMP Brush Pipe) files.
 *
 * Copyright (C) 1999 Tor Lillqvist
 * Copyright (C) 2000 Jens Lautenbacher, Sven Neumann
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

  /* Example of how to call file_gih_save from script-fu:
     
  (let ((ranks (cons-array 1 'byte)))
    (aset ranks 0 12)
    (file-gih-save 1
		   img
		   drawable
		   "foo.gih"
		   "foo.gih"
		   100
		   "test brush"
		   125
		   125
		   3
		   4
		   1
		   ranks
		   1
		   '("random")))
  */


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpparasiteio.h>

#include "app/brush_header.h"

#include "libgimp/stdplugins-intl.h"


#define DUMMY_PATTERN_NAME "x"

#define MAXDESCLEN 256

/* Parameters applicable each time we save a gih, saved in the 
 * main gimp application between invocations of this plug-in.
 */
static struct
{
  guint  spacing;
  guchar description[MAXDESCLEN+1];
} info =
/* Initialize to this, change if non-interactive later */
{  
  10,
  "GIMP Brush Pipe"
};


static gint run_flag = 0;
static gint num_useable_layers;


static GimpPixPipeParams gihparms;

typedef struct
{
  GimpOrientationType orientation;
  gint32 image;
  gint32 toplayer;
  gint nguides;
  gint32 *guides;
  gint *value;
  GtkWidget *count_label;	/* Corresponding count adjustment, */
  gint *count;			/* cols or rows */
  gint *other_count;		/* And the other one */
  GtkObject *ncells;
  GtkObject *rank0;
  GtkWidget *warning_label;
} SizeAdjustmentData;

/* static gint32 *vguides, *hguides;       */
/* static gint nvguides = 0, nhguides = 0; */

/* Declare some local functions.
 */
static void   query    (void);
static void   run      (gchar      *name,
			gint        nparams,
			GimpParam  *param,
			gint       *nreturn_vals,
			GimpParam **return_vals);

static gint      gih_save_dialog (gint32        image_ID);
static gboolean  save_one_brush  (gint          fd,
				  GimpPixelRgn *pixel_rgn);
static gboolean  gih_save_image  (gchar        *filename,
				  gint32        image_ID,
				  gint32        orig_image_ID,
				  gint32        drawable_ID);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef gih_save_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the brush pipe in" },
    { GIMP_PDB_STRING,   "raw_filename", "The name of the file to save the brush pipe in" },
    { GIMP_PDB_INT32,    "spacing",      "Spacing of the brush" },
    { GIMP_PDB_STRING,   "description",  "Short description of the brush pipe" },
    { GIMP_PDB_INT32,    "cell_width",	 "Width of the brush cells" },
    { GIMP_PDB_INT32,    "cell_height",	 "Width of the brush cells" },
    { GIMP_PDB_INT8,     "display_cols",   "Display column number" },
    { GIMP_PDB_INT8,     "display_rows",   "Display row number" },
    { GIMP_PDB_INT32,     "dimension",	 "Dimension of the brush pipe" },
    /* The number of rank and sel args depend on the dimension */ 
    { GIMP_PDB_INT8ARRAY,"rank",		 "Ranks of the dimensions" },
    { GIMP_PDB_INT32,     "dimension",	 "Dimension (again)" },
    { GIMP_PDB_STRINGARRAY, "sel",	 "Selection modes" }
  };
  static const gint ngih_save_args = (sizeof (gih_save_args) /
				      sizeof (gih_save_args[0]));

  gimp_install_procedure ("file_gih_save",
			  "saves images in GIMP brush pipe format", 
			  "This plug-in saves an image in the GIMP brush pipe format. The image must have an alpha chnannel and can be multi-layered, and additionally the layers can be divided into a rectangular array of brushes.",
			  "Tor Lillqvist",
			  "Tor Lillqvist",
			  "1999",
			  "<Save>/GIH",
			  "RGBA, GRAYA",
			  GIMP_PLUGIN,
                          ngih_save_args, 0,
			  gih_save_args, NULL);
  
  gimp_register_save_handler ("file_gih_save",
			      "gih",
			      "");
}


static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpRunModeType  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  GimpParasite *pipe_parasite;
  gint32        image_ID;
  gint32        orig_image_ID;
  gint32        drawable_ID;
  gint32       *layer_ID;
  gint          nlayers, layer;
  gchar        *layer_name;
  gint          i;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *return_vals  = values;
  *nreturn_vals = 1;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      INIT_I18N_UI();
    }
  else
    {
      INIT_I18N();
    }

  if (strcmp (name, "file_gih_save") == 0)
    {
      image_ID = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32; 

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init ("gih", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "GIH",
				      GIMP_EXPORT_CAN_HANDLE_RGB    |
				      GIMP_EXPORT_CAN_HANDLE_GRAY   |
				      GIMP_EXPORT_CAN_HANDLE_ALPHA  | 
				      GIMP_EXPORT_CAN_HANDLE_LAYERS |
				      GIMP_EXPORT_NEEDS_ALPHA);
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      layer_ID = gimp_image_get_layers (image_ID, &nlayers);
      num_useable_layers = 0;
      for (layer = 0; layer < nlayers; layer++)
	{
	  if (!gimp_drawable_has_alpha (layer_ID[layer]))
	    {
	      layer_name = gimp_layer_get_name (layer_ID[layer]);
	      g_message (_("Layer %s doesn't have an alpha channel, skipped"),
			 layer_name);
	      g_free (layer_name);
	    }
	  num_useable_layers++;
	}

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gih_save", &info);
	  pipe_parasite =
	    gimp_image_parasite_find (orig_image_ID,
				      "gimp-brush-pipe-parameters");
	  gimp_pixpipe_params_init (&gihparms);
	  if (pipe_parasite)
	    gimp_pixpipe_params_parse (pipe_parasite->data, &gihparms);

	  if (!gih_save_dialog (image_ID))
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  if (nparams != 15)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      info.spacing = param[5].data.d_int32;
	      strncpy (info.description, param[6].data.d_string, MAXDESCLEN);
	      info.description[MAXDESCLEN] = 0;

	      gimp_pixpipe_params_init (&gihparms);

	      gihparms.cellwidth  =  param[7].data.d_int32;
	      gihparms.cellheight =  param[8].data.d_int32;
	      gihparms.cols       =  param[9].data.d_int8;
	      gihparms.rows       = param[10].data.d_int8;
	      gihparms.dim        = param[11].data.d_int32;
	      gihparms.ncells     = 1; 

	      if (param[13].data.d_int32 != gihparms.dim)
		{
		  status = GIMP_PDB_CALLING_ERROR;
		}
	      else
		{
		  for (i = 0; i < gihparms.dim; i++)
		    {
		      gihparms.rank[i]      = param[12].data.d_int8array[i];
		      gihparms.selection[i] = g_strdup (param[14].data.d_stringarray[i]);
		      gihparms.ncells       *= gihparms.rank[i];
		    }
		}
	    }
	  break;
	  
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_gih_save", &info);
	  pipe_parasite =
	    gimp_image_parasite_find (orig_image_ID,
				      "gimp-brush-pipe-parameters");
	  gimp_pixpipe_params_init (&gihparms);
	  if (pipe_parasite)
	    gimp_pixpipe_params_parse (pipe_parasite->data, &gihparms);
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (gih_save_image (param[3].data.d_string, 
			      image_ID, orig_image_ID, drawable_ID))
	    {
	      gimp_set_data ("file_gih_save", &info, sizeof (info));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);

  values[0].data.d_status = status;
}

static void
size_adjustment_callback (GtkWidget *widget,
			  gpointer   data)
{
  /* Unfortunately this doesn't work, sigh. The guides don't show up unless
   * you manually force a redraw of the image.
   */
  gint  i;
  gint  size;
  gint  newn;
  SizeAdjustmentData *adj = (SizeAdjustmentData *) data;
  gchar buf[10];

  for (i = 0; i < adj->nguides; i++)
    gimp_image_delete_guide (adj->image, adj->guides[i]);

  g_free (adj->guides);
  adj->guides = NULL;
  gimp_displays_flush ();

  *(adj->value) = GTK_ADJUSTMENT (widget)->value;

  if (adj->orientation == GIMP_VERTICAL)
    {
      size = gimp_image_width (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
	adj->guides[i] = gimp_image_add_vguide (adj->image,
						*(adj->value) * (i+1));
    }
  else
    {
      size = gimp_image_height (adj->image);
      newn = size / *(adj->value);
      adj->nguides = newn - 1;
      adj->guides = g_new (gint32, adj->nguides);
      for (i = 0; i < adj->nguides; i++)
	adj->guides[i] = gimp_image_add_hguide (adj->image,
						*(adj->value) * (i+1));
    }
  gimp_displays_flush ();
  sprintf (buf, "%2d", newn);
  gtk_label_set_text (GTK_LABEL (adj->count_label), buf);

  *(adj->count) = newn;

  if (newn * *(adj->value) != size)
    gtk_widget_show (GTK_WIDGET (adj->warning_label));
  else
    gtk_widget_hide (GTK_WIDGET (adj->warning_label));

  if (adj->ncells != NULL)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj->ncells),
			      *(adj->other_count) * *(adj->count));
  if (adj->rank0 != NULL)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj->rank0),
			      *(adj->other_count) * *(adj->count));
}

static void
entry_callback (GtkWidget *widget,
		gpointer   data)
{
  if (data == info.description)
    {
      strncpy (info.description, 
	       gtk_entry_get_text (GTK_ENTRY (widget)), MAXDESCLEN);
      info.description[MAXDESCLEN]  = 0;
    }
}

static void
cb_callback (GtkWidget *widget,
	     gpointer   data)
{
  *((gchar **) data) = gtk_entry_get_text (GTK_ENTRY (widget));
}

static void
ok_callback (GtkWidget *widget,
	     gpointer    data)
{
  int i;

  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    gihparms.selection[i] = g_strdup (gihparms.selection[i]);

  run_flag = 1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
gih_save_dialog (gint32 image_ID)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *dimtable;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *entry;
  GtkWidget *box;
  GtkWidget *cb;
  GList     *cbitems = NULL;
  gint       i;
  gchar      buffer[100];
  SizeAdjustmentData cellw_adjust;
  SizeAdjustmentData cellh_adjust;
  gint32    *layer_ID;
  gint32     nlayers;

  /* Setup default values */
  if (gihparms.rows >= 1 && gihparms.cols >= 1)
    gihparms.ncells = num_useable_layers * gihparms.rows * gihparms.cols;
  else
    gihparms.ncells = num_useable_layers;

  if (gihparms.cellwidth == 1 && gihparms.cellheight == 1)
    {
      gihparms.cellwidth  = gimp_image_width (image_ID)  / gihparms.cols;
      gihparms.cellheight = gimp_image_height (image_ID) / gihparms.rows;
    }

  dlg = gimp_dialog_new (_("Save as Brush Pipe"), "gih",
			 gimp_standard_help_func, "filters/gpb.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,
			 
			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,
			 
			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* The main table */
  table = gtk_table_new (8, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /*
   * Spacing: __
   */
  spinbutton = gimp_spin_button_new (&adjustment, info.spacing,
				     1, 1000, 1, 10, 10, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Spacing (Percent):"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &info.spacing);

  /*
   * Description: ___________
   */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Description:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      info.description);

  /*
   * Cell size: __ x __ pixels
   */
  box = gtk_hbox_new (FALSE, 4);

  spinbutton = gimp_spin_button_new (&adjustment,
				     gihparms.cellwidth,
				     2, gimp_image_width (image_ID), 1, 1, 1,
				     1, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);

  layer_ID = gimp_image_get_layers (image_ID, &nlayers);
  cellw_adjust.orientation = GIMP_VERTICAL;
  cellw_adjust.image       = image_ID;
  cellw_adjust.toplayer    = layer_ID[nlayers-1];
  cellw_adjust.nguides     = 0;
  cellw_adjust.guides      = NULL;
  cellw_adjust.value       = &gihparms.cellwidth;
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (size_adjustment_callback),
		      &cellw_adjust);
  gtk_widget_show (spinbutton);

  label = gtk_label_new ("x");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adjustment,
				     gihparms.cellheight,
				     2, gimp_image_height (image_ID), 1, 1, 1,
				     1, 0);
  gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, FALSE, 0);
  cellh_adjust.orientation = GIMP_HORIZONTAL;
  cellh_adjust.image       = image_ID;
  cellh_adjust.toplayer    = layer_ID[nlayers-1];
  cellh_adjust.nguides     = 0;
  cellh_adjust.guides      = NULL;
  cellh_adjust.value       = &gihparms.cellheight;
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (size_adjustment_callback),
		      &cellh_adjust);
  gtk_widget_show (spinbutton);

  label = gtk_label_new ( _("Pixels"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Cell Size:"), 1.0, 0.5,
			     box, 1, FALSE);

  g_free (layer_ID);

  /*
   * Number of cells: ___
   */
  spinbutton = gimp_spin_button_new (&adjustment,
				     gihparms.ncells, 1, 1000, 1, 10, 10,
				     1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Number of Cells:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &gihparms.ncells);

  if (gihparms.dim == 1)
    cellw_adjust.ncells = cellh_adjust.ncells = adjustment;
  else
    cellw_adjust.ncells = cellh_adjust.ncells = NULL;

  /*
   * Display as: __ rows x __ cols
   */
  box = gtk_hbox_new (FALSE, 0);

  g_snprintf (buffer, sizeof (buffer), "%2d", gihparms.rows);
  label = gtk_label_new (buffer);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellh_adjust.count_label = label;
  cellh_adjust.count       = &gihparms.rows;
  cellh_adjust.other_count = &gihparms.cols;
  gtk_widget_show (label);

  label = gtk_label_new (_(" Rows of "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_snprintf (buffer, sizeof (buffer), "%2d", gihparms.cols);
  label = gtk_label_new (buffer);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellw_adjust.count_label = label;
  cellw_adjust.count       = &gihparms.cols;  
  cellw_adjust.other_count = &gihparms.rows;
  gtk_widget_show (label);

  label = gtk_label_new (_(" Columns on each Layer"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_(" (Width Mismatch!) "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellw_adjust.warning_label = label;
  
  label = gtk_label_new (_(" (Height Mismatch!) "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  cellh_adjust.warning_label = label;

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
			     _("Display as:"), 1.0, 0.5,
			     box, 1, FALSE);
  
  /*
   * Dimension: ___
   */
  spinbutton = gimp_spin_button_new (&adjustment, gihparms.dim,
				     1, 5, 1, 1, 1, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
			     _("Dimension:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &gihparms.dim);

  /*
   * Ranks: __ __ __ __ __
   */
  dimtable = gtk_table_new (1, GIMP_PIXPIPE_MAXDIM, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (dimtable), 4);
  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    {
      box = gtk_hbox_new (FALSE, 0);
      gtk_table_attach (GTK_TABLE (dimtable), box, i, i + 1, 0, 1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (box);

      spinbutton = gimp_spin_button_new (&adjustment,
					 gihparms.rank[i], 0, 100, 1, 1, 1,
					 1, 0);
      gtk_box_pack_start (GTK_BOX (box), spinbutton, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			  GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
			  &gihparms.rank[i]);
      gtk_widget_show (spinbutton);
      if (i == 0)
	{ 
	  if (gihparms.dim == 1)
	    cellw_adjust.rank0 = cellh_adjust.rank0 = adjustment;
	  else
	    cellw_adjust.rank0 = cellh_adjust.rank0 = NULL;
	}
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
			     _("Ranks:"), 1.0, 0.5,
			     dimtable, 1, FALSE);

  /*
   * Selection: ______ ______ ______ ______ ______
   */
  cbitems = g_list_append (cbitems, "incremental");
  cbitems = g_list_append (cbitems, "angular");
  cbitems = g_list_append (cbitems, "random");
  cbitems = g_list_append (cbitems, "velocity");
  cbitems = g_list_append (cbitems, "pressure");
  cbitems = g_list_append (cbitems, "xtilt");
  cbitems = g_list_append (cbitems, "ytilt");

  box = gtk_hbox_new (TRUE, 4);
  for (i = 0; i < GIMP_PIXPIPE_MAXDIM; i++)
    {
      cb = gtk_combo_new ();
      gtk_combo_set_popdown_strings (GTK_COMBO (cb), cbitems);
      if (gihparms.selection[i])
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cb)->entry)
			    , gihparms.selection[i]);
      else
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cb)->entry), "random");
      gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (cb)->entry), FALSE);

      gtk_box_pack_start (GTK_BOX (box), cb, FALSE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (GTK_COMBO (cb)->entry), "changed",
			  GTK_SIGNAL_FUNC (cb_callback),
			  &gihparms.selection[i]);
      gtk_widget_show (cb);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 7,
			     _("Selection:"), 1.0, 0.5,
			     box, 1, FALSE);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  for (i = 0; i < cellw_adjust.nguides; i++)
    gimp_image_delete_guide (image_ID, cellw_adjust.guides[i]);
  for (i = 0; i < cellh_adjust.nguides; i++)
    gimp_image_delete_guide (image_ID, cellh_adjust.guides[i]);

  if (run_flag)
    {
      /* Fix up bogus values */
      gihparms.ncells =
	MIN (gihparms.ncells, 
	     num_useable_layers * gihparms.rows * gihparms.cols);
    }
  return run_flag;
}

static gboolean
save_one_brush (gint          fd,
		GimpPixelRgn *pixel_rgn)
{
  BrushHeader  header;
  guint        x;
  guint        y;
  guchar      *buffer;

  g_return_val_if_fail (fd != -1, FALSE);
  g_return_val_if_fail (pixel_rgn != NULL, FALSE);

  if (pixel_rgn->bpp != 2 && pixel_rgn->bpp != 4)
    return FALSE;

  if (pixel_rgn->w < 1 || pixel_rgn->h < 1)
    return FALSE;

  header.header_size  = 
    g_htonl (sizeof (header) + strlen (info.description) + 1);
  header.version      = g_htonl (2);
  header.width        = g_htonl (pixel_rgn->w);
  header.height       = g_htonl (pixel_rgn->h);
  header.bytes        = g_htonl (pixel_rgn->bpp == 2 ? 1 : 4);
  header.magic_number = g_htonl (GBRUSH_MAGIC);
  header.spacing      = g_htonl (info.spacing);

  if (write (fd, &header, sizeof (header)) != sizeof (header)) 
    return FALSE;
  
  if (write (fd, info.description, strlen (info.description) + 1) !=
      strlen (info.description) + 1) 
    return FALSE;
  
  buffer = g_malloc (pixel_rgn->w * pixel_rgn->bpp);

  for (y = 0; y < pixel_rgn->h; y++)
    {
      gimp_pixel_rgn_get_row (pixel_rgn, buffer,
			      0 + pixel_rgn->x, y + pixel_rgn->y,
			      pixel_rgn->w);

      if (pixel_rgn->bpp == 2) /* GRAYA */
	{
	  for (x = 0; x < pixel_rgn->w; x++)
	    {
	      guchar value = 255 - buffer[2 * x];

	      if (write (fd, &value, 1) != 1)
		{
		  g_free (buffer);
		  return FALSE;
		}
	    }
	}
      else if (pixel_rgn->bpp == 4) /* RGBA */
	{
	  if (write (fd, buffer, pixel_rgn->w * pixel_rgn->bpp) != 
	      pixel_rgn->w * pixel_rgn->bpp) 
	    {
	      g_free (buffer);
	      return FALSE;
	    }
	}
    }

  g_free (buffer);

  return TRUE;
}

static gboolean
gih_save_image (gchar  *filename,
		gint32  image_ID,
		gint32  orig_image_ID,
		gint32  drawable_ID)
{
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  GimpParasite *pipe_parasite;
  gchar *header;
  gchar *msg, *parstring;
  gint32 *layer_ID;
  gint fd;
  gint nlayers, layer, row, col;
  gint imagew, imageh, offsetx, offsety;
  gint k, x, y, thisx, thisy, xnext, ynext, thisw, thish;

  if (gihparms.ncells < 1)
    return FALSE;

  imagew = gimp_image_width (image_ID);
  imageh = gimp_image_height (image_ID);
  gimp_tile_cache_size (gimp_tile_height () * imagew * 4);

  msg = g_strdup_printf (_("Saving %s:"), filename);
  gimp_progress_init (msg);
  g_free (msg);

  fd = open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644);
  
  if (fd == -1) 
    {
      g_message( _("Unable to open %s"), filename);
      return FALSE;
    }

  parstring = gimp_pixpipe_params_build (&gihparms);

  header = g_strdup_printf ("%s\n%d %s\n",
			    info.description, gihparms.ncells, parstring);

  if (write (fd, header, strlen (header)) != strlen (header))
    {
      g_free (header);
      g_free (parstring);
      close (fd);
      return FALSE;
    }

  g_free (header);

  pipe_parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
				     GIMP_PARASITE_PERSISTENT,
				     strlen (parstring) + 1, parstring);
  gimp_image_parasite_attach (orig_image_ID, pipe_parasite);
  gimp_parasite_free (pipe_parasite);

  g_free (parstring);

  layer_ID = gimp_image_get_layers (image_ID, &nlayers);
  
  k = 0;
  for (layer = 0; layer < nlayers; layer++)
    {
      if (!gimp_drawable_has_alpha (layer_ID[layer]))
	continue;

      drawable = gimp_drawable_get (layer_ID[layer]);
      gimp_drawable_offsets (layer_ID[layer], &offsetx, &offsety);

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
	      gimp_pixel_rgn_init (&pixel_rgn, drawable, thisx, thisy, 
				   thisw, thish, FALSE, FALSE);
	      
	      if (! save_one_brush (fd, &pixel_rgn))
		{
		  close (fd);
		  return FALSE;
		}

	      k++;
	      gimp_progress_update ((gdouble) k / gihparms.ncells);
	    }
	}

    }
  
  gimp_progress_update (1.0);

  close (fd);

  return TRUE;
}
