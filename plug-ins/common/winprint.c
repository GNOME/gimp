/* Print plug-in for the GIMP on Windows.
 * Copyright 1999 Tor Lillqvist <tml@iki.fi>
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
 */

/*
 * TODO:
 *
 * Own dialog box, with similar options as the Unix print plug-in?
 * (at least those that aren't already covered in the Windows standard
 * page setup and/or print dialogs):
 * - brighness and gamma adjustment
 * - scaling
 * - image placement on paper (at least centering should be offered)
 *
 * Speed up the StretchBlt'ing. Now we StretchBlt one pixel row at
 * a time, and in pieces. Quite ad hoc.
 *
 * Handle the file_print procedure's parameters as much as possible
 * like the print plug-in does. (They are currently cheerfully ignored.)
 *
 * Etc.
 */

#include "config.h"
#include <stdlib.h>
#include <windows.h>

#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

#define NAME_PRINT "file_print"
#define NAME_PAGE_SETUP "file_page_setup"

#define PING() g_message ("%s: %d", __FILE__, __LINE__)

static struct
{
  PRINTDLG prDlg;
  PAGESETUPDLG psDlg;
  int devmodeSize;
} vars =
{
  { 0, },
  { 0, },
  0
};

/* These two functions lifted from the print plug-in and modified
 * to build BGR format pixel rows.
 */

static void
indexed_to_bgr(guchar *indexed,
               guchar *bgrout,
               int     width,
               int     bpp,
               guchar *cmap,
	       int     ncolours)
{
  if (bpp == 1)
    {
      /* No alpha in image. */

      while (width > 0)
	{
	  bgrout[2] = cmap[*indexed * 3 + 0];
	  bgrout[1] = cmap[*indexed * 3 + 1];
	  bgrout[0] = cmap[*indexed * 3 + 2];
	  bgrout += 3;
	  indexed ++;
	  width --;
	}
    }
  else
    {
      /* Indexed alpha image. */

      while (width > 0)
	{
	  bgrout[2] = cmap[indexed[0] * 3 + 0] * indexed[1] / 255 +
	    255 - indexed[1];
	  bgrout[1] = cmap[indexed[0] * 3 + 1] * indexed[1] / 255 +
	    255 - indexed[1];
	  bgrout[0] = cmap[indexed[0] * 3 + 2] * indexed[1] / 255 +
	    255 - indexed[1];
	  bgrout += 3;
	  indexed += bpp;
	  width --;
	}
    }
}

static void
rgb_to_bgr(guchar *rgbin,
           guchar *bgrout,
           int     width,
           int     bpp,
           guchar *cmap,
	   int	   ncolours)

{
  if (bpp == 3)
    {
      /* No alpha in image. */

      while (width > 0)
	{
	  bgrout[2] = rgbin[0];
	  bgrout[1] = rgbin[1];
	  bgrout[0] = rgbin[2];
      rgbin += 3;
      bgrout += 3;
      width --;
	}
    }
  else
    {
      /* RGBA image. */

      while (width > 0)
	{
	  bgrout[2] = rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3];
	  bgrout[1] = rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3];
	  bgrout[0] = rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3];
	  rgbin += bpp;
	  bgrout += 3;
	  width --;
	}
    }
}

/* Respond to a plug-in query. */

