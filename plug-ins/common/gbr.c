/*
 * gbr plug-in version 1.00
 * Loads/saves version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 * 
 * Added in GBR version 1 support after learning that there wasn't a 
 * tool to read them.  
 * July 6, 1998 by Seth Burgess <sjburges@gimp.org>
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc. 
 */

#include "config.h"

#include <glib.h>		/* Include early for NATIVE_WIN32 */
#include <setjmp.h>
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

#ifdef NATIVE_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "app/brush_header.h"


/* Declare local data types
 */

typedef struct {
	char description[256];
	unsigned int spacing;
} t_info;

t_info info = {   /* Initialize to this, change if non-interactive later */
	"GIMP Brush",     
	10
};

int run_flag = 0;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
                          gint32  image_ID,
                          gint32  drawable_ID);

static gint   save_dialog ();
static void close_callback(GtkWidget * widget, gpointer data);
static void ok_callback(GtkWidget * widget, gpointer data);
static void entry_callback(GtkWidget * widget, gpointer data);

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
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "spacing", "Spacing of the brush" },
    { PARAM_STRING, "description", "Short description of the brush" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gbr_load",
                          "loads files of the .gbr file format",
                          "FIXME: write help",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Load>/GBR",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gbr_save",
                          "saves files in the .gbr file format",
                          "Yeah!",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Save>/GBR",
                          "GRAY",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gbr_load", "gbr", "", "20,string,GIMP");
  gimp_register_save_handler ("file_gbr_save", "gbr", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;
	GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_image = -1;

  if (strcmp (name, "file_gbr_load") == 0) {
		image_ID = load_image (param[1].data.d_string);

		if (image_ID != -1) {
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].data.d_image = image_ID;
		} else {
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
		*nreturn_vals = 2;
	}
  else if (strcmp (name, "file_gbr_save") == 0) {
		switch (run_mode) {
			case RUN_INTERACTIVE:
				/*  Possibly retrieve data  */
				gimp_get_data("file_gbr_save", &info);
				if (!save_dialog())
					return;
				break;
			case RUN_NONINTERACTIVE:  /* FIXME - need a real RUN_NONINTERACTIVE */
				if (nparams != 7)
					status = STATUS_CALLING_ERROR;
				if (status == STATUS_SUCCESS)
					{
					info.spacing = (param[5].data.d_int32);
				    strncpy (info.description, param[6].data.d_string, 256);	
					}
			    break;
			case RUN_WITH_LAST_VALS:
				gimp_get_data ("file_gbr_save", &info);
				break;
		}

		if (save_image (param[3].data.d_string, param[1].data.d_int32,
				param[2].data.d_int32)) {
			gimp_set_data ("file_gbr_save", &info, sizeof(info));
			values[0].data.d_status = STATUS_SUCCESS;
		} else
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		*nreturn_vals = 1;
	}
}

static gint32 load_image (char *filename) {
	char *temp;
	int fd;
	BrushHeader ph;
	gchar *buffer;
	gint32 image_ID, layer_ID;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	int version_extra;

	temp = g_malloc(strlen (filename) + 11);
	sprintf(temp, "Loading %s:", filename);
	gimp_progress_init(temp);
	g_free (temp);

	fd = open(filename, O_RDONLY | _O_BINARY);

	if (fd == -1) {
		return -1;
	}

	if (read(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return -1;
	}

  /*  rearrange the bytes in each unsigned int  */
	ph.header_size = g_ntohl(ph.header_size);
	ph.version = g_ntohl(ph.version);
	ph.width = g_ntohl(ph.width);
	ph.height = g_ntohl(ph.height);
	ph.bytes = g_ntohl(ph.bytes);
	ph.magic_number = g_ntohl(ph.magic_number);
	ph.spacing = g_ntohl(ph.spacing);

	/* How much extra to add ot the header seek - 1 needs a bit more */
	version_extra = 0;
	
	if (ph.version == 1) {
		/* Version 1 didn't know about spacing */	
		ph.spacing=25;
	 	/* And we need to rewind the handle a bit too */
		lseek (fd, -8, SEEK_CUR);
		version_extra=8;
		}
	/* Version 1 didn't know about magic either */
	if ((ph.version != 1 && 
			(ph.magic_number != GBRUSH_MAGIC || ph.version != 2)) ||
			ph.header_size <= sizeof(ph)) {
		close(fd);
		return -1;
	}

	if (lseek(fd, ph.header_size - sizeof(ph) + version_extra, SEEK_CUR) !=
			ph.header_size) {
		close(fd);
		return -1; 
	}
 
	/* Now there's just raw data left. */

 	 /*
	  * Create a new image of the proper size and 
          * associate the filename with it.
	  */

  image_ID = gimp_image_new(ph.width, ph.height, (ph.bytes >= 3) ? RGB : GRAY);
  gimp_image_set_filename(image_ID, filename);

  layer_ID = gimp_layer_new(image_ID, "Background", ph.width, ph.height,
			(ph.bytes >= 3) ? RGB_IMAGE : GRAY_IMAGE, 100, NORMAL_MODE);
	gimp_image_add_layer(image_ID, layer_ID, 0);

  drawable = gimp_drawable_get(layer_ID);
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);

	buffer = g_malloc(ph.width * ph.bytes);

	for (line = 0; line < ph.height; line++) {
		if (read(fd, buffer, ph.width * ph.bytes) != ph.width * ph.bytes) {
			close(fd);
			g_free(buffer);
			return -1;
		}
		gimp_pixel_rgn_set_row(&pixel_rgn, (guchar *)buffer, 0, line, ph.width);
		gimp_progress_update((double) line / (double) ph.height);
	}

	gimp_drawable_flush(drawable);

	return image_ID;
}

static gint save_image (char *filename, gint32 image_ID, gint32 drawable_ID) {
	int fd;
	BrushHeader ph;
	unsigned char *buffer;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	char *temp;

	if (gimp_drawable_type(drawable_ID) != GRAY_IMAGE)
		return FALSE;

	temp = g_malloc(strlen (filename) + 10);
	sprintf(temp, "Saving %s:", filename);
	gimp_progress_init(temp);
	g_free(temp);

	drawable = gimp_drawable_get(drawable_ID);
	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, FALSE, FALSE);

	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644);

	if (fd == -1) {
		g_message("Unable to open %s", filename);
		return 0;
	}

	ph.header_size = g_htonl(sizeof(ph) + strlen(info.description) + 1);
	ph.version = g_htonl(2);
	ph.width = g_htonl(drawable->width);
	ph.height = g_htonl(drawable->height);
	ph.bytes = g_htonl(drawable->bpp);
	ph.magic_number = g_htonl(GBRUSH_MAGIC);
	ph.spacing = g_htonl(info.spacing);

	if (write(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return 0;
	}

	if (write(fd, info.description, strlen(info.description) + 1) !=
			strlen(info.description) + 1) {
		close(fd);
		return 0;
	}

	buffer = g_malloc(drawable->width * drawable->bpp);
	for (line = 0; line < drawable->height; line++) {
		gimp_pixel_rgn_get_row(&pixel_rgn, buffer, 0, line, drawable->width);
		if (write(fd, buffer, drawable->width * drawable->bpp) !=
				drawable->width * drawable->bpp) {
			close(fd);
			return 0;
		}
		gimp_progress_update((double) line / (double) drawable->height);
	}
	g_free(buffer);

	close(fd);

	return 1;
}


