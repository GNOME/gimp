/*  
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
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
 *  This plug-in will not compile on anything other than a Win32
 *  platform.  Although the TWAIN documentation implies that there
 *  is TWAIN support available on Macintosh, I neither have a 
 *  Macintosh nor the interest in porting this.  If anyone else
 *  has an interest, consult www.twain.org for more information on
 *  interfacing to TWAIN.
 *
 * Known problems:
 * - Multiple image transfers will hang the plug-in.  The current
 *   configuration compiles with a maximum of single image transfers.
 */

/* 
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted 
 *                     images.
 */
#include <glib.h>		/* Needed when compiling with gcc */

#include <stdio.h>
#include <windows.h>
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
    logFile = fopen("c:\\twain.log", "w");

  time_of_day = time(NULL);
  ctime_string = ctime(&time_of_day);
  ctime_string[19] = '\0';

  fprintf(logFile, "[%s] ", (ctime_string + 11));
  va_start(args, format);
  vfprintf(logFile, format, args);
  fflush(logFile);
  va_end(args);
}

/*
 * LogLastWinError
 *
 * Log the last Windows error as returned by
 * GetLastError.
 */
void
LogLastWinError(void) 
{
	LPVOID lpMsgBuf;
	
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	
	LogMessage("%s\n", lpMsgBuf);
	
	/* Free the buffer. */
	LocalFree( lpMsgBuf );
	
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

/*
 * LogLastWinError
 *
 * Log the last Windows error as returned by
 * GetLastError.
 */
void
LogLastWinError(void) 
{
}

#endif /* DEBUG */
