/*
 * pat plug-in version 1.01
 * Loads/saves version 1 GIMP .pat files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 */

#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "app/pattern_header.h"
#include <netinet/in.h>


/* Declare local data types
 */

char description[256] = "GIMP Pattern";
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
    { PARAM_STRING, "description", "Short description of the pattern" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_pat_load",
                          "loads files of the .pat file format",
                          "FIXME: write help",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Load>/PAT",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_pat_save",
                          "saves files in the .pat file format",
                          "Yeah!",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          "<Save>/PAT",
                          "RGB*, GRAY*, U16_RGB*, U16_GRAY*, FLOAT_RGB, FLOAT_RGB*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_pat_load", "pat", "", "20,string,GPAT");
  gimp_register_save_handler ("file_pat_save", "pat", "");
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

  if (strcmp (name, "file_pat_load") == 0) {
		image_ID = load_image (param[1].data.d_string);

		if (image_ID != -1) {
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].data.d_image = image_ID;
		} else {
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
		*nreturn_vals = 2;
	}
  else if (strcmp (name, "file_pat_save") == 0) {
		switch (run_mode) {
			case RUN_INTERACTIVE:
				/*  Possibly retrieve data  */
				gimp_get_data("file_pat_save", description);
				if (!save_dialog())
					return;
				break;
			case RUN_NONINTERACTIVE:
				if (nparams != 6)
					status = STATUS_CALLING_ERROR;
		      else
					 strcpy(description, param[5].data.d_string);
			case RUN_WITH_LAST_VALS:
				gimp_get_data ("file_pat_save", description);
				break;
		}

		if (save_image (param[3].data.d_string, param[1].data.d_int32,
				param[2].data.d_int32)) {
			gimp_set_data ("file_pat_save", description, 256);
			values[0].data.d_status = STATUS_SUCCESS;
		} else
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		*nreturn_vals = 1;
	}
}

