 /* PCX loading and saving file filter for the Gimp
 *  -Francisco Bustamante
 *
 * This filter's code is based on the GIF loading/saving filter for the GIMP,
 * by Peter Mattis, and Spencer Kimball. However, code from the "netpbm" package
 * is no longer used in this plug-in.
 *
 * Fixes to RLE saving by Nick Lamb (njl195@ecs.soton.ac.uk)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

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
                          "Loads files of Zsoft PCX file format",
                          "FIXME: write help for pcx_load",
                          "Francisco Bustamante",
                          "Francisco Bustamante",
                          "1997",
                          "<Load>/PCX",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_pcx_save",
                          "Saves files in ZSoft PCX file format",
                          "FIXME: write help for pcx_save",
                          "Francisco Bustamante",
                          "Francisco Bustamante",
                          "1997",
                          "<Save>/PCX",
			  "INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_pcx_load", "pcx", "",
    "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");
  gimp_register_save_handler ("file_pcx_save", "pcx", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
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
	  /*  Possibly retrieve data  */
	    /*gimp_get_data ("file_pcx_save", &gsvals);*/

	  /*  First acquire information with a dialog  */
		/*  return;*/
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 5)
	    status = STATUS_CALLING_ERROR;
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	    /*	  gimp_get_data ("file_pcx_save", &gsvals);*/
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  /*  Store psvals data  */
	    /*	  gimp_set_data ("file_pcx_save", &gsvals, sizeof (GIFSaveVals));*/

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
}


#define MAXCOLORMAPSIZE  256

#define TRUE             1
#define FALSE            0

#define OK               0
#define ERROR            1

#define CM_RED           0
#define CM_GREEN         1
#define CM_BLUE          2

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)

#define GRAYSCALE        1
#define COLOR            2
#define RGB              3

typedef unsigned char CMap[3][MAXCOLORMAPSIZE];

static struct {
    unsigned char manufacturer;
    unsigned char version;
    unsigned char encoding;
    unsigned char bitsperpixel;
    short int x1, y1;
    short int x2, y2;
    short int hdpi;
    short int vdpi;
    unsigned char colormap[48];
    unsigned char reserved;
    unsigned char nplanes;
    short int bytesperline;
    short int paletteinfo;
    short int hscreensize;
    short  int vscreensize;
    unsigned char filler[54];
} pcx_header;

int verbose = TRUE;

static gint32 ReadImage (FILE *, char *, int, int, int, int, int);

static gint32
load_image (char *filename)
{
  FILE *fd;
  char * name_buf;
  int imageCount = 0;
  gint32 image_ID = -1;
  int img_height, img_width;
  short int bpp;

  name_buf = g_malloc (strlen (filename) + 11);

  sprintf (name_buf, "Loading %s:", filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  fd = fopen (filename, "rb");
  if (!fd) {
      printf ("PCX: can't open \"%s\"\n", filename);
      return -1;
    }

  if(!ReadOK(fd, &pcx_header, 128)) {
	  printf("PCX: error reading file \"%s\"\n", filename);
	  return -1;
  }

  if(pcx_header.manufacturer != 10) {
      printf ("PCX: error reading magic number\n");
      return -1;
  }

 bpp = pcx_header.bitsperpixel * pcx_header.nplanes;

  ++imageCount;

  img_width = pcx_header.x2 - pcx_header.x1 + 1;
  img_height = pcx_header.y2 - pcx_header.y1 + 1;

  image_ID = ReadImage (fd, filename,
			img_width,
			img_height,
			MAXCOLORMAPSIZE,
			COLOR,
			imageCount);
  return image_ID;
}

int
load_monochrome(FILE *fp,
		int len,
		int height,
		char *buffer,
		int bytes_per_line);

int
load_8bit(FILE *fp,
	  int len,
	  int height,
	  char *buffer,
	  int bytes_per_line);

int
load_24bit(FILE *fp,
	  int len,
	  int height,
	  char *buffer,
	  int bytes_per_line);

static gint32
ReadImage (FILE *fd,
	   char *filename,
	   int   len,
	   int   height,
	   int   ncols,
	   int   format,
	   int   number)
{
  static gint32 image_ID;
  gint32 layer_ID;
  /*  gint32 mask_ID; */
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  guchar *dest;
  guchar gimp_cmap[768];
  static gint frame_number = 1;
  int bpp;
  gint cur_progress, max_progress;
  gint i, j;
  gchar framename[20];
  gboolean alpha_frame = FALSE;

  if (frame_number == 1 )
    {
      image_ID = gimp_image_new (len, height, INDEXED);
      gimp_image_set_filename (image_ID, filename);

      layer_ID = gimp_layer_new (image_ID, "Background",
				 len, height,
				 INDEXED_IMAGE, 100, NORMAL_MODE);
    }
  else
    {
      sprintf(framename, "Frame %d", frame_number);
      layer_ID = gimp_layer_new (image_ID, framename,
				 len, height,
				 INDEXEDA_IMAGE, 100, NORMAL_MODE);
      alpha_frame = TRUE;
    }

  frame_number++;

  bpp = pcx_header.bitsperpixel * pcx_header.nplanes;

  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);

  cur_progress = 0;
  max_progress = height;

  if (alpha_frame)
    dest = (guchar *) g_malloc (len * height * 2);
  else
    dest = (guchar *) g_malloc (len * height);

  if (verbose)
    printf ("PCX: reading %d by %d PCX image, ncols=%d\n",
	    len, height, ncols);

  if((bpp == 1) && !alpha_frame) {
	  load_monochrome(fd, len, height, dest, pcx_header.bytesperline);
  }
  else if((bpp == 8) && !alpha_frame) {
	  load_8bit(fd, len, height, dest, pcx_header.bytesperline);
  }
  else {
	  return -1;
  }

  if((pcx_header.bitsperpixel == 8) && (pcx_header.nplanes == 1)) {
      fseek(fd, -768L, SEEK_END);

      for (i = 0, j = 0; i < ncols; i++) {
	  gimp_cmap[j++] = fgetc(fd);
	  gimp_cmap[j++] = fgetc(fd);
	  gimp_cmap[j++] = fgetc(fd);
      }
  }
  else if(pcx_header.bitsperpixel == 1) {
      gimp_cmap[0] = 0;
      gimp_cmap[1] = 0;
      gimp_cmap[2] = 0;
      gimp_cmap[3] = 255;
      gimp_cmap[4] = 255;
      gimp_cmap[5] = 255;
  }

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, 0, drawable->width, drawable->height);

  g_free (dest);

  gimp_image_set_cmap (image_ID, gimp_cmap, ncols);
  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image_ID;
}  /*read_image*/

