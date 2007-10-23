/*
 * WinSnap Win32 Window Capture Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 07/14/1999
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
 * Screenshot
 * TWAIN
 *
 * Any suggestions, bug-reports or patches are welcome.
 *
 * This plug-in provides Win32 users with screen snapshot ability.
 *
 */

/*
 * Revision history
 *
 *  (07/13/99)  v0.5   First working version (internal)
 *  (07/14/99)  v0.55  Switched to g_error, g_message, etc.
 *  (07/16/99)  v0.60  Better timing, comment cleanup
 *  (07/16/99)  v0.70  Switched name
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "resource.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"

/*
 * Plug-in Definitions
 */
#define PLUG_IN_NAME        "plug_in_winsnap"
#define HELP_ID             "plug-in-winsnap"
#define PLUG_IN_PRINT_NAME  "WinSnap"
#define PLUG_IN_HELP        "This plug-in will capture an image of a Win32 window or desktop"
#define PLUG_IN_AUTHOR      "Craig Setera (setera@home.com)"
#define PLUG_IN_COPYRIGHT   "Craig Setera"
#define PLUG_IN_VERSION     "v0.70 (07/16/1999)"

/*
 * Application definitions
 */
#define SELECT_FRAME		0
#define SELECT_CLIENT		1
#define SELECT_WINDOW		2

#define SHOW_WINDOW		FALSE
#define APP_NAME		PLUG_IN_NAME
#define WM_DOCAPTURE		(WM_USER + 100)

/* File variables */
static int			captureType;
static char			buffer[512];
static guchar		       *capBytes = NULL;
static HWND			mainHwnd = NULL;
static HINSTANCE		hInst = NULL;
static HCURSOR			selectCursor = 0;
static ICONINFO			iconInfo;

/* Forward declarations */
static void query (void);
static void run   (const gchar      *name,
		   gint              nparams,
		   const GimpParam  *param,
		   gint             *nreturn_vals,
		   GimpParam       **return_vals);

static void sendBMPToGimp(HBITMAP hBMP, HDC hDC, RECT rect);

BOOL CALLBACK dialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/* Data structure holding data between runs */
typedef struct {
  gint root;
  guint delay;
  gint decor;
} WinSnapValues;

/* Default WinSnap values */
static WinSnapValues winsnapvals =
{
  FALSE,
  0,
  TRUE,
};

/* The dialog information */
typedef struct {
#ifdef CAN_SET_DECOR
  GtkWidget *decor_button;
#endif
  GtkWidget *single_button;
  GtkWidget *root_button;
  GtkWidget *delay_spinner;
} WinSnapInterface;

/* The dialog data */
static WinSnapInterface winsnapintf =
{
#ifdef CAN_SET_DECOR
  NULL,
#endif
  NULL,
  NULL,
  NULL
};

/* This plug-in's functions */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* We create a DIB section to hold the grabbed area. The scanlines in
 * DIB sections are aligned ona LONG (four byte) boundary. Its pixel
 * data is in RGB (BGR actually) format, three bytes per pixel.
 *
 * GIMP uses no alignment for its pixel regions. The GIMP image we
 * create is of type RGB, i.e. three bytes per pixel, too. Thus in
 * order to be able to quickly transfer all of the image at a time, we
 * must use a DIB section and pixel region the scanline width in
 * bytes of which is evenly divisible with both 3 and 4. I.e. it must
 * be a multiple of 12 bytes, or in pixels, a multiple of four pixels.
 */

#define ROUND4(width) (((width-1)/4+1)*4)

/******************************************************************
 * Debug stuff
 ******************************************************************/

/*
 * formatWindowsError
 *
 * Format the latest Windows error message into
 * a readable string.  Store in the provided
 * buffer.
 */
static void
formatWindowsError(char *buffer, int buf_size)
{
  LPVOID lpMsgBuf;

  /* Format the message */
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf, 0, NULL );

  /* Copy to the buffer */
  strncpy(buffer, lpMsgBuf, buf_size - 1);

  LocalFree(lpMsgBuf);
}

