/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
 *
 * Updated for Mac OS X support
 * Brion Vibber <brion@pobox.com>
 * 07/22/2004
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * Based on (at least) the following plug-ins:
 * Screenshot
 * GIF
 * Randomize
 *
 * Any suggestions, bug-reports or patches are welcome.
 *
 * This plug-in interfaces to the TWAIN support library in order
 * to capture images from TWAIN devices directly into GIMP images.
 * The plug-in is capable of acquiring the following type of
 * images:
 * - B/W (1 bit images translated to grayscale B/W)
 * - Grayscale up to 16 bits per pixel
 * - RGB up to 16 bits per sample (24, 30, 36, etc.)
 * - Paletted images (both Gray and RGB)
 *
 * Prerequisites:
 * Should compile and run on both Win32 and Mac OS X 10.3 (possibly
 * also on 10.2).
 *
 * Known problems:
 * - Multiple image transfers will hang the plug-in.  The current
 *   configuration compiles with a maximum of single image transfers.
 * - On Mac OS X, canceling doesn't always close things out fully.
 * - Epson TWAIN driver on Mac OS X crashes the plugin when scanning.
 */

/*
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted
 *                     images.
 *  (07/23/04)  v0.6   Added Mac OS X support.
 */
#include "config.h"

#include <glib.h>		/* Needed when compiling with gcc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tw_platform.h"
#include "tw_local.h"

#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

#include "tw_func.h"
#include "tw_util.h"

#ifdef _DEBUG
#include "tw_dump.h"
#endif /* _DEBUG */

/*
 * Plug-in Definitions
 */
#define PLUG_IN_NAME        "twain-acquire"
#define PLUG_IN_DESCRIPTION N_("Capture an image from a TWAIN datasource")
#define PLUG_IN_HELP        "This plug-in will capture an image from a TWAIN datasource"
#define PLUG_IN_AUTHOR      "Craig Setera (setera@home.com)"
#define PLUG_IN_COPYRIGHT   "Craig Setera"
#define PLUG_IN_VERSION     "v0.6 (07/22/2004)"

#ifdef _DEBUG
#define PLUG_IN_D_NAME      "twain-acquire-dump"
#define PLUG_IN_R_NAME      "twain-acquire-read"
#endif /* _DEBUG */

/*
 * Application definitions
 */
#define MAX_IMAGES 1

/*
 * Definition of the run states
 */
#define RUN_STANDARD 0
#define RUN_DUMP     1
#define RUN_READDUMP 2

/* Global variables */
pTW_SESSION twSession = NULL;

static char *destBuf = NULL;
#ifdef _DEBUG
static int twain_run_mode = RUN_STANDARD;
#endif

/* Forward declarations */
void preTransferCallback   (void             *clientData);
int  beginTransferCallback (pTW_IMAGEINFO     imageInfo,
                            void             *clientData);
int  dataTransferCallback  (pTW_IMAGEINFO     imageInfo,
                            pTW_IMAGEMEMXFER  imageMemXfer,
                            void             *clientData);
int  endTransferCallback   (int               completionState,
                            int               pendingCount,
                            void             *clientData);
void postTransferCallback  (int               pendingCount,
                            void             *clientData);

static void query (void);
static void run   (const gchar      *name,
		   gint              nparams,
		   const GimpParam  *param,
		   gint             *nreturn_vals,
		   GimpParam       **return_vals);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

extern void set_gimp_PLUG_IN_INFO_PTR (GimpPlugInInfo *);

/* Data structure holding data between runs */
/* Currently unused... Eventually may be used
 * to track dialog data.
 */
typedef struct
{
  gchar  sourceName[34];
  gfloat xResolution;
  gfloat yResolution;
  gint   xOffset;
  gint   yOffset;
  gint   width;
  gint   height;
  gint   imageType;
} TwainValues;

/* Default Twain values */
static TwainValues twainvals =
{
  "",
  100.0, 100.0,
  0, 0,
  0, 0,
  TWPT_RGB
};

/* The standard callback functions */
TXFR_CB_FUNCS standardCbFuncs =
{
  preTransferCallback,
  beginTransferCallback,
  dataTransferCallback,
  endTransferCallback,
  postTransferCallback
};

/******************************************************************
 * Dump handling
 ******************************************************************/

