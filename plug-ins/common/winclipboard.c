/*
 * WinClipboard Win32 Windoze Copy&Paste Plug-in
 * Copyright (C) 1999 Hans Breuer
 * Hans Breuer, Hans@Breuer.org
 * 08/07/99
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
 *
 * Based on (at least) the following plug-ins:
 *  Header
 *  WinSnap
 *
 * Any suggestions, bug-reports or patches are welcome.
 */

#include "config.h"

#include <windows.h>
#include <stdlib.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* History:
 *
 *   08/07/99 Implementation and release.
 *	 08/10/99 Big speed increase by using gimp_tile_cache_size()
 *			  Thanks to Kevin Turner's documentation at:
 *			  http://www.poboxes.com/kevint/gimp/doc/plugin-doc-2.1.html
 *
 * TODO (maybe):
 *
 *   - Support for 4,2,1 bit bitmaps
 *   - Unsupported formats could be delegated to GIMP Loader (e.g. wmf)
 *   - ...
 */

/* How many steps the progress control should do
 */
#define PROGRESS_STEPS 25
#define StepProgress(one,all) \
	(0 == (one % ((all / PROGRESS_STEPS)+1)))

/* FIXME: I'll use -1 as IMAGE_NONE. Is it correct ???
 */
#define IMAGE_NONE -1

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

/* Plugin function prototypes
 */
static int CB_CopyImage (gboolean    interactive,
			 gint32      image_ID,
			 gint32      drawable_ID);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query ()
{
  static GimpParamDef copy_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"             }
  };

  gimp_install_procedure ("plug_in_clipboard_copy",
                          "copy image to clipboard",
                          "Copies the active drawable to the clipboard.",
                          "Hans Breuer",
                          "Hans Breuer",
                          "1999",
                          N_("Copy to Clipboard"),
                          "INDEXED*, RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (copy_args), 0,
                          copy_args, NULL);

  gimp_plugin_menu_register ("plug_in_clipboard_copy",
                             "<Image>/Edit");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];
  GimpRunMode run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  INIT_I18N();

  if (strcmp (name, "plug_in_clipboard_copy") == 0)
    {
      *nreturn_vals = 1;
      if (CB_CopyImage (GIMP_RUN_INTERACTIVE==run_mode,
			param[1].data.d_int32,
			param[2].data.d_int32))
	values[0].data.d_status = GIMP_PDB_SUCCESS;
      else
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
}

/* Plugin Function implementation
 */