/******************************************************************
 * Bitmap capture and handling
 ******************************************************************/

/*
 * primDoWindowCapture
 *
 * The primitive window capture functionality.  Accepts
 * the two device contexts and the rectangle to be
 * captured.
 */
static HBITMAP
primDoWindowCapture(HDC hdcWindow, HDC hdcCompat, RECT rect)
{
  HBITMAP	hbmCopy;
  HGDIOBJ	oldObject;
  BITMAPINFO	bmi;

  int width = (rect.right - rect.left);
  int height = (rect.bottom - rect.top);

  /* Create the bitmap info header */
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = ROUND4(width);
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  bmi.bmiHeader.biXPelsPerMeter =
  bmi.bmiHeader.biYPelsPerMeter = 0;
  bmi.bmiHeader.biClrUsed = 0;
  bmi.bmiHeader.biClrImportant = 0;

  /* Create the bitmap storage space */
  hbmCopy = CreateDIBSection(hdcCompat,
			     (BITMAPINFO *) &bmi,
			     DIB_RGB_COLORS,
			     &capBytes, NULL, 0);
  if (!hbmCopy) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error creating DIB section: %s", buffer);
  return NULL;
  }

  /* Select the bitmap into the compatible DC. */
  oldObject = SelectObject(hdcCompat, hbmCopy);
  if (!oldObject) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error selecting object: %s", buffer);
    return NULL;
  }

  /* Copy the data from the application to the bitmap.  Even if we did
   * round up the width, BitBlt only the actual data.
   */
  if (!BitBlt(hdcCompat, 0,0,
	      width, height,
	      hdcWindow, 0,0,
	      SRCCOPY)) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error copying bitmap: %s", buffer);
    return NULL;
  }

  /* Restore the original object */
  SelectObject(hdcCompat, oldObject);

  return hbmCopy;
}

/*
 * doCapture
 *
 * Do the capture.  Accepts the window
 * handle to be captured or the NULL value
 * to specify the root window.
 */
static int
doCapture(HWND selectedHwnd)
{
  HDC		hdcSrc;
  HDC		hdcCompat;
  HRGN		capRegion;
  HWND		oldForeground;
  RECT		rect;
  HBITMAP	hbm;

  /* Try and get everything out of the way before the
   * capture.
   */
  Sleep(500 + winsnapvals.delay * 1000);

  /* Are we capturing a window or the whole screen */
  if (selectedHwnd) {

    /* Set to foreground window */
    oldForeground = GetForegroundWindow();
    SetForegroundWindow(selectedHwnd);
    BringWindowToTop(selectedHwnd);

    Sleep(500);

    /* Build a region for the capture */
    GetWindowRect(selectedHwnd, &rect);
    capRegion = CreateRectRgn(rect.left, rect.top,
			      rect.right, rect.bottom);
    if (!capRegion) {
      formatWindowsError(buffer, sizeof buffer);
      g_error("Error creating region: %s", buffer);
      return FALSE;
    }

    /* Get the device context for the selected
     * window.  Create a memory DC to use for the
     * Bit copy.
     */
    hdcSrc = GetDCEx(selectedHwnd, capRegion,
		     DCX_WINDOW | DCX_PARENTCLIP | DCX_INTERSECTRGN);
  } else {
    /* Get the device context for the whole screen */
    hdcSrc = CreateDC("DISPLAY", NULL, NULL, NULL);

    /* Get the screen's rectangle */
    rect.top = 0;
    rect.bottom = GetDeviceCaps(hdcSrc, VERTRES);
    rect.left = 0;
    rect.right = GetDeviceCaps(hdcSrc, HORZRES);
  }

  if (!hdcSrc) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error getting device context: %s", buffer);
    return FALSE;
  }
  hdcCompat = CreateCompatibleDC(hdcSrc);
  if (!hdcCompat) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error getting compat device context: %s", buffer);
    return FALSE;
  }

  /* Do the window capture */
  hbm = primDoWindowCapture(hdcSrc, hdcCompat, rect);
  if (!hbm)
    return FALSE;

  /* Release the device context */
  ReleaseDC(selectedHwnd, hdcSrc);

  /* Replace the previous foreground window */
  if (selectedHwnd && oldForeground)
    SetForegroundWindow(oldForeground);

  /* Send the bitmap */
  if (hbm != NULL) {
    sendBMPToGimp(hbm, hdcCompat, rect);
  }

  return TRUE;
}