#ifdef _DEBUG
/* The dumper callback functions */
TXFR_CB_FUNCS dumperCbFuncs =
{
  dumpPreTransferCallback,
  dumpBeginTransferCallback,
  dumpDataTransferCallback,
  dumpEndTransferCallback,
  dumpPostTransferCallback
};

void
setRunMode (char *argv[])
{
  char *exeName = strrchr (argv[0], '\\') + 1;

  LogMessage ("Executable name: %s\n", exeName);

  if (!_stricmp (exeName, DUMP_NAME))
    twain_run_mode = RUN_DUMP;

  if (!_stricmp (exeName, RUNDUMP_NAME))
    twain_run_mode = RUN_READDUMP;
}
#endif /* _DEBUG */

#ifndef TWAIN_ALTERNATE_MAIN
MAIN ()
#endif

int
scanImage (void)
{
#ifdef _DEBUG
  if (twain_run_mode == RUN_READDUMP)
    return readDumpedImage (twSession);
  else
#endif /* _DEBUG */
    return getImage (twSession);
}

/*
 * initTwainAppIdentity
 *
 * Initialize and return our application's identity for
 * the TWAIN runtime.
 */
static pTW_IDENTITY
getAppIdentity (void)
{
  pTW_IDENTITY appIdentity = g_new (TW_IDENTITY, 1);

  /* Set up the application identity */
  appIdentity->Id = 0;
  appIdentity->Version.MajorNum = 0;
  appIdentity->Version.MinorNum = 1;
  appIdentity->Version.Language = TWLG_USA;
  appIdentity->Version.Country = TWCY_USA;
  strcpy(appIdentity->Version.Info, "GIMP TWAIN 0.6");
  appIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
  appIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;
  appIdentity->SupportedGroups = DG_IMAGE;
  strcpy(appIdentity->Manufacturer, "Craig Setera");
  strcpy(appIdentity->ProductFamily, "GIMP");
  strcpy(appIdentity->ProductName, "GIMP");

  return appIdentity;
}

/*
 * initializeTwain
 *
 * Do the necessary TWAIN initialization.  This sets up
 * our TWAIN session information.  The session stuff is
 * something built by me on top of the standard TWAIN
 * datasource manager calls.
 */
pTW_SESSION
initializeTwain (void)
{
  pTW_IDENTITY appIdentity;

  /* Get our application's identity */
  appIdentity = getAppIdentity ();

  /* Create a new session object */
  twSession = newSession (appIdentity);

  /* Register our image transfer callback functions */
#ifdef _DEBUG
  if (twain_run_mode == RUN_DUMP)
    registerTransferCallbacks (twSession, &dumperCbFuncs, NULL);
  else
#endif /* _DEBUG */
    registerTransferCallbacks (twSession, &standardCbFuncs, NULL);

  return twSession;
}

/******************************************************************
 * GIMP Plug-in entry points
 ******************************************************************/

/*
 * Plug-in Parameter definitions
 */
#define NUMBER_IN_ARGS 1
#define IN_ARGS { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" }
#define NUMBER_OUT_ARGS 2
#define OUT_ARGS \
	{ GIMP_PDB_INT32, "image-count", "Number of acquired images" }, \
	{ GIMP_PDB_INT32ARRAY, "image-ids", "Array of acquired image identifiers" }


/*
 * query
 *
 * The plug-in is being queried.  Install our procedure for
 * acquiring.
 */