int
load_monochrome(FILE *fp,
		int len,
		int height,
		char *buffer,
		int bytes_per_line)
{
	guchar *temp;
	gint v;
	gint cur_x = 0, cur_y = 0;
	gint times;
	gint cur_progress = 0, max_progress = height;
	int cur_bit;

	while((cur_y < height) && (!feof(fp))) {
		v = fgetc(fp);
		if(v >= 192) {
			times = v - 192;
			v = fgetc(fp);
			while(times > 0) {
				times --;
				for(cur_bit = 7; cur_bit >= 0; cur_bit--) {
					temp = buffer + (cur_y * len) + cur_x;
					if(BitSet(v, 1 << cur_bit)) {
						*temp = 1;
					}
					else {
						*temp = 0;
					}
					++cur_x;
					if(cur_x == len) {
						cur_x = 0;
						++cur_y;
						cur_progress++;
						if ((cur_progress % 10) == 0)
							gimp_progress_update ((double) cur_progress / (double) max_progress);
					}
				}
			}
		}
		else {
			for(cur_bit = 7; cur_bit >= 0; cur_bit --) {
				temp = buffer + (cur_y * len) + cur_x;
				if(BitSet(v, 1 << cur_bit)) {
					*temp = 1;
				}
				else *temp = 0;
				++cur_x;
				if(cur_x == len) {
					cur_x = 0;
					++cur_y;
					cur_progress++;
					if ((cur_progress % 10) == 0)
						gimp_progress_update ((double) cur_progress / (double) max_progress);
				}
			}
		}
		if(cur_x == len) {
			cur_x = 0;
			++cur_y;
			cur_progress++;
			if ((cur_progress % 10) == 0)
				gimp_progress_update ((double) cur_progress / (double) max_progress);
		}
		if(cur_y == height)
			break;
	}
	return OK;
}

int
load_8bit(FILE *fp,
	  int len,
	  int height,
	  char *buffer,
	  int bytes_per_line)
{
	guchar *temp;
	gint v;
	gint cur_progress = 0, max_progress = height;
	gint times;
	gint cur_x = 0, cur_y = 0;

	while(cur_y < height) {
		v = fgetc(fp);
		if(v >= 192) {
			times = v - 192;
			v = fgetc(fp);
			while(times > 0) {
				times--;
				temp = buffer + (cur_y * len) + cur_x;
				*temp = v;
				++cur_x;
				if(cur_x == len) {
					cur_x = 0;
					cur_y ++;
					cur_progress++;
					if ((cur_progress % 10) == 0)
						gimp_progress_update ((double) cur_progress / (double) max_progress);
				}
			}
		}
		else {
			temp = buffer + (cur_y * len) + cur_x;
			*temp = v;
			++cur_x;
			if(cur_x == len) {
				cur_x = 0;
				cur_y ++;
				cur_progress++;
				if((cur_progress % 10) == 0)
					gimp_progress_update((double) cur_progress / (double) max_progress);
			}
		}
		if(cur_x == len) {
			cur_x = 0;
			cur_y ++;
			cur_progress++;
			if((cur_progress % 10) == 0)
				gimp_progress_update((double) cur_progress / (double) max_progress);
		}
	}
	return OK;
}

int
load_24bit(FILE *fp,
	  int len,
	  int height,
	  char *buffer,
	  int bytes_per_line)
{
	return OK;
}

#define MAXCOLORS 256

gint
colorstobpp(int colors);

gint
save_8bit(FILE *out,
	  guchar *buffer,
	  int len,
	  int height);