/******************************************************************
 * Win32 entry point and setup...
 ******************************************************************/

#define DINV 3

/*
 * highlightWindowFrame
 *
 * Highlight (or unhighlight) the specified
 * window handle's frame.
 */
static void
highlightWindowFrame(HWND hWnd)
{
  HDC     hdc;
  RECT    rc;

  if (!IsWindow(hWnd))
    return;

  hdc = GetWindowDC(hWnd);
  GetWindowRect(hWnd, &rc);
  OffsetRect(&rc, -rc.left, -rc.top);

  if (!IsRectEmpty(&rc)) {
    PatBlt(hdc, rc.left, rc.top, rc.right-rc.left, DINV, DSTINVERT);
    PatBlt(hdc, rc.left, rc.bottom-DINV, DINV, -(rc.bottom-rc.top-2*DINV),
	   DSTINVERT);
    PatBlt(hdc, rc.right-DINV, rc.top+DINV, DINV, rc.bottom-rc.top-2*DINV,
	   DSTINVERT);
    PatBlt(hdc, rc.right, rc.bottom-DINV, -(rc.right-rc.left), DINV,
	   DSTINVERT);
  }

  ReleaseDC(hWnd, hdc);
  UpdateWindow(hWnd);
}

/*
 * setCaptureType
 *
 * Set the capture type.  Should be one of:
 * SELECT_FRAME
 * SELECT_CLIENT
 * SELECT_WINDOW
 */
void
setCaptureType(int capType)
{
  captureType = capType;
}

/*
 * myWindowFromPoint
 *
 * Map to the appropriate window from the
 * specified point.  The chosen window is
 * based on the current capture type.
 */
static HWND
myWindowFromPoint(POINT pt)
{
  HWND myHwnd;
  HWND nextHwnd;

  switch (captureType) {
  case SELECT_FRAME:
  case SELECT_CLIENT:
    nextHwnd = WindowFromPoint(pt);

    do {
      myHwnd = nextHwnd;
      nextHwnd = GetParent(myHwnd);
    } while (nextHwnd);

    return myHwnd;
    break;

  case SELECT_WINDOW:
    return WindowFromPoint(pt);
    break;
  }

  return WindowFromPoint(pt);
}

/*
 * dialogProc
 *
 * The window procedure for the window
 * selection dialog box.
 */
