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
 */

/*
 * Windows platform-specific code
 */

#include "config.h"

#include <libgimp/gimp.h>

#include "tw_platform.h"
#include "tw_func.h"
#include "tw_util.h"
#include "tw_local.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int twainMessageLoop(pTW_SESSION);
int TwainProcessMessage(LPMSG lpMsg, pTW_SESSION twSession);

extern GimpPlugInInfo PLUG_IN_INFO;
extern pTW_SESSION initializeTwain ();
#ifdef _DEBUG
extern void setRunMode(char *argv[]);
#endif


#define APP_NAME "TWAIN"
#define SHOW_WINDOW 0
#define WM_TRANSFER_IMAGE (WM_USER + 100)

/* main bits */
static HWND        hwnd = NULL;
static HINSTANCE   hInst = NULL;

/* Storage for the DLL handle */
static HINSTANCE hDLL = NULL;

/* Storage for the entry point into the DSM */
static DSMENTRYPROC dsmEntryPoint = NULL;


/*
 * callDSM
 *
 * Call the specified function on the data source manager.
 */
TW_UINT16
callDSM(pTW_IDENTITY pOrigin,
	pTW_IDENTITY pDest,
	TW_UINT32    DG,
	TW_UINT16    DAT,
	TW_UINT16    MSG,
	TW_MEMREF    pData)
{
  /* Call the function */
  return (*dsmEntryPoint) (pOrigin, pDest, DG, DAT, MSG, pData);
}

/*
 * twainIsAvailable
 *
 * Return boolean indicating whether TWAIN is available
 */
int
twainIsAvailable(void)
{
  /* Already loaded? */
  if (dsmEntryPoint) {
    return TRUE;
  }

  /* Attempt to load the library */
  hDLL = LoadLibrary(TWAIN_DLL_NAME);
  if (hDLL == NULL)
    return FALSE;

  /* Look up the entry point for use */
  dsmEntryPoint = (DSMENTRYPROC) GetProcAddress(hDLL, "DSM_Entry");
  if (dsmEntryPoint == NULL)
    return FALSE;

  return TRUE;
}

TW_HANDLE
twainAllocHandle (size_t size)
{
  return GlobalAlloc(GHND, size);
}

TW_MEMREF
twainLockHandle (TW_HANDLE handle)
{
  return GlobalLock (handle);
}

void
twainUnlockHandle (TW_HANDLE handle)
{
  GlobalUnlock (handle);
}

void
twainFreeHandle (TW_HANDLE handle)
{
  GlobalFree (handle);
}

gboolean
twainSetupCallback (pTW_SESSION twSession)
{
  /* Callbacks go through the window messaging system */
  return TRUE;
}

/*
 * unloadTwainLibrary
 *
 * Unload the TWAIN dynamic link library
 */
int
unloadTwainLibrary(pTW_SESSION twSession)
{
  /* Explicitly free the SM library */
  if (hDLL) {
    FreeLibrary(hDLL);
    hDLL=NULL;
  }

  /* the data source id will no longer be valid after
   * twain is killed.  If the id is left around the
   * data source can not be found or opened
	 */
  DS_IDENTITY(twSession)->Id = 0;

	/* We are now back at state 1 */
  twSession->twainState = 1;
  LogMessage("Source Manager successfully closed\n");

  return TRUE;
}

/*
 * TwainProcessMessage
 *
 * Returns TRUE if the application should process message as usual.
 * Returns FALSE if the application should skip processing of this message
 */
int
TwainProcessMessage(LPMSG lpMsg, pTW_SESSION twSession)
{
  TW_EVENT   twEvent;

  twSession->twRC = TWRC_NOTDSEVENT;

  /* Only ask Source Manager to process event if there is a Source connected. */
  if (DSM_IS_OPEN(twSession) && DS_IS_OPEN(twSession)) {
		/*
		 * A Source provides a modeless dialog box as its user interface.
		 * The following call relays Windows messages down to the Source's
		 * UI that were intended for its dialog box.  It also retrieves TWAIN
		 * messages sent from the Source to our Application.
		 */
		twEvent.pEvent = (TW_MEMREF) lpMsg;
		twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT,
			(TW_MEMREF) &twEvent);

		/* Check the return code */
		if (twSession->twRC == TWRC_NOTDSEVENT) {
			return FALSE;
		}

		/* Process the message as necessary */
		processTwainMessage(twEvent.TWMessage, twSession);
  }

  /* tell the caller what happened */
  return (twSession->twRC == TWRC_DSEVENT);
}