guchar *pixels;

gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
	GPixelRgn pixel_rgn;
	GDrawable *drawable;
	GDrawableType drawable_type;
	int Red[MAXCOLORS];
	int Green[MAXCOLORS];
	int Blue[MAXCOLORS];
	guchar *cmap;
	int width, height;
	int bpp= 0;
	int colors;
	FILE *outfile;
	guchar *name_buf;
	int i;

	drawable = gimp_drawable_get(drawable_ID);
	drawable_type = gimp_drawable_type(drawable_ID);
	gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

	name_buf = (guchar *) g_malloc(strlen(filename) + 11);
	sprintf(name_buf, "Saving %s:", filename);
	gimp_progress_init(name_buf);
	g_free(name_buf);

	switch(drawable_type) {
		case INDEXED_IMAGE :
			cmap = gimp_image_get_cmap(image_ID, &colors);
			for(i = 0; i < colors; i++) {
				Red[i] = *cmap++;
				Green[i] = *cmap++;
				Blue[i] = *cmap++;
			}
			for( ; i < 256; i++) {
				Red[i] = 0;
				Green[i] = 0;
				Blue[i] = 0;
			}
			bpp = colorstobpp(colors);
			break;

		case INDEXEDA_IMAGE : case RGBA_IMAGE :
			fprintf(stderr, "PCX: unable to save alpha images");
			return FALSE;
			break;

		case RGB_IMAGE :
			bpp = 24;
			break;

		default :
			fprintf(stderr, "PCX: you should not receive this error for any reason\n");
			return FALSE;
			break;
	}

	if((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "PCX: Can't open \"%s\"\n", filename);
		return FALSE;
	}

	pixels = (guchar *) g_malloc(drawable->width * drawable->height);

	gimp_pixel_rgn_get_rect(&pixel_rgn, pixels, 0, 0, drawable->width, drawable->height);
	width = drawable->width;
	height = drawable->height;

	pcx_header.manufacturer = 10;
	pcx_header.version = 5;
	pcx_header.encoding = 1;
	pcx_header.x1 = 0;
	pcx_header.y1 = 0;
	pcx_header.x2 = width - 1;
	pcx_header.y2 = height - 1;
	pcx_header.paletteinfo = 1;
	if(bpp <= 8) {
		pcx_header.bytesperline = width;
		pcx_header.bitsperpixel = 8;
		pcx_header.nplanes = 1;
	}
	else {
		pcx_header.bitsperpixel = 8;
		pcx_header.nplanes = 3;
	}
	pcx_header.hdpi = 300;
	pcx_header.vdpi = 300;
	pcx_header.reserved = 0;

	if(bpp <= 4) {
		for(i = 0; i < colors; i++) {
			pcx_header.colormap[(i*3)] = Red[i];
			pcx_header.colormap[(i*3) + 1] = Green[i];
			pcx_header.colormap[(i*3) + 2] = Blue[i];
		}
	}

	fwrite(&pcx_header, 128, 1, outfile);

	if(bpp == 8) {
		save_8bit(outfile, pixels, width, height);
		fputc(0x0c, outfile);
		fputc(0x0c, outfile);
		for(i = 0; i <= 255; i++) {
			fputc(Red[i], outfile);
			fputc(Green[i], outfile);
			fputc(Blue[i], outfile);
		}
	}

	gimp_drawable_detach(drawable);
	g_free(pixels);

	fclose(outfile);
	return TRUE;
}

gint
save_8bit(FILE *out,
	  guchar *buffer,
	  int len,
	  int height)
{
	guchar *temp;
	int times;
	gint max;
	unsigned char v;
	gint cur_x = 0, cur_y = 0;
	gint cur_progress = 0, max_progress = height;

	printf("saving %d * %d\n", len, height);

	while(cur_y < height) {
		while(cur_x < len) {
			temp = (cur_y * len) + cur_x + buffer;
			v = *temp;
			times = 1;
			++cur_x;
			temp = (cur_y * len) + cur_x + buffer;
			if(cur_x + 62 > len) {
				max = len;
			}
			else
				max = cur_x + 62;
			while((*temp == v) && (cur_x < max)) {
				times ++;
				cur_x ++;
				temp = (cur_y * len) + cur_x + buffer;
			}
			if(times == 1) {
				if(v >= 0xC0) {
					fputc(0xC0 + 1, out);
					fputc(v, out);
				}
				else fputc(v, out);
			}
			else {
				max = 0xC0 + times;
				fputc(max, out);
				fputc(v, out);
			}
		}
		cur_x = 0;
		cur_y++;
		cur_progress++;
		if((cur_progress % 10) == 0)
			gimp_progress_update((double) cur_progress / (double) max_progress);
	}
	return OK;
}


gint
colorstobpp(int colors) {
	int bpp;
	if(colors <= 2)
		bpp = 1;
	else if(colors <= 4)
		bpp = 2;
	else if(colors <= 16)
		bpp = 4;
	else if(colors <= 256)
		bpp = 8;
	else
		bpp = 24;
	return bpp;
}
/* The End */
