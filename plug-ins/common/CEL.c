/* cel.c -- KISS CEL file format plug-in for The GIMP
 * (copyright) 1997,1998 Nick Lamb (njl195@ecs.soton.ac.uk)
 *
 * Skeleton cloned from Michael Sweet's PNG plug-in. KISS format courtesy
 * of the KISS/GS documentation. Problem reports to the above address
 */

/* History:
 * 0.1	Very limited functionality (modern 4bit only)
 * 0.2  Default palette (nice yellows) is automatically used
 * 0.3  Support for the older (pre KISS/GS) cell format
 * 0.4  First support for saving images
 * 0.5  Show copyright date, not version number, thanks to DbBrowser
 * 0.6  File dialogs, palette handling, better magic behaviour
 * 0.7  Handle interactivity settings, tidy up
 * 1.0  Fixed for GIMP 0.99.27 running on GTK+ 1.0.0, and released
 * 1.1  Oops, #include unistd.h, thanks Tamito Kajiyama
 * 1.2  Changed email address, tidied up
 * 1.3  Added g_message features, fixed Saving bugs...
 * 1.4  Offsets work (needed them for a nice example set)
 *
 * Possible future additions:
 *  +   Save (perhaps optionally?) the palette in a KCF
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

static void query(void);
static void run(char *name, int nparams, GParam *param,
                int *nreturn_vals, GParam **return_vals);
static gint load_palette(FILE *fp, guchar palette[]);
static gint32 load_image(char *file, char *brief);
static gint save_image(char *file, char *brief,
                gint32 image, gint32 layer);
static gint palette_dialog(char *title);

/* Globals... */

GPlugInInfo	PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

char *palette_file= NULL;
size_t data_length= 0;

/* Let GIMP library handle initialisation (and inquisitive users) */

int main(int argc, char *argv[]) {
  return gimp_main(argc, argv);
}

/* GIMP queries plug-in for parameters etc. */

static void query(void) {
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "Filename to load image from" },
    { PARAM_STRING, "raw_filename", "Name entered" },
    { PARAM_STRING, "palette_filename", "Filename to load palette from" },
  };

  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int		nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int		nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);
  static GParamDef	save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "Filename to save image to" },
    { PARAM_STRING, "raw_filename", "Name entered" },
    { PARAM_STRING, "palette_filename", "Filename to save palette to" },
  };
  static int		nsave_args = sizeof (save_args) / sizeof (save_args[0]);


  gimp_install_procedure("file_cel_load",
      "Loads files in KISS CEL file format",
      "This plug-in loads individual KISS cell files.",
      "Nick Lamb", "Nick Lamb", "May 1998", "<Load>/CEL", NULL, PROC_PLUG_IN, 
      nload_args, nload_return_vals, load_args, load_return_vals);

  gimp_register_magic_load_handler("file_cel_load", "cel",
                                   "", "0,string,KiSS\040");

  gimp_install_procedure("file_cel_save",
      "Saves files in KISS CEL file format",
      "This plug-in saves individual KISS cell files.",
      "Nick Lamb", "Nick Lamb", "May 1998", "<Save>/CEL", "INDEXEDA",
      PROC_PLUG_IN, nsave_args, 0, save_args, NULL);

  gimp_register_save_handler("file_cel_save", "cel", "");
}

static void run(char *name, int nparams, GParam *param,
                int *nreturn_vals, GParam **return_vals) {

  gint32 image;	/* image ID after load */
  gint status; /* status after save */
  static GParam	values[2]; /* Return values */
  GRunModeType run_mode;

  run_mode = param[0].data.d_int32;

  /* Set up default return values */

  *nreturn_vals = 1;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;
  *return_vals  = values;

  if (strcmp(name, "file_cel_load") == 0) {
    if (run_mode != RUN_NONINTERACTIVE) {
        gimp_get_data ("file_cel_save:length", &data_length);
        if (data_length > 0) {
          palette_file= (char *) g_malloc(data_length);
          gimp_get_data ("file_cel_save:data", palette_file);
        } else {
          palette_file= g_strdup("*.kcf");
          data_length= strlen(palette_file) + 1;
        }
    }

    if (run_mode == RUN_NONINTERACTIVE) {
      palette_file= param[3].data.d_string;
      data_length= strlen(palette_file) + 1;
    } else if (run_mode == RUN_INTERACTIVE) {

      /* Let user choose KCF palette (cancel ignores) */
      palette_dialog("Load KISS Palette");
      gimp_set_data ("file_cel_save:length", &data_length, sizeof (size_t));
      gimp_set_data ("file_cel_save:data", palette_file, data_length);
    }

    image = load_image(param[1].data.d_string, param[2].data.d_string);

    if (image != -1) {
      *nreturn_vals = 2;
      values[1].type         = PARAM_IMAGE;
      values[1].data.d_image = image;
    } else {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }

  } else if (strcmp (name, "file_cel_save") == 0) {

    status = save_image(param[3].data.d_string, param[4].data.d_string,
                        param[1].data.d_int32, param[2].data.d_int32);
    if (status != TRUE) {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
    } else {
      gimp_set_data ("file_cel_save:length", &data_length, sizeof (size_t));
      gimp_set_data ("file_cel_save:data", palette_file, data_length);
    }
  } else {
    values[0].data.d_status = STATUS_EXECUTION_ERROR;
  }
}

