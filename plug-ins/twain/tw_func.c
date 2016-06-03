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
 * Added for Win x64 support, changed data source selection
 * Jens M. Plonka <jens.plonka@gmx.de>
 * 11/25/2011
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 *  (11/25/11)  v0.7   Added Win x64 support, changed data source selection.
 */

#include "config.h"

#include <glib.h>   /* Needed when compiling with gcc */

#include "libgimp/gimp.h"

#include "tw_platform.h"
#include "tw_func.h"
#include "tw_util.h"
#include "tw_local.h"

TW_USERINTERFACE ui;

/*
 * Twain error code to string mappings
 */
static int twainErrorCount = 0;
static char *twainErrors[] =
{
  "No error",
  "Failure due to unknown causes",
  "Not enough memory to perform operation",
  "No Data Source",
  "DS is connected to max possible apps",
  "DS or DSM reported error, application shouldn't",
  "Unknown capability",
  "Unrecognized MSG DG DAT combination",
  "Data parameter out of range",
  "DG DAT MSG out of expected sequence",
  "Unknown destination App/Src in DSM_Entry",
  "Capability not supported by source",
  "Operation not supported by capability",
  "Capability has dependency on other capability",
  "File System operation is denied (file is protected)",
  "Operation failed because file already exists.",
  "File not found",
  "Operation failed because directory is not empty",
  "The feeder is jammed",
  "The feeder detected multiple pages",
  "Error writing the file (disk full?)",
  "The device went offline prior to or during this operation",
  NULL
};

/*
 * twainError
 *
 * Return the TWAIN error message associated
 * with the specified error code.
 */
char *
twainError (int errorCode)
{
  /* Check whether we've counted */
  if (twainErrorCount == 0)
  {
    while (twainErrors[twainErrorCount++]) {}
  }

  /* Check out of bounds */
  if (errorCode >= twainErrorCount)
  {
    return "Unknown TWAIN Error Code";
  }
  else
  {
    return twainErrors[errorCode];
  }
}

/*
 * currentTwainError
 *
 * Return the current TWAIN error message.
 */
char *
currentTwainError (pTW_SESSION twSession)
{
  TW_STATUS twStatus;

  /* Get the current status code from the DSM */
  twSession->twRC = DSM_GET_STATUS(twSession, twStatus);

  /* Return the mapped error code */
  return twainError (twStatus.ConditionCode);
}

/*
 * getImage
 *
 * This is a "high-level" function that can be called in order
 * to take all of the steps necessary to kick off an image-transfer
 * from a user-specified TWAIN datasource. The data will be passed
 * back to the callback function specified in the session structure.
 */
int
getImage (pTW_SESSION twSession)
{
  /* Do some sanity checking first and bail
   * if necessary.
   */

  if (!twainIsAvailable ())
  {
    return FALSE;
  }

  /* One step at a time */
  if (!openDSM (twSession))
  {
    return FALSE;
  }

  if (!selectDS (twSession))
  {
    return FALSE;
  }

  if (!openDS (twSession))
  {
    return FALSE;
  }

  requestImageAcquire (twSession, TRUE);

  return TRUE;
}

/*
 * openDSM
 *
 * Open the data source manager
 */
int
openDSM (pTW_SESSION twSession)
{
  /* Make sure that we aren't already open */
  if (DSM_IS_OPEN(twSession))
  {
    return TRUE;
  }

  /* Open the data source manager */
  twSession->twRC = DSM_OPEN(twSession);

  /* Check the return code */
  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      /* We are now at state 3 */
      twSession->twainState = 3;
      return TRUE;

    case TWRC_FAILURE:
      log_message ("OpenDSM failure: %s\n", currentTwainError(twSession) );
      break;
  }

  return FALSE;
}

/*
 * adjust_selected_data_source
 *
 * Selects the
 */