static gint32 load_image (char *filename) {
	char *temp;
	int fd;
	PatternHeader ph;
	gchar *buffer;
	gint32 image_ID, layer_ID;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	gint bytes;
	gint layer_type, image_type;
	
	temp = g_malloc(strlen (filename) + 11);
	sprintf(temp, "Loading %s:", filename);
	gimp_progress_init(temp);
	g_free (temp);

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	if (read(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return -1;
	}

        /*  rearrange the bytes in each unsigned int  */
	ph.header_size = ntohl(ph.header_size);
	ph.version = ntohl(ph.version);
	ph.width = ntohl(ph.width);
	ph.height = ntohl(ph.height);
	ph.type = ntohl(ph.type);
	ph.magic_number = ntohl(ph.magic_number);

	if (ph.magic_number != GPATTERN_MAGIC || 
			ph.version > 2 || 
			ph.version < 1 ||
			ph.header_size <= sizeof(ph)) {
		close(fd);
		return -1;
	}

	if (lseek(fd, ph.header_size - sizeof(ph), SEEK_CUR) !=
			ph.header_size) {
		close(fd);
		return -1;
	}

	/* In version 1 the type field was bytes */ 
	if (ph.version == 1) 
	{ 
		if (ph.type == 1)      /*in version 1, 1 byte is 8bit gray*/
		{
			ph.type = GRAY_IMAGE;
		}
		else if (ph.type == 3) /*in version 1, 3 bytes is 8bit color */
		{
			ph.type = RGB_IMAGE;
		}
		else
		{
			close(fd);
			return -1; 
		}
	}

	switch (ph.type)
	{
		case RGB_IMAGE:
			image_type = RGB;
			layer_type = RGB_IMAGE;
			break;
		case RGBA_IMAGE:
			image_type = RGB;
			layer_type = RGBA_IMAGE;
			break;
		case GRAY_IMAGE:
			image_type = GRAY;
			layer_type = GRAY_IMAGE;
			break;
		case GRAYA_IMAGE:
			image_type = GRAY;
			layer_type = GRAYA_IMAGE;
			break;
		case U16_RGB_IMAGE:
			image_type = U16_RGB;
			layer_type = U16_RGB_IMAGE;
			break;
		case U16_RGBA_IMAGE:
			image_type = U16_RGB;
			layer_type = U16_RGBA_IMAGE;
			break;
		case U16_GRAY_IMAGE:
			image_type = U16_GRAY;
			layer_type = U16_GRAY_IMAGE;
			break;
		case U16_GRAYA_IMAGE:
			image_type = U16_GRAY;
			layer_type = U16_GRAYA_IMAGE;
			break;
		case FLOAT_RGB_IMAGE:
			image_type = FLOAT_RGB;
			layer_type = FLOAT_RGB_IMAGE;
			break;
		case FLOAT_RGBA_IMAGE:
			image_type = FLOAT_RGB;
			layer_type = FLOAT_RGBA_IMAGE;
			break;
		case FLOAT_GRAY_IMAGE:
			image_type = FLOAT_GRAY;
			layer_type = FLOAT_GRAY_IMAGE;
			break;
		case FLOAT_GRAYA_IMAGE:
			image_type = FLOAT_GRAY;
			layer_type = FLOAT_GRAYA_IMAGE;
			break;
		default:
			close (fd);
			return -1;
	}
	/* Now there's just raw data left. */

	/* Create a new image of the proper size and associate the filename with it. */
  	image_ID = gimp_image_new(ph.width, ph.height, image_type);
  	gimp_image_set_filename(image_ID, filename);

  	layer_ID = gimp_layer_new(image_ID, "Background", ph.width, ph.height,
			layer_type, 100, NORMAL_MODE);
	gimp_image_add_layer(image_ID, layer_ID, 0);

  	drawable = gimp_drawable_get(layer_ID);
  	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);

	bytes = gimp_drawable_bpp (layer_ID);
	buffer = g_malloc(ph.width * bytes);

	for (line = 0; line < ph.height; line++) {
		if (read(fd, buffer, ph.width * bytes) != ph.width * bytes) {
			close(fd);
			g_free(buffer);
			return -1;
		}
		gimp_pixel_rgn_set_row(&pixel_rgn, buffer, 0, line, ph.width);
		gimp_progress_update((double) line / (double) ph.height);
	}
	gimp_drawable_flush(drawable);

	return image_ID;
}

static gint save_image (char *filename, gint32 image_ID, gint32 drawable_ID) {
	int fd;
	PatternHeader ph;
	unsigned char *buffer;
	GDrawable *drawable;
	gint line;
	GPixelRgn pixel_rgn;
	char *temp;

	temp = g_malloc(strlen (filename) + 10);
	sprintf(temp, "Saving %s:", filename);
	gimp_progress_init(temp);
	g_free(temp);

	drawable = gimp_drawable_get(drawable_ID);
	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, FALSE, FALSE);

	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == -1) {
		return 0;
	}

	ph.header_size = htonl(sizeof(ph) + strlen(description) + 1);
	ph.version = htonl(2);
	ph.width = htonl(drawable->width);
	ph.height = htonl(drawable->height);
	ph.type = htonl(gimp_drawable_type (drawable_ID));
	ph.magic_number = htonl(GPATTERN_MAGIC);

	if (write(fd, &ph, sizeof(ph)) != sizeof(ph)) {
		close(fd);
		return 0;
	}

	if (write(fd, description, strlen(description) + 1) !=
			strlen(description) + 1) {
		close(fd);
		return 0;
	}

	buffer = g_malloc(drawable->width * drawable->bpp);
	if (buffer == NULL) {
		close(fd);
		return 0;
	}
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

	argc = 1;
	argv = g_new(gchar *, 1);
	argv[0] = g_strdup("plasma");

	gtk_init(&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());

	dlg = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg), "Save As Pattern");
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
	table = gtk_table_new(1, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	/**********************
	 * label
	 **********************/
	label = gtk_label_new("Description:");
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
	gtk_entry_set_text(GTK_ENTRY(entry), description);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
			(GtkSignalFunc) entry_callback, description);
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
	if (data == description)
		strncpy(description, gtk_entry_get_text(GTK_ENTRY(widget)), 256);
}
