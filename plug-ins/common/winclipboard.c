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
#include <windows.h>
#include <stdlib.h>

#include "libgimp/gimp.h"

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
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

/* Plugin function prototypes
 */
static int CB_CopyImage (gboolean interactive,
			             gint32   image_ID,
						 gint32   drawable_ID);

static int CB_PasteImage (gboolean   interactive,
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
  static GParamDef copy_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
  };
  static int ncopy_args = sizeof (copy_args) / sizeof (copy_args[0]);

  gimp_install_procedure ("plug_in_clipboard_copy",
                          "copy image to clipboard",
                          "Copies the active drawable to the clipboard.",
                          "Hans Breuer",
                          "Hans Breuer",
                          "1999",
                          "<Image>/Edit/Copy to Clipboard",
                          "INDEXED*, RGB*",
                          PROC_PLUG_IN,
                          ncopy_args, 0,
                          copy_args, NULL);

  gimp_install_procedure ("plug_in_clipboard_paste",
                          "paste image from clipboard",
                          "Paste image from clipboard into active image.",
                          "Hans Breuer",
                          "Hans Breuer",
                          "1999",
                          "<Image>/Edit/Paste from Clipboard",
                          "INDEXED*, RGB*",
                          PROC_PLUG_IN,
                          ncopy_args, 0,
                          copy_args, NULL);

  gimp_install_procedure ("extension_clipboard_paste",
                          "Get image from clipboard",
                          "Get an image from the Windows clipboard, creating a new image",
                          "Hans Breuer",
                          "Hans Breuer",
                          "1999",
                          "<Toolbox>/File/Acquire/From Clipboard",
                          "",
                          PROC_EXTENSION,
                          ncopy_args, 0,
                          copy_args, NULL);

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

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "plug_in_clipboard_copy") == 0)
  {
	  *nreturn_vals = 1;
      if (CB_CopyImage (RUN_INTERACTIVE==run_mode,
					    param[1].data.d_int32, 
						param[2].data.d_int32))
		values[0].data.d_status = STATUS_SUCCESS;
      else
		values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else if (strcmp (name, "plug_in_clipboard_paste") == 0)
	{
	  *nreturn_vals = 1;
      if (CB_PasteImage (RUN_INTERACTIVE==run_mode,
		                 param[1].data.d_int32, 
						 param[2].data.d_int32))
		values[0].data.d_status = STATUS_SUCCESS;
      else
		values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
  else if (strcmp (name, "extension_clipboard_paste") == 0)
	{
	  *nreturn_vals = 1;
      if (CB_PasteImage (RUN_INTERACTIVE==run_mode,
		                 IMAGE_NONE,
						 IMAGE_NONE))
		values[0].data.d_status = STATUS_SUCCESS;
      else
		values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
}

/* Plugin Function implementation
 */
static int
CB_CopyImage (gboolean interactive,
			  gint32   image_ID,
			  gint32   drawable_ID)
{
	GDrawable *drawable;
	GDrawableType drawable_type;
	GPixelRgn pixel_rgn;
	gchar* sStatus = NULL;

	int nSizeDIB=0;
	int nSizePal=0;
	int nSizeLine=0; /* DIB lines are 32 bit aligned */

	HANDLE hDIB;
	BOOL   bRet;

	drawable = gimp_drawable_get (drawable_ID);
	drawable_type = gimp_drawable_type (drawable_ID);
	gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

	/* allocate room for DIB */
	if (INDEXED_IMAGE == drawable_type)
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

	hDIB = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, nSizeDIB);
	if (NULL == hDIB)
	{
	  g_warning("Failed to allocate DIB");
	  bRet = FALSE;
	}

	/* fill header info */
	if (bRet)
	{
	  BITMAPINFOHEADER* pInfo;

	  bRet = FALSE;
	  pInfo = GlobalLock(hDIB);
	  if (pInfo)
	  {
		  pInfo->biSize   = sizeof(BITMAPINFOHEADER);
		  pInfo->biWidth  = drawable->width;
		  pInfo->biHeight = drawable->height;
		  pInfo->biPlanes = 1;
		  pInfo->biBitCount = (INDEXED_IMAGE == drawable_type ? 8 : 24);
		  pInfo->biCompression = BI_RGB; /* none */
		  pInfo->biSizeImage = 0; /* not calculated/needed */
		  pInfo->biXPelsPerMeter =
		  pInfo->biYPelsPerMeter = 0;
		  /* color map size */
		  pInfo->biClrUsed = (INDEXED_IMAGE == drawable_type ? 256 : 0);
		  pInfo->biClrImportant = 0; /* all */

		  GlobalUnlock(hDIB);
		  bRet = TRUE;
	  } /* (pInfo) */
	  else
		  g_warning("Failed to lock DIB Header");
	}

	/* fill color map */
	if (bRet && (INDEXED_IMAGE == drawable_type))
	{
	  char*    pBmp;

	  bRet = FALSE;
	  pBmp = GlobalLock(hDIB);
	  if (pBmp)
	  {
		  RGBQUAD* pPal;
		  int nColors;
		  unsigned char *cmap;
		  pPal = (RGBQUAD*)(pBmp + sizeof(BITMAPINFOHEADER));
		  nSizePal = sizeof(RGBQUAD) * 256;

		  /* get the gimp colormap */
		  cmap = gimp_image_get_cmap (image_ID, &nColors);

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
		  else
			  g_warning("Can't get color map");
		  GlobalUnlock(hDIB);
	  } /* (pBmp) */
	  else
		  g_warning("Failed to lock DIB Palette");
	} /* indexed */

	/* following the slow part ... */
	if (interactive)
	  gimp_progress_init ("Copying ...");

	/* speed it up with: */
	gimp_tile_cache_size(  drawable->width * gimp_tile_height() 
							* drawable->bpp );

	/* copy data to DIB */
	if (bRet)
	{
	  unsigned char* pData;

	  bRet = FALSE;
	  pData = GlobalLock(hDIB);

	  if (pData)
	  {
		  unsigned char* pLine;

		  /* calculate real offset */
		  pData += (sizeof(BITMAPINFOHEADER) + nSizePal);

		  pLine = g_new (guchar, drawable->width * drawable->bpp);

		  if (pLine)
		  {
			  if (INDEXED_IMAGE == drawable_type)
			  {
				  int x, y;
				  for (y = 0; y < drawable->height; y++)
				  {
					  if ((interactive) && (StepProgress(y,drawable->height)))
						  gimp_progress_update((double)y / drawable->height);

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
						  gimp_progress_update((double)y / drawable->height);

					  gimp_pixel_rgn_get_row (&pixel_rgn, pLine, 0, 
											  drawable->height-y-1, /* invert it */
											  drawable->width);
					  for (x = 0; x < drawable->width; x++)
					  {
						  /* RGBQUAD: blue, green, red, reserved */
						  pData[x*3+y*nSizeLine]   = pLine[x*drawable->bpp+2]; /* blue */
						  pData[x*3+y*nSizeLine+1] = pLine[x*drawable->bpp+1]; /* green */
						  pData[x*3+y*nSizeLine+2] = pLine[x*drawable->bpp];   /* red */
						  //pData[x+y*drawable->width*3+3] = 0;          /* reserved */
					  }
				  }
			  }

			  g_free(pLine);
			  bRet = TRUE;
		  } /* (pLine) */
		  else
			  g_warning("Failed to get line buffer");

		  GlobalUnlock(hDIB);
	  } /* (pData) */
	  else
		  g_warning("Failed to lock DIB Data");
	} /* copy data to DIB */

	/* copy DIB to ClipBoard */
	if (bRet)	  
	{
	  if (!OpenClipboard(NULL))
	  {
		  g_warning( "Cannot open the Clipboard!" );
		  bRet = FALSE;
	  }
	  else
	  {
		  if (bRet &&  !EmptyClipboard())
		  {
			  g_warning( "Cannot empty the Clipboard" );
			  bRet = FALSE;
		  }
		  if (bRet)
		  {
			  if (NULL != SetClipboardData(CF_DIB, hDIB))
				  hDIB = NULL; /* data now owned by clipboard */
			  else
				  g_warning ("Failed to set clipboard data ");
			  
		  }

		  if (!CloseClipboard())
			  g_warning("Failed to close Clipboard");
	  }
	}
	/* done */
	if (hDIB) GlobalFree(hDIB);

	gimp_drawable_detach (drawable);

	return bRet;
} /* CB_CopyImage */

static int
CB_PasteImage (gboolean interactive,
			   gint32   image_ID,
			   gint32   drawable_ID)
{
	UINT fmt;
	BOOL bRet=TRUE;
	HANDLE hDIB;
	gint32 nWidth  = 0;
	gint32 nHeight = 0;
	gint32 nBitsPS = 0;
	gint32 nColors = 0;

	if (!OpenClipboard(NULL))
	{
		g_warning("Failed to open clipboard");
		return FALSE;
	}

	fmt = EnumClipboardFormats(0);
	while ((CF_BITMAP != fmt) && (CF_DIB != fmt) && (0 != fmt))
		fmt = EnumClipboardFormats(fmt);

	if (0 == fmt) 
	{
		g_message("Unsupported format or Clipboard empty!");
		bRet = FALSE;
	}

	/* there is something supported */
	if (bRet) 
	{
		hDIB = GetClipboardData(CF_DIB);

		if (NULL == hDIB)
		{
			g_warning("Can't get Clipboard data");
			bRet = FALSE;
		}
	}

	/* read header */
	if (bRet && hDIB) 
	{
		BITMAPINFOHEADER* pInfo;

		pInfo = GlobalLock(hDIB);
		if (NULL == pInfo)
		{
			g_warning("Can't lock clipboard data!");
			bRet = FALSE;
		}

		if ((bRet) &&
			((pInfo->biSize != sizeof(BITMAPINFOHEADER)
			 || (pInfo->biCompression != BI_RGB))))
		{
			g_warning("Unupported bitmap format!");
			bRet = FALSE;
		}

		if (bRet && pInfo) 
		{
			nWidth  = pInfo->biWidth;
			nHeight = pInfo->biHeight;
			nBitsPS = pInfo->biBitCount;
			nColors = pInfo->biClrUsed;
		}

		GlobalUnlock(hDIB);
	}

	if ((0 != nWidth) && (0 != nHeight))
	{
		GDrawable* drawable;
		GPixelRgn pixel_rgn;
		char* pData;
		GParam *params;
		gint retval;
		gboolean bIsNewImage = TRUE;
		gint oldBPP=0;

		/* Check if clipboard data and existing image are compatible */
		if (IMAGE_NONE != drawable_ID)
		{
			drawable = gimp_drawable_get(drawable_ID);
			oldBPP = drawable->bpp;
			gimp_drawable_detach(drawable);
		}

		if ((IMAGE_NONE == image_ID)
			|| (3 != oldBPP) || (24 != nBitsPS))
		{
			/* create new image */
			image_ID = gimp_image_new (nWidth, nHeight, nBitsPS <= 8 ? INDEXED : RGB);
			gimp_image_disable_undo(image_ID);
			drawable_ID = gimp_layer_new (image_ID, "Background", nWidth, nHeight, 
										  nBitsPS <= 8 ? INDEXED_IMAGE : RGB_IMAGE, 
										  100, NORMAL_MODE);
		}
		else
		{
			/* ??? gimp_convert_rgb(image_ID);
			 */
			drawable_ID = gimp_layer_new (image_ID, "Pasted", nWidth, nHeight, 
										  nBitsPS <= 8 ? INDEXED_IMAGE : RGB_IMAGE, 
								          100, NORMAL_MODE);
			bIsNewImage = FALSE;
		}
		
		gimp_image_add_layer(image_ID,drawable_ID,-1);
		drawable = gimp_drawable_get(drawable_ID);

		gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

	    /* following the slow part ... */
		if (interactive)
			gimp_progress_init ("Pasteing ...");

		pData = GlobalLock(hDIB);
		if (pData)
		{
			guchar* pLine;
			RGBQUAD* pPal;
			int nSizeLine=0; /* DIB lines are 32 bit aligned */

			nSizeLine = ((nWidth*(nBitsPS/8)-1)/4+1)*4;

			/* adjust pointer */
			pPal  = (RGBQUAD*)(pData + sizeof(BITMAPINFOHEADER));
			pData = (guchar*)pPal  + sizeof(RGBQUAD) * nColors;

			/*  create palette */
			if (0 != nColors)
			{
				int c;
				guchar* cmap;

				cmap = g_new(guchar, nColors*3);
				if (cmap)
				{
					for (c = 0; c < nColors; c++)
					{
						cmap[c*3]   = pPal[c].rgbRed;
						cmap[c*3+1] = pPal[c].rgbGreen;
						cmap[c*3+2] = pPal[c].rgbBlue;
					}
					gimp_image_set_cmap(image_ID, cmap, nColors);
					g_free(cmap);
				}
			}

			/* speed it up with: */
			gimp_tile_cache_size(  drawable->width * gimp_tile_height() 
									* drawable->bpp );
			
			/* change data format and copy data */
			if (24 == nBitsPS)
			{
				pLine = g_new(guchar, drawable->width*drawable->bpp);
				if (pLine)
				{
					int y;
					for (y = 0; y < drawable->height; y++)
					{
						int x;

					    if ((interactive) && (StepProgress(y,drawable->height)))
						    gimp_progress_update((double)y / drawable->height);

						for (x = 0; x < drawable->width; x++)
						{
							pLine[x*drawable->bpp]   = pData[y*nSizeLine+x*3+2];
							pLine[x*drawable->bpp+1] = pData[y*nSizeLine+x*3+1];
							pLine[x*drawable->bpp+2] = pData[y*nSizeLine+x*3];
						}
						/* copy data to GIMP */
						gimp_pixel_rgn_set_rect(&pixel_rgn, pLine, 0, drawable->height-1-y, drawable->width, 1);
					}
					g_free(pLine);
				}
			}
			else if (8 == nBitsPS)
			{
				int y;
				/* copy line by line */
				for (y = 0; y < drawable->height; y++)
				{
					if ((interactive) && (StepProgress(y,drawable->height)))
						gimp_progress_update((double)y / drawable->height);

					pLine = pData + y*nSizeLine; /* adjust pointer */
			        gimp_pixel_rgn_set_row(&pixel_rgn, pLine, 0, drawable->height-1-y, drawable->width);
				}
			}
			else
			{
				/* copy and shift bits */
				g_message("%d bits per sample not yet supported!", nBitsPS);
			}
			  
		}
		
		gimp_drawable_flush(drawable);
		gimp_drawable_detach(drawable);

		/* Don't miss to display the new image!
		 */
		if (bIsNewImage)
			gimp_display_new (image_ID);
		else
		{
			gimp_layer_set_visible(drawable_ID, TRUE);
			gimp_displays_flush();
		}
	}

	/* done */
	/* clear clipboard? */
	if (NULL != hDIB) GlobalFree(hDIB);

	CloseClipboard();
	
	/* shouldn't this be done by caller?? */
	gimp_image_enable_undo(image_ID);
	return bRet;
} /* CB_PasteImage */

/*
 * Local Variables:
 * tab-width: 4
 * End:
 */
