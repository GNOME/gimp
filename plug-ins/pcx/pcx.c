/* pcx.c GIMP plug-in for loading & saving PCX files */

/* This code is based in parts on code by Francisco Bustamante, but the
   largest portion of the code has been rewritten and is now maintained
   occasionally by Nick Lamb njl195@ecs.soton.ac.uk */

/* New for 1998 -- Load 1, 4, 8 & 24 bit PCX files */
/*              -- Save 8 & 24 bit PCX files */

/* Please contact me if you can't use your PCXs with this tool, I want
   The GIMP to have the best file filters on the planet */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* Declare plug-in functions.  */

static void query (void);
static void run (char *name, int nparams, GParam *param, int *nreturn_vals,
                 GParam **return_vals);

GPlugInInfo PLUG_IN_INFO = { NULL, NULL, query, run, };

MAIN ()

static void query () {
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
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
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_pcx_load",
                          "Loads files in Zsoft PCX file format",
                          "FIXME: write help for pcx_load",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@ecs.soton.ac.uk>",
                          "1997",
                          "<Load>/PCX",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_pcx_save",
                          "Saves files in ZSoft PCX file format",
                          "FIXME: write help for pcx_save",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@ecs.soton.ac.uk>",
                          "1997",
                          "<Save>/PCX",
			  "INDEXED, RGB, GRAY",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_pcx_load", "pcx", "",
    "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");
  gimp_register_save_handler ("file_pcx_save", "pcx", "");
}

/* Declare internal functions. */

static gint32 load_image (char *filename);
static void load_1(FILE *fp, int width, int height, char *buffer, int bytes);
static void load_4(FILE *fp, int width, int height, char *buffer, int bytes);
static void load_8(FILE *fp, int width, int height, char *buffer, int bytes);
static void load_24(FILE *fp, int width, int height, char *buffer, int bytes);
static void readline(FILE *fp, guchar* buffer, int bytes);

static gint save_image (char *filename, gint32 image, gint32 layer);
static void save_8(FILE *fp, int width, int height, guchar *buffer);
static void save_24(FILE *fp, int width, int height, guchar *buffer);
static void writeline(FILE *fp, guchar *buffer, int bytes);

/* Plug-in implementation */

static void run (char *name, int nparams, GParam *param, int *nreturn_vals,
     GParam **return_vals) {
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_pcx_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[0].data.d_status = STATUS_SUCCESS;
          values[1].type = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          values[0].data.d_status = STATUS_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_pcx_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  break;

	case RUN_NONINTERACTIVE:
	  if (nparams != 5)
	    status = STATUS_CALLING_ERROR;
	  break;

	case RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32,
                                              param[2].data.d_int32)) {
	values[0].data.d_status = STATUS_SUCCESS;
      } else {
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
      }
    }
}

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

guchar mono[6]= {0, 0, 0, 255, 255, 255};

static struct {
    unsigned char manufacturer;
    unsigned char version;
    unsigned char compression;
    unsigned char bpp;
    short int x1, y1;
    short int x2, y2;
    short int hdpi;
    short int vdpi;
    unsigned char colormap[48];
    unsigned char reserved;
    unsigned char planes;
    short int bytesperline;
    short int color;
    unsigned char filler[58];
} pcx_header;