int
adjust_selected_data_source (pTW_SESSION twSession)
{
  pTW_DATA_SOURCE dataSource;
  pTW_IDENTITY dsIdentity;
  gchar *dsName;

  twSession->dsIdentity = NULL;
  /* get all available data source as a linked list into twSession */
  dataSource = get_available_ds (twSession);

  while (dataSource != NULL)
  {
    dataSource = twSession->dataSources;
    /* for each data source create an own menu item */
    dsIdentity = dataSource->dsIdentity;
    dsName = dsIdentity->ProductName;
    if (strcmp (dsName, twSession->name) == 0)
    {
      twSession->dsIdentity = dataSource->dsIdentity;
      twSession->twRC = TWRC_SUCCESS;
      return TRUE;
    }
  }

  return FALSE;
}

/*
 * selectDS
 *
 * Select a datasource using the TWAIN user
 * interface.
 */
int
selectDS (pTW_SESSION twSession)
{
  /* The datasource manager must be open */
  if (DSM_IS_CLOSED(twSession))
  {
    log_message ("Can't select data source with closed source manager.\n");
    return FALSE;
  }

  if ((strcmp (MID_SELECT, twSession->name) == 0)
      || (adjust_selected_data_source (twSession) == FALSE))
  {
    /* Ask TWAIN to present the source select dialog */
    twSession->twRC = DSM_SELECT_USER(twSession);
  }

  /* Check the return to determine what the user decided
   * to do.
   */
  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      return TRUE;

    case TWRC_FAILURE:
      log_message ("Error selecting datasource: %s\n", currentTwainError(twSession));
      break;
  }

  return FALSE;
}

/*
 * openDS
 *
 * Open a data source using the TWAIN user interface.
 */
int
openDS (pTW_SESSION twSession)
{
  /* The datasource manager must be open */
  if (DSM_IS_CLOSED(twSession))
  {
    log_message ("The DSM needs to be opened first.\n");
    return FALSE;
  }

  /* Is the data source already open? */
  if (DS_IS_OPEN(twSession))
  {
    log_message ("A source has already been opened, please close it first.\n");
    return FALSE;
  }

  /* Open the TWAIN datasource */
  twSession->twRC = DSM_OPEN_DS(twSession);

  /* Check the return to determine what the user decided
   * to do.
   */
  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      /* We are now in TWAIN state 4 */
      twSession->twainState = 4;
      twainSetupCallback (twSession);
      return TRUE;

    case TWRC_FAILURE:
      log_message ("Error opening data source: %s\n", currentTwainError(twSession));
      break;
  }

  return FALSE;
}

/*
 * setBufferedXfer
 */
static int
setBufferedXfer (pTW_SESSION twSession)
{
  TW_CAPABILITY bufXfer;
  pTW_ONEVALUE pvalOneValue;

  /* Make sure the data source is open first */
  if (DS_IS_CLOSED(twSession))
  {
    return FALSE;
  }

  /* Create the capability information */
  bufXfer.Cap = ICAP_XFERMECH;
  bufXfer.ConType = TWON_ONEVALUE;
  bufXfer.hContainer = twainAllocHandle (sizeof(TW_ONEVALUE));
  if (bufXfer.hContainer == NULL)
  {
    log_message ("Error allocating memory for XFer mechanism.\n");
    return FALSE;
  }
  pvalOneValue = (pTW_ONEVALUE) twainLockHandle (bufXfer.hContainer);
  pvalOneValue->ItemType = TWTY_UINT16;
  pvalOneValue->Item = TWSX_MEMORY;
  twainUnlockHandle (bufXfer.hContainer);

  /* Make the call to the source manager */
  twSession->twRC = DSM_XFER_SET(twSession, bufXfer);
  if (twSession->twRC == TWRC_FAILURE)
  {
    log_message ("Could not set capability: %s\n", currentTwainError(twSession));
  }

  /* Free the container */
  twainUnlockHandle (bufXfer.hContainer);
  twainFreeHandle (bufXfer.hContainer);

  /* Let the caller know what happened */
  return (twSession->twRC==TWRC_SUCCESS);
}

/*
 * requestImageAcquire
 *
 * Request that the acquire user interface
 * be displayed.  This may or may not cause
 * an image to actually be transferred.
 */