/* Load CEL image into The GIMP */

static gint32 load_image(char *file, char *brief) {
  FILE*		fp;		/* Read file pointer */
  char		*progress;	/* Title for progress display */
  guchar	header[32];	/* File header */
  int		height, width,	/* Dimensions of image */
  		offx, offy,	/* Layer offets */
  		colours;	/* Number of colours */

  gint32	image,		/* Image */
		layer;		/* Layer */
  guchar	*palette,	/* 24 bit palette */
		*buffer,	/* Temporary buffer */
  		*line;		/* Pixel data */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */

  int		i, j, k;	/* Counters */


 /* Open the file for reading */
  fp = fopen(file, "r");

  if (fp == NULL) {
    g_message("%s\nis not present or is unreadable", file);
    gimp_quit();
  }

  progress= g_malloc(strlen(brief) + 10);
  sprintf(progress, "Loading %s:", brief);
  gimp_progress_init(progress);

 /* Get the image dimensions and create the image... */

  fread(header, 4, 1, fp);

  if (strncmp(header, "KiSS", 4)) {
    colours= 16;
    width= header[0] + (256 * header[1]);
    height= header[2] + (256 * header[3]);
    offx= 0;
    offy= 0;
  } else { /* New-style image file, read full header */
    fread(header, 28, 1, fp);
    colours= (1 << header[1]);
    width= header[4] + (256 * header[5]);
    height= header[6] + (256 * header[7]);
    offx= header[8] + (256 * header[9]);
    offy= header[10] + (256 * header[11]);
  }

  image = gimp_image_new(width + offx, height + offy, INDEXED);

  if (image == -1) {
    g_message("Can't create a new image");
    gimp_quit();
  }

  gimp_image_set_filename(image, file);

 /* Create an indexed-alpha layer to hold the image... */

  layer = gimp_layer_new(image, "Background", width, height,
                         INDEXEDA_IMAGE, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);
  gimp_layer_set_offsets(layer, offx, offy);

 /* Get the drawable and set the pixel region for our load... */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

 /* Read the image in and give it to the GIMP a line at a time */

  buffer = g_new(guchar, width);
  line = g_new(guchar, (width+1) * 2);

  for (i = 0; i < height && !feof(fp); ++i) {

    switch (colours) {
    case 16:
      fread(buffer, (width+1)/2, 1, fp);
      for (j = 0, k = 0; j < width*2; j+= 4, ++k) { 
        if (buffer[k] / 16 == 0) {
          line[j]= 16;
          line[j+1]= 0;
        } else {
          line[j]= (buffer[k] / 16) - 1;
          line[j+1]= 255;
        }
        if (buffer[k] % 16 == 0) {
          line[j+2]= 16;
          line[j+3]= 0;
        } else {
          line[j+2]= (buffer[k] % 16) - 1;
          line[j+3]= 255;
        }
      }
      break;
    case 256:
      fread(buffer, width, 1, fp);
      for (j = 0, k = 0; j < width*2; j+= 2, ++k) {
        if (buffer[k] == 0) {
          line[j]= 255;
          line[j+1]= 0;
        } else {
          line[j]= buffer[k] - 1;
          line[j+1]= 255;
        }
      }
      break;
    default:
      g_error("Unsupported number of colours (%d)\n", colours);
      gimp_quit();
    }
    
    gimp_pixel_rgn_set_rect(&pixel_rgn, line, 0, i, drawable->width, 1);
    gimp_progress_update((float) i / (float) height);
  }

 /* Close image files, give back allocated memory */

  fclose(fp);
  g_free(buffer);
  g_free(line);

 /* Use palette from file or otherwise default grey palette */
  palette = g_new(guchar, colours*3);

 /* Open the file for reading if user picked one */
  if (palette_file == NULL) {
    fp= NULL;
  } else {
    fp = fopen(palette_file, "r");
  }

  if (fp != NULL) {
    colours= load_palette(fp, palette);
  } else {
    for (i= 0; i < colours; ++i) {
      palette[i*3]= palette[i*3+1]= palette[i*3+2]= i * 256 / colours;
    }
  }

  gimp_image_set_cmap(image, palette + 3, colours - 1);

 /* Close palette file, give back allocated memory */

  fclose(fp);
  g_free(palette);

 /* Now get everything redrawn and hand back the finished image */

  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);

  return (image);
}

