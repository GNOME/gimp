/*
 * PSD Plugin version 1.9.9.8 (BETA)
 * This GIMP plug-in is designed to load Adobe Photoshop(tm) files (.PSD)
 *
 * Adam D. Moss <adam@gimp.org> <adam@foxbox.org>
 *
 *     If this plug-in fails to load a file which you think it should,
 *     please tell me what seemed to go wrong, and anything you know
 *     about the image you tried to load.  Please don't send big PSD
 *     files to me without asking first.
 *
 *          Copyright (C) 1997-98 Adam D. Moss
 *          Copyright (C) 1996    Torsten Martinsen
 * Portions Copyright (C) 1995    Peter Mattis
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

/*
 * Adobe and Adobe Photoshop are trademarks of Adobe Systems
 * Incorporated that may be registered in certain jurisdictions.
 */

/*
 * Revision history:
 *
 *  98.04.26 / v1.9.9.8 / Adam D. Moss
 *       Implemented Aux-channels for layered files.  Got rid
 *       of <endian.h> nonsense.  Improved Layer Mask padding.
 *       Enforced num_layers/num_channels limit checks.
 *
 *  98.04.23 / v1.9.9.5 / Adam D. Moss
 *       Got Layer Masks working, got Aux-channels working
 *       for unlayered files, fixed 'raw' channel loading, fixed
 *       some other mini-bugs, slightly better progress meters.
 *       Thanks to everyone who is helping with the testing!
 *
 *  98.04.21 / v1.9.9.1 / Adam D. Moss
 *       A little cleanup.  Implemented Layer Masks but disabled
 *       them again - PS masks can be a different size to their
 *       owning layer, unlike those in GIMP.
 *
 *  98.04.19 / v1.9.9.0 / Adam D. Moss
 *       Much happier now.
 *
 *  97.03.13 / v1.9.0 / Adam D. Moss
 *       Layers, channels and masks, oh my.
 *       + Bugfixes & rearchitecturing.
 *
 *  97.01.30 / v1.0.12 / Torsten Martinsen
 *       Flat PSD image loading.
 */

/*
 * TODO:
 *
 *      Crush 16bpp channels
 *	CMYK -> RGB
 *	Load BITMAP mode
 *
 *      File saving.  (I am not going to be able to do this for
 *         practical reasons.  Suggest someone works on it as a
 *         separate plugin - please let me know.)
 *
 *      Reduce memory requirements!
 */

/*
 * BUGS:
 *
 *      Sometimes creates a superfluous aux channel?  Harmless.
 */



/* *** DEFINES *** */
/* the max number of layers that this plugin should try to load */
#define MAX_LAYERS 100
/* the max number of channels that this plugin should let a layer have */
#define MAX_CHANNELS 30
/* *** END OF DEFINES *** */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "libgimp/gimp.h"


/* Local types etc
 */
typedef enum
{
  PSD_UNKNOWN_IMAGE,
  PSD_RGB_IMAGE,
  PSD_RGBA_IMAGE,
  PSD_GRAY_IMAGE,
  PSD_GRAYA_IMAGE,
  PSD_INDEXED_IMAGE,
  PSD_INDEXEDA_IMAGE
} psd_imagetype;



/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
GDrawableType psd_type_to_gimp_type
                         (psd_imagetype psdtype);
GImageType psd_type_to_gimp_base_type
                         (psd_imagetype psdtype);
GLayerMode psd_lmode_to_gimp_lmode
                         (gchar modekey[4]);
static gint32 load_image (char   *filename);



GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};



typedef struct PsdChannel
{
  gchar *name;
  guchar *data;
  gint type;

  guint32 compressedsize;
  
} PSDchannel;



typedef struct PsdLayer
{
  gint num_channels;
  PSDchannel channel[MAX_CHANNELS];
  
  gint32 x;
  gint32 y;
  guint32 width;
  guint32 height;
  
  gchar blendkey[4];
  guchar opacity;
  gchar clipping;

  gboolean protecttrans;
  gboolean visible;

  gchar* name;

  gint32 lm_x;
  gint32 lm_y;
  gint32 lm_width;
  gint32 lm_height;

} PSDlayer;



typedef struct PsdImage
{
  gint num_layers;
  PSDlayer layer[MAX_LAYERS];

  gboolean absolute_alpha;

  gint type;
  
  gulong colmaplen;
  guchar *colmapdata;

  guint num_aux_channels;
  PSDchannel aux_channel[MAX_CHANNELS];

  gchar *caption;

  guint active_layer_num;

} PSDimage;



static PSDimage psd_image;



static struct {
    gchar signature[4];
    gushort version;
    guchar reserved[6];
    gushort channels;
    gulong rows;
    gulong columns;
    gushort bpp;
    gushort mode;
    gulong imgreslen;
    gulong miscsizelen;
    gushort compression;
    gushort * rowlength;
    long  imgdatalen;
} PSDheader;


static gchar * modename[] = {
    "Bitmap", 
    "Grayscale", 
    "Indexed Colour", 
    "RGB Colour", 
    "CMYK Colour", 
    "<invalid>", 
    "<invalid>", 
    "Multichannel", 
    "Duotone", 
    "Lab Colour", 
    "<invalid>"
};


static const gchar *prog_name = "PSD";


static void unpack_pb_channel(FILE *fd, guchar *dst, gint32 unpackedlen,
				  guint32 *offset);
static void decode(long clen, long uclen, gchar *src, guchar *dst, int step);
static void packbitsdecode(long *clenp, long uclen,
			   gchar *src, guchar *dst, int step);
static void cmyk2rgb(guchar *src, guchar *destp,
		     long width, long height, int alpha);
static void cmykp2rgb(guchar *src, guchar *destp,
		      long width, long height, int alpha);
static void cmyk_to_rgb(int *c, int *m, int *y, int *k);
static guchar getguchar(FILE *fd, gchar *why);
static gshort getgshort(FILE *fd, gchar *why);
static glong getglong(FILE *fd, gchar *why);
static void xfread(FILE *fd, void *buf, long len, gchar *why);
static void *xmalloc(size_t n);
static void read_whole_file(FILE *fd);
static void reshuffle_cmap(guchar *map256);
static gchar *getpascalstring(FILE *fd, gchar *why);
void throwchunk(size_t n, FILE * fd, gchar *why);
void dumpchunk(size_t n, FILE * fd, gchar *why);


MAIN()


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

  /*  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { PARAM_INT32, "fillorder", "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);*/

  gimp_install_procedure ("file_psd_load",
                          "loads files of the Photoshop(tm) PSD file format",
                          "FIXME: write help for psd_load",
                          "Adam D. Moss & Torsten Martinsen",
                          "Adam D. Moss & Torsten Martinsen",
                          "1996-1997",
                          "<Load>/PSD",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  /*  gimp_install_procedure ("file_psd_save",
                          "saves files in the Photoshop(tm) PSD file format",
                          "FIXME: write help for psd_save",
                          "Adam D. Moss & Torsten Martinsen",
                          "Adam D. Moss & Torsten Martinsen",
                          "1996-1997",
                          "<Save>/PSD",
			  "RGB, GRAY, INDEXED",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);*/

  gimp_register_load_handler ("file_psd_load", "psd", "");
  /*  gimp_register_save_handler ("file_psd_save", "psd", "");*/
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
  /*  GStatusType status = STATUS_SUCCESS;*/
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_psd_load") == 0)
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
}


GDrawableType
psd_type_to_gimp_type (psd_imagetype psdtype)
{
  switch(psdtype)
    {
    case PSD_RGBA_IMAGE: return(RGBA_IMAGE);
    case PSD_RGB_IMAGE: return(RGB_IMAGE);
    case PSD_GRAYA_IMAGE: return(GRAYA_IMAGE);
    case PSD_GRAY_IMAGE: return(GRAY_IMAGE);
    case PSD_INDEXEDA_IMAGE: return(INDEXEDA_IMAGE);
    case PSD_INDEXED_IMAGE: return(INDEXED_IMAGE);
    default: return(RGB_IMAGE);
    }
}