static gint32 load_image (char *filename) {
  FILE *fd;
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  gchar *message;
  gint offset_x, offset_y, height, width;
  gint32 image, layer;
  guchar *dest, cmap[768];

  message = g_malloc (strlen (filename) + 11);
  sprintf (message, "Loading %s:", filename);
  gimp_progress_init (message);
  g_free (message);

  fd = fopen (filename, "rb");
  if (!fd) {
    fprintf(stderr, "PCX: Can't open \"%s\"\n", filename);
    return -1;
  }

  if (fread(&pcx_header, 128, 1, fd) == 0) {
    fprintf(stderr, "PCX: Can't read header from \"%s\"\n", filename);
    return -1;
  }

  if(pcx_header.manufacturer != 10) {
    fprintf(stderr, "PCX: File \"%s\" is not a PCX file\n", filename);
    return -1;
  }

  offset_x = pcx_header.x1;
  offset_y = pcx_header.y1;
  width = pcx_header.x2 - offset_x + 1;
  height = pcx_header.y2 - offset_y + 1;

  if (pcx_header.planes == 3 && pcx_header.bpp == 8) {
    image= gimp_image_new (width, height, RGB);
    layer= gimp_layer_new (image, "Background", width, height,
                                  RGB_IMAGE, 100, NORMAL_MODE);
  } else {
    image= gimp_image_new (width, height, INDEXED);
    layer= gimp_layer_new (image, "Background", width, height,
                                  INDEXED_IMAGE, 100, NORMAL_MODE);
  }
  gimp_image_set_filename (image, filename);
  gimp_image_add_layer (image, layer, 0);
  gimp_layer_set_offsets (layer, offset_x, offset_y);
  drawable = gimp_drawable_get (layer);

  if (pcx_header.planes == 1 && pcx_header.bpp == 1) {
  dest = (guchar *) g_malloc (width * height);
    load_1(fd, width, height, dest, pcx_header.bytesperline);
    gimp_image_set_cmap (image, mono, 2);
  } else if (pcx_header.planes == 4 && pcx_header.bpp == 1) {
  dest = (guchar *) g_malloc (width * height);
    load_4(fd, width, height, dest, pcx_header.bytesperline);
    gimp_image_set_cmap (image, pcx_header.colormap, 16);
  } else if (pcx_header.planes == 1 && pcx_header.bpp == 8) {
  dest = (guchar *) g_malloc (width * height);
    load_8(fd, width, height, dest, pcx_header.bytesperline);
    fseek(fd, -768L, SEEK_END);
    fread(cmap, 768, 1, fd);
    gimp_image_set_cmap (image, cmap, 256);
  } else if (pcx_header.planes == 3 && pcx_header.bpp == 8) {
  dest = (guchar *) g_malloc (width * height * 3);
    load_24(fd, width, height, dest, pcx_header.bytesperline);
  } else {
    fprintf(stderr, "PCX: Unusual PCX flavour, giving up\n");
    return -1;
  }

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, 0, width, height);

  g_free (dest);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image;
}

static void load_8(FILE *fp, int width, int height, char *buffer, int bytes) {
  int row;
  guchar *line;
  line= (guchar *) g_malloc(bytes);

  for (row= 0; row < height; buffer+= width, ++row) {
    readline(fp, line, bytes);
    memcpy(buffer, line, width);
    gimp_progress_update ((double) row / (double) height);
  }

  g_free(line);
}

static void load_24(FILE *fp, int width, int height, char *buffer, int bytes) {
  int x, y, c;
  guchar *line;
  line= (guchar *) g_malloc(bytes * 3);

  for (y= 0; y < height; buffer+= width * 3, ++y) {
    for (c= 0; c < 3; ++c) {
      readline(fp, line, bytes);
      for (x= 0; x < width; ++x) {
        buffer[x * 3 + c]= line[x];
      }
    }
    gimp_progress_update ((double) y / (double) height);
  }

  g_free(line);
}

static void load_1(FILE *fp, int width, int height, char *buffer, int bytes) {
  int x, y;
  guchar *line;
  line= (guchar *) g_malloc(bytes);

  for (y= 0; y < height; buffer+= width, ++y) {
    readline(fp, line, bytes);
    for (x= 0; x < width; ++x) {
      if (line[x / 8] & (128 >> (x % 8)))
        buffer[x]= 1;
      else
        buffer[x]= 0;
    }
    gimp_progress_update ((double) y / (double) height);
  }

  g_free(line);
}

static void load_4(FILE *fp, int width, int height, char *buffer, int bytes) {
  int x, y, c;
  guchar *line;
  line= (guchar *) g_malloc(bytes);

  for (y= 0; y < height; buffer+= width, ++y) {
    for (x= 0; x < width; ++x) buffer[x]= 0;
    for (c= 0; c < 4; ++c) {
      readline(fp, line, bytes);
      for (x= 0; x < width; ++x) {
        if (line[x / 8] & (128 >> (x % 8)))
          buffer[x]+= (1 << c);
      }
    }
    gimp_progress_update ((double) y / (double) height);
  }

  g_free(line);
}

static void readline(FILE *fp, guchar *buffer, int bytes) {
  static guchar count= 0, value= 0;

  if (pcx_header.compression) {
    while (bytes--) {
      if (count == 0) {
        value= fgetc(fp);
        if (value < 0xc0) {
          count= 1;
        } else {
          count= value - 0xc0;
          value= fgetc(fp);
        }
      }
      count--;
      *(buffer++)= value;
    }
  } else {
    fread(buffer, bytes, 1, fp);
  }
}