static void
query (void)
{
  static const GimpParamDef args[] = { IN_ARGS };
  static const GimpParamDef return_vals[] = { OUT_ARGS };

#ifdef _DEBUG
  if (twain_run_mode == RUN_DUMP)
    {
      /* the installation of the plugin */
      gimp_install_procedure (PLUG_IN_D_NAME,
                              PLUG_IN_DESCRIPTION,
                              PLUG_IN_HELP,
                              PLUG_IN_AUTHOR,
                              PLUG_IN_COPYRIGHT,
                              PLUG_IN_VERSION,
                              "TWAIN (Dump)...",
                              NULL,
                              GIMP_PLUGIN,
                              NUMBER_IN_ARGS,
                              NUMBER_OUT_ARGS,
                              args,
                              return_vals);

      gimp_plugin_menu_register (PLUG_IN_D_NAME, "<Image>/File/Create/Acquire");
    }
  else if (twain_run_mode == RUN_READDUMP)
    {
      /* the installation of the plugin */
      gimp_install_procedure (PLUG_IN_R_NAME,
                              PLUG_IN_DESCRIPTION,
                              PLUG_IN_HELP,
                              PLUG_IN_AUTHOR,
                              PLUG_IN_COPYRIGHT,
                              PLUG_IN_VERSION,
                              "TWAIN (Read)...",
                              NULL,
                              GIMP_PLUGIN,
                              NUMBER_IN_ARGS,
                              NUMBER_OUT_ARGS,
                              args,
                              return_vals);

      gimp_plugin_menu_register (PLUG_IN_R_NAME, "<Image>/File/Create/Acquire");
    }
  else
#endif /* _DEBUG */
    {
      /* the installation of the plugin */
      gimp_install_procedure (PLUG_IN_NAME,
                              PLUG_IN_DESCRIPTION,
                              PLUG_IN_HELP,
                              PLUG_IN_AUTHOR,
                              PLUG_IN_COPYRIGHT,
                              PLUG_IN_VERSION,
                              N_("_Scanner/Camera..."),
                              NULL,
                              GIMP_PLUGIN,
                              NUMBER_IN_ARGS,
                              NUMBER_OUT_ARGS,
                              args,
                              return_vals);

      gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/File/Create/Acquire");
    }
}


/* Return values storage */
static GimpParam values[3];

/*
 * run
 *
 * The plug-in is being requested to run.
 * Capture an image from a TWAIN datasource
 */
static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpRunMode run_mode = param[0].data.d_int32;

  /* Initialize the return values
   * Always return at least the status to the caller.
   */
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  /* Before we get any further, verify that we have
   * TWAIN and that there is actually a datasource
   * to be used in doing the acquire.
   */
  if (! twainIsAvailable ())
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
      return;
    }

  /* Set up the rest of the return parameters */
  values[1].type         = GIMP_PDB_INT32;
  values[1].data.d_int32 = 0;
  values[2].type              = GIMP_PDB_INT32ARRAY;
  values[2].data.d_int32array = g_new (gint32, MAX_IMAGES);

  /* How are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Retrieve values from the last run...
       * Currently ignored
       */
      gimp_get_data (PLUG_IN_NAME, &twainvals);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Currently, we don't do non-interactive calls.
       * Bail if someone tries to call us non-interactively
       */
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Retrieve values from the last run...
       * Currently ignored
       */
      gimp_get_data (PLUG_IN_NAME, &twainvals);
      break;

    default:
      break;
    }

  /* Have we succeeded so far? */
  if (values[0].data.d_status == GIMP_PDB_SUCCESS)
    twainMain ();

  /* Check to make sure we got at least one valid
   * image.
   */
  if (values[1].data.d_int32 > 0)
    {
      /* An image was captured from the TWAIN
       * datasource.  Do final Interactive
       * steps.
       */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* Store variable states for next run */
          gimp_set_data (PLUG_IN_NAME, &twainvals, sizeof (TwainValues));
        }

      /* Set return values */
      *nreturn_vals = 3;
    }
  else
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
}

/***********************************************************************
 * Image transfer callback functions
 ***********************************************************************/

/* Data used to carry data between each of
 * the callback function calls.
 */
typedef struct
{
  gint32        image_id;
  gint32        layer_id;
  GeglBuffer   *buffer;
  const Babl   *format;
  pTW_PALETTE8  paletteData;
  int           totalPixels;
  int           completedPixels;
} ClientDataStruct, *pClientDataStruct;

/*
 * preTransferCallback
 *
 * This callback function is called before any images
 * are transferred.  Set up the one time only stuff.
 */
void
preTransferCallback (void *clientData)
{
  /* Initialize our progress dialog */
  gimp_progress_init (_("Transferring data from scanner/camera"));
}

/*
 * beginTransferCallback
 *
 * The following function is called at the beginning
 * of each image transfer.
 */