static void
query(void)
{
  static GParamDef	print_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" },
    { PARAM_STRING,	"printer",	"Printer" },
    { PARAM_STRING,	"ppd_file",	"PPD file" },
    { PARAM_INT32,	"output_type",	"Output type (0 = gray, 1 = color)" },
    { PARAM_STRING,	"resolution",	"Resolution (\"300\", \"720\", etc.)" },
    { PARAM_STRING,	"media_size",	"Media size (\"Letter\", \"A4\", etc.)" },
    { PARAM_STRING,	"media_type",	"Media type (\"Plain\", \"Glossy\", etc.)" },
    { PARAM_STRING,	"media_source",	"Media source (\"Tray1\", \"Manual\", etc.)" },
    { PARAM_INT32,	"brightness",	"Brightness (0-200%)" },
    { PARAM_FLOAT,	"scaling",	"Output scaling (0-100%, -PPI)" },
    { PARAM_INT32,	"orientation",	"Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { PARAM_INT32,	"left",		"Left offset (points, -1 = centered)" },
    { PARAM_INT32,	"top",		"Top offset (points, -1 = centered)" }
  };
  static int		print_nargs =
    sizeof(print_args) / sizeof(print_args[0]);

  static GParamDef	pagesetup_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" },
  };
  static int		pagesetup_nargs =
    sizeof(pagesetup_args) / sizeof(pagesetup_args[0]);

  INIT_I18N();

  gimp_install_procedure(
      NAME_PRINT,
      _("This plug-in prints images from the GIMP."),
      _("Prints images to any printer recognized by Windows."),
      "Tor Lillqvist <tml@iki.fi>",
      "Copyright 1999 Tor Lillqvist",
      "$Id$",
      N_("<Image>/File/Print"),
      "RGB*,GRAY*,INDEXED*",
      PROC_PLUG_IN,
      print_nargs,
      0,
      print_args,
      NULL);

  gimp_install_procedure(
      NAME_PAGE_SETUP,
      _("This plug-in sets up the page for printing from the GIMP."),
      _("Sets up the page parameters for printing to any Windows printer."),
      "Tor Lillqvist <tml@iki.fi>",
      "Copyright 1999 Tor Lillqvist",
      "$Id$",
      N_("<Image>/File/Page Setup"),
      "RGB*,GRAY*,INDEXED*",
      PROC_PLUG_IN,
      pagesetup_nargs,
      0,
      pagesetup_args,
      NULL);
}

