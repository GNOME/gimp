/* cel.c -- KISS CEL file format plug-in for The GIMP
 * (copyright) 1997 Nick Lamb (njl195@ecs.soton.ac.uk)
 *
 * Skeleton cloned from Michael Sweet's PNG plug-in. KISS format courtesy
 * of the KISS/GS documentation. Problem reports to the above address
 */

/* History:
 * 0.1	Very limited functionality (modern 4bit only)
 * 0.2  Default palette (nice yellows) is automatically used
 * 0.3  Support for the older (pre KISS/GS) cell format
 * 0.4  First support for saving images
 * Future additions:
 *  +   Automagically or via dialog box choose a KCF palette
 *  +   Save (perhaps optionally?) the palette in a KCF
 *  +   Support offsets -- like GIF?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#define VERSION "0.4"

static void query(void);
static void run(char *name, int nparams, GParam *param,
                int *nreturn_vals, GParam **return_vals);
static gint32 load_image(char *file, char *brief);
static gint save_image(char *file, char *brief,
                gint32 image, gint32 layer);

/* Globals... */

GPlugInInfo	PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* Let GIMP library handle initialisation (and inquisitive users) */

int main(int argc, char *argv[]) {
  return (gimp_main(argc, argv));
}

/* GIMP queries plug-in for parameters etc. */

static void query(void) {
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
  static int		nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int		nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);
  static GParamDef	save_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Drawable to save" },
    { PARAM_STRING,	"filename",	"The name of the file to save the image in" },
    { PARAM_STRING,	"raw_filename",	"The name of the file to save the image in" },
  };
  static int		nsave_args = sizeof (save_args) / sizeof (save_args[0]);


  gimp_install_procedure("file_cel_load",
      "Loads files in KISS CEL file format",
      "This plug-in loads individual KISS cell files.",
      "Nick Lamb", "Nick Lamb", VERSION,
      "<Load>/CEL", NULL, PROC_PLUG_IN, 
      nload_args, nload_return_vals, load_args, load_return_vals);

  gimp_register_magic_load_handler("file_cel_load", "cel",
                                   "", "0,string,KiSS");

  gimp_install_procedure("file_cel_save",
      "Saves files in KISS CEL file format",
      "This plug-in saves individual KISS cell files.",
      "Nick Lamb", "Nick Lamb", VERSION, "<Save>/CEL", "INDEXED*",
      PROC_PLUG_IN, nsave_args, 0, save_args, NULL);

  gimp_register_save_handler("file_cel_save", "cel", "");
}

static void run(char *name, int nparams, GParam *param,
                int *nreturn_vals, GParam **return_vals) {

  gint32 image;	/* image ID after load */
  static GParam	values[2]; /* Return values */

  /* Set up default return values */

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (strcmp(name, "file_cel_load") == 0) {
    image = load_image(param[1].data.d_string, param[2].data.d_string);

    if (image != -1) {
      *nreturn_vals = 2;
      values[1].type         = PARAM_IMAGE;
      values[1].data.d_image = image;
    } else {
      values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }

  } else if (strcmp (name, "file_cel_save") == 0) {
    save_image(param[3].data.d_string, param[4].data.d_string,
               param[1].data.d_int32, param[2].data.d_int32);
  } else {
    values[0].data.d_status = STATUS_EXECUTION_ERROR;
  }
}

/* Load CEL image into The GIMP */