static gint save_dialog()
{
	GtkWidget *dlg;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *table;
	gchar **argv;
	gint argc;
	gchar buffer[12];

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("plasma");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	dlg = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg), "Save As Brush");
	gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
										 (GtkSignalFunc) close_callback, NULL);

	/*  Action area  */
	button = gtk_button_new_with_label("OK");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
										 (GtkSignalFunc) ok_callback,
										 dlg);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
														(GtkSignalFunc) gtk_widget_destroy,
														GTK_OBJECT(dlg));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* The main table */
	/* Set its size (y, x) */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	/**********************
	 * label
	 **********************/
	label = gtk_label_new("Spacing:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	/************************
	 * The entry
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 200, 0);
	sprintf(buffer, "%i", info.spacing);
	gtk_entry_set_text(GTK_ENTRY(entry), buffer);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, &info.spacing);
	gtk_widget_show(entry);

	/**********************
	 * label
	 **********************/
	label = gtk_label_new("Description:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0,
			0);
	gtk_widget_show(label);

	/************************
	 * The entry
	 ************************/
	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_set_usize(entry, 200, 0);
	gtk_entry_set_text(GTK_ENTRY(entry), info.description);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, info.description);
	gtk_widget_show(entry);

	gtk_widget_show(dlg);

	gtk_main();
	gdk_flush();

	return run_flag;
}

static void close_callback(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

static void ok_callback(GtkWidget * widget, gpointer data)
{
	run_flag = 1;
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void entry_callback(GtkWidget * widget, gpointer data)
{
	if (data == info.description)
		strncpy(info.description, gtk_entry_get_text(GTK_ENTRY(widget)), 256);
	else if (data == &info.spacing)
		info.spacing = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
}