BOOL CALLBACK
dialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static int	 mouseCaptured;
  static int	 buttonDown;
  static HCURSOR oldCursor;
  static RECT	 bitmapRect;
  static HWND	 highlightedHwnd = NULL;

  switch (msg) {
  case WM_INITDIALOG:
    {
      int nonclientHeight;
      HWND hwndGroup;
      RECT dlgRect;
      RECT clientRect;
      RECT groupRect;
      BITMAP bm;

      /* Set the mouse capture flag */
      buttonDown = 0;
      mouseCaptured = 0;

      /* Calculate the bitmap dimensions */
      GetObject(iconInfo.hbmMask, sizeof(BITMAP), (VOID *)&bm);

      /* Calculate the dialog window dimensions */
      GetWindowRect(hwndDlg, &dlgRect);

      /* Calculate the group box dimensions */
      hwndGroup = GetDlgItem(hwndDlg, IDC_GROUP);
      GetWindowRect(hwndGroup, &groupRect);
      OffsetRect(&groupRect, -dlgRect.left, -dlgRect.top);

      /* The client's rectangle */
      GetClientRect(hwndDlg, &clientRect);

      /* The non-client height */
      nonclientHeight = (dlgRect.bottom - dlgRect.top) -
	(clientRect.bottom - clientRect.top);

      /* Calculate the bitmap rectangle */
      bitmapRect.top = ((groupRect.top + groupRect.bottom) / 2) -
	(bm.bmHeight / 2);
      bitmapRect.top -= nonclientHeight;
      bitmapRect.bottom = bitmapRect.top + bm.bmHeight;
      bitmapRect.left = ((groupRect.left + groupRect.right) / 2) - (bm.bmWidth / 2);
      bitmapRect.right = bitmapRect.left + bm.bmWidth;
    }
  break;

  case WM_LBUTTONDOWN:
    /* Track the button down state */
    buttonDown = 1;
    break;

  case WM_LBUTTONUP:
    buttonDown = 0;

    /* If we have mouse captured
     * we do this stuff.
     */
    if (mouseCaptured) {
      HWND  selectedHwnd;
      POINT cursorPos;

      /* Release the capture */
      mouseCaptured = 0;
      SetCursor(oldCursor);
      ReleaseCapture();

      /* Remove the highlight */
      if (highlightedHwnd)
	highlightWindowFrame(highlightedHwnd);
      RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE);

      /* Return the selected window */
      GetCursorPos(&cursorPos);
      selectedHwnd = myWindowFromPoint(cursorPos);
      EndDialog(hwndDlg, (int) selectedHwnd);
    }
    break;

  case WM_MOUSEMOVE:
    /* If the mouse is captured, show
     * the window which is tracking
     * under the mouse position.
     */
    if (mouseCaptured) {
      HWND  currentHwnd;
      POINT cursorPos;

      /* Get the window */
      GetCursorPos(&cursorPos);
      currentHwnd = myWindowFromPoint(cursorPos);

      /* Do the highlighting */
      if (highlightedHwnd != currentHwnd) {
	if (highlightedHwnd)
	  highlightWindowFrame(highlightedHwnd);
	if (currentHwnd)
	  highlightWindowFrame(currentHwnd);
	highlightedHwnd = currentHwnd;
      }
      /* If the mouse has not been captured,
       * try to figure out if we should capture
       * the mouse.
       */
    } else if (buttonDown) {
      POINT cursorPos;

      /* Get the current client position */
      GetCursorPos(&cursorPos);
      ScreenToClient(hwndDlg, &cursorPos);

      /* Check if within the rectangle formed
       * by the bitmap
       */
      if (PtInRect(&bitmapRect, cursorPos)) {
	mouseCaptured = 1;
	oldCursor = SetCursor(selectCursor);
	SetCapture(hwndDlg);
	RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
      }
    }

    break;

  case WM_PAINT:
    {
      HDC          hDC;
      PAINTSTRUCT  ps;

      /* If the mouse is not captured draw
       * the cursor image
       */
      if (!mouseCaptured) {
	hDC = BeginPaint(hwndDlg, &ps);
	DrawIconEx(hDC, bitmapRect.left, bitmapRect.top, selectCursor,
		   0, 0, 0, NULL, DI_NORMAL);
	EndPaint(hwndDlg, &ps);
      }
    }
  break;

  case WM_COMMAND:
    /* Handle the cancel button */
    switch (LOWORD(wParam)) {
    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      return TRUE;
      break;
    }

  }

  return FALSE;
}

/* Don't use the normal WinMain from gimp.h */
#define WinMain WinMain_no_thanks
MAIN()
#undef WinMain

/*
 * WinMain
 *
 * The standard gimp plug-in WinMain won't quite cut it for
 * this plug-in.
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

  /*
   * Now, call gimp_main... This is what the normal WinMain()
   * would do.
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

  /* Get some resources */
