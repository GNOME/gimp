/* bmp.c                                          */
/* Version 0.51	                                  */
/* This is a File input and output filter for the */
/* Gimp. It loads and saves images in windows(TM) */
/* bitmap format.                                 */
/* Some Parts that deal with the interaction with */
/* the Gimp are taken from the GIF plugin by      */
/* Peter Mattis & Spencer Kimball and from the    */
/* PCX plugin by Francisco Bustamante.            */
/*                                                */
/* Alexander.Schulz@stud.uni-karlsruhe.de         */

/* Changes:   28.11.1997 Noninteractive operation */
/*            16.03.1998 Endian-independent!!     */
/*	      21.03.1998 Little Bug-fix		  */
/*            06.04.1998 Bugfix in Padding        */
/*            11.04.1998 Arch. cleanup (-Wall)    */
/*                       Parses gtkrc             */
/*            14.04.1998 Another Bug in Padding   */
/*            28.04.1998 RLE-Encoding rewritten   */
/*            29.10.1998 Changes by Tor Lillqvist */
/*                       <tml@iki.fi> to support  */
/*                       16 and 32 bit images     */
/*            28.11.1998 Bug in RLE-read-padding  */
/*                       fixed.                   */
/*            19.12.1999 Resolution support added */
/*            06.05.2000 Overhaul for 16&24-bit   */
/*                       plus better OS/2 code    */
/*                       by njl195@zepler.org.uk  */

/* 
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "bmp.h"

#include "libgimp/stdplugins-intl.h"

FILE  *errorfile;
gchar *prog_name = "bmp";
gchar *filename;
gint   interactive_bmp;

struct Bitmap_File_Head_Struct Bitmap_File_Head;
struct Bitmap_Head_Struct Bitmap_Head;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (gchar   *name,
                          gint     nparams,
                          GimpParam  *param,
                          gint    *nreturn_vals,
                          GimpParam **return_vals);

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
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw_filename", "The name entered" },
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
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
    { GIMP_PDB_STRING,   "raw_filename", "The name entered" },
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);
  
  INIT_I18N();

  gimp_install_procedure ("file_bmp_load",
                          "Loads files of Windows BMP file format",
                          "Loads files of Windows BMP file format",
                          "Alexander Schulz",
                          "Alexander Schulz",
                          "1997",
                          "<Load>/BMP",
                          NULL,
                          GIMP_PLUGIN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_bmp_save",
                          "Saves files in Windows BMP file format",
                          "Saves files in Windows BMP file format",
                          "Alexander Schulz",
                          "Alexander Schulz",
                          "1997",
                          "<Save>/BMP",
                          "INDEXED, GRAY, RGB",
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_bmp_load",
				    "bmp",
				    "",
				    "0,string,BM");
  gimp_register_save_handler       ("file_bmp_save",
				    "bmp",
				    "");
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;
  
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_bmp_load") == 0)
    {
       INIT_I18N();

       switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
	  interactive_bmp = TRUE;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          interactive_bmp = FALSE;
	  if (nparams != 3)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

       if (status == GIMP_PDB_SUCCESS)
	 {
	   image_ID = ReadBMP (param[1].data.d_string);

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
    }
  else if (strcmp (name, "file_bmp_save") == 0)
    {
      INIT_I18N();

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init ("bmp", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "BMP", 
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
	  interactive_bmp = TRUE;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          interactive_bmp = FALSE;
	  if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          interactive_bmp = FALSE;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
	{
	  status = WriteBMP (param[3].data.d_string, image_ID, drawable_ID);
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

gint32 
ToL (guchar *puffer)
{
  return (puffer[0] | puffer[1]<<8 | puffer[2]<<16 | puffer[3]<<24);
}

gint16 
ToS (guchar *puffer)
{
  return (puffer[0] | puffer[1]<<8);
}

void 
FromL (gint32  wert, 
       guchar *bopuffer)
{
  bopuffer[0] = (wert & 0x000000ff)>>0x00;
  bopuffer[1] = (wert & 0x0000ff00)>>0x08;
  bopuffer[2] = (wert & 0x00ff0000)>>0x10;
  bopuffer[3] = (wert & 0xff000000)>>0x18;
}

void  
FromS (gint16  wert, 
       guchar *bopuffer)
{
  bopuffer[0] = (wert & 0x00ff)>>0x00;
  bopuffer[1] = (wert & 0xff00)>>0x08;
}