static gint load_palette(FILE *fp, guchar palette[]) {
  guchar	header[32];	/* File header */
  guchar	buffer[2];
  int		i, bpp, colours= 0;

  fread(header, 4, 1, fp);
  if (!strncmp(header, "KiSS", 4)) {
    fread(header+4, 28, 1, fp);
    bpp= header[5];
    colours= header[8] + header[9] * 256;
    if (bpp == 12) {
      for (i= 0; i < colours; ++i) {
        fread(buffer, 1, 2, fp);
        palette[i*3]= buffer[0] & 0xf0;
        palette[i*3+1]= (buffer[1] & 0x0f) * 16;
        palette[i*3+2]= (buffer[0] & 0x0f) * 16;
      }
    } else {
      fread(palette, colours, 3, fp);
    }
  } else {
    colours= 16; bpp= 12;
    fseek(fp, 0, SEEK_SET);
    for (i= 0; i < colours; ++i) {
      fread(buffer, 1, 2, fp);
      palette[i*3]= buffer[0] & 0xf0;
      palette[i*3+1]= (buffer[1] & 0x0f) * 16;
      palette[i*3+2]= (buffer[0] & 0x0f) * 16;
    }
  }

  return colours;
}

static gint save_image(char *file, char *brief, gint32 image, gint32 layer) {
  FILE*		fp;		/* Write file pointer */
  char		*progress;	/* Title for progress display */
  guchar	header[32];	/* File header */
  gint		colours, type,	/* Number of colours, type of layer */
  		offx, offy;	/* Layer offsets */

  guchar	*buffer,	/* Temporary buffer */
  		*line;		/* Pixel data */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */

  int		i, j, k;	/* Counters */

 /* Check that this is an indexed image, fail otherwise */
  type= gimp_drawable_type(layer);
  if (type != INDEXEDA_IMAGE) {
    g_message("Only an indexed-alpha image can be saved in CEL format");
    gimp_quit();
  }

  /* Find out how offset this layer was */
  gimp_drawable_offsets(layer, &offx, &offy);

  drawable = gimp_drawable_get(layer);

 /* Open the file for writing */
  fp = fopen(file, "w");

  if (fp == NULL) {
    g_message("Couldn't write image to\n%s", file);
    gimp_quit();
  }

  progress= g_malloc(strlen(brief) + 9);
  sprintf(progress, "Saving %s:", brief);
  gimp_progress_init(progress);

 /* Headers */
  bzero(header, 32);
  strcpy(header, "KiSS");
  header[4]= 0x20;

 /* Work out whether to save as 8bit or 4bit */
  gimp_image_get_cmap(image, &colours);
  if (colours > 15) {
    header[5]= 8;
  } else {
    header[5]= 4;
  }

 /* Fill in the blanks ... */
  header[8]= drawable->width % 256;
  header[9]= drawable->width / 256;
  header[10]= drawable->height % 256;
  header[11]= drawable->height / 256;
  header[12]= offx % 256;
  header[13]= offx / 256;
  header[14]= offy % 256;
  header[15]= offy / 256;
  fwrite(header, 32, 1, fp);

 /* Arrange for memory etc. */
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);
  buffer = g_new(guchar, drawable->width);
  line = g_new(guchar, (drawable->width+1) * 2);

 /* Get the image from the GIMP one line at a time and write it out */
  for (i = 0; i < drawable->height; ++i) {
    gimp_pixel_rgn_get_rect(&pixel_rgn, line, 0, i, drawable->width, 1);
    memset(buffer, 0, drawable->width);

    if (colours > 16) {
      for (j = 0, k = 0; j < drawable->width*2; j+= 2, ++k) {
        if (line[j+1] > 127) {
          buffer[k]= line[j] + 1;
        }
      }
      fwrite(buffer, drawable->width, 1, fp);

    } else {
      for (j = 0, k = 0; j < drawable->width*2; j+= 4, ++k) { 
        buffer[k]= 0;
        if (line[j+1] > 127) {
          buffer[k]+= (line[j] + 1)<< 4;
        }
        if (line[j+3] > 127) {
          buffer[k]+= (line[j+2] + 1);
        }
      }
      fwrite(buffer, (drawable->width+1)/2, 1, fp);
    }

    gimp_progress_update((float) i / (float) drawable->height);
  }

 /* Close files, give back allocated memory */
  fclose(fp);
  g_free(buffer);
  g_free(line);

  return TRUE;
}

static void palette_ok (GtkWidget  *widget, GtkWidget **fs) {

  g_free(palette_file);
  palette_file= g_strdup(gtk_file_selection_get_filename
                                     (GTK_FILE_SELECTION(fs)));
  data_length= strlen(palette_file) + 1;
  gtk_widget_destroy (GTK_WIDGET (fs));
}

static void palette_cancel (GtkWidget  *widget, GtkWidget **window) {
  gtk_main_quit ();
}

static gint palette_dialog(char *title) {
  gchar **argv;
  gint argc = 1;
  GtkWidget *dialog;

  argv= g_malloc(sizeof(gchar *));
  argv[0]= strdup("CEL file-filter");
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dialog= gtk_file_selection_new(title);
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog), palette_file);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->ok_button),
                      "clicked", (GtkSignalFunc) palette_ok, dialog);
  gtk_signal_connect (GTK_OBJECT (dialog),
                      "destroy", (GtkSignalFunc) palette_cancel, NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (dialog)->cancel_button), "clicked", (GtkSignalFunc) palette_cancel, GTK_OBJECT (dialog));

  gtk_widget_show(dialog);

  gtk_main ();
  gdk_flush ();
  return 0;
}