GLayerMode
psd_lmode_to_gimp_lmode (gchar modekey[4])
{
  if (strncmp(modekey, "norm", 4)==0) return(NORMAL_MODE);
  if (strncmp(modekey, "dark", 4)==0) return(DARKEN_ONLY_MODE);
  if (strncmp(modekey, "lite", 4)==0) return(LIGHTEN_ONLY_MODE);
  if (strncmp(modekey, "hue ", 4)==0) return(HUE_MODE);
  if (strncmp(modekey, "sat ", 4)==0) return(SATURATION_MODE);
  if (strncmp(modekey, "colr", 4)==0) return(COLOR_MODE);
  if (strncmp(modekey, "mul ", 4)==0) return(MULTIPLY_MODE);
  if (strncmp(modekey, "scrn", 4)==0) return(SCREEN_MODE);
  if (strncmp(modekey, "diss", 4)==0) return(DISSOLVE_MODE);
  if (strncmp(modekey, "diff", 4)==0) return(DIFFERENCE_MODE);

  if (strncmp(modekey, "lum ", 4)==0) return(VALUE_MODE);    /* ? */

  printf("PSD: Warning - unsupported layer-blend mode '%c%c%c%c', using 'addition' mode\n", modekey[0], modekey[1], modekey[2], modekey[3]);

  if (strncmp(modekey, "over", 4)==0) return(ADDITION_MODE); /* ? */
  if (strncmp(modekey, "hLit", 4)==0) return(/**/ADDITION_MODE);
  if (strncmp(modekey, "sLit", 4)==0) return(/**/ADDITION_MODE);

  printf("PSD: Warning - UNKNOWN layer-blend mode, reverting to 'normal'\n");

  return(NORMAL_MODE);
}



GImageType
psd_type_to_gimp_base_type (psd_imagetype psdtype)
{
  switch(psdtype)
    {
    case PSD_RGBA_IMAGE:
    case PSD_RGB_IMAGE: return(RGB);
    case PSD_GRAYA_IMAGE:
    case PSD_GRAY_IMAGE: return(GRAY);
    case PSD_INDEXEDA_IMAGE:
    case PSD_INDEXED_IMAGE: return(INDEXED);
    default:
      printf("PSD: Error: Can't convert PSD imagetype to GIMP imagetype\n");
      gimp_quit();
      return(RGB);
    }
}



GImageType
psd_mode_to_gimp_base_type (gushort psdtype)
{
  switch(psdtype)
    {
    case 1: return(GRAY);
    case 2: return(INDEXED);
    case 3: return(RGB);
    default:
      printf("PSD: Error: Can't convert PSD mode to GIMP base imagetype\n");
      gimp_quit();
      return(RGB);
    }
}



static void
reshuffle_cmap(guchar *map256)
{
  guchar *tmpmap;
  int i;

  tmpmap = xmalloc(768);

  for (i=0;i<256;i++)
    {
      tmpmap[i*3  ] = map256[i];
      tmpmap[i*3+1] = map256[i+256];
      tmpmap[i*3+2] = map256[i+512];
    }

  for (i=0;i<768;i++)
    {
      map256[i] = tmpmap[i];
    }

  g_free(tmpmap);
}



