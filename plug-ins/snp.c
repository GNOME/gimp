/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Plugin to load MicroEyes SNP format animations
 * v1.1 - Adam D. Moss, adam@foxbox.org
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
 * Changelog:
 *
 * 97/07/08
 * v1.1: SNP Animations only use 192 colour entries, so the resulting
 *       GIMP INDEXED image should reflect this.
 *       Added progress bar.
 *
 * 97/07/06
 * v1.0: Initial release.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"



static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);


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
  gimp_install_procedure ("file_snp_load",
                          "Loads animations of the MicroEyes SNP file format",
                          "FIXME: write help for snp_load",
                          "Adam D. Moss",
                          "Adam D. Moss",
                          "1997",
                          "<Load>/Snp",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);
  gimp_register_load_handler ("file_snp_load", "snp", "");
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

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_snp_load") == 0)
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
  else
    g_assert (FALSE);
}


short unsigned int getword(FILE *fp)
{
  int c1,c2;

  c1 = fgetc(fp);
  c2 = fgetc(fp);

  return(c1|(c2<<8));
}


unsigned int getlongword(FILE *fp)
{
  int c1,c2,c3,c4;

  c1 = fgetc(fp);
  c2 = fgetc(fp);
  c3 = fgetc(fp);
  c4 = fgetc(fp);

  return(c1|(c2<<8)|(c3<<16)|(c4<<24));
}

 
static gint32
load_image (char *filename)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  gint32 image_ID;
  gint32 layer_ID;
  gchar *temp;

  guchar pal[768];
  gint i,j,k, frames, delay, wwidth, wheight;
  FILE *fp;
  guchar *data;

  gchar layername[200];  /* FIXME? */


  temp = g_malloc (strlen (filename) + 12);
  if (!temp) gimp_quit ();
  g_free (temp);


  
  gimp_progress_init ("Loading SNP file...");



  fp = fopen(filename,"rb");


  k=getword(fp);
  printf("ComputerEyes SNIP version %d\n",k);

  frames=getword(fp);
  printf("%d frames, ",frames);

  wwidth=getword(fp);
  wheight=getword(fp);

  data = g_malloc(wwidth * wheight * 2);


  printf("%dx%d\n",wwidth,wheight);

  getword(fp);  /* reserved */
  delay = getword(fp);
  getword(fp);  /* reserved */
  getword(fp);  /* reserved */



  
  image_ID = gimp_image_new (wwidth, wheight, INDEXED);
  gimp_image_set_filename (image_ID, filename);
  


  for (i=0;i<768;i++)
    {
      pal[i] = (fgetc(fp)*255L) / 63L;
    }

  gimp_image_set_cmap (image_ID,
		       &pal[64*3],
		       256-64);

  
  for (i=0;i<frames+1;i++)
    {
      getlongword(fp);
    }
  
  for (i=0;i<frames;i++)
    {
      j=0;

      gimp_progress_update (((double)i) / ((double)frames));
      
      sprintf(layername, "Frame %d (%dms)",
	      i, (1000/18)*delay);
      layer_ID = gimp_layer_new (image_ID, layername,
				 wwidth,
				 wheight,
				 INDEXEDA_IMAGE, 100, NORMAL_MODE);
      gimp_image_add_layer (image_ID, layer_ID, 0);
      drawable = gimp_drawable_get (layer_ID);
      
      k = fgetc(fp);
      while ((k>0) && (j<wwidth*wheight))
        {
          if (k>63)
	    {
	      data[j*2] = k-64;
	      data[j*2+1] = 255;
	      j++;
	    }
          else
	    {
	      j+=k;
	    }
          k=fgetc(fp);
        }

      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0,0, wwidth,wheight);
      
      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);
    }    

  gimp_progress_update (1.0);

  g_free (data);

  return image_ID;

}