gint save_image (char *filename, gint32 image, gint32 layer) {
  FILE *fp;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  guchar *cmap= 0, *pixels;
  gint offset_x, offset_y, width, height;
  gchar *message;
  int colors, i;

  drawable= gimp_drawable_get(layer);
  drawable_type= gimp_drawable_type(layer);
  gimp_drawable_offsets (layer, &offset_x, &offset_y);
  width= drawable->width;
  height= drawable->height;
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  message = g_malloc (strlen (filename) + 11);
  sprintf (message, "Saving %s:", filename);
  gimp_progress_init (message);
  g_free (message);

  pcx_header.manufacturer = 0x0a;
  pcx_header.version = 5;
  pcx_header.compression = 1;

  switch(drawable_type) {
    case INDEXED_IMAGE:
      cmap= gimp_image_get_cmap(image, &colors);
      pcx_header.bpp = 8;
      pcx_header.bytesperline = width;
      pcx_header.planes = 1;
      pcx_header.color = 1;
      break;

    case RGB_IMAGE:
      pcx_header.bpp = 8;
      pcx_header.planes = 3;
      pcx_header.color = 1;
      pcx_header.bytesperline = width;
      break;

    case GRAY_IMAGE:
      pcx_header.bpp = 8;
      pcx_header.planes = 1;
      pcx_header.color = 2;
      pcx_header.bytesperline = width;
      break;

    default:
      fprintf(stderr, "PCX: Can't save this image type\n");
      return -1;
      break;
  }
  
  if ((fp = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "PCX: Can't open \"%s\"\n", filename);
    return -1;
  }

  pixels= (guchar *) g_malloc(width * height * pcx_header.planes);
  gimp_pixel_rgn_get_rect(&pixel_rgn, pixels, 0, 0, width, height);

  pcx_header.x1 = offset_x;
  pcx_header.y1 = offset_y;
  pcx_header.x2 = offset_x + width - 1;
  pcx_header.y2 = offset_y + height - 1;

  pcx_header.hdpi = 300;
  pcx_header.vdpi = 300;
  pcx_header.reserved = 0;

  fwrite(&pcx_header, 128, 1, fp);

  switch(drawable_type) {
    case INDEXED_IMAGE:
      save_8(fp, width, height, pixels);
      fputc(0x0c, fp);
      fwrite(cmap, colors, 3, fp);
      for (i= colors; i < 256; i++) {
        fputc(0, fp); fputc(0, fp); fputc(0, fp);
      }
      break;
    case RGB_IMAGE:
      save_24(fp, width, height, pixels);
      break;
    case GRAY_IMAGE:
      save_8(fp, width, height, pixels);
      fputc(0x0c, fp);
      for (i= 0; i < 256; i++) {
        fputc((guchar) i, fp); fputc((guchar) i, fp); fputc((guchar) i, fp);
      }
      break;
    default:
      fprintf(stderr, "PCX: Can't save this image type\n");
      return -1;
      break;
  }

  gimp_drawable_detach(drawable);
  g_free(pixels);

  fclose(fp);
  return TRUE;
}

static void save_8(FILE *fp, int width, int height, guchar *buffer) {
  int row;

  for (row= 0; row < height; ++row) {
    writeline(fp, buffer, width);
    buffer+= width;
    gimp_progress_update((double) row / (double) height);
  }
}

static void save_24(FILE *fp, int width, int height, guchar *buffer) {
  int x, y, c;
  guchar *line;
  line= (guchar *) g_malloc(width);

  for (y= 0; y < height; ++y) {
    for (c= 0; c < 3; ++c) {
      for (x= 0; x < width; ++x) {
        line[x]= buffer[(3*x) + c];
      }
      writeline(fp, line, width);
    }
    buffer+= width * 3;
    gimp_progress_update((double) y / (double) height);
  }
  g_free (line);
}

static void writeline(FILE *fp, guchar *buffer, int bytes) {
  guchar value, count;
  guchar *finish= buffer+ bytes;

  while (buffer < finish) {
    value= *(buffer++);
    count= 1;

    while (buffer < finish && count < 63 && *buffer == value) {
      count++; buffer++;
    }

    if (value < 0xc0 && count == 1) {
      fputc(value, fp);
    } else {
      fputc(0xc0 + count, fp);
      fputc(value, fp);
    }
  }
}