int
beginTransferCallback (pTW_IMAGEINFO  imageInfo,
                       void          *clientData)
{
  pClientDataStruct theClientData = g_new (ClientDataStruct, 1);

  const Babl *format;
  int         imageType;
  int         layerType;


#ifdef _DEBUG
  logBegin (imageInfo, clientData);
#endif

  /* Decide on the image type */
  switch (imageInfo->PixelType)
    {
    case TWPT_BW:
    case TWPT_GRAY:
      /* Set up the image and layer types */
      imageType = GIMP_GRAY;
      layerType = GIMP_GRAY_IMAGE;

      format = babl_format ("Y' u8");
      break;

    case TWPT_RGB:
      /* Set up the image and layer types */
      imageType = GIMP_RGB;
      layerType = GIMP_RGB_IMAGE;

      format = babl_format ("R'G'B' u8");
      break;

    case TWPT_PALETTE:
      /* Get the palette data */
      theClientData->paletteData = g_new (TW_PALETTE8, 1);
      twSession->twRC = callDSM (APP_IDENTITY (twSession),
                                 DS_IDENTITY (twSession),
                                 DG_IMAGE, DAT_PALETTE8, MSG_GET,
                                 (TW_MEMREF) theClientData->paletteData);
      if (twSession->twRC != TWRC_SUCCESS)
        return FALSE;

      switch (theClientData->paletteData->PaletteType)
        {
        case TWPA_RGB:
          /* Set up the image and layer types */
          imageType = GIMP_RGB;
          layerType = GIMP_RGB_IMAGE;

          format = babl_format ("R'G'B' u8");
          break;

        case TWPA_GRAY:
          /* Set up the image and layer types */
          imageType = GIMP_GRAY;
          layerType = GIMP_GRAY_IMAGE;

          format = babl_format ("Y' u8");
          break;

        default:
          return FALSE;
        }
      break;

    default:
      /* We don't know how to deal with anything other than
       * the types listed above.  Bail for any other image
       * type.
       */
      return FALSE;
    }

  /* Create the GIMP image */
  theClientData->image_id = gimp_image_new (imageInfo->ImageWidth,
                                            imageInfo->ImageLength,
                                            imageType);

  /* Set the actual resolution */
  gimp_image_set_resolution (theClientData->image_id,
                             FIX32ToFloat (imageInfo->XResolution),
                             FIX32ToFloat (imageInfo->YResolution));
  gimp_image_set_unit (theClientData->image_id, GIMP_UNIT_INCH);

  /* Create a layer */
  theClientData->layer_id = gimp_layer_new (theClientData->image_id,
                                            _("Background"),
                                            imageInfo->ImageWidth,
                                            imageInfo->ImageLength,
                                            layerType, 100,
                                            GIMP_LAYER_MODE_NORMAL);

  /* Add the layer to the image */
  gimp_image_insert_layer (theClientData->image_id,
                           theClientData->layer_id, -1, 0);

  /* Update the progress dialog */
  theClientData->totalPixels     = imageInfo->ImageWidth * imageInfo->ImageLength;
  theClientData->completedPixels = 0;

  gimp_progress_update (0.0);

  theClientData->buffer = gimp_drawable_get_buffer (theClientData->layer_id);
  theClientData->format = format;

  /* Store our client data for the data transfer callbacks */
  if (clientData)
    g_free (clientData);

  setClientData (twSession, (void *) theClientData);

  /* Make sure to return TRUE to continue the image
   * transfer
   */
  return TRUE;
}

/*
 * bitTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Black/White.
 *
 * Black and white data is unpacked from bit data
 * into byte data and written into a gray scale GIMP
 * image.
 */