/*
 * twainMessageLoop
 *
 * Process Win32 window messages and provide special handling
 * of TWAIN specific messages.  This loop will not exit until
 * the application exits.
 */
int
twainMessageLoop(pTW_SESSION twSession)
{
  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0)) {
    if (DS_IS_CLOSED(twSession) || !TwainProcessMessage(&msg, twSession)) {
      TranslateMessage((LPMSG)&msg);
      DispatchMessage(&msg);
    }
  }

  return msg.wParam;
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

void twainQuitApplication ()
{
  PostQuitMessage (0);
}


/******************************************************************
 * Win32 entry point and setup...
 ******************************************************************/

/*
 * WinMain
 *
 * The standard gimp entry point won't quite cut it for
 * this plug-in.  This plug-in requires creation of a
 * standard Win32 window (hidden) in order to receive
 * and process window messages on behalf of the TWAIN
 * datasource.
 */
int APIENTRY
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{

  /*
   * Normally, we would do all of the Windows-ish set up of
   * the window classes and stuff here in WinMain.  But,
   * the only time we really need the window and message
   * queues is during the plug-in run.  So, all of that will
   * be done during run().  This avoids all of the Windows
   * setup stuff for the query().  Stash the instance handle now
   * so it is available from the run() procedure.
   */
  hInst = hInstance;

#ifdef _DEBUG
  /* When in debug version, we allow different run modes...
   * make sure that it is correctly set.
   */
  setRunMode(__argv);
#endif /* _DEBUG */

  /*
   * Now, call gimp_main... This is what the MAIN() macro
   * would usually do.
   */
  return gimp_main(&PLUG_IN_INFO, __argc, __argv);
}

/*
 * main
 *
 * allow to build as console app as well
 */
int main (int argc, char *argv[])
{
#ifdef _DEBUG
  /* When in debug version, we allow different run modes...
   * make sure that it is correctly set.
   */
  setRunMode(__argv);
#endif /* _DEBUG */

  /*
   * Now, call gimp_main... This is what the MAIN() macro
   * would usually do.
   */
  return gimp_main(&PLUG_IN_INFO, __argc, __argv);
}

/*
 * InitApplication
 *
 * Initialize window data and register the window class
 */
BOOL
InitApplication(HINSTANCE hInstance)
{
  WNDCLASS wc;
  BOOL retValue;

  /*
   * Fill in window class structure with parameters to describe
   * the main window.
   */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC) WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszClassName = APP_NAME;
  wc.lpszMenuName = NULL;

  /* Register the window class and stash success/failure code. */
  retValue = RegisterClass(&wc);

  /* Log error */
  if (!retValue)
    LogLastWinError();

  return retValue;
}

/*
 * InitInstance
 *
 * Create the main window for the application.  Used to
 * interface with the TWAIN datasource.
 */
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow, pTW_SESSION twSession)
{
  /* Create our window */
  hwnd = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW,
		      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		      NULL, NULL, hInstance, NULL);

  if (!hwnd) {
    return (FALSE);
  }

  /* Register our window handle with the TWAIN
   * support.
   */
  registerWindowHandle(twSession, hwnd);

  /* Schedule the image transfer by posting a message */
  PostMessage(hwnd, WM_TRANSFER_IMAGE, 0, 0);

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  return TRUE;
}

/*
 * twainWinMain
 *
 * This is the function that represents the code that
 * would normally reside in WinMain (see above).  This
 * function will get called during run() in order to set
 * up the windowing environment necessary for TWAIN to
 * operate.
 */
int
twainMain()
{
  /* Initialize the twain information */
  pTW_SESSION twSession = initializeTwain();

  /* Perform instance initialization */
  if (!InitApplication(hInst))
    return (FALSE);

  /* Perform application initialization */
  if (!InitInstance(hInst, SHOW_WINDOW, twSession))
    return (FALSE);

  /*
   * Call the main message processing loop...
   * This call will not return until the application
   * exits.
   */
  return twainMessageLoop(twSession);
}

/*
 * WndProc
 *
 * Process window message for the main window.
 */
LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {

  case WM_TRANSFER_IMAGE:
    /* Get an image */
    scanImage ();
    break;

  case WM_DESTROY:
    LogMessage("Exiting application\n");
    PostQuitMessage(0);
    break;

  default:
    return (DefWindowProc(hWnd, message, wParam, lParam));
  }
  return 0;
}