#ifdef _MSC_VER
  /* For some reason this works only with MSVC */
  selectCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SELECT));
#else
  selectCursor = LoadCursor(NULL, IDC_CROSS);
#endif
  GetIconInfo(selectCursor, &iconInfo);

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
  if (!retValue) {
    formatWindowsError(buffer, sizeof buffer);
    g_error("Error registering class: %s", buffer);
    return retValue;
  }

  return retValue;
}

/*
 * InitInstance
 *
 * Create the main window for the application.
 */
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  /* Create our window */
  mainHwnd = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW,
			  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
			  NULL, NULL, hInstance, NULL);

  if (!mainHwnd) {
    return (FALSE);
  }

  ShowWindow(mainHwnd, nCmdShow);
  UpdateWindow(mainHwnd);

  return TRUE;
}

/*
 * winsnapWinMain
 *
 * This is the function that represents the code that
 * would normally reside in WinMain (see above).  This
 * function will get called during run() in order to set
 * up the windowing environment necessary for WinSnap to
 * operate.
 */
int
winsnapWinMain(void)
{
  MSG msg;

  /* Perform instance initialization */
  if (!InitApplication(hInst))
    return (FALSE);

  /* Perform application initialization */
  if (!InitInstance(hInst, SHOW_WINDOW))
    return (FALSE);

  /* Main message loop */
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (msg.wParam);
}

/*
 * WndProc
 *
 * Process window message for the main window.
 */
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HWND selectedHwnd;

  switch (message) {

  case WM_CREATE:
    /* The window is created... Send the capture message */
    PostMessage(hwnd, WM_DOCAPTURE, 0, 0);
    break;

  case WM_DOCAPTURE:
    /* Get the selected window handle */
    selectedHwnd = (HWND) DialogBox(hInst, MAKEINTRESOURCE(IDD_SELECT),
				    hwnd, (DLGPROC) dialogProc);
    if (selectedHwnd)
      doCapture(selectedHwnd);

    PostQuitMessage(0);

    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return (DefWindowProc(hwnd, message, wParam, lParam));
  }

  return 0;
}

/*
 * doRootWindowCapture
 *
 * Capture the root window
 */
static void
doRootWindowCapture(void)
{
  /* Do the window capture */
  doCapture(0);
}

/*
 * doNonRootWindowCapture
 *
 * Capture a selected window.
 */
static void
doWindowCapture(void)
{
  /* Start up a standard Win32
   * message handling loop for
   * selection of the window
   * to be captured
   */
  winsnapWinMain();
}

/******************************************************************
 * Snapshot configuration dialog
 ******************************************************************/


/*
 * snap_toggle_update
 *
 * The radio buttons have been updated.
 */
static void
snap_toggle_update (GtkWidget *widget,
		    gpointer   radio_button)
{
  gint *toggle_val = (gint *) radio_button;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

#ifdef CAN_SET_DECOR
  if (widget == winsnapintf.single_button)
    gtk_widget_set_sensitive (winsnapintf.decor_button, *toggle_val);
#endif /* CAN_SET_DECOR */
}

/*
 * snap_dialog
 *
 * Bring up the GTK dialog for setting snapshot
 * parameters.
 */