static char bitMasks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
static int
bitTransferCallback (pTW_IMAGEINFO     imageInfo,
                     pTW_IMAGEMEMXFER  imageMemXfer,
                     void             *clientData)
{
  int   row, col, offset;
  char *srcBuf;
  int   rows = imageMemXfer->Rows;
  int   cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Allocate a buffer as necessary */
  if (! destBuf)
    destBuf = g_new (char, rows * cols);

  /* Unpack the image data from bits into bytes */
  srcBuf = (char *) imageMemXfer->Memory.TheMem;
  offset = 0;
  for (row = 0; row < rows; row++)
    {
      for (col = 0; col < cols; col++)
        {
          char byte = srcBuf[(row * imageMemXfer->BytesPerRow) + (col / 8)];
          destBuf[offset++] = ((byte & bitMasks[col % 8]) != 0) ? 255 : 0;
        }
    }

  /* Update the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * oneBytePerSampleTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Grayscale or RGB.  This transfer
 * mode is quicker than the modes that require translation
 * from a greater number of bits per sample down to the
 * 8 bits per sample understood by GIMP.
 */
static int
oneBytePerSampleTransferCallback (pTW_IMAGEINFO     imageInfo,
                                  pTW_IMAGEMEMXFER  imageMemXfer,
                                  void             *clientData)
{
  int   row;
  char *srcBuf;
  int   bytesPerPixel = imageInfo->BitsPerPixel / 8;
  int   rows = imageMemXfer->Rows;
  int   cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Allocate a buffer as necessary */
  if (! destBuf)
    destBuf = g_new (char, rows * cols * bytesPerPixel);

  /* The bytes coming from the source may not be padded in
   * a way that GIMP is terribly happy with.  It is
   * possible to transfer row by row, but that is particularly
   * expensive in terms of performance.  It is much cheaper
   * to rearrange the data and transfer it in one large chunk.
   * The next chunk of code rearranges the incoming data into
   * a non-padded chunk for GIMP.
   */
  srcBuf = (char *) imageMemXfer->Memory.TheMem;
  for (row = 0; row < rows; row++)
    {
      /* Copy the current row */
      memcpy ((destBuf + (row * bytesPerPixel * cols)),
              (srcBuf + (row * imageMemXfer->BytesPerRow)),
              (bytesPerPixel * cols));
    }

  /* Update the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * twoBytesPerSampleTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Grayscale or RGB.
 */
static int
twoBytesPerSampleTransferCallback (pTW_IMAGEINFO     imageInfo,
                                   pTW_IMAGEMEMXFER  imageMemXfer,
                                   void             *clientData)
{
  static float  ratio = 0.00390625;
  int           row, col, sample;
  char         *destByte;
  int           rows = imageMemXfer->Rows;
  int           cols = imageMemXfer->Columns;
  TW_UINT16    *samplePtr;

  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Allocate a buffer as necessary */
  if (! destBuf)
    destBuf = g_new (char, rows * cols * imageInfo->SamplesPerPixel);

  /* The bytes coming from the source may not be padded in
   * a way that GIMP is terribly happy with.  It is
   * possible to transfer row by row, but that is particularly
   * expensive in terms of performance.  It is much cheaper
   * to rearrange the data and transfer it in one large chunk.
   * The next chunk of code rearranges the incoming data into
   * a non-padded chunk for GIMP.  This function must also
   * reduce from multiple bytes per sample down to single byte
   * per sample.
   */
  /* Work through the rows */
  for (row = 0; row < rows; row++)
    {
      /* The start of this source row */
      samplePtr = (TW_UINT16 *)
        ((char *) imageMemXfer->Memory.TheMem + (row * imageMemXfer->BytesPerRow));

      /* The start of this dest row */
      destByte = destBuf + (row * imageInfo->SamplesPerPixel * cols);

      /* Work through the columns */
      for (col = 0; col < cols; col++)
        {
          /* Finally, work through each of the samples */
          for (sample = 0; sample < imageInfo->SamplesPerPixel; sample++)
            {
              /* Get the value */
              TW_UINT16 value = *samplePtr;

              /* Move the sample pointer */
              samplePtr++;

              /* Place in the destination */
              *destByte = (char) ((float) value * (float) ratio);
              destByte++;
            }
        }
    }

  /* Send the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * palettedTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is paletted.  This does not create
 * an indexed image type in GIMP because for some
 * reason it does not allow creation of a specific
 * palette.  This function will create an RGB or Gray
 * image and use the palette to set the details of
 * the pixels.
 */
static int
palettedTransferCallback (pTW_IMAGEINFO     imageInfo,
                          pTW_IMAGEMEMXFER  imageMemXfer,
                          void             *clientData)
{
  int   channelsPerEntry;
  int   row, col;
  int   rows = imageMemXfer->Rows;
  int   cols = imageMemXfer->Columns;
  char *destPtr = NULL;
  char *srcPtr = NULL;

  /* Get the client data */
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Look up the palette entry size */
  channelsPerEntry =
    (theClientData->paletteData->PaletteType == TWPA_RGB) ? 3 : 1;

  /* Allocate a buffer as necessary */
  if (! destBuf)
    destBuf = g_new (char, rows * cols * channelsPerEntry);

  /* Work through the rows */
  destPtr = destBuf;
  for (row = 0; row < rows; row++)
    {
      srcPtr = (char *) ((char *) imageMemXfer->Memory.TheMem +
                         (row * imageMemXfer->BytesPerRow));

      /* Work through the columns */
      for (col = 0; col < cols; col++)
        {
          /* Get the palette index */
          int index = (unsigned char) *srcPtr;

          srcPtr++;

          switch (theClientData->paletteData->PaletteType)
            {
            case TWPA_GRAY:
              *destPtr = theClientData->paletteData->Colors[index].Channel1;
              destPtr++;
              break;

            case TWPA_RGB:
              *destPtr = theClientData->paletteData->Colors[index].Channel1;
              destPtr++;
              *destPtr = theClientData->paletteData->Colors[index].Channel2;
              destPtr++;
              *destPtr = theClientData->paletteData->Colors[index].Channel3;
              destPtr++;
            }
        }
    }

  /* Send the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * dataTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source.
 */
int
dataTransferCallback (pTW_IMAGEINFO     imageInfo,
                      pTW_IMAGEMEMXFER  imageMemXfer,
                      void             *clientData)
{
#ifdef _DEBUG
  logData (imageInfo, imageMemXfer, clientData);
#endif

  /* Choose the appropriate transfer handler */
  switch (imageInfo->PixelType)
    {
    case TWPT_PALETTE:
      return palettedTransferCallback (imageInfo, imageMemXfer, clientData);

    case TWPT_BW:
      return bitTransferCallback (imageInfo, imageMemXfer, clientData);

    case TWPT_GRAY:
    case TWPT_RGB:
      switch (imageInfo->BitsPerPixel / imageInfo->SamplesPerPixel)
        {
        case 8:
          return oneBytePerSampleTransferCallback (imageInfo, imageMemXfer,
                                                   clientData);

        case 16:
          return twoBytesPerSampleTransferCallback (imageInfo, imageMemXfer,
                                                    clientData);

        default:
          return FALSE;
        }

    default:
      return FALSE;
    }
}

/*
 * endTransferCallback
 *
 * The following function is called at the end of the
 * image transfer.  The caller will be handed
 * the image transfer completion state.  The
 * following values (defined in twain.h) are
 * possible:
 *
 * TWRC_XFERDONE
 *  The transfer completed successfully
 * TWRC_CANCEL
 *  The transfer was completed by the user
 * TWRC_FAILURE
 *  The transfer failed.
 */
int
endTransferCallback (int   completionState,
                     int   pendingCount,
                     void *clientData)
{
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  LogMessage ("endTransferCallback: CompState = %d, pending = %d\n",
              completionState, pendingCount);

  /* Clean up and detach from the drawable */
  if (destBuf)
    {
      g_free (destBuf);
      destBuf = NULL;
    }

  g_object_unref (theClientData->buffer);

  /* Make sure to check our return code */
  if (completionState == TWRC_XFERDONE)
    {
      /* We have a completed image transfer */
      values[2].type = GIMP_PDB_INT32ARRAY;
      values[2].data.d_int32array[values[1].data.d_int32++] =
        theClientData->image_id;

      /* Display the image */
      LogMessage ("Displaying image %d\n", theClientData->image_id);
      gimp_display_new (theClientData->image_id);
    }
  else
    {
      /* The transfer did not complete successfully */
      LogMessage ("Deleting image\n");
      gimp_image_delete (theClientData->image_id);
    }

  /* Shut down if we have received all of the possible images */
  return (values[1].data.d_int32 < MAX_IMAGES);
}

/*
 * postTransferCallback
 *
 * This callback function will be called
 * after all possible images have been
 * transferred.
 */
void
postTransferCallback (int   pendingCount,
                      void *clientData)
{
  /* Shut things down. */
  if (pendingCount != 0)
    cancelPendingTransfers(twSession);

  /* This will close the datasource and datasource
   * manager.  Then the message queue will be shut
   * down and the run() procedure will finally be
   * able to finish.
   */
  disableDS (twSession);
  closeDS (twSession);
  closeDSM (twSession);

  /* Post a message to close up the application */
  twainQuitApplication ();
}
