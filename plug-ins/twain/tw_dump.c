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

#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"

#include "tw_dump.h"
#include "tw_func.h"
#include "tw_util.h"

/* File variables */
static FILE *outputFile = NULL;
extern pTW_SESSION twSession;

/*
 * dumpPreTransferCallback
 *
 * This callback function is called before any images
 * are transferred.  Set up the one time only stuff.
 */
void
dumpPreTransferCallback(void *clientData)
{
  /* Open our output file... Not settable... Always
   * write to the root directory.  Simplistic, but
   * gets the job done.
   */
  outputFile = g_fopen(DUMP_FILE, "wb");
}

/*
 * dumpBeginTransferCallback
 *
 * The following function is called at the beginning
 * of each image transfer.
 */
int
dumpBeginTransferCallback(pTW_IMAGEINFO imageInfo, void *clientData)
{
  logBegin(imageInfo, clientData);

  /* Dump the image information */
  fwrite((void *) imageInfo, sizeof(TW_IMAGEINFO), 1, outputFile);

  /* Keep going */
  return TRUE;
}

/*
 * dumpDataTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source.
 */
int
dumpDataTransferCallback(pTW_IMAGEINFO imageInfo,
			 pTW_IMAGEMEMXFER imageMemXfer,
			 void *clientData)
{
  int flag = 1;

  logData(imageInfo, imageMemXfer, clientData);

  /* Write a flag that says that this is a data packet */
  fwrite((void *) &flag, sizeof(int), 1, outputFile);

  /* Dump the memory information */
  fwrite((void *) imageMemXfer, sizeof(TW_IMAGEMEMXFER), 1, outputFile);
  fwrite((void *) imageMemXfer->Memory.TheMem,
	 1, imageMemXfer->BytesWritten, outputFile);

  /* Keep going */
  return TRUE;
}

/*
 * dumpEndTransferCallback
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
dumpEndTransferCallback(int completionState, int pendingCount, void *clientData)
{
  int flag = 0;

  /* Write a flag that says that this is a data packet */
  fwrite((void *) &flag, sizeof(int), 1, outputFile);

  /* Write the necessary data */
  fwrite(&completionState, sizeof(int), 1, outputFile);
  fwrite(&pendingCount, sizeof(int), 1, outputFile);

  /* Only ever transfer a single image */
  return FALSE;
}

/*
 * dumpPostTransferCallback
 *
 * This callback function will be called
 * after all possible images have been
 * transferred.
 */
void
dumpPostTransferCallback(int pendingCount, void *clientData)
{
  char buffer[128];

  /* Shut things down. */
  if (pendingCount != 0)
    cancelPendingTransfers(twSession);

  /* This will close the datasource and datasource
   * manager.  Then the message queue will be shut
   * down and the run() procedure will finally be
   * able to finish.
   */
  disableDS(twSession);
  closeDS(twSession);
  closeDSM(twSession);

  /* Close the dump file */
  fclose(outputFile);

  /* Tell the user */
  sprintf(buffer, "Image dumped to file %s\n", DUMP_FILE);
  gimp_message(buffer);

  /* Post a message to close up the application */
  twainQuitApplication ();
}

/*
 * readDumpedImage
 *
 * Get a previously dumped image.
 */
void readDumpedImage(pTW_SESSION twSession)
{
  int moreData;
  int completionState;
  int pendingCount;

  TW_IMAGEINFO imageInfo;
  TW_IMAGEMEMXFER imageMemXfer;

  /* Open our output file... Not settable... Always
   * write to the root directory.  Simplistic, but
   * gets the job done.
   */
  FILE *inputFile = g_fopen(DUMP_FILE, "rb");

  /*
   * Inform our application that we are getting ready
   * to transfer images.
   */
  (*twSession->transferFunctions->preTxfrCb)(NULL);

  /* Read the image information */
  fread((void *) &imageInfo, sizeof(TW_IMAGEINFO), 1, inputFile);

  /* Call the begin transfer callback */
  if (!(*twSession->transferFunctions->txfrBeginCb)(&imageInfo, twSession->clientData))
    return;

  /* Read all of the data packets */
  fread((void *) &moreData, sizeof(int), 1, inputFile);
  while (moreData) {
    /* Read the memory information */
    fread((void *) &imageMemXfer, sizeof(TW_IMAGEMEMXFER), 1, inputFile);
    imageMemXfer.Memory.TheMem = (TW_MEMREF) g_malloc (imageMemXfer.BytesWritten);
    fread((void *) imageMemXfer.Memory.TheMem,
	  1, imageMemXfer.BytesWritten, inputFile);

    /* Call the data transfer callback function */
    if (!(*twSession->transferFunctions->txfrDataCb)
	(&imageInfo,
	 &imageMemXfer,
	 twSession->clientData))
      return;

    /* Clean up the memory */
    g_free (imageMemXfer.Memory.TheMem);

    /* Check for continuation */
    fread((void *) &moreData, sizeof(int), 1, inputFile);
  }

  /* Grab the final information */
  fread(&completionState, sizeof(int), 1, inputFile);
  fread(&pendingCount, sizeof(int), 1, inputFile);

  if (twSession->transferFunctions->txfrEndCb)
    (*twSession->transferFunctions->txfrEndCb)(completionState, 0,
					       twSession->clientData);

  /* Post a message to close up the application */
  twainQuitApplication ();
}