/*
 * 'run()' - Run the plug-in...
 */

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  GDrawable    *drawable;
  GRunModeType	run_mode;
  GStatusType   status = STATUS_SUCCESS;
  GParam       *values;
  GPixelRgn	rgn;
  guchar       *cmap;		/* Colourmap (indexed images only) */
  DEVMODE      *dmp;
  int		ncolours;
  int		width, height;
  int		devWidth, devHeight;
  int		y;
  double	devY, devYstep;
  int		iDevY, iDevYstep;
  int		iDevLeftMargin, iDevTopMargin;
  guchar       *inRow;
  guchar       *bgrRow;
  HGLOBAL	hDevMode, hDevNames;
  DOCINFO	docInfo;
  double	devResX, devResY;
  double	imgResX, imgResY;
  HDC		hdcMem;
  HBITMAP	hBitmap;
  HANDLE	oldBm;
  BITMAPV4HEADER bmHeader;

  INIT_I18N();

  run_mode = param[0].data.d_int32;

  values = g_new(GParam, 1);

  values[0].type = PARAM_STATUS;

  *nreturn_vals = 1;
  *return_vals  = values;

  hDevMode = NULL;
  hDevNames = NULL;

  drawable = gimp_drawable_get(param[2].data.d_drawable);

  width  = drawable->width;
  height = drawable->height;

  if (strcmp (name, NAME_PRINT) == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  if (gimp_get_data_size (NAME_PRINT) > 0)
	    {
	      g_assert (gimp_get_data_size (NAME_PRINT) == sizeof (vars));
	      gimp_get_data (NAME_PRINT, &vars);
	      if (vars.devmodeSize > 0)
		{
		  /* Restore saved DEVMODE. */
		  g_assert (gimp_get_data_size (NAME_PRINT "devmode")
			    == vars.devmodeSize);
		  hDevMode = GlobalAlloc (GMEM_MOVEABLE, vars.devmodeSize);
		  g_assert (hDevMode != NULL);
		  dmp = GlobalLock (hDevMode);
		  g_assert (dmp != NULL);
		  gimp_get_data (NAME_PRINT "devmode", dmp);
		  GlobalUnlock (hDevMode);
		  vars.prDlg.hDevMode = hDevMode;
		}
	      else
		{
		  vars.prDlg.hDevMode = NULL;
		}
	    }
	  else
	    vars.prDlg.Flags = 0;
	  vars.prDlg.hwndOwner = NULL;
	  vars.prDlg.hDevNames = NULL;
	  vars.prDlg.Flags |=
	    PD_RETURNDC | PD_DISABLEPRINTTOFILE | PD_HIDEPRINTTOFILE
	    | PD_NOSELECTION;
	  vars.prDlg.nMinPage = vars.prDlg.nMaxPage = 0;
	  vars.prDlg.nCopies = 1;
	  vars.prDlg.lStructSize = sizeof (PRINTDLG);
	  if (!PrintDlg (&vars.prDlg))
	    {
	      if (CommDlgExtendedError ())
		g_message (_("PrintDlg failed: %d"),
			   CommDlgExtendedError ());
	      status = STATUS_EXECUTION_ERROR;
	      break;
	    }
	  hDevMode = vars.prDlg.hDevMode;
	  hDevNames = vars.prDlg.hDevNames;
	  break;

	case RUN_NONINTERACTIVE:
	  if (nparams >= 3)	/* Printer name? */
	    {
	      
	    }
	  if (nparams >= 5)	/* PPD file */
	    {
	      /* Ignored */
	    }
	  status = STATUS_EXECUTION_ERROR;
	  break;

	default:
	  status = STATUS_CALLING_ERROR;
	  break;
	}

      /*
       * Print the image.
       */

      if (status == STATUS_SUCCESS)
	{
	  /* Check if support for BitBlt */
	  if (!(GetDeviceCaps(vars.prDlg.hDC, RASTERCAPS) & RC_BITBLT)) 
	    {
	      status = STATUS_EXECUTION_ERROR;
	      g_message (_("Printer doesn't support bitmaps"));
	    }
	}

      if (status == STATUS_SUCCESS)
	{
	  /* Set the tile cache size. */
	  
	  if (height > width)
	    gimp_tile_cache_ntiles((drawable->height + gimp_tile_width() - 1) /
				   gimp_tile_width() + 1);
	  else
	    gimp_tile_cache_ntiles((width + gimp_tile_width() - 1) /
				   gimp_tile_width() + 1);
	  
	  /* Is the image indexed?  If so we need the colourmap. */
	  
	  if (gimp_image_base_type(param[1].data.d_image) == INDEXED)
	    cmap = gimp_image_get_cmap(param[1].data.d_image, &ncolours);
	  else
	    {
	      cmap = NULL;
	      ncolours = 0;
	    }
	  
	  /* Start print job. */
	  docInfo.cbSize = sizeof (DOCINFO);
	  docInfo.lpszDocName =
	    gimp_image_get_filename (param[1].data.d_image);
	  docInfo.lpszOutput = NULL;
	  docInfo.lpszDatatype = NULL;
	  docInfo.fwType = 0;

	  if (StartDoc (vars.prDlg.hDC, &docInfo) == SP_ERROR)
	    status = STATUS_EXECUTION_ERROR;
	}

      if (status == STATUS_SUCCESS)
	{
	  /* Prepare printer to accept a page. */
	  if (StartPage (vars.prDlg.hDC) <= 0)
	    {
	      status = STATUS_EXECUTION_ERROR;
	      g_message (_("StartPage failed"));
	      AbortDoc (vars.prDlg.hDC);
	    }
	}
      
      /* Actually print. */

      if (status == STATUS_SUCCESS)
	{
	  gimp_progress_init(_("Printing..."));

	  gimp_pixel_rgn_init(&rgn, drawable, 0, 0,
			      width, height,
			      FALSE, FALSE);

	  inRow = g_malloc (width * drawable->bpp);

	  hdcMem = CreateCompatibleDC (vars.prDlg.hDC);
	  
	  bmHeader.bV4Size = sizeof (BITMAPV4HEADER);
	  bmHeader.bV4Width = width;
	  bmHeader.bV4Height = -1;
	  bmHeader.bV4Planes = 1;
	  bmHeader.bV4BitCount = 24;
	  bmHeader.bV4V4Compression = BI_RGB;
	  bmHeader.bV4SizeImage = 0;
	  bmHeader.bV4XPelsPerMeter = 0;
	  bmHeader.bV4YPelsPerMeter = 0;
	  bmHeader.bV4ClrUsed = 0;
	  bmHeader.bV4ClrImportant = 0;
	  bmHeader.bV4CSType = 0;
	  
	  hBitmap = CreateDIBSection (hdcMem,
				      (BITMAPINFO *) &bmHeader,
				      DIB_RGB_COLORS,
				      (PVOID *) &bgrRow,
				      NULL,
				      0);
	  if (hBitmap == NULL)
	    {
	      status = STATUS_EXECUTION_ERROR;
	      g_message (_("CreateDIBSection failed"));
	      AbortDoc (vars.prDlg.hDC);
	    }
	}

      if (status == STATUS_SUCCESS)
	{
	  void (*pixel_transfer)(guchar *, guchar *, int, int, guchar *, int);

	  if (cmap != NULL)
	    pixel_transfer = indexed_to_bgr;
	  else
	    pixel_transfer = rgb_to_bgr;

	  devResX = GetDeviceCaps(vars.prDlg.hDC, LOGPIXELSX); 
	  devResY = GetDeviceCaps(vars.prDlg.hDC, LOGPIXELSY); 
	  gimp_image_get_resolution (param[1].data.d_image,
				     &imgResX, &imgResY);

	  /* Here we assume that the printer's resolution is many
	   * times higher than the image's. Otherwise we probably
	   * get strange artefacts from StretchBlt'ing a row at
	   * a time, if each image row maps to a little over one
	   * printer row (or even less than one)?
	   */

	  devWidth = (devResX / imgResX) * width;
	  devHeight = (devResY / imgResY) * height;
	  devYstep = (double) devHeight / height;
#if 0
	  g_message ("devWidth = %d, devHeight = %d, devYstep = %g",
		     devWidth, devHeight, devYstep);
#endif
	  if (!SetStretchBltMode (vars.prDlg.hDC, HALFTONE))
	    g_message (_("SetStretchBltMode failed (warning only)"));

	  oldBm = SelectObject (hdcMem, hBitmap);
	  
	  if (vars.psDlg.Flags & PSD_MARGINS)
	    if (vars.psDlg.Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
	      {
		/* Hundreths of millimeters */
		iDevLeftMargin = vars.psDlg.rtMargin.left / 2540.0 * devResX;
		iDevTopMargin = vars.psDlg.rtMargin.top / 2540.0 * devResY;
	      }
	    else
	      {
		/* Thousandths of inches */
		iDevLeftMargin = vars.psDlg.rtMargin.left / 1000.0 * devResX;
		iDevTopMargin = vars.psDlg.rtMargin.top / 1000.0 * devResY;
	      }
	  else
	    iDevLeftMargin = iDevTopMargin = 0;

	  devY = 0.0;
	  iDevY = 0;
	  for (y = 0; y < height; y++)
	    {
	      if ((y & 0x0F) == 0)
		gimp_progress_update ((double)y / (double)drawable->height);

	      gimp_pixel_rgn_get_row (&rgn, inRow, 0, y, width);

	      (* pixel_transfer) (inRow, bgrRow, width, drawable->bpp, cmap, ncolours);
	      iDevYstep = ((int) (devY + devYstep)) - iDevY;
	      if (iDevYstep > 0)
		{
		  /* StretchBlt seems to fail if we do it in "too large"
		   * pieces at a time, a least with my printer.
		   * Arbitrarily limit to 4000 destination columns
		   * at a time...
		   */
		  int x, iDevX, xstep, iDevXstep;

		  iDevX = 0;
		  x = 0;
		  iDevXstep = 4000;
		  xstep = (imgResX / devResX ) * iDevXstep;
		  while (x <= width)
		    {
		      int w, devW;

		      if (x + xstep > width)
			w = width - x;
		      else
			w = xstep;
		      if (iDevX + iDevXstep > devWidth)
			devW = devWidth - iDevX;
		      else
			devW = iDevXstep;
		      if (!StretchBlt (vars.prDlg.hDC,
				       iDevX + iDevLeftMargin,
				       iDevY + iDevTopMargin,
				       devW, iDevYstep,
				       hdcMem, x, 0, w, 1, SRCCOPY))
			{
			  status = STATUS_EXECUTION_ERROR;
			  g_message (_("StretchBlt (hDC, %d, %d, "
				       "%d, %d, "
				       "hdcMem, %d, 0, %d, 1, SRCCOPY) "
				       "failed, error = %d, y = %d"),
				     iDevX, iDevY,
				     devW, iDevYstep,
				     x, w,
				     GetLastError (), y);
			  AbortDoc (vars.prDlg.hDC);
			  break;
			}
		      x += xstep;
		      iDevX += iDevXstep;
		    }
		}
	      devY += devYstep;
	      iDevY += iDevYstep;
	    }

	  SelectObject (hdcMem, oldBm);
	  DeleteObject (hBitmap);
	  gimp_progress_update (1.0);
	}
      
      if (status == STATUS_SUCCESS)
	{
	  if (EndPage (vars.prDlg.hDC) <= 0)
	    {
	      status = STATUS_EXECUTION_ERROR;
	      g_message (_("EndPage failed"));
	      EndDoc (vars.prDlg.hDC);
	    }
	}

      if (status == STATUS_SUCCESS)
	{
	  EndDoc (vars.prDlg.hDC);
	}

      DeleteDC (vars.prDlg.hDC);
    }
  else if (strcmp (name, NAME_PAGE_SETUP) == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  if (gimp_get_data_size (NAME_PRINT) > 0)
	    {
	      g_assert (gimp_get_data_size (NAME_PRINT) == sizeof (vars));
	      gimp_get_data (NAME_PRINT, &vars);
	      if (vars.devmodeSize > 0)
		{
		  /* Restore saved DEVMODE. */
		  g_assert (gimp_get_data_size (NAME_PRINT "devmode")
			    == vars.devmodeSize);
		  hDevMode = GlobalAlloc (GMEM_MOVEABLE, vars.devmodeSize);
		  g_assert (hDevMode != NULL);
		  dmp = GlobalLock (hDevMode);
		  g_assert (dmp != NULL);
		  gimp_get_data (NAME_PRINT "devmode", dmp);
		  GlobalUnlock (hDevMode);
		  vars.psDlg.hDevMode = hDevMode;
		}
	      else
		{
		  vars.psDlg.hDevMode = NULL;
		}
	    }
	  else
	    vars.psDlg.Flags = 0;
	  vars.psDlg.hwndOwner = NULL;
	  vars.psDlg.hDevNames = NULL;
	  vars.psDlg.lStructSize = sizeof (PAGESETUPDLG);
	  if (!PageSetupDlg (&vars.psDlg))
	    {
	      if (CommDlgExtendedError ())
		g_message (_("PageSetupDlg failed: %d"),
			   CommDlgExtendedError ());
	      status = STATUS_EXECUTION_ERROR;
	      break;
	    }
	  vars.psDlg.Flags |= PSD_MARGINS;
	  hDevMode = vars.psDlg.hDevMode;
	  hDevNames = vars.psDlg.hDevNames;
	  break;

	default:
	  status = STATUS_CALLING_ERROR;
	  break;
	}
    }
  else
    status = STATUS_CALLING_ERROR;


  /* Store data. */
  if (status == STATUS_SUCCESS && run_mode == RUN_INTERACTIVE)
    {
      /* Save DEVMODE */
      dmp = GlobalLock (hDevMode);
      vars.devmodeSize = dmp->dmSize + dmp->dmDriverExtra;
#if 0
      g_message("vars.devmodeSize = %d, DeviceName = %.*s, Orientation = %s, PaperSize = %s, "
		"Scale = %d%%, Copies = %d, PrintQuality = %s, "
		"%s, ICMMethod = %s, ICMIntent = %s, MediaType = %s, "
		"DitherType = %s",
		vars.devmodeSize,
		CCHDEVICENAME, dmp->dmDeviceName,
		(dmp->dmOrientation == DMORIENT_PORTRAIT ? "PORTRAIT" :
		 (dmp->dmOrientation == DMORIENT_LANDSCAPE ? "LANDSCAPE" :
		  "?")),
		(dmp->dmPaperSize == DMPAPER_LETTER ? "LETTER" :
		 (dmp->dmPaperSize == DMPAPER_LEGAL ? "LEGAL" :
		  (dmp->dmPaperSize == DMPAPER_A3 ? "A3" :
		   (dmp->dmPaperSize == DMPAPER_A4 ? "A4" :
		    "?"))))
		,
		dmp->dmScale, dmp->dmCopies,
		(dmp->dmPrintQuality == DMRES_HIGH ? "HIGH" :
		 (dmp->dmPrintQuality == DMRES_MEDIUM ? "MEDIUM" :
		  (dmp->dmPrintQuality == DMRES_LOW ? "LOW" :
		   (dmp->dmPrintQuality == DMRES_DRAFT ? "DRAFT" :
		    "?")))),
		(dmp->dmColor == DMCOLOR_COLOR ? "COLOR" :
		 (dmp->dmColor == DMCOLOR_MONOCHROME ? "MONOCHROME" :
		  "?")),
		(dmp->dmICMMethod == DMICMMETHOD_NONE ? "NONE" :
		 (dmp->dmICMMethod == DMICMMETHOD_SYSTEM ? "SYSTEM" :
		  (dmp->dmICMMethod == DMICMMETHOD_DRIVER ? "DRIVER" :
		   (dmp->dmICMMethod == DMICMMETHOD_DEVICE ? "DEVICE" :
		    "?")))),
		(dmp->dmICMIntent == DMICM_CONTRAST ? "CONTRAST" :
		 (dmp->dmICMIntent == DMICM_SATURATE ? "SATURATE" :
		  "?")),
		(dmp->dmMediaType == DMMEDIA_STANDARD ? "STANDARD" :
		 (dmp->dmMediaType == DMMEDIA_GLOSSY ? "GLOSSY" :
		  (dmp->dmMediaType == DMMEDIA_TRANSPARENCY ? "TRANSPARENCY" :
		   "?"))),
		(dmp->dmDitherType == DMDITHER_NONE ? "NONE" :
		 (dmp->dmDitherType == DMDITHER_COARSE ? "COARSE" :
		  (dmp->dmDitherType == DMDITHER_FINE ? "FINE" :
		   (dmp->dmDitherType == DMDITHER_LINEART ? "LINEART" :
		    (dmp->dmDitherType == DMDITHER_ERRORDIFFUSION ? "ERRORDIFFUSION" :
		     (dmp->dmDitherType == DMDITHER_GRAYSCALE ? "GRAYSCALE" :
		     
		      "?")))))));
#endif
      gimp_set_data (NAME_PRINT "devmode", dmp, vars.devmodeSize);
      GlobalUnlock (hDevMode);
      gimp_set_data (NAME_PRINT, &vars, sizeof(vars));
    }
  
  if (hDevMode != NULL)
    GlobalFree (hDevMode);
  if (hDevNames != NULL)
    GlobalFree (hDevNames);

  values[0].data.d_status = status;
  gimp_drawable_detach(drawable);
}

GPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN()
