/*
 * gbr plug-in version 1.00
 * Loads/saves version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 * 
 * Added in GBR version 1 support after learning that there wasn't a 
 * tool to read them.  
 * July 6, 1998 by Seth Burgess <sjburges@gimp.org>
 *
 * Dec 17, 2000
 * Load and save GIMP brushes in GRAY or RGBA.  jtl + neo
 * 
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc. 
 */

#include "config.h"

#include <glib.h>		/* Include early for G_OS_WIN32 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "app/brush_header.h"
#include "app/pattern_header.h"

#include "libgimp/stdplugins-intl.h"


/* Declare local data types
 */

typedef struct
{
  gchar description[256];
  gint  spacing;
} t_info;

t_info info = 
{   /* Initialize to this, change if non-interactive later */
  "GIMP Brush",     
  10
};

gboolean run_flag = FALSE;


/* Declare some local functions.
 */
static void   query          (void);
static void   run            (gchar      *name,
			      gint        nparams,
			      GimpParam  *param,
			      gint       *nreturn_vals,
			      GimpParam **return_vals);

static gint32 load_image     (gchar      *filename);
static gint   save_image     (gchar      *filename,
			      gint32      image_ID,
			      gint32      drawable_ID);

static gint   save_dialog    (void);
static void   ok_callback    (GtkWidget  *widget, 
			      gpointer    data);