static gboolean
snap_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkObject *adj;
  GSList    *radio_group = NULL;
  gint	     radio_pressed[2];
  gint	     decorations;
  gboolean   run;

  /* Set defaults */
  radio_pressed[0] = (winsnapvals.root == FALSE);
  radio_pressed[1] = (winsnapvals.root == TRUE);
  decorations      = winsnapvals.decor;

  /* Init GTK  */
  gimp_ui_init ("winsnap", FALSE);

  /* Main Dialog */
  dialog = gimp_dialog_new (PLUG_IN_PRINT_NAME, "winsnap",
                            NULL, 0,
			    gimp_standard_help_func, HELP_ID,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("Grab"),        GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  winsnapintf.single_button =
    gtk_radio_button_new_with_label (radio_group,
                                     _("Grab a single window"));
  gtk_box_pack_start (GTK_BOX (vbox), winsnapintf.single_button, FALSE, FALSE, 0);

  g_signal_connect (winsnapintf.single_button, "toggled",
		    G_CALLBACK (snap_toggle_update),
                    &radio_pressed[0]);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (winsnapintf.single_button),
				radio_pressed[0]);
  gtk_widget_show (winsnapintf.single_button);

  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (winsnapintf.single_button));

  winsnapintf.root_button =
    gtk_radio_button_new_with_label (radio_group,
                                     _("Grab the whole screen"));
  gtk_box_pack_start (GTK_BOX (vbox), winsnapintf.root_button, FALSE, FALSE, 0);
  g_signal_connect (winsnapintf.root_button, "toggled",
                    G_CALLBACK (snap_toggle_update),
                    &radio_pressed[1]);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (winsnapintf.root_button),
				radio_pressed[1]);
  gtk_widget_show (winsnapintf.root_button);

  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (winsnapintf.root_button));

  /* with delay */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("after"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  winsnapintf.delay_spinner = gimp_spin_button_new (&adj,
                                                    winsnapvals.delay,
                                                    0.0, 100.0,
                                                    1.0, 5.0, 0.0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox),
                      winsnapintf.delay_spinner, FALSE, FALSE, 0);
  gtk_widget_show (winsnapintf.delay_spinner);

  label = gtk_label_new (_("Seconds delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

#ifdef CAN_SET_DECOR
  /* With decorations */
  winsnapintf.decor_button =
    gtk_check_button_new_with_label (_("Include decorations"));
  g_signal_connect (winsnapintf.decor_button, "toggled",
                    G_CALLBACK (snap_toggle_update),
                    &decorations);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (winsnapintf.decor_button),
				decorations);
  gtk_box_pack_end (GTK_BOX (vbox), winsnapintf.decor_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (winsnapintf.decor_button, radio_pressed[0]);
  gtk_widget_show (winsnapintf.decor_button);
#endif /* CAN_SET_DECOR */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      winsnapvals.root  = radio_pressed[1];
      winsnapvals.decor = decorations;
      winsnapvals.delay =
        gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (winsnapintf.delay_spinner));
    }

  gtk_widget_destroy (dialog);

  return run;
}

/******************************************************************
 * GIMP Plug-in entry points
 ******************************************************************/

/*
 * Plug-in Parameter definitions
 */
#define NUMBER_IN_ARGS 3
#define IN_ARGS { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },\
                { GIMP_PDB_INT32,    "root",      "Root window { TRUE, FALSE }" },\
                { GIMP_PDB_INT32,    "decorations", \
									"Include Window Decorations { TRUE, FALSE }" }

#define NUMBER_OUT_ARGS 1
#define OUT_ARGS { GIMP_PDB_IMAGE,   "image",     "Output image" }


/*
 * query
 *
 * The plug-in is being queried.  Install our procedure for
 * capturing windows.
 */