int
requestImageAcquire (pTW_SESSION twSession, gboolean showUI)
{
  /* Make sure in the correct state */
  if (DS_IS_CLOSED(twSession))
  {
    log_message ("You need to open the data source first.\n");
    return FALSE;
  }

  /* Set the transfer mode */
  if (setBufferedXfer (twSession))
  {
    /* Set the UI information */
    ui.ShowUI = TRUE;
    ui.ModalUI = TRUE;
    /* In Windows, the callbacks are sent to the window message handler */
    ui.hParent = twSession->hwnd;

    /* Make the call to the source manager */
    twSession->twRC = DSM_ENABLE_DS(twSession, ui);

    switch (twSession->twRC)
    {
      case TWRC_SUCCESS:
        /* We are now at a new twain state */
        twSession->twainState = 5;
        return TRUE;

	  case TWRC_FAILURE:
       log_message ("Error enabeling data source: %s\n", currentTwainError(twSession));
	   break;
    }
  }
  return FALSE;
}

/*
 * disableDS
 *
 * Disable the datasource associated with twSession.
 */
void
disableDS(pTW_SESSION twSession)
{
  /* Verify the datasource is enabled */
  if (DS_IS_DISABLED(twSession))
  {
    log_message ("You need to enable the data source first.\n");
    return;
  }

  /* Make the call to the source manager */
  twSession->twRC = DSM_DISABLE_DS(twSession, ui);

  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      /* We are now at a new twain state */
      twSession->twainState = 4;
    break;

    case TWRC_FAILURE:
      log_message  ("Can't disable data source: %s\n", currentTwainError(twSession));
      break;
  }
}

/*
 * closeDS
 *
 * Close the datasource associated with the
 * specified session.
 */
void
closeDS (pTW_SESSION twSession)
{
  /* Can't close a closed data source */
  if (DS_IS_CLOSED(twSession))
  {
    log_message ("closeDS: Data source already closed\n");
    return;
  }

  /* Open the TWAIN datasource */
  twSession->twRC = DSM_CLOSE_DS(twSession);

  /* Check the return to determine what the user decided
   * to do.
   */
  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      /* We are now in TWAIN state 3 */
      twSession->twainState = 3;
      break;

    case TWRC_FAILURE:
      log_message ("Error closing data source: %s\n", currentTwainError(twSession));
      break;
  }
}

/*
 * closeDSM
 *
 * Close the data source manager
 */
void
closeDSM (pTW_SESSION twSession)
{
  if (DSM_IS_CLOSED(twSession))
  {
    log_message ("Data Source Manager not open\n");
  }
  else
  {
    twSession->twRC = DSM_CLOSE(twSession);

    switch (twSession->twRC)
    {
      case TWRC_SUCCESS:
        /* We are now in state 2 */
        twSession->twainState = 2;
        break;

      case TWRC_FAILURE:
        log_message ("Error closing Data Source Manager: %s\n", currentTwainError(twSession));
        break;
    }
  }
}

/*
 * beginImageTransfer
 *
 * Begin an image transfer.
 */
static int
beginImageTransfer (pTW_SESSION twSession, pTW_IMAGEINFO imageInfo)
{
  /* Clear our structures */
  memset (imageInfo, 0, sizeof (TW_IMAGEINFO));

  /* Query the image information */
  twSession->twRC = DSM_GET_IMAGE(twSession, imageInfo);

  /* Check the return code */
  switch (twSession->twRC)
  {
    case TWRC_SUCCESS:
      /* Call the begin transfer callback if registered */
      if (twSession->transferFunctions->txfrBeginCb)
      {
        if (!(*twSession->transferFunctions->txfrBeginCb) (imageInfo, twSession->clientData))
        {
          return FALSE;
        }
      }
      /* We should continue */
      return TRUE;
	case TWRC_FAILURE:
      log_message ("Get Image Info failure - %s\n", currentTwainError(twSession));
      break;
  }
  return FALSE;
}

/*
 * transferImage
 *
 * The Source indicated it is ready to transfer data. It is
 * waiting for the application to inquire about the image details,
 * initiate the actual transfer, and, hence, transition the session
 * from State 6 to 7.  Return the reason for exiting the transfer.
 */