static gint32 load_image(char *file, char *brief) {
  FILE*		fp;		/* Read file pointer */
  char		progress[255];	/* Title for progress display */
  guchar	header[32];	/* File header */
  int		height, width,	/* Dimensions of image */
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

  sprintf(progress, "Loading %s:", brief);
  gimp_progress_init(progress);

 /* Get the image dimensions and create the image... */

  fread(header, 4, 1, fp);

  if (strncmp((char *)header, "KiSS", 4)) {
    colours= 16;
    width= header[0] + (256 * header[1]);
    height= header[2] + (256 * header[3]);
  } else {
    fread(header, 28, 1, fp);
    colours= (1 << header[1]);
    width= header[4] + (256 * header[5]);
    height= header[6] + (256 * header[7]);
  }

  image = gimp_image_new(width, height, INDEXED);

  if (image == -1) {
    g_print("Can't allocate new image\n");
    gimp_quit();
  }

  gimp_image_set_filename(image, file);

  palette = g_new(guchar, colours*3);

 /* FIXME Default palette -- hopefully nasty enough to encourage a fix */
  for (i= 1; i < colours; ++i)
    palette[i*3+1]= palette[i*3]= i * 256 / colours;

  gimp_image_set_cmap(image, palette, colours);

 /* Create an indexed-alpha layer to hold the image... */

  layer = gimp_layer_new(image, "Background", width, height,
                         INDEXEDA_IMAGE, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

 /* Get the drawable and set the pixel region for our load... */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

 /* Read the image in and give it to the GIMP a line at a time */

  buffer = g_new(guchar, width);
  line = g_new(guchar, (width+1) * 2);

  for (i = 0; i < height; ++i) {

    switch (colours) {
    case 16:
      fread(buffer, (width+1)/2, 1, fp);
      for (j = 0, k = 0; j < width*2; j+= 4, ++k) { 
        line[j]= buffer[k] / 16;
        line[j+1]= line[j] ? 255 : 0;
        line[j+2]= buffer[k] & 15;
        line[j+3]= line[j+2] ? 255 : 0;
      }
      break;
    case 256:
      fread(buffer, width, 1, fp);
      for (j = 0, k = 0; j < width*2; j+= 2, ++k) {
        line[j]= buffer[k];
        line[j+1]= line[j] ? 255 : 0;
      }
      break;
    }
    
    gimp_pixel_rgn_set_rect(&pixel_rgn, line, 0, i, drawable->width, 1);
    gimp_progress_update((float) i / (float) height);
  }

 /* Close files, give back allocated memory */

  fclose(fp);
  free(buffer);
  free(line);
  free(palette);

 /* Now get everything redrawn and hand back the finished image */

  gimp_drawable_flush(drawable);
  gimp_drawable_detach(drawable);

  return (image);
}

static gint save_image(char *file, char *brief, gint32 image, gint32 layer) {
  FILE*		fp;		/* Write file pointer */
  char		progress[255];	/* Title for progress display */
  guchar	header[32];	/* File header */
  gint		colours, type;	/* Number of colours, type of layer */

  guchar	*buffer,	/* Temporary buffer */
  		*line;		/* Pixel data */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */

  int		i, j, k;	/* Counters */

 /* Check that this is an indexed image, fail otherwise */
  type= gimp_drawable_type(layer);
  if (type != INDEXED_IMAGE && type != INDEXEDA_IMAGE) {
    g_print("GIMP tried to save a non-indexed image as CEL.\n");
    return FALSE;
  }

  drawable = gimp_drawable_get(layer);

 /* Open the file for writing */
  fp = fopen(file, "w");

  sprintf(progress, "Saving %s:", brief);
  gimp_progress_init(progress);

 /* Headers */
  memset(header,(int)0,(size_t)32);
  strcpy((char *)header, "KiSS");
  header[4]= 0x20;

 /* Work out whether to save as 8bit or 4bit */
  gimp_image_get_cmap(image, &colours);
  if (colours > 16) {
    header[5]= 8;
  } else {
    header[5]= 4;
  }

 /* Fill in the blanks ... */
  header[8]= drawable->width % 256;
  header[9]= drawable->width / 256;
  header[10]= drawable->height % 256;
  header[11]= drawable->height / 256;
  fwrite(header, 32, 1, fp);

 /* Arrange for memory etc. */
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);
  buffer = g_new(guchar, drawable->width);
  line = g_new(guchar, (drawable->width+1) * 2);

 /* Get the image from the GIMP one line at a time and write it out */
  for (i = 0; i < drawable->height; ++i) {
    gimp_pixel_rgn_get_rect(&pixel_rgn, line, 0, i, drawable->width, 1);

    switch (colours) {
    case 16:
      for (j = 0, k = 0; j < drawable->width*2; j+= 4, ++k) { 
        buffer[k]= 16 * (line[j+1] ? line[j] : 0)
                 + (line[j+3] ? line[j+2] : 0);
      }
      fwrite(buffer, (drawable->width+1)/2, 1, fp);
      break;
    case 256:
      for (j = 0, k = 0; j < drawable->width*2; j+= 2, ++k) {
        buffer[k]= line[j+1] ? line[j] : 0;
      }
      fwrite(buffer, drawable->width, 1, fp);
      break;
    }

    gimp_progress_update((float) i / (float) drawable->height);
  }

 /* Close files, give back allocated memory */
  fclose(fp);
  free(buffer);
  free(line);

  return TRUE;
}