static void
query(void)
{
  static const GimpParamDef args[] = { IN_ARGS };
  static const GimpParamDef return_vals[] = { OUT_ARGS };

  /* the installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          N_("Capture a window or desktop image"),
                          PLUG_IN_HELP,
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("_Screen Shot..."),
                          NULL,
                          GIMP_PLUGIN,
                          NUMBER_IN_ARGS,
                          NUMBER_OUT_ARGS,
                          args,
                          return_vals);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Toolbox>/File/Acquire");
  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/File/Acquire");
}

/* Return values storage */
static GimpParam values[2];

/*
 * run
 *
 * The plug-in is being requested to run.
 * Capture an window image.
 */
static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpRunMode run_mode;

  /* Initialize the return values
   * Always return at least the status to the caller.
   */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
  *nreturn_vals = 1;
  *return_vals = values;

  /* Get the runmode from the in-parameters */
  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /* Set up the rest of the return parameters */
  values[1].type = GIMP_PDB_INT32;
  values[1].data.d_int32 = 0;

  /* Get the data from last run */
  gimp_get_data(PLUG_IN_NAME, &winsnapvals);

  /* How are we running today? */
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    /* Get information from the dialog */
    if (!snap_dialog())
      return;
    break;

  case GIMP_RUN_NONINTERACTIVE:
  case GIMP_RUN_WITH_LAST_VALS:
    if (!winsnapvals.root)
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    break;

  default:
    break;
  } /* switch */

  /* Do the actual window capture */
  if (winsnapvals.root)
    doRootWindowCapture();
  else
    doWindowCapture();

  /* Check to make sure we got at least one valid
   * image.
   */
  if (values[1].data.d_int32 > 0) {
    /* A window was captured.
     * Do final Interactive steps.
     */
    if (run_mode == GIMP_RUN_INTERACTIVE) {
      /* Store variable states for next run */
      gimp_set_data(PLUG_IN_NAME, &winsnapvals, sizeof(WinSnapValues));
    }

    /* Set return values */
    *nreturn_vals = 2;
  } else {
    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
  }
}

/******************************************************************
 * GIMP format and transfer functions
 ******************************************************************/

/*
 * flipRedAndBlueBytes
 *
 * Microsoft has chosen to provide us a very nice (not!)
 * interface for retrieving bitmap bits.  DIBSections have
 * RGB information as BGR instead.  So, we have to swap
 * the silly red and blue bytes before sending to the
 * GIMP.
 */
static void
flipRedAndBlueBytes(int width, int height)
{
  int		i, j;
  guchar	*bufp;
  guchar	temp;

  j = 0;
  while (j < height) {
    i = width;
    bufp = capBytes + j*ROUND4(width)*3;
    while (i--) {
      temp = bufp[2];
      bufp[2] = bufp[0];
      bufp[0] = temp;
      bufp += 3;
    }
    j++;
  }
}

/*
 * sendBMPToGIMP
 *
 * Take the captured data and send it across
 * to GIMP.
 */
static void
sendBMPToGimp(HBITMAP hBMP, HDC hDC, RECT rect)
{
  int		width, height;
  int		imageType, layerType;
  gint32	image_id;
  gint32	layer_id;
  GimpPixelRgn	pixel_rgn;
  GimpDrawable    *drawable;

  /* Our width and height */
  width = (rect.right - rect.left);
  height = (rect.bottom - rect.top);

  /* Check that we got the memory */
  if (!capBytes) {
    g_message (_("No data captured"));
    return;
  }

  /* Flip the red and blue bytes */
  flipRedAndBlueBytes(width, height);

  /* Set up the image and layer types */
  imageType = GIMP_RGB;
  layerType = GIMP_RGB_IMAGE;

  /* Create the GIMP image and layers */
  image_id = gimp_image_new(width, height, imageType);
  layer_id = gimp_layer_new(image_id, _("Background"),
			    ROUND4(width), height,
			    layerType, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer(image_id, layer_id, 0);

  /* Get our drawable */
  drawable = gimp_drawable_get(layer_id);

  gimp_tile_cache_size(ROUND4(width) * gimp_tile_height() * 3);

  /* Initialize a pixel region for writing to the image */
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0,
		      ROUND4(width), height, TRUE, FALSE);

  gimp_pixel_rgn_set_rect(&pixel_rgn, (guchar *) capBytes,
			  0, 0, ROUND4(width), height);

  /* HB: update data BEFORE size change */
  gimp_drawable_flush(drawable);
  /* Now resize the layer down to the correct size if necessary. */
  if (width != ROUND4(width)) {
    gimp_layer_resize (layer_id, width, height, 0, 0);
    gimp_image_resize (image_id, width, height, 0, 0);
  }
  /* Finish up */
  gimp_drawable_detach(drawable);
  gimp_display_new (image_id);

  return;
}