static void
dispatch_resID(guint ID, FILE *fd, guint32 *offset, guint32 Size)
{
  if ( (ID < 0x0bb6) && (ID >0x07d0) )
    {
      printf ("\t\tPath data is irrelevant to GIMP at this time.\n");
      throwchunk(Size, fd, "dispatch_res path throw");
      (*offset) += Size;
    }
  else
    switch (ID)
      {
      case 0x03ee:
	{
	  gint32 remaining = Size;
	  
	  printf ("\t\tALPHA CHANNEL NAMES:\n");
	  if (Size > 0)
	    {
	      do
		{
		  guint32 alpha_name_len;
		  
		  psd_image.aux_channel[psd_image.num_aux_channels].name =
		    getpascalstring(fd, "alpha channel name");
		  
		  alpha_name_len =
		    strlen(psd_image.aux_channel[psd_image.num_aux_channels].name);
		  
		  printf("\t\t\tname: \"%s\"\n",
			 psd_image.aux_channel[psd_image.num_aux_channels].name);
		  
		  psd_image.num_aux_channels++;

		  if (psd_image.num_aux_channels > MAX_CHANNELS)
		    {
		      printf("\nPSD: Sorry - this image has too many aux channels.  Tell Adam!\n");
		      gimp_quit();
		    }
		  
		  (*offset) += alpha_name_len+1;
		  remaining -= alpha_name_len+1;
		}
	      while (remaining > 0);
	    }

	  if (remaining)
	    throwchunk(remaining, fd, "alphaname padding 0 throw");
	}
	break;
      case 0x03ef:
	  printf("\t\tDISPLAYINFO STRUCTURE: unhandled\n");
	  throwchunk(Size, fd, "dispatch_res");
	  (*offset) += Size;
	break;
      case 0x03f0: /* FIXME: untested */
	{
	  guint32 caption_len;
	  
	  psd_image.caption = getpascalstring(fd, "caption string");
	  caption_len = strlen(psd_image.caption);
	  
	  printf("\t\t\tcontent: \"%s\"\n",psd_image.caption);
	  (*offset) += caption_len+1;
	}
	break;
      case 0x03f2:
	printf("\t\tBACKGROUND COLOR: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;	
      case 0x03f4:
	printf("\t\tGREY/MULTICHANNEL HALFTONING INFO: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03f5:
	printf("\t\tCOLOUR HALFTONING INFO: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03f6:
	printf("\t\tDUOTONE HALFTONING INFO: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03f7:
	printf("\t\tGREYSCALE/MULTICHANNEL TRANSFER FUNCTION: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03f8:
	printf("\t\tCOLOUR TRANSFER FUNCTION: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03f9:
	printf("\t\tDUOTONE TRANSFER FUNCTION: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03fa:
	printf("\t\tDUOTONE IMAGE INFO: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03fb:
	printf("\t\tEFFECTIVE BLACK/WHITE VALUES: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x03fe:
	printf("\t\tQUICK MASK INFO: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x0400:
	{
	  printf("\t\tLAYER STATE INFO:\n");
	  psd_image.active_layer_num = getgshort(fd, "ID target_layer_num");

	  printf("\t\t\ttarget: %d\n",(gint)psd_image.active_layer_num);
	  (*offset) += 2;
	}
	break;
      case 0x0402:
	printf("\t\tLAYER GROUP INFO: unhandled\n");
	printf("\t\t\t(Inferred number of layers: %d)\n",(gint)(Size/2));
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      case 0x0405:
	printf ("\t\tIMAGE MODE FOR RAW FORMAT: unhandled\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;

      case 0x03e9:
      case 0x03ed:
      case 0x03f1:
      case 0x03f3:
      case 0x03fd:
      case 0x0401:
      case 0x0404:
      case 0x0406:
      case 0x0bb7:
      case 0x2710:
	printf ("\t\t<Field is irrelevant to GIMP at this time.>\n");
	throwchunk(Size, fd, "dispatch_res");	
	(*offset) += Size;
	break;
	
      case 0x03e8:
      case 0x03eb:
	printf ("\t\t<Obsolete Photoshop 2.0 field.>\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
	
      case 0x03fc:
      case 0x03ff:
      case 0x0403:
	printf ("\t\t<Obsolete field.>\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
	
      default:
	printf ("\t\t<Undocumented field.>\n");
	throwchunk(Size, fd, "dispatch_res");
	(*offset) += Size;
	break;
      }
}


static void
do_layer_record(FILE *fd, guint32 *offset, gint layernum)
{
  gint32 top, left, bottom, right;
  guint32 extradatasize, layermaskdatasize, layerrangesdatasize;
  guint32 totaloff;
  gchar sig[4];
  guchar flags;
  gint i;

  printf("\t\t\tLAYER RECORD (layer %d)\n", (int)layernum);


  /* table 11-12 */
  top = getglong (fd, "layer top");
  (*offset)+=4;
  left = getglong (fd, "layer left");
  (*offset)+=4;
  bottom = getglong (fd, "layer bottom");
  (*offset)+=4;
  right = getglong (fd, "layer right");
  (*offset)+=4;

  psd_image.layer[layernum].x = left;
  psd_image.layer[layernum].y = top;
  psd_image.layer[layernum].width = right-left;
  psd_image.layer[layernum].height = bottom-top;
  
  printf("\t\t\t\tLayer extents: (%d,%d) -> (%d,%d)\n",left,top,right,bottom);
  
  psd_image.layer[layernum].num_channels =
    getgshort (fd, "layer num_channels");
  (*offset)+=2;

  if (psd_image.layer[layernum].num_channels > MAX_CHANNELS)
    {
      printf("\nPSD: Sorry - this image has too many channels.  Tell Adam!\n");
      gimp_quit();
    }
  
  printf("\t\t\t\tNumber of channels: %d\n",
	 (int)psd_image.layer[layernum].num_channels);

  for (i=0;i<psd_image.layer[layernum].num_channels;i++)
    {
      /* table 11-13 */
      printf("\t\t\t\tCHANNEL LENGTH INFO (%d)\n", i);

      psd_image.layer[layernum].channel[i].type	= getgshort(fd, "channel id");
      (*offset)+=2;
      printf("\t\t\t\t\tChannel TYPE: %d\n",
	     psd_image.layer[layernum].channel[i].type);

      psd_image.layer[layernum].channel[i].compressedsize =
	getglong(fd, "channeldatalength");
      (*offset)+=4;
      printf("\t\t\t\t\tChannel Data Length: %d\n",
	     psd_image.layer[layernum].channel[i].compressedsize);
    }

  xfread(fd, sig, 4, "layer blend sig");
  (*offset)+=4;
  
  if (strncmp(sig, "8BIM", 4)!=0)
    {
      printf("PSD: Error - layer blend signature is incorrect. :-(\n");
      gimp_quit();
    }

  xfread(fd, psd_image.layer[layernum].blendkey, 4, "layer blend key");
  (*offset)+=4;

  printf("\t\t\t\tBlend type: PSD(\"%c%c%c%c\") = GIMP(%d)\n",
	 psd_image.layer[layernum].blendkey[0],
	 psd_image.layer[layernum].blendkey[1],
	 psd_image.layer[layernum].blendkey[2],
	 psd_image.layer[layernum].blendkey[3],
	 psd_lmode_to_gimp_lmode(psd_image.layer[layernum].blendkey));

  psd_image.layer[layernum].opacity = getguchar(fd, "layer opacity");
  (*offset)++;

  printf("\t\t\t\tLayer Opacity: %d\n", psd_image.layer[layernum].opacity);

  psd_image.layer[layernum].clipping = getguchar(fd, "layer clipping");
  (*offset)++;

  printf("\t\t\t\tLayer Clipping: %d (%s)\n",
	 psd_image.layer[layernum].clipping,
	 psd_image.layer[layernum].clipping==0?"base":"non-base");

  flags = getguchar(fd, "layer flags");
  (*offset)++;

  printf("\t\t\t\tLayer Flags: %d (%s, %s)\n", flags,
	 flags&1?"preserve transparency":"don't preserve transparency",
	 flags&2?"visible":"not visible");

  psd_image.layer[layernum].protecttrans = (flags&1) ? TRUE : FALSE;
  psd_image.layer[layernum].visible = (flags&2) ? FALSE : TRUE;

  getguchar(fd, "layer record filler");
  (*offset)++;

  extradatasize = getglong(fd, "layer extra data size");
  (*offset)+=4;
  printf("\t\t\t\tEXTRA DATA SIZE: %d\n",extradatasize);

  /* FIXME: should do something with this data */
  /*throwchunk(extradatasize, fd, "layer extradata throw");
  (*offset) += extradatasize;*/

  totaloff = (*offset) + extradatasize;

  /* table 11-14 */
  layermaskdatasize = getglong(fd, "layer mask data size");
  (*offset)+=4;
  printf("\t\t\t\t\tLAYER MASK DATA SIZE: %d\n",layermaskdatasize);
  
  if (layermaskdatasize)
    {
      top    = getglong(fd, "lmask top");
      (*offset) += 4;
      left   = getglong(fd, "lmask left");
      (*offset) += 4;
      bottom = getglong(fd, "lmask bottom");
      (*offset) += 4;
      right  = getglong(fd, "lmask right");
      (*offset) += 4;

      psd_image.layer[layernum].lm_x = left;
      psd_image.layer[layernum].lm_y = top;
      psd_image.layer[layernum].lm_width = right-left;
      psd_image.layer[layernum].lm_height = bottom-top;

      getglong(fd, "lmask data throw");
      (*offset) += 4;

      /*      throwchunk(layermaskdatasize, fd, "layer mask data throw");
      (*offset) += layermaskdatasize;*/
    }


  layerrangesdatasize = getglong(fd, "layer ranges data size");
  (*offset)+=4;
  printf("\t\t\t\t\t\tLAYER RANGES DATA SIZE: %d\n",layermaskdatasize);
  
  if (layerrangesdatasize)
    {
      throwchunk(layerrangesdatasize, fd, "layer ranges data throw");
      (*offset) += layerrangesdatasize;
    }

  psd_image.layer[layernum].name = getpascalstring(fd, "layer name");
  (*offset) += strlen(psd_image.layer[layernum].name)+1;
  /*  offset += ((strlen(psd_image.layer[layernum].name)+1)-1)/4 + 4;*/

  printf("\t\t\t\t\t\tLAYER NAME: '%s'\n",psd_image.layer[layernum].name);
  
  if (totaloff-(*offset) > 0)
    {
      printf("Warning: layer record dross: ");
      dumpchunk(totaloff-(*offset), fd, "layer record dross throw");
      (*offset) = totaloff;
    }
}


static void
do_layer_struct(FILE *fd, guint32 *offset)
{
  gint i;

  printf("\t\tLAYER STRUCTURE SECTION\n");
  
  psd_image.num_layers = getgshort(fd, "layer struct numlayers");
  (*offset)+=2;

  printf("\t\t\tCanonical number of layers: %d%s\n",
	 psd_image.num_layers>0?
	 (int)psd_image.num_layers:abs(psd_image.num_layers),
	 psd_image.num_layers>0?"":" (absolute/alpha)");

  if (psd_image.num_layers<0)
    {
      psd_image.num_layers = -psd_image.num_layers;
      psd_image.absolute_alpha = TRUE;
    }
  else
    {
      psd_image.absolute_alpha = FALSE;
    }

  if (psd_image.num_layers > MAX_LAYERS)
    {
      printf("\nPSD: Sorry - this image has too many layers.  Tell Adam!\n");
      gimp_quit();
    }

  for (i=0; i<psd_image.num_layers; i++)
    {
      do_layer_record(fd, offset, i);
    }
}


static void
do_layer_pixeldata(FILE *fd, guint32 *offset)
{
  guint16 compression;
  gint layeri, channeli;
  guchar *tmpline;

  for (layeri=0; layeri<psd_image.num_layers; layeri++)
    {
      tmpline = xmalloc(psd_image.layer[layeri].width + 1);

      for (channeli=0;
	   channeli<psd_image.layer[layeri].num_channels;
	   channeli++)
	{
	  int width, height;

	  if (psd_image.layer[layeri].channel[channeli].type == -2)
	    {
	      width = psd_image.layer[layeri].lm_width;
	      height = psd_image.layer[layeri].lm_height;
	    }
	  else
	    {
	      width = psd_image.layer[layeri].width;
	      height = psd_image.layer[layeri].height;
	    }

	  compression = getgshort(fd, "layer channel compression type");
	  (*offset)+=2;
	      
	  printf("\t\t\tLayer (%d) Channel (%d:%d) Compression: %d (%s)\n",
		 layeri,
		 channeli,
		 psd_image.layer[layeri].channel[channeli].type,
		 compression,
		 compression==0?"raw":(compression==1?"RLE":"UNKNOWN!"));
	      
	  printf("\t\t\t\tLoading channel data (%d bytes)...\n",
		 width*height);
	      
	  psd_image.layer[layeri].channel[channeli].data =
	    xmalloc (width * height);
	      
	  switch (compression)
	    {
	    case 0: /* raw data */
	      {
		/* FIXME: only lightly tested */
		gint linei;
		    
		for (linei=0;
		     linei<height;
		     linei++)
		  {
		    xfread(fd,
			   (psd_image.layer[layeri].channel[channeli].data+
			    linei * width),
			   width,
			   "raw channel line");
		    (*offset) += width;
		  }

		/* Pad raw data to multiple of 2? */
#if 0
		if ((height *
		     width) & 1)
		  {
		    getguchar(fd, "raw channel padding");
		    (*offset)++;
		  }
#endif
	      }
	      break;
	    case 1: /* RLE, one row at a time, padded to an even width */
	      {
		gint linei;
		gint blockread;
		    
		/* we throw this away because in theory we can trust the
		   data to unpack to the right length... hmm... */
		throwchunk(height * 2,
			   fd, "widthlist");
		(*offset) += height * 2;

		blockread = (*offset);
		    
		for (linei=0;
		     linei<height;
		     linei++)
		  {
		    /*printf(" %d ", *offset);*/
		    unpack_pb_channel(fd, tmpline,
				      width 
				      /*(width&1)*/,
				      offset);
		    memcpy((psd_image.layer[layeri].channel[channeli].data+
			    linei * width),
			   tmpline,
			   width);
		  }
		    
		printf("\t\t\t\t\tActual compressed size was %d bytes\n",
		       (*offset)-blockread);
	      }
	      break;
	    default: /* *unknown* */
	      printf("*** Unknown compression type in channel.\n");
	      gimp_quit();
	      break;
	    }
	}
	  
      g_free(tmpline);
    }

  /*  printf("\n[[%ld]]\n", getglong(fd, "uhhhh"));
  (*offset)+=4;*/
}


static void
do_layers(FILE *fd, guint32 *offset)
{
  guint32 section_length;

  section_length = getglong(fd, "layerinfo sectionlength");
  (*offset)+=4;

  printf("\tLAYER INFO SECTION\n");
  printf("\t\tSECTION LENGTH: %u\n",section_length);
  
  do_layer_struct(fd, offset);

  do_layer_pixeldata(fd, offset);
}



static void
do_layer_and_mask(FILE *fd)
{
  guint32 offset = 0;
  guint32 Size = PSDheader.miscsizelen;


  guint32 offset_now = ftell(fd);


  printf("LAYER AND MASK INFO\n");
  printf("\tSECTION LENGTH: %u\n",Size);

  if (Size == 0) return;

  do_layers(fd, &offset);


  if (offset < Size)
    {
      printf("PSD: Supposedly there are %d bytes of mask info left.\n",
	     Size-offset);
      if ((Size-offset == 4) || (Size-offset == 24))
	printf("     That sounds good to me.\n");
      else
	printf("     That sounds strange to me.\n");


      /*      if ((getguchar(fd, "mask info throw")!=0) ||
	  (getguchar(fd, "mask info throw")!=0) ||
	  (getguchar(fd, "mask info throw")!=0) ||
	  (getguchar(fd, "mask info throw")!=0))
	{
	  printf("*** This mask info block looks pretty bogus.\n");
	}*/
    }
  else
    printf("PSD: Stern warning - no mask info.\n");


  /* If 'offset' wasn't being buggily updated, we wouldn't need this. */
  fseek(fd, Size+offset_now, SEEK_SET);
}



static void
do_image_resources(FILE *fd)
{
  guint16 ID;
  gchar *Name;
  guint32 Size;

  guint32 offset = 0;

  printf("IMAGE RESOURCE BLOCK:\n");

  /* FIXME: too trusting that the file isn't corrupt */
  while (offset < PSDheader.imgreslen-1)
    {
      gchar sig[4];
      
      xfread(fd, sig, 4, "imageresources signature");

      if (strncmp(sig, "8BIM", 4) != 0)
	{
	  printf("PSD: Error - imageresources block has invalid signature.\n");
	  gimp_quit();
	}
      offset += 4;

      /* generic information about a block ID */


      ID = getgshort(fd, "ID num");
      offset+=2;
      printf("\tID: 0x%04x / ",ID);


      Name = getpascalstring(fd, "ID name");
      offset += strlen(Name)+1;

      if (!strlen(Name)&1)
	{
	  throwchunk(1, fd, "ID name throw");
	  offset ++;
	}
      g_free(Name);

      Size = getglong(fd, "ID Size");
      offset+=4;
      printf("Size: %d\n", Size);

      dispatch_resID(ID, fd, &offset, Size);

      if (Size&1)
	{
	  throwchunk(1, fd, "ID content throw");
	  offset ++;
	}
    }

  /*  if (offset != PSDheader.imgreslen)
    {
      printf("\tSucking imageres byte...\n");
      throwchunk(1, fd, "imageres suck");
      offset ++;
    }*/

}


static
guchar* chans_to_GRAYA (guchar* grey, guchar* alpha, gint numpix)
{
  guchar* rtn;
  int i; /* clearly, 'i' stands for 'Imaginative loop variable' */

  if ((grey==NULL)||(alpha==NULL))
    {
      printf("NULL channel - eep!");
      return NULL;
    }

  rtn = xmalloc(numpix * 2);

  for (i=0; i<numpix; i++)
    {
      rtn[i*2  ] = grey[i];
      rtn[i*2+1] = alpha[i];
    }

  return (rtn);
}


static
guchar* chans_to_RGB (guchar* red, guchar* green, guchar* blue, gint numpix)
{
  guchar* rtn;
  int i;

  if ((red==NULL)||(green==NULL)||(blue==NULL))
    {
      printf("NULL channel - eep!");
      return NULL;
    }

  rtn = xmalloc(numpix * 3);

  for (i=0; i<numpix; i++)
    {
      rtn[i*3  ] = red[i];
      rtn[i*3+1] = green[i];
      rtn[i*3+2] = blue[i];
    }

  return (rtn);
}



static
guchar* chans_to_RGBA (guchar* red, guchar* green, guchar* blue,
		       guchar* alpha,
		       gint numpix)
{
  guchar* rtn;
  int i;
  gboolean careful = FALSE;

  if ((red==NULL)||(green==NULL)||(blue==NULL)||(alpha==NULL))
    {
      printf("chans_to_RGBA : NULL channel - eep!");
      careful = TRUE;
    }

  rtn = xmalloc(numpix * 4);

  if (!careful)
    {
      for (i=0; i<numpix; i++)
	{
	  rtn[i*4  ] = red[i];
	  rtn[i*4+1] = green[i];
	  rtn[i*4+2] = blue[i];
	  rtn[i*4+3] = alpha[i];
	}
    }
  else
    {
      for (i=0; i<numpix; i++)
	{
	  rtn[i*4  ] = (red==NULL) ? 0 : red[i];
	  rtn[i*4+1] = (green==NULL) ? 0 : green[i];
	  rtn[i*4+2] = (blue==NULL) ? 0 : blue[i];
	  rtn[i*4+3] = (alpha==NULL) ? 0 : alpha[i];
	}
    }

  return (rtn);
}


static
gboolean psd_layer_has_alpha(PSDlayer* layer)
{
  int i;

  for (i=0; i<layer->num_channels; i++)
    {
      if (layer->channel[i].type == -1)
	{
	  return TRUE;
	}
    }

  return FALSE;
}


static
void extract_data_and_channels(guchar* src, gint gimpstep, gint psstep,
			       gint32 image_ID, GDrawable* drawable,
			       gint width, gint height)
{
  guchar* primary_data;
  guchar* aux_data;
  GPixelRgn pixel_rgn;

  printf("Extracting primary channel data (%d channels)\n"
	 "\tand %d auxiliary channels.\n", gimpstep, psstep-gimpstep);

  primary_data = xmalloc(width * height * gimpstep);
  {
    int pix,chan;

    for (pix=0; pix<width*height; pix++)
      {
	for (chan=0; chan<gimpstep; chan++)
	  {
	    primary_data [pix*gimpstep + chan] =
	      src [pix*psstep + chan];
	  }
      }
    
    gimp_pixel_rgn_init (&pixel_rgn, drawable,
			 0, 0, drawable->width, drawable->height,
			 TRUE, FALSE);
    gimp_pixel_rgn_set_rect (&pixel_rgn, primary_data,
			     0, 0, drawable->width, drawable->height);    
  
    gimp_drawable_flush (drawable);
    gimp_drawable_detach (drawable);
  }
  g_free(primary_data);


  aux_data = xmalloc(width * height);
  {
    int pix, chan;
    gint32 channel_ID;
    GDrawable* chdrawable;
    guchar colour[3]= {0, 0, 0};


    for (chan=gimpstep; chan<psstep; chan++)
      {
	for (pix=0; pix<width*height; pix++)
	  {
	    aux_data [pix] =
	      src [pix*psstep + chan];
	  }
	
	channel_ID = gimp_channel_new(image_ID,
				      psd_image.aux_channel[chan-gimpstep].name,
				      width, height,
				      100.0, colour);
	gimp_image_add_channel(image_ID, channel_ID, 0);
	gimp_channel_set_visible(channel_ID, FALSE);

	chdrawable = gimp_drawable_get(channel_ID);

	gimp_pixel_rgn_init (&pixel_rgn, chdrawable,
			     0, 0, chdrawable->width, chdrawable->height,
			     TRUE, FALSE);
	gimp_pixel_rgn_set_rect (&pixel_rgn, aux_data,
				 0, 0, chdrawable->width, chdrawable->height);
	
	gimp_drawable_flush (chdrawable);
	gimp_drawable_detach (chdrawable);
      }
  }
  g_free(aux_data);

  printf("Done with that.\n\n");
}


static
void extract_channels(guchar* src, gint num_wanted, gint psstep,
		      gint32 image_ID,
		      gint width, gint height)
{
  guchar* aux_data;
  GPixelRgn pixel_rgn;

  printf("Extracting %d/%d auxiliary channels.\n", num_wanted, psstep);

  aux_data = xmalloc(width * height);
  {
    int pix, chan;
    gint32 channel_ID;
    GDrawable* chdrawable;
    guchar colour[3]= {0, 0, 0};

    for (chan=psstep-num_wanted; chan<psstep; chan++)
      {
	for (pix=0; pix<width*height; pix++)
	  {
	    aux_data [pix] =
	      src [pix*psstep + chan];
	  }
	
	channel_ID = gimp_channel_new(image_ID,
				      psd_image.aux_channel[chan-(psstep-num_wanted)].name,
				      width, height,
				      100.0, colour);
	gimp_image_add_channel(image_ID, channel_ID, 0);
	gimp_channel_set_visible(channel_ID, FALSE);

	chdrawable = gimp_drawable_get(channel_ID);

	gimp_pixel_rgn_init (&pixel_rgn, chdrawable,
			     0, 0, chdrawable->width, chdrawable->height,
			     TRUE, FALSE);
	gimp_pixel_rgn_set_rect (&pixel_rgn, aux_data,
				 0, 0, chdrawable->width, chdrawable->height);
	
	gimp_drawable_flush (chdrawable);
	gimp_drawable_detach (chdrawable);
      }
  }
  g_free(aux_data);

  printf("Done with that.\n\n");
}


static void
resize_mask(guchar* src, guchar* dest,
	    gint32 src_x, gint32 src_y,
	    gint32 src_w, gint32 src_h,
	    gint32 dest_w, gint32 dest_h)
{
  int x,y;

  printf("--> %p %p : %d %d . %d %d . %d %d\n",
	 src, dest,
	 src_x, src_y,
	 src_w, src_h,
	 dest_w, dest_h);

  for (y=0; y<dest_h; y++)
    {
      for (x=0; x<dest_w; x++)
	{
	  if ((x>src_x) && (x<src_x+src_w) &&
	      (y>src_y) && (y<src_y+src_h))
	    {
	      dest[dest_w * y + x] =
		src[src_w * (y-src_y) + (x-src_x)];
	    }
	  else
	    {
	      dest[dest_w * y + x] = 255;
	    }
	}
    }
}


static gint32
load_image(char *name)
{
  FILE *fd;
  gboolean want_aux;
  char *name_buf;
  unsigned char *cmykbuf;
  static int number = 1;
  char *temp;
  guchar *dest;
  long channels, nguchars;
  psd_imagetype imagetype;
  int cmyk = 0, step = 1;
  gint32 image_ID = -1;
  gint32 layer_ID = -1;
  GDrawable *drawable = NULL;
  GPixelRgn pixel_rgn;
  gint32 iter;

    
  printf("------- %s ---------------------------------\n",name);


  name_buf = xmalloc(strlen(name) + 11);
  if (number > 1)
    sprintf (name_buf, "%s-%d", name, number);
  else
    sprintf (name_buf, "%s", name);
  sprintf(name_buf, "Loading %s:", name);

  gimp_progress_init(name_buf);

  fd = fopen(name, "rb");
  if (fd==NULL) {
    printf("%s: can't open \"%s\"\n", prog_name, name);
    return(-1);
  }





  read_whole_file(fd);






  if (psd_image.num_layers > 0) /* PS3-style */
    {
      int lnum;
      GImageType gimagetype;

      gimagetype = psd_mode_to_gimp_base_type(PSDheader.mode);
      image_ID =
	gimp_image_new (PSDheader.columns, PSDheader.rows, gimagetype);
      gimp_image_set_filename (image_ID, name);
      
      for (lnum=0; lnum<psd_image.num_layers; lnum++)
	{
	  gint numc;
	  guchar* merged_data = NULL;

	  numc = psd_image.layer[lnum].num_channels;

	  printf("Hey, it's a LAYER with %d channels!\n", numc);
	  
	  switch (gimagetype)
	    {
	    case GRAY:
	      {
		printf("It's GRAY.\n");
		if (!psd_layer_has_alpha(&psd_image.layer[lnum]))
		  {
		    merged_data = xmalloc(psd_image.layer[lnum].width *
					  psd_image.layer[lnum].height);
		    memcpy(merged_data, psd_image.layer[lnum].channel[0].data,
			   psd_image.layer[lnum].width *
			   psd_image.layer[lnum].height);
		  }
		else
		  {
		    merged_data =
		      chans_to_GRAYA (psd_image.layer[lnum].channel[1].data,
				      psd_image.layer[lnum].channel[0].data,
				      psd_image.layer[lnum].width *
				      psd_image.layer[lnum].height);
		  }
		  
		layer_ID = gimp_layer_new (image_ID,
					   psd_image.layer[lnum].name,
					   psd_image.layer[lnum].width,
					   psd_image.layer[lnum].height,
					   (numc==1) ? GRAY_IMAGE:GRAYA_IMAGE,
					   (100.0*(double)psd_image.layer[lnum].opacity)/255.0,
					   psd_lmode_to_gimp_lmode(psd_image.layer[lnum].blendkey));

	      }; break; /* case GRAY */
	    case RGB:
	      {
		printf("It's RGB.\n");
		if (!psd_layer_has_alpha(&psd_image.layer[lnum]))
		  {
		    merged_data =
		      chans_to_RGB (psd_image.layer[lnum].channel[0].data,
				    psd_image.layer[lnum].channel[1].data,
				    psd_image.layer[lnum].channel[2].data,
				    psd_image.layer[lnum].width *
				    psd_image.layer[lnum].height);
		  }
		else
		  {
		    merged_data =
		      chans_to_RGBA (psd_image.layer[lnum].channel[1].data,
				     psd_image.layer[lnum].channel[2].data,
				     psd_image.layer[lnum].channel[3].data,
				     psd_image.layer[lnum].channel[0].data,
				     psd_image.layer[lnum].width *
				     psd_image.layer[lnum].height);
		  }
		  
		layer_ID = gimp_layer_new (image_ID,
					   psd_image.layer[lnum].name,
					   psd_image.layer[lnum].width,
					   psd_image.layer[lnum].height,
					   (numc==3) ? RGB_IMAGE:RGBA_IMAGE,
					   (100.0*(double)psd_image.layer[lnum].opacity)/255.0,
					   psd_lmode_to_gimp_lmode(psd_image.layer[lnum].blendkey));

	      }; break; /* case RGB */
	    default:
	      {
		printf("Error: Sorry, can't deal with a layered image of this type.\n");
		gimp_quit();
	      }; break; /* default */
	    }


	  gimp_image_add_layer (image_ID, layer_ID, 0);


	  /* Do a layer mask if it exists */
	  for (iter=0; iter<psd_image.layer[lnum].num_channels; iter++)
	    {
	      if (psd_image.layer[lnum].channel[iter].type == -2) /* is mask */
		{
		  gint32 mask_id;
		  guchar* lm_data;

		  lm_data = xmalloc(psd_image.layer[lnum].width *
				    psd_image.layer[lnum].height);
		  {
		    /* PS layer masks can be a different size to
		       their owning layer, so we have to resize them. */
		    resize_mask(psd_image.layer[lnum].channel[iter].data,
				lm_data,
				psd_image.layer[lnum].lm_x-
				psd_image.layer[lnum].x,
				psd_image.layer[lnum].lm_y-
				psd_image.layer[lnum].y,
				psd_image.layer[lnum].lm_width,
				psd_image.layer[lnum].lm_height,
				psd_image.layer[lnum].width,
				psd_image.layer[lnum].height);
		    
		    mask_id = gimp_layer_create_mask(layer_ID, 0);
		    gimp_image_add_layer_mask(image_ID, layer_ID, mask_id);
		    
		    drawable = gimp_drawable_get (mask_id);
		    
		    gimp_pixel_rgn_init (&pixel_rgn, drawable,
					 0, 0, 
					 psd_image.layer[lnum].width,
					 psd_image.layer[lnum].height,
					 TRUE, FALSE);
		    gimp_pixel_rgn_set_rect (&pixel_rgn,
					     lm_data,
					     0, 0, 
					     psd_image.layer[lnum].width,
					     psd_image.layer[lnum].height);
		  }
		  g_free(lm_data);
		  
		  gimp_drawable_flush (drawable);
		  gimp_drawable_detach (drawable);
		}
	    }


	  gimp_layer_translate(layer_ID,
			       psd_image.layer[lnum].x,
			       psd_image.layer[lnum].y);
	  
	  gimp_layer_set_preserve_transparency(layer_ID, psd_image.layer[lnum].protecttrans);
	  gimp_layer_set_visible(layer_ID, psd_image.layer[lnum].visible);

	  drawable = gimp_drawable_get (layer_ID);
	  
	  gimp_pixel_rgn_init (&pixel_rgn, drawable,
			       0, 0, 
			       psd_image.layer[lnum].width,
			       psd_image.layer[lnum].height,
			       TRUE, FALSE);
	  gimp_pixel_rgn_set_rect (&pixel_rgn,
				   merged_data,
				   0, 0, 
				   psd_image.layer[lnum].width,
				   psd_image.layer[lnum].height);
	  
	  gimp_drawable_flush (drawable);
	  gimp_drawable_detach (drawable);
	  
	  if (merged_data)
	    g_free(merged_data);

	  gimp_progress_update ((double)(lnum+1.0) /
				(double)psd_image.num_layers);
	}
    }



  if ((psd_image.num_aux_channels > 0) &&
      (psd_image.num_layers > 0))
    {
      want_aux = TRUE;
      printf("::::::::::: WANT AUX :::::::::::::::::::::::::::::::::::::::\n");
    }
  else
    {
      want_aux = FALSE;
    }



  if (want_aux || (psd_image.num_layers==0)) /* PS2-style - NO LAYERS. */
    {
      
      printf("Image data %ld chars\n", PSDheader.imgdatalen);

      step = PSDheader.channels;
    
      imagetype = PSD_UNKNOWN_IMAGE;
      switch (PSDheader.mode)
	{
	case 0:		/* Bitmap */
	  break;
	case 1:		/* Grayscale */
	  imagetype = PSD_GRAY_IMAGE;
	  break;
	case 2:		/* Indexed Colour */
	  imagetype = PSD_INDEXED_IMAGE;
	  break;
	case 3:		/* RGB Colour */
	  imagetype = PSD_RGB_IMAGE;
	  break;
	case 4:		/* CMYK Colour */
	  cmyk = 1;
	  switch (PSDheader.channels)
	    {
	    case 4:
	      imagetype = PSD_RGB_IMAGE;
	      break;
	    case 5:
	      imagetype = PSD_RGBA_IMAGE;
	      break;
	    default:
	      printf("%s: cannot handle CMYK with more than 5 channels\n",
		     prog_name);
	      return(-1);
	      break;
	    }
	  break;
	case 7:		/* Multichannel (?) */
	case 8:		/* Duotone */
	case 9:		/* Lab Colour */
	default:
	  break;
	}


      if (imagetype == PSD_UNKNOWN_IMAGE)
	{
	  printf("%s: Image type %d (%s) is not supported in this data format\n",
		 prog_name, PSDheader.mode,
		 (PSDheader.mode > 10) ?
		 "<out of range>" : modename[PSDheader.mode]);

	  return(-1);
	}

      if (PSDheader.bpp != 8)
	{
	  printf("%s: The GIMP only supports 8-bit deep PSD images\n",
		 prog_name);
	  return(-1);
	}

      printf(
	     "psd:%d gimp:%d gimpbase:%d\n",
	     imagetype,
	     psd_type_to_gimp_type(imagetype),
	     psd_type_to_gimp_base_type(imagetype)
	     );


      if (!want_aux)
	{
	  image_ID = gimp_image_new (PSDheader.columns, PSDheader.rows,
				     psd_type_to_gimp_base_type(imagetype));
	  gimp_image_set_filename (image_ID, name);
	  if (psd_type_to_gimp_base_type(imagetype) == INDEXED)
	    {
	      if ((psd_image.colmaplen%3)!=0)
		printf("PSD: Colourmap looks screwed! Aiee!\n");
	      if (psd_image.colmaplen==0)
		printf("PSD: Indexed image has no colourmap!\n");
	      if (psd_image.colmaplen!=768)
		printf("PSD: Warning: Indexed image is %ld!=256 colours.\n",
		       psd_image.colmaplen/3);
	      if (psd_image.colmaplen==768)
		{
		  reshuffle_cmap(psd_image.colmapdata);
		  gimp_image_set_cmap (image_ID,
				       psd_image.colmapdata,
				       256);
		}
	    }

	  layer_ID = gimp_layer_new (image_ID, "Background",
				     PSDheader.columns, PSDheader.rows,
				     psd_type_to_gimp_type(imagetype),
				     100, NORMAL_MODE);

	  g_free(name_buf);

	  gimp_image_add_layer (image_ID, layer_ID, 0);
	  drawable = gimp_drawable_get (layer_ID);
    
	}

    
      if (want_aux)
	{
	  switch (PSDheader.mode)
	    {
	    case 1:		/* Grayscale */
	      channels = 1;
	      break;
	    case 2:		/* Indexed Colour */
	      channels = 1;
	      break;
	    case 3:		/* RGB Colour */
	      channels = 3;
	      break;
	    default:
	      printf("aux? Aieeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee!!!!!!!!!\n");
	      channels = 1;
	      break;
	    }
	  
	  channels = PSDheader.channels - channels;
	  
	  if (psd_image.absolute_alpha)
	    {
	      channels--;
	    }
	}
      else
	{
	  channels = gimp_drawable_bpp(drawable->id);
	}


      dest = xmalloc( step * PSDheader.columns * PSDheader.rows );

      
      gimp_progress_update ((double)0.0);


      if (PSDheader.compression == 1)
	{
	  nguchars = PSDheader.columns * PSDheader.rows;
	  temp = xmalloc(PSDheader.imgdatalen);
	  xfread(fd, temp, PSDheader.imgdatalen, "image data");
	  if (!cmyk)
	    {
	      gimp_progress_update ((double)0.25);
	      decode(PSDheader.imgdatalen, nguchars, temp, dest, step);
	    }
	  else
	    {
	      gimp_progress_update ((double)0.25);
	      cmykbuf = xmalloc(step * nguchars);
	      decode(PSDheader.imgdatalen, nguchars, temp, cmykbuf, step);
	      
	      gimp_progress_update ((double)0.50);
	      cmyk2rgb(cmykbuf, dest, PSDheader.columns, PSDheader.rows,
		       step > 4);
	      g_free(cmykbuf);
	    }
	  g_free(temp);
	}
      else
	{
	  if (!cmyk)
	    {
	      gimp_progress_update ((double)0.50);
	      xfread(fd, dest, PSDheader.imgdatalen, "image data");
	    }
	  else
	    {	  
	      gimp_progress_update ((double)0.25);
	      cmykbuf = xmalloc(PSDheader.imgdatalen);
	      xfread(fd, cmykbuf, PSDheader.imgdatalen, "image data");
	      gimp_progress_update ((double)0.50);
	      cmykp2rgb(cmykbuf, dest,
			PSDheader.columns, PSDheader.rows, step > 4);
	      g_free(cmykbuf);
	    }
	}


      if (want_aux) /* want_aux */
	{
	  extract_channels(dest, channels, step,
			   image_ID,
			   PSDheader.columns, PSDheader.rows);
	  goto finish_up;
	}
      else
	{
	  gimp_progress_update ((double)0.96);

	  if (channels == step) /* gimp bpp == psd bpp */
	    {

	      if (psd_type_to_gimp_type(imagetype)==INDEXEDA_IMAGE)
		{
		  printf("@@@@ Didn't know about this.\n");
		  for (iter=0; iter<drawable->width*drawable->height; iter++)
		    {
		      dest[iter*2+1] = 255;
		    }
		}
	  
	      gimp_pixel_rgn_init (&pixel_rgn, drawable,
				   0, 0, drawable->width, drawable->height,
				   TRUE, FALSE);
	      gimp_pixel_rgn_set_rect (&pixel_rgn, dest,
				       0, 0, drawable->width, drawable->height);

	      gimp_drawable_flush (drawable);
	      gimp_drawable_detach (drawable);
	    }
	  else
	    {
	      printf("Uhhh... uhm... extra channels... heavy...\n");
	  
	      extract_data_and_channels(dest, channels, step,
					image_ID, drawable,
					drawable->width, drawable->height);	  
	    }
	}



    finish_up:
      
      g_free (dest);
      
      if (psd_image.colmaplen > 0)
	g_free(psd_image.colmapdata);
    }


  gimp_displays_flush();

  printf("--- %d layers : pos %ld : a-alph %d ---\n",
	 psd_image.num_layers, (long int)ftell(fd),
	 psd_image.absolute_alpha);

  return(image_ID);
}



static void
decode(long clen, long uclen, char * src, guchar * dst, int step)
{
    gint i, j;
    gint32 l;
    gushort * w;

    l = clen;
    for (i = 0; i < PSDheader.rows*PSDheader.channels; ++i)
	l -= PSDheader.rowlength[i];
    if (l)
	printf("*** %ld should be zero\n", (long)l);

    w = PSDheader.rowlength;
    
    packbitsdecode(&clen, uclen, src, dst++, step);
    for (j = 0; j < step-1; ++j) {
	for (i = 0; i < PSDheader.rows; ++i)
	    src += *w++;
	packbitsdecode(&clen, uclen, src, dst++, step);
    }
    printf("clen %ld\n", clen);
}



/*
 * Decode a PackBits data stream.
 */
static void
packbitsdecode(long * clenp, long uclen, char * src, guchar * dst, int step)
{
    gint n, b;
    gint32 clen = *clenp;
    
    while ((clen > 0) && (uclen > 0)) {
	n = (int) *src++;
	if (n >= 128)
	    n -= 256;
	if (n < 0) {		/* replicate next guchar -n+1 times */
	    clen -= 2;
	    if (n == -128)	/* nop */
		continue;
	    n = -n + 1;
	    uclen -= n;
	    for (b = *src++; n > 0; --n) {
		*dst = b;
		dst += step;
	    }
	} else {		/* copy next n+1 guchars literally */
	    for (b = ++n; b > 0; --b) {
		*dst = *src++;
		dst += step;
	    }
	    uclen -= n;
	    clen -= n+1;
	}
    }
    if (uclen > 0) {
	printf("%s: unexpected EOF while reading image data\n", prog_name);
	gimp_quit();
    }
    *clenp = clen;
}



/*
 * Decode a PackBits channel from file.
 */
static void
unpack_pb_channel(FILE *fd, guchar *dst, gint32 unpackedlen, guint32 *offset)
{
    int n, b;
    gint32 upremain = unpackedlen;

    while (upremain > 0)
      {
	(*offset)++;
	n = (int) getguchar(fd, "packbits1");

	if (n >= 128)
	  n -= 256;
	if (n < 0)
	  {		/* replicate next guchar -n+1 times */
	    if (n == -128)	/* nop */
	      continue;
	    n = -n + 1;
	    /*	    upremain -= n;*/

	    b = getguchar(fd, "packbits2");
	    (*offset)++;
	    for (; n > 0; --n)
	      {
		*dst = b;
		dst ++;
		upremain--;
	      }
	  }
	else
	  {		/* copy next n+1 guchars literally */
	    for (b = ++n; b > 0; --b)
	      {
		*dst = getguchar(fd, "packbits3");;
		(*offset)++;
		dst ++;
		upremain--;
	      }
	    /*	    upremain -= n;*/
	  }
      }

    if (upremain < 0)
      {
	printf("*** Unpacking overshot destination (%d) buffer by %d bytes!\n",
	       unpackedlen,
	       -upremain);
      }
}

static void
cmyk2rgb(unsigned char * src, unsigned char * dst,
	 long width, long height, int alpha)
{
    int r, g, b, k;
    int i, j;


    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    r = *src++;
	    g = *src++;
	    b = *src++;
	    k = *src++;

	    cmyk_to_rgb (&r, &g, &b, &k);
	  
	    *dst++ = r;
	    *dst++ = g;
	    *dst++ = b;

	    if (alpha)
		*dst++ = *src++;
	}
	if ((i % 5) == 0)
	    gimp_progress_update ((double)(2.0*(double)height)/((double)height+(double)i));
    }
}


/*
 * Decode planar CMYK(A) to RGB(A).
 */
static void
cmykp2rgb(unsigned char * src, unsigned char * dst,
	  long width, long height, int alpha)
{
    int r, g, b, k;
    int i, j;
    long n;
    guchar *rp, *gp, *bp, *kp, *ap;

    n = width * height;
    rp = src;
    gp = rp + n;
    bp = gp + n;
    kp = bp + n;
    ap = kp + n;
    
    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    r = *rp++;
	    g = *gp++;
	    b = *bp++;
	    k = *kp++;

	    cmyk_to_rgb (&r, &g, &b, &k);
	  
	    *dst++ = r;
	    *dst++ = g;
	    *dst++ = b;

	    if (alpha)
		*dst++ = *ap++;
	}
	if ((i % 5) == 0)
	  gimp_progress_update ((double)(2.0*(double)height)/((double)height+(double)i));
    }
}


static void
cmyk_to_rgb(gint *c, gint *m, gint *y, gint *k)
{
#if 0
    gint cyan, magenta, yellow, black;

    cyan = *c;
    magenta = *m;
    yellow = *y;
    black = *k;

    if (black > 0) {
	cyan -= black;
	magenta -= black;
	yellow -= black;
    }
    *c = 255 - cyan;
    *m = 255 - magenta;
    *y = 255 - yellow;
    if (*c < 0) *c = 0; else if (*c > 255) *c = 255;
    if (*m < 0) *m = 0; else if (*m > 255) *m = 255;
    if (*y < 0) *y = 0; else if (*y > 255) *y = 255;
#endif
}


void
dumpchunk(size_t n, FILE * fd, gchar *why)
{
  guint32 i;

  printf("\n");

  for (i=0;i<n;i++)
    {
      printf("%02x ",(int)getguchar(fd, why));
    }

  printf("\n");
}


void
throwchunk(size_t n, FILE * fd, gchar *why)
{
  gchar *tmpchunk;

  if (n==0)
    {
      return;
    }

  tmpchunk = xmalloc(n);
  xfread(fd, tmpchunk, n, why);
  g_free(tmpchunk);
}


#if 0
static guchar *
getchunk(size_t n, FILE * fd, char *why)
{
  guchar *tmpchunk;

  tmpchunk = xmalloc(n);
  xfread(fd, tmpchunk, n, why);
  return(tmpchunk);  /* caller should free memory */
}
#endif


static gchar *
getpascalstring(FILE *fd, gchar *why)
{
  gchar *tmpchunk;
  guchar len;

  xfread(fd, &len, 1, why);

  tmpchunk = xmalloc(len+1);

  if (len==0)
    {
      tmpchunk[0]=0;
      return(tmpchunk);
    }

  xfread(fd, tmpchunk, len, why);
  tmpchunk[len]=0;
  
  return(tmpchunk); /* caller should free memory */
}


static guchar
getguchar(FILE *fd, char *why)
{
  guchar tmp;

  xfread(fd, &tmp, 1, why);

  return(tmp);
}


static gshort
getgshort(FILE *fd, char *why)
{
  gushort w;
  guchar b1, b2;
  
  xfread(fd, &b1, 1, why);
  xfread(fd, &b2, 1, why);
  
  w = (b1*256) + b2;
  
  return ((gshort) w);
}


static glong
getglong(FILE *fd, char * why)
{
    unsigned char s1, s2, s3, s4;
    gulong w;

    xfread(fd, &s1, 1, why);
    xfread(fd, &s2, 1, why);
    xfread(fd, &s3, 1, why);
    xfread(fd, &s4, 1, why);

    w = (s1*256*256*256) + (s2*256*256) + (s3*256) + s4;

    return (glong) w;
}


static void
xfread(FILE * fd, void * buf, long len, char * why)
{
    if (fread(buf, len, 1, fd) == 0) {
	printf("%s: unexpected EOF while reading '%s' chunk\n",
	       prog_name, why);
	gimp_quit();
    }
}


static void *
xmalloc(size_t n)
{
    void *p;

    if (n == 0)
      {
	printf("\nWARNING: %s: xmalloc asked for zero-sized chunk\n",
	       prog_name);
	return (NULL);
      }

    if ((p = g_malloc(n)) != NULL)
	return p;

    printf("%s: out of memory\n", prog_name);
    gimp_quit();
    return NULL;
}


static void
read_whole_file(FILE * fd)
{
    guint16 w;
    gint32 pos;
    gchar dummy[6];
    gint i;
    
    xfread(fd, &PSDheader.signature, 4, "signature");
    PSDheader.version = getgshort(fd, "version");
    xfread(fd, &dummy, 6, "reserved");
    PSDheader.channels = getgshort(fd, "channels");
    PSDheader.rows = getglong(fd, "rows");
    PSDheader.columns = getglong(fd, "columns");
    PSDheader.bpp = getgshort(fd, "depth");
    PSDheader.mode = getgshort(fd, "mode");


    psd_image.num_layers = 0;
    psd_image.type = PSDheader.mode;
    psd_image.colmaplen = 0;
    psd_image.num_aux_channels = 0;


    psd_image.colmaplen = getglong(fd, "color data length");

    if (psd_image.colmaplen > 0)
      {
	psd_image.colmapdata = xmalloc(psd_image.colmaplen);
	xfread(fd, psd_image.colmapdata, psd_image.colmaplen, "colormap");
      }


    PSDheader.imgreslen = getglong(fd, "image resource length");
    if (PSDheader.imgreslen > 0)
      {
	do_image_resources(fd);
      }


    PSDheader.miscsizelen = getglong(fd, "misc size data length");
    if (PSDheader.miscsizelen > 0)
      {
	do_layer_and_mask(fd);
      }


    PSDheader.compression = getgshort(fd, "compression");
    printf("<<compr:%d>>", (int)PSDheader.compression);
    if (PSDheader.compression == 1) /* RLE */
      {
	PSDheader.rowlength = xmalloc(PSDheader.rows *
				      PSDheader.channels * sizeof(gushort));
	for (i = 0; i < PSDheader.rows*PSDheader.channels; ++i)
	  PSDheader.rowlength[i] = getgshort(fd, "x");
      }
    pos = ftell(fd);
    fseek(fd, 0, SEEK_END);
    PSDheader.imgdatalen = ftell(fd)-pos;
    fseek(fd, pos, SEEK_SET);


    if (strncmp(PSDheader.signature, "8BPS", 4) != 0)
      {
	printf("%s: not an Adobe Photoshop PSD file\n", prog_name);
	gimp_quit();
      }
    if (PSDheader.version != 1)
      {
	printf("%s: bad version number '%d', not 1\n",
	       prog_name, PSDheader.version);
	gimp_quit();
      }
    w = PSDheader.mode;
    printf("HEAD:\n"
	   "\tChannels %d\n\tRows %ld\n\tColumns %ld\n\tDepth %d\n\tMode %d (%s)\n"
	   "\tColour data %ld guchars\n",
	   PSDheader.channels, PSDheader.rows,
	   PSDheader.columns, PSDheader.bpp,
	   w, modename[w < 10 ? w : 10],
	   psd_image.colmaplen);
    /*    printf("\tImage resource length: %lu\n", PSDheader.imgreslen);*/
    printf("\tLayer/Mask Data length: %lu\n", PSDheader.miscsizelen);
    w = PSDheader.compression;
    printf("\tCompression %d (%s)\n", w, w ? "RLE" : "raw");
}