static void
transferImage (pTW_SESSION twSession, pTW_IMAGEINFO imageInfo)
{
  TW_SETUPMEMXFER setupMemXfer;
  TW_IMAGEMEMXFER imageMemXfer;
  char *buffer;

  /* Clear our structures */
  memset (&setupMemXfer, 0, sizeof (TW_SETUPMEMXFER));
  memset (&imageMemXfer, 0, sizeof (TW_IMAGEMEMXFER));

  /* Find out how the source would like to transfer... */
  twSession->twRC = DSM_XFER_START(twSession, setupMemXfer);

  /* Allocate the buffer for the transfer */
  buffer = g_new (char, setupMemXfer.Preferred);
  imageMemXfer.Memory.Flags = TWMF_APPOWNS | TWMF_POINTER;
  imageMemXfer.Memory.Length = setupMemXfer.Preferred;
  imageMemXfer.Memory.TheMem = (TW_MEMREF) buffer;

  /* Get the data */
  do
  {
    /* Setup for the memory transfer */
    imageMemXfer.Compression = TWON_DONTCARE16;
    imageMemXfer.BytesPerRow = TWON_DONTCARE32;
    imageMemXfer.Columns = TWON_DONTCARE32;
    imageMemXfer.Rows = TWON_DONTCARE32;
    imageMemXfer.XOffset = TWON_DONTCARE32;
    imageMemXfer.YOffset = TWON_DONTCARE32;
    imageMemXfer.BytesWritten = TWON_DONTCARE32;

    /* Get the next block of memory */
    twSession->twRC = DSM_XFER_GET(twSession, imageMemXfer);

    if ((twSession->twRC == TWRC_SUCCESS) ||
        (twSession->twRC == TWRC_XFERDONE))
    {
      /* Call dataTransferCallback */
      if (!CB_XFER_DATA(twSession, imageInfo, imageMemXfer))
      {
        /* Callback function requested to cancel */
        twSession->twRC = TWRC_CANCEL;
        break;
      }
    }
  } while (twSession->twRC == TWRC_SUCCESS);

  /* Free the memory buffer */
  g_free (imageMemXfer.Memory.TheMem);
}

/*
 * endPendingTransfer
 *
 * Cancel the currently pending transfer.
 * Return the count of pending transfers.
 */
static int
endPendingTransfer (pTW_SESSION twSession)
{
  TW_PENDINGXFERS pendingXfers;

  twSession->twRC = DSM_XFER_STOP(twSession, pendingXfers);

  if (!pendingXfers.Count)
  {
    twSession->twainState = 5;
  }

  return pendingXfers.Count;
}

/*
 * cancelPendingTransfers
 *
 * Cancel all pending image transfers.
 */
void
cancelPendingTransfers (pTW_SESSION twSession)
{
  TW_PENDINGXFERS pendingXfers;

  twSession->twRC = DSM_XFER_RESET(twSession, pendingXfers);
}

/*
 * endImageTransfer
 *
 * Finish transferring an image.  Return the count
 * of pending images.
 */
static int
endImageTransfer (pTW_SESSION twSession, int *pendingCount)
{
  gboolean continueTransfers;
  int exitCode = twSession->twRC;

  /* Have now exited the transfer for some reason... Figure out
   * why and what to do about it
   */
  switch (twSession->twRC)
  {
    case TWRC_XFERDONE:
    case TWRC_CANCEL:
      *pendingCount = endPendingTransfer (twSession);
      break;

    case TWRC_FAILURE:
      log_message ("Failure received: %s\n", currentTwainError(twSession));
      *pendingCount = endPendingTransfer (twSession);
      break;
  }

  /* Call the end transfer callback */
  CB_XFER_END(twSession, continueTransfers, exitCode, pendingCount);

  return (*pendingCount && continueTransfers);
}

/*
 * transferImages
 *
 * Transfer all of the images that are available from the
 * datasource.
 */
