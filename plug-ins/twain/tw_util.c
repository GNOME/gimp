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

#include <glib/gstdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tw_util.h"
#ifdef _DEBUG
#include "tw_func.h"
FILE *logFile = NULL;
#endif

#ifdef _DEBUG

/*
 * LogMessage
 */
void
LogMessage(char *format, ...)
{
  va_list args;
  time_t time_of_day;
  char *ctime_string;

  /* Open the log file as necessary */
  if (!logFile)
    logFile = g_fopen(DEBUG_LOGFILE, "w");

  time_of_day = time(NULL);
  ctime_string = ctime(&time_of_day);
  ctime_string[19] = '\0';

  fprintf(logFile, "[%s] ", (ctime_string + 11));
  va_start(args, format);
  vfprintf(logFile, format, args);
  fflush(logFile);
  va_end(args);
}

void
logBegin(pTW_IMAGEINFO imageInfo, void *clientData)
{
  int i;
  char buffer[256];

  LogMessage("\n");
  LogMessage("*************************************\n");
  LogMessage("\n");
  LogMessage("Image transfer begin:\n");
  /*	LogMessage("\tClient data: %s\n", (char *) clientData); */

  /* Log the image information */
  LogMessage("Image information:\n");
  LogMessage("\tXResolution: %f\n", FIX32ToFloat(imageInfo->XResolution));
  LogMessage("\tYResolution: %f\n", FIX32ToFloat(imageInfo->YResolution));
  LogMessage("\tImageWidth: %d\n", imageInfo->ImageWidth);
  LogMessage("\tImageLength: %d\n", imageInfo->ImageLength);
  LogMessage("\tSamplesPerPixel: %d\n", imageInfo->SamplesPerPixel);
  sprintf(buffer, "\tBitsPerSample: {");
  for (i = 0; i < 8; i++) {
    if (imageInfo->BitsPerSample[i])
      strcat(buffer, "1");
    else
      strcat(buffer, "0");

    if (i != 7)
      strcat(buffer, ",");
  }
  LogMessage("%s}\n", buffer);

  LogMessage("\tBitsPerPixel: %d\n", imageInfo->BitsPerPixel);
  LogMessage("\tPlanar: %s\n", (imageInfo->Planar ? "TRUE" : "FALSE"));
  LogMessage("\tPixelType: %d\n", imageInfo->PixelType);
  /* Compression */

}

void
logData(pTW_IMAGEINFO imageInfo,
	pTW_IMAGEMEMXFER imageMemXfer,
	void *clientData)
{
  LogMessage("Image transfer callback called:\n");
  LogMessage("\tClient data: %s\n", (char *) clientData);
  LogMessage("Memory block transferred:\n");
  LogMessage("\tBytesPerRow = %d\n", imageMemXfer->BytesPerRow);
  LogMessage("\tColumns = %d\n", imageMemXfer->Columns);
  LogMessage("\tRows = %d\n", imageMemXfer->Rows);
  LogMessage("\tXOffset = %d\n", imageMemXfer->XOffset);
  LogMessage("\tYOffset = %d\n", imageMemXfer->YOffset);
  LogMessage("\tBytesWritten = %d\n", imageMemXfer->BytesWritten);
}

#else

/*
 * LogMessage
 */
void
LogMessage(char *format, ...)
{
}

#endif /* DEBUG */