static int
CB_CopyImage (gboolean interactive,
	      gint32   image_ID,
	      gint32   drawable_ID)
{
  GimpDrawable *drawable;
  GimpImageType drawable_type;
  GimpPixelRgn pixel_rgn;
  gchar *sStatus = NULL;

  int nSizeDIB=0;
  int nSizePal=0;
  int nSizeLine=0; /* DIB lines are 32 bit aligned */

  HANDLE hDIB;
  BOOL bRet;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  /* allocate room for DIB */
  if (GIMP_INDEXED_IMAGE == drawable_type || GIMP_GRAY_IMAGE == drawable_type)
    {
      nSizeLine = ((drawable->width-1)/4+1)*4;
      nSizeDIB = sizeof(RGBQUAD) * 256 /* always full color map size */
	+ nSizeLine * drawable->height
	+ sizeof (BITMAPINFOHEADER);
    }
  else
    {
      nSizeLine = ((drawable->width*3-1)/4+1)*4;
      nSizeDIB = nSizeLine * drawable->height
	+ sizeof (BITMAPINFOHEADER);
    }

  hDIB = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, nSizeDIB);
  if (NULL == hDIB)
    {
      g_message ("Failed to allocate DIB");
      bRet = FALSE;
    }

  /* fill header info */
  if (bRet)
    {
      BITMAPINFOHEADER *pInfo;

      bRet = FALSE;
      pInfo = GlobalLock (hDIB);
      if (pInfo)
	{
	  pInfo->biSize   = sizeof(BITMAPINFOHEADER);
	  pInfo->biWidth  = drawable->width;
	  pInfo->biHeight = drawable->height;
	  pInfo->biPlanes = 1;
	  pInfo->biBitCount =
	    (GIMP_INDEXED_IMAGE == drawable_type || GIMP_GRAY_IMAGE == drawable_type ? 8 : 24);
	  pInfo->biCompression = BI_RGB; /* none */
	  pInfo->biSizeImage = 0; /* not calculated/needed */
	  pInfo->biXPelsPerMeter =
	    pInfo->biYPelsPerMeter = 0;
	  /* color map size */
	  pInfo->biClrUsed =
	    (GIMP_INDEXED_IMAGE == drawable_type || GIMP_GRAY_IMAGE == drawable_type ? 256 : 0);
	  pInfo->biClrImportant = 0; /* all */

	  GlobalUnlock (hDIB);
	  bRet = TRUE;
	} /* (pInfo) */
      else
	g_message("Failed to lock DIB Header");
    }

  /* fill color map */
  if (bRet && (GIMP_INDEXED_IMAGE == drawable_type  || GIMP_GRAY_IMAGE == drawable_type))
    {
      char *pBmp;

      bRet = FALSE;
      pBmp = GlobalLock (hDIB);
      if (pBmp)
	{
	  RGBQUAD *pPal;
	  int nColors;
	  unsigned char *cmap = NULL;
	  pPal = (RGBQUAD*)(pBmp + sizeof(BITMAPINFOHEADER));
	  nSizePal = sizeof(RGBQUAD) * 256;

	  /* get the gimp colormap */
	  if (GIMP_GRAY_IMAGE != drawable_type)
	    cmap = gimp_image_get_colormap (image_ID, &nColors);

	  if (cmap)
	    {
	      int i;
	      for (i = 0; (i < 256) && (i < nColors); i++)
		{
		  pPal[i].rgbReserved = 0; /* is this alpha? */
		  pPal[i].rgbRed      = cmap[3*i];
		  pPal[i].rgbGreen    = cmap[3*i+1];
		  pPal[i].rgbBlue     = cmap[3*i+2];
		}

	      g_free(cmap);
	      bRet = TRUE;
	    } /* (cmap) */
	  else if (GIMP_GRAY_IMAGE == drawable_type)
	    {
	      /* fill with identity palette */
	      int i;
	      for (i = 0; (i < 256) && (i < nColors); i++)
		{
		  pPal[i].rgbReserved = 0; /* is this alpha? */
		  pPal[i].rgbRed      = i;
		  pPal[i].rgbGreen    = i;
		  pPal[i].rgbBlue     = i;
		}

	      bRet = TRUE;
	    }
	  else
	    g_message ("Can't get color map");
	  GlobalUnlock (hDIB);
	} /* (pBmp) */
      else
	g_message ("Failed to lock DIB Palette");
    } /* indexed or grayscale */

  /* following the slow part ... */
  if (interactive)
    gimp_progress_init (_("Copying..."));

  /* speed it up with: */
  gimp_tile_cache_size (drawable->width * gimp_tile_height () * drawable->bpp);

  /* copy data to DIB */
  if (bRet)
    {
      unsigned char *pData;

      bRet = FALSE;
      pData = GlobalLock (hDIB);

      if (pData)
	{
	  unsigned char *pLine;

	  /* calculate real offset */
	  pData += (sizeof(BITMAPINFOHEADER) + nSizePal);

	  pLine = g_new (guchar, drawable->width * drawable->bpp);

	  if (GIMP_INDEXED_IMAGE == drawable_type || GIMP_GRAY_IMAGE == drawable_type)
	    {
	      int x, y;
	      for (y = 0; y < drawable->height; y++)
		{
		  if ((interactive) && (StepProgress(y,drawable->height)))
		    gimp_progress_update ((double)y / drawable->height);

		  gimp_pixel_rgn_get_row (&pixel_rgn, pLine, 0,
					  drawable->height-y-1, /* invert it */
					  drawable->width);
		  for (x = 0; x < drawable->width; x++)
		    pData[x+y*nSizeLine] = pLine[x*drawable->bpp];
		}
	    }
	  else
	    {
	      int x, y;
	      for (y = 0; y < drawable->height; y++)
		{
		  if ((interactive) && (StepProgress(y,drawable->height)))
		    gimp_progress_update ((double)y / drawable->height);

		  gimp_pixel_rgn_get_row (&pixel_rgn, pLine, 0,
					  drawable->height-y-1, /* invert it */
					  drawable->width);
		  for (x = 0; x < drawable->width; x++)
		    {
		      /* RGBQUAD: blue, green, red, reserved */
		      pData[x*3+y*nSizeLine]   = pLine[x*drawable->bpp+2]; /* blue */
		      pData[x*3+y*nSizeLine+1] = pLine[x*drawable->bpp+1]; /* green */
		      pData[x*3+y*nSizeLine+2] = pLine[x*drawable->bpp];   /* red */
		      /*pData[x+y*drawable->width*3+3] = 0;*/          /* reserved */
		    }
		}
	    }

	  g_free (pLine);
	  bRet = TRUE;

	  GlobalUnlock (hDIB);
	} /* (pData) */
      else
	g_message("Failed to lock DIB Data");
    } /* copy data to DIB */

  /* copy DIB to ClipBoard */
  if (bRet)
    {
      if (!OpenClipboard (NULL))
	{
	  g_message ("Cannot open the Clipboard!");
	  bRet = FALSE;
	}
      else
	{
	  if (bRet && !EmptyClipboard ())
	    {
	      g_message ("Cannot empty the Clipboard");
	      bRet = FALSE;
	    }
	  if (bRet)
	    {
	      if (NULL != SetClipboardData (CF_DIB, hDIB))
		hDIB = NULL; /* data now owned by clipboard */
	      else
		g_message ("Failed to set clipboard data ");
	    }

	  if (!CloseClipboard ())
	    g_message ("Failed to close Clipboard");
	}
    }
  /* done */
  if (hDIB)
    GlobalFree(hDIB);

  gimp_drawable_detach (drawable);

  return bRet;
} /* CB_CopyImage */