static void
transferImages (pTW_SESSION twSession)
{
  TW_IMAGEINFO imageInfo;
  int pendingCount;

  /* Check the image transfer callback function
   * before even attempting to do the transfer
   */
  if (!twSession || !twSession->transferFunctions || !twSession->transferFunctions->txfrDataCb)
  {
    log_message ("Attempting image transfer without callback function.\n");
    return;
  }

  /*
   * Inform our application that we are getting ready
   * to transfer images.
   */
  if (twSession->transferFunctions->preTxfrCb)
  {
    CB_XFER_PRE(twSession);
  }

  /* Loop through the available images */
  do
  {
    /* Move to the new state */
    twSession->twainState = 6;

    /* Begin the image transfer */
    if (!beginImageTransfer (twSession, &imageInfo))
    {
      continue;
    }

    /* Call the image transfer function */
    transferImage (twSession, &imageInfo);

  } while (endImageTransfer (twSession, &pendingCount));

  /*
   * Inform our application that we are done
   * transferring images.
   */
  CB_XFER_POST(twSession, pendingCount);
}

void
processTwainMessage (TW_UINT16 message, pTW_SESSION twSession)
{
  switch (message)
  {
    case MSG_XFERREADY:
      transferImages (twSession);
      break;

    case MSG_CLOSEDSREQ:
      /* Disable the datasource, Close the Data source
       * and close the data source manager
       */
      disableDS (twSession);
      closeDS (twSession);
      closeDSM (twSession);
      break;

    /* No message from the Source to the App break;
     * possible new message
     */
    case MSG_NULL:
    default:
      break;
  }
}

/**********************************************************************
 * Session related functions
 **********************************************************************/

/*
 * newSession
 *
 * Create a new TWAIN session.
 */
pTW_SESSION
newSession (pTW_IDENTITY appIdentity)
{
  /* Create the structure */
  pTW_SESSION session = g_new (TW_SESSION, 1);

  /* Set the structure fields */
  session->hwnd = 0;
  session->twRC = TWRC_SUCCESS;
  session->appIdentity = appIdentity;
  session->dsIdentity = g_new (TW_IDENTITY, 1);
  session->dsIdentity->Id = 0;
  session->dsIdentity->ProductName[0] = '\0';
  session->transferFunctions = NULL;

  if (twainIsAvailable ())
  {
    session->twainState = 2;
  }
  else
  {
    session->twainState = 0;
  }

  return session;
}

/*
 * get_available_ds
 *
 * Get the list of available data sources.
 */
pTW_DATA_SOURCE
get_available_ds (pTW_SESSION twSession)
{
  pTW_DATA_SOURCE data_source = NULL;
  pTW_IDENTITY dsIdentity;

  if(DSM_IS_CLOSED(twSession))
  {
    log_message ("You need to open the DSM first.\n");
    return NULL;
  }

  if (twSession != NULL)
  {
    dsIdentity = g_new (TW_IDENTITY, 1);
    twSession->twRC = DSM_GET_FIRST_DS(twSession, dsIdentity);

    switch (twSession->twRC)
    {
      case TWRC_SUCCESS:
        data_source = g_new (TW_DATA_SOURCE, 1);
        data_source->dsIdentity = dsIdentity;
        data_source->dsNext = NULL;
        twSession->dataSources = data_source;
        break;

    case TWRC_ENDOFLIST:
        twSession->dataSources = NULL;
        break;

      case TWRC_FAILURE:
        log_message ("Error getting first data source: %s\n", currentTwainError(twSession));
        break;
    }

    /* get the rest of the data sources */
    while (TWRC_SUCCESS == twSession->twRC)
    {
      dsIdentity = g_new (TW_IDENTITY, 1);
      twSession->twRC = DSM_GET_NEXT_DS(twSession, dsIdentity);

      switch (twSession->twRC)
      {
        case TWRC_SUCCESS:
          data_source->dsNext = g_new (TW_DATA_SOURCE, 1);
          data_source = data_source->dsNext;
          data_source->dsIdentity = dsIdentity;
          data_source->dsNext = NULL;
          break;

        case TWRC_ENDOFLIST:
          break;

        case TWRC_FAILURE:
          log_message ("Error getting next data source: %s\n", currentTwainError(twSession));
          break;
      }
    }
  }

  return twSession->dataSources;
}