static void   entry_callback (GtkWidget  *widget, 
			      gpointer    data);


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
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode",       "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",       "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename",   "The name of the file to load" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",          "Output image" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "spacing",      "Spacing of the brush" },
    { GIMP_PDB_STRING,   "description",  "Short description of the brush" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gbr_load",
                          "Loads GIMP brushes (1 or 4 bpp and old .gpb format)",
                          "FIXME: write help",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "1997-2000",
                          "<Load>/GBR",
                          NULL,
                          GIMP_PLUGIN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gbr_save",
                          "saves files in the .gbr file format",
                          "Yeah!",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "1997-2000",
                          "<Save>/GBR",
                          "RGBA, GRAY",
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gbr_load",
				    "gbr, gpb",
				    "",
				    "20, string, GIMP");
  gimp_register_save_handler       ("file_gbr_save",
				    "gbr",
				    "");
}

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam     values[2];
  GimpRunModeType      run_mode;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  gint32               image_ID;
  gint32               drawable_ID;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_gbr_load") == 0) 
    {
      image_ID = load_image (param[1].data.d_string);
      
      if (image_ID != -1) 
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_gbr_save") == 0) 
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  INIT_I18N_UI();
	  gimp_ui_init ("gbr", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "GBR", 
				      GIMP_EXPORT_CAN_HANDLE_GRAY  |
				      GIMP_EXPORT_CAN_HANDLE_RGB   |
				      GIMP_EXPORT_CAN_HANDLE_ALPHA);
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  INIT_I18N();
	  break;
	}

      switch (run_mode) 
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gbr_save", &info);
	  if (! save_dialog ())
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:  
	  /* FIXME - need a real GIMP_RUN_NONINTERACTIVE */
	  if (nparams != 7)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      info.spacing = (param[5].data.d_int32);
	      strncpy (info.description, param[6].data.d_string, 256);	
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data ("file_gbr_save", &info);
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID)) 
	    {
	      gimp_set_data ("file_gbr_save", &info, sizeof(info));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32 
load_image (gchar *filename) 
{
  gchar         *temp;
  gint           fd;
  BrushHeader    bh;
  guchar        *brush_buf   = NULL;
  gint32         image_ID;
  gint32         layer_ID;
  GimpDrawable  *drawable;
  GimpPixelRgn   pixel_rgn;
  gint           version_extra;
  GimpImageBaseType  base_type;
  GimpImageType      image_type;
  
  temp = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (temp);
  g_free (temp);
  
  fd = open (filename, O_RDONLY | _O_BINARY);
  
  if (fd == -1) 
    {
      return -1;
    }
  
  if (read (fd, &bh, sizeof (bh)) != sizeof (bh)) 
    {
      close (fd);
      return -1;
    }

  /*  rearrange the bytes in each unsigned int  */
  bh.header_size  = g_ntohl (bh.header_size);
  bh.version      = g_ntohl (bh.version);
  bh.width        = g_ntohl (bh.width);
  bh.height       = g_ntohl (bh.height);
  bh.bytes        = g_ntohl (bh.bytes);
  bh.magic_number = g_ntohl (bh.magic_number);
  bh.spacing      = g_ntohl (bh.spacing);
  
  /* How much extra to add to the header seek - 1 needs a bit more */
  version_extra = 0;
  
  if (bh.version == 1) 
    {
      /* Version 1 didn't know about spacing */	
      bh.spacing = 25;
      /* And we need to rewind the handle a bit too */
      lseek (fd, -8, SEEK_CUR);
      version_extra = 8;
    }
  /* Version 1 didn't know about magic either */
  if ((bh.version != 1 && 
       (bh.magic_number != GBRUSH_MAGIC || bh.version != 2)) ||
      bh.header_size <= sizeof (bh)) 
    {
      close (fd);
      return -1;
    }
  
  if (lseek (fd, bh.header_size - sizeof (bh) + version_extra, SEEK_CUR) 
      != bh.header_size) 
    {
      close (fd);
      return -1; 
    }
 
  /* Now there's just raw data left. */
  
  brush_buf = g_malloc (bh.width * bh.height * bh.bytes);

  if (read (fd, brush_buf, 
	    bh.width * bh.height * bh.bytes) != bh.width * bh.height * bh.bytes) 
    {
      close (fd);
      g_free (brush_buf);
      return -1;
    }


  if (bh.bytes == 1)
    {
      PatternHeader ph;

      /*  For backwards-compatibility, check if a pattern follows.
	  The obsolete .gpb format did it this way.  */

      if (read (fd, &ph, sizeof(ph)) == sizeof(ph)) 
	{
	  /*  rearrange the bytes in each unsigned int  */
	  ph.header_size  = g_ntohl (ph.header_size);
	  ph.version      = g_ntohl (ph.version);
	  ph.width        = g_ntohl (ph.width);
	  ph.height       = g_ntohl (ph.height);
	  ph.bytes        = g_ntohl (ph.bytes);
	  ph.magic_number = g_ntohl (ph.magic_number);

	  if (ph.magic_number == GPATTERN_MAGIC && ph.version == 1 &&
	      ph.header_size > sizeof (ph) &&
	      ph.bytes == 3 && ph.width == bh.width && ph.height == bh.height &&
	      lseek (fd, ph.header_size - sizeof (ph), SEEK_CUR) > 0)
	    {
	      guchar *plain_brush = brush_buf;
	      gint    i;

	      bh.bytes = 4;
	      brush_buf = g_malloc (4 * bh.width * bh.height);
	      
	      for (i = 0; i < ph.width * ph.height; i++)
		{
		  if (read (fd, brush_buf + i * 4, 3) != 3)
		    {
		      close (fd);
		      g_free (plain_brush);
		      g_free (brush_buf);
		      return -1;
		    }
		  brush_buf[i * 4 + 3] = plain_brush[i];
		}
	      g_free (plain_brush);
	    }
	}
    }

  /*
   * Create a new image of the proper size and 
   * associate the filename with it.
   */
  
  switch (bh.bytes)
    {
    case 1:
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAY_IMAGE;
      break;
    case 4:
      base_type = GIMP_RGB;
      image_type = GIMP_RGBA_IMAGE;
      break;
    default:
      g_message ("Unsupported brush depth: %d\nGIMP Brushes must be GRAY or RGBA\n",
		 bh.bytes);
      return -1;
    }

  image_ID = gimp_image_new (bh.width, bh.height, base_type);
  gimp_image_set_filename (image_ID, filename);
  
  layer_ID = gimp_layer_new (image_ID, _("Background"), 
			     bh.width, bh.height, 
			     image_type, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 
		       0, 0, drawable->width, drawable->height, 
		       TRUE, FALSE);

  gimp_pixel_rgn_set_rect (&pixel_rgn, (guchar *) brush_buf, 
			   0, 0, bh.width, bh.height);

  if (image_type == GIMP_GRAY_IMAGE)
    gimp_invert (layer_ID);

  close (fd);
  g_free (brush_buf);

  gimp_drawable_flush (drawable);
  
  return image_ID;
}

static gint 
save_image (gchar  *filename, 
	    gint32  image_ID, 
	    gint32  drawable_ID) 
{
  gint          fd;
  BrushHeader   bh;
  guchar       *buffer;
  GimpDrawable *drawable;
  gint          line;
  gint          x;
  GimpPixelRgn  pixel_rgn;
  gchar        *temp;
  
  if (gimp_drawable_type (drawable_ID) != GIMP_GRAY_IMAGE &&
      gimp_drawable_type (drawable_ID) != GIMP_RGBA_IMAGE)
    {
      g_message (_("GIMP brushes are either GRAYSCALE or RGBA\n"));
      return FALSE;
    }

  temp = g_strdup_printf (_("Saving %s:"), filename);
  gimp_progress_init (temp);
  g_free (temp);
  
  drawable = gimp_drawable_get (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 
		       0, 0, drawable->width, drawable->height, 
		       FALSE, FALSE);
  
  fd = open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644);
  
  if (fd == -1) 
    {
      g_message( _("Unable to open %s"), filename);
      return FALSE;
    }

  bh.header_size  = g_htonl (sizeof (bh) + strlen (info.description) + 1);
  bh.version      = g_htonl (2);
  bh.width        = g_htonl (drawable->width);
  bh.height       = g_htonl (drawable->height);
  bh.bytes        = g_htonl (drawable->bpp);
  bh.magic_number = g_htonl (GBRUSH_MAGIC);
  bh.spacing      = g_htonl (info.spacing);
  
  if (write (fd, &bh, sizeof (bh)) != sizeof (bh)) 
    {
      close (fd);
      return FALSE;
    }
  
  if (write (fd, info.description, strlen(info.description) + 1) !=
      strlen (info.description) + 1) 
    {
      close(fd);
      return FALSE;
    }
  
  buffer = g_malloc (drawable->width * drawable->bpp);
  for (line = 0; line < drawable->height; line++) 
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, line, drawable->width);

      if (drawable->bpp == 1)
	{
	  for (x = 0; x < drawable->width; x++)
	    buffer[x] = 255 - buffer[x];
	}

      if (write (fd, buffer, 
		 drawable->width * drawable->bpp) != drawable->width * drawable->bpp) 
	{
	  g_free (buffer);
	  close (fd);
	  return FALSE;
	}
      gimp_progress_update ((gdouble) line / (gdouble) drawable->height);
    }

  g_free (buffer);  
  close (fd);
  
  return TRUE;
}

static gint 
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *spinbutton;
  GtkObject *adj;

  dlg = gimp_dialog_new (_("Save as Brush"), "gbr",
			 gimp_standard_help_func, "filters/gbr.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT(dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* The main table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj,
				     info.spacing, 1, 1000, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Spacing:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &info.spacing);

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Description:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      info.description);
  
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();
  
  return run_flag;
}

static void 
ok_callback (GtkWidget *widget, 
	     gpointer   data)
{
  run_flag = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
entry_callback (GtkWidget *widget, 
		gpointer   data)
{
  strncpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}
