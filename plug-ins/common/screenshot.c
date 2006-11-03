/*
 *  ScreenShot plug-in
 *  Copyright 1998-2000 Sven Neumann <sven@gimp.org>
 *  Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 *
 */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WIN32)
#include <windows.h>
#endif

#include "libgimp/stdplugins-intl.h"


/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (screenshot_icon)
#endif
#ifdef __GNUC__
static const guint8 screenshot_icon[] __attribute__ ((__aligned__ (4))) =
#else
static const guint8 screenshot_icon[] =
#endif
{
  ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (2735) */
  "\0\0\12\307"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (128) */
  "\0\0\0\200"
  /* width (32) */
  "\0\0\0\40"
  /* height (32) */
  "\0\0\0\40"
  /* pixel_data: */
  "\326\0\0\0\0\1eee\13\230\0\0\0\0\1\15\15\15z\207\0\0\0\377\1\14\14\14"
  "W\227\0\0\0\0\11\0\0\0\377\306\306\306\354\364\364\364\377\372\372\372"
  "\377\375\375\375\377\372\372\372\377\357\357\357\377\257\257\257\310"
  "\0\0\0\377\226\0\0\0\0\12]]]\14\0\0\0\377\304\304\304\377\354\354\354"
  "\377\367\367\367\377\372\372\372\377\366\366\366\377\352\352\352\377"
  "\257\257\257\354\0\0\0\377\226\0\0\0\0\4QQQ+\0\0\0\377}}}\374\177\177"
  "\177\366\204\200\200\200\366\2xxx\377\0\0\0\377\226\0\0\0\0\4\35\35\35"
  "[\0\0\0\377333\343AAA\206\202PPPL\5KKKYBBB\211555\377\0\0\0\377'''\30"
  "\225\0\0\0\0\4\6\6\6\226\0\0\0\377\35\35\35\324GGGc\203QQQU\4""777p\35"
  "\35\35\377\0\0\0\377\6\6\6K\224\0\0\0\0\5\0\0\0\33\7\7\7\317\0\0\0\377"
  "ZZZ\377eee\377\203fff\377\4ddd\377EEE\377\0\0\0\377\0\0\0\201\215\0\0"
  "\0\0\6\0\0\0\4\0\0\0\20\0\0\0\30\2\2\1""4\0\0\0B\0\0\0S\202\0\0\0\377"
  "\3''&\264ZYY\365uuu\377\205\217\217\217\377\3sss\377HHH\357\17\17\17"
  "\330\202\0\0\0\377\3\0\0\0\233\37\37\37B\0\0\0\16\205\0\0\0\0\2\0\0\0"
  "\14\0\0\0u\206\0\0\0\377\6XXX\354LLL\354OOO\345ppp\343\233\233\233\377"
  "\252\252\252\377\205\265\265\265\377\10\262\262\262\377\221\221\221\377"
  "PPP\372LLL\361===\353\36\36\36\343\37\37\37\330\0\0\0""0\203\0\0\0\0"
  "\1\0\0\0\217\203\0\0\0\377\15kjh\377dca\377gec\377jig\377srp\377\243"
  "\243\243\377eee\377\345\345\345\377\340\340\340\377|||\377\255\255\255"
  "\377\213\213\213\377\212\212\212\377\204\204\204\204\377\10\206\206\206"
  "\377\232\232\232\377xxx\377ooo\377|||\377sss\377HHH\367\27\27\27\243"
  "\203\0\0\0\0\35\0\0\0\377cb^\377oom\377|{y\377a`^\377YYY\377222\377U"
  "TS\377vur\377\217\217\217\377ZZZ\377\346\346\346\377\336\336\336\377"
  "\203\203\203\377\242\242\242\377\213\213\213\377\220\217\217\377\201"
  "\200\200\377zzx\377mlk\377rqp\377vvu\377\204\204\204\377\213\213\213"
  "\377lll\377xxx\377\177\177\177\377ppp\377\0\0\0\377\203\0\0\0\0\35\0"
  "\0\0\377\261\257\254\377\206\205\202\377\201\200|\377onk\377onl\377N"
  "ML\377\200\200~\377feb\377@@@\377bbb\377RRR\377fff\377\262\262\262\377"
  "\221\221\221\377\221\220\216\377rrp\377vwu\377iji\377QSR\377XYX\377c"
  "eb\377_`]\377\202\202\201\377rrr\377iii\377bbb\377nnn\377\0\0\0\377\203"
  "\0\0\0\0\23\0\0\0\377\210\207\201\377\265\264\262\377\262\262\260\377"
  "\302\301\277\377\307\307\306\377\267\267\266\377`_]\377\"\"\40\377JJ"
  "J\377sss\377iii\377www\377\264\263\263\377\200\200\177\377hkh\377[_]"
  "\377_bb\377]aa\377\202RVV\377\5JNK\377>B=\377QSO\377hig\377jjj\377\202"
  "\177\177\177\377\1\0\0\0\377\203\0\0\0\0\35\0\0\0\377\202\200w\377\177"
  "}t\377tqh\377ljb\377a^W\377WUO\377\77=9\377$#\40\377LLL\377uuu\377pp"
  "p\377\236\236\236\377\222\223\222\377\\__\377bfe\377JNP\377BHI\377CH"
  "J\377BGJ\377>CE\3776::\377/3.\377/2,\377:=8\377jih\377}}}\377\200\200"
  "\200\377\0\0\0\377\203\0\0\0\0\35\0\0\0\377\202\177w\377\200}u\377tq"
  "h\377mjc\377a_X\377WUO\377@\77;\377.,*\377KKK\377mmm\377yyy\377\224\223"
  "\222\377aee\377X]]\377\77DD\377VYW\377\\_^\377MQS\377GLM\377TXZ\377Q"
  "TV\377;@A\377,/*\377**#\377SRO\377zzz\377\177\177\177\377\0\0\0\377\203"
  "\0\0\0\0\12\0\0\0\377\201~u\377\177|t\377sph\377mjb\377a_X\377WUO\377"
  "@>9\377/.+\377KKK\377\202ttt\377\21{}}\377QVW\377DHG\377`bb\377599\377"
  "\20\21\21\377\6\6\6\377\11\11\11\377\25\27\27\377DFG\377SVW\3777<>\377"
  "+,(\377860\377poo\377zzz\377\0\0\0\377\203\0\0\0\0\23\0\0\0\377\200}"
  "u\377\177|t\377spg\377ljb\377a_X\377WTN\377@>9\377/.+\377KKK\377ttt\377"
  "bdd\377rvw\377\77DD\377\\_^\3778;9\377444\377FFF\377111\377\202\37\37"
  "\37\377\10\35\36\36\3778<<\377DIL\377154\377*)\"\377nmk\377\200\200\200"
  "\377\0\0\0\377\203\0\0\0\0>\0\0\0\377\177}u\377~{s\377sph\377ljb\377"
  "`^W\377UTM\377@=9\377..*\377III\377nnn\377_`a\377@FG\377HLL\377LPQ\377"
  ",-+\377\251\251\251\377\200\200\200\377\242\242\242\377OOO\377\77\77"
  "\77\377111\377)++\377HMP\377BIK\377\34\34\27\377NMK\377qqq\377\0\0\0"
  "\377\0\0\0\2\0\0\0\1\0\0\0\0\0\0\0\377\177}t\377~{s\377rog\377lia\377"
  "`^W\377USM\377\77=8\377..*\377III\377lll\377ghh\3779>\77\377SWV\377F"
  "JG\377$$#\377lll\377\242\242\242\377\363\363\363\377\221\221\221\377"
  "ddd\377DDD\377111\377GLO\377HNQ\377\35\35\32\377QPN\377lll\377\0\0\0"
  "\377\0\0\0\6\202\0\0\0\3\36\0\0\0\377\177|t\377}zs\377rog\377kha\377"
  "`]W\377USM\377>=8\377.-*\377HHH\377rrr\377dee\3777;<\377LRS\377:>:\377"
  "\10\10\7\377CCC\377yyy\377\233\233\233\377\242\242\242\377{{{\377TTT"
  "\377<<<\377INQ\377HMQ\377$%#\377mmk\377ttt\377\0\0\0\377\0\0\0\21\202"
  "\0\0\0\10""3\0\0\0\377~{s\377}zr\377qog\377kha\377_]V\377TRL\377><8\377"
  ".-*\377JJJ\377uuu\377iij\3777;<\3779AC\377387\377\21\22\20\377\37\37"
  "\37\377\77\77\77\377ddd\377{{{\377ppp\377\223\223\223\377@AA\377CJM\377"
  "\77FI\377-/.\377ssr\377ppp\377\0\0\0\377\0\0\0\31\0\0\0\14\0\0\0\16\0"
  "\0\0\377~{t\377}zr\377qnf\377jh`\377^\\V\377SQL\377><7\377.-*\377HHH"
  "\377www\377non\377/12\3776>>\377;CF\377%)&\377\33\34\33\377111\377DD"
  "D\377\202TTT\377\23FFG\377MOR\377;BG\377058\377DEE\377uuu\377NNN\374"
  "\21\21\21\302\0\0\0\34\0\0\0\15\0\0\0\17\0\0\0\264NLJ\377{xq\377ome\377"
  "jg`\377`]V\377TQL\377973\377\204\0\0\0\377\14\22\23\23\3774:;\377;CG"
  "\377>DC\377*.-\377'((\377111\377;;;\377<>>\377FJM\377FLO\377=BG\377\203"
  "\0\0\0\377\7\10\10\10\273\1\1\1l\0\0\0\26\0\0\0\11\0\0\0\12\0\0\0m\0"
  "\0\0\256\206\0\0\0\377\40\3\3\2\301\0\0\0\236\0\0\0n\0\0\0t\1\1\1\204"
  "\17\20\21\354\0\0\0\377Z]^\377RVW\377<AC\3779>B\377\77EH\377HLO\377["
  "]_\377UY[\377\0\0\0\377\14\15\16\266\0\0\0_\0\0\0I\0\0\0-\0\0\0\32\0"
  "\0\0\13\0\0\0\4\0\0\0\5\0\0\0\14\0\0\0#\0\0\0;\0\0\0P\0\0\0\\\3\3\3f"
  "\11\10\10[\4\4\4N\202\0\0\0B\24\0\0\0E\0\0\0O\0\0\0[\4\5\5\212\20\22"
  "\24\321\0\0\0\377INR\377BIM\377BHL\377Z^`\377KOR\377NRU\377\0\0\0\377"
  "\17\20\21\265\5\5\6c\0\0\0@\0\0\0-\0\0\0\31\0\0\0\15\0\0\0\4\202\0\0"
  "\0\1\4\0\0\0\3\0\0\0\6\0\0\0\11\0\0\0\13\202\0\0\0\15\13\0\0\0\17\0\0"
  "\0\22\0\0\0\24\0\0\0\26\0\0\0\32\0\0\0&\0\0\0""0\0\0\0<\1\1\2M\11\12"
  "\13\232\34\37!\320\204\0\0\0\377\10\16\17\17\300\14\14\14w\0\0\0<\0\0"
  "\0.\0\0\0\35\0\0\0\21\0\0\0\7\0\0\0\3\204\0\0\0\0\6\0\0\0\1\0\0\0\2\0"
  "\0\0\1\0\0\0\2\0\0\0\1\0\0\0\2\202\0\0\0\3\23\0\0\0\5\0\0\0\6\0\0\0\17"
  "\0\0\0\26\0\0\0\37\0\0\0#\0\0\0-\4\4\5@\3\3\3[\0\0\0p\2\3\3d\7\10\10"
  "O\7\10\10""7\0\0\0&\0\0\0\35\0\0\0\24\0\0\0\12\0\0\0\5\0\0\0\1\217\0"
  "\0\0\0\6\0\0\0\3\0\0\0\5\0\0\0\11\0\0\0\14\0\0\0\20\0\0\0\23\202\0\0"
  "\0\25\7\0\0\0\23\0\0\0\21\0\0\0\15\0\0\0\13\0\0\0\7\0\0\0\4\0\0\0\2\222"
  "\0\0\0\0\5\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\5\0\0\0\7\202\0\0\0\10\5\0\0"
  "\0\6\0\0\0\5\0\0\0\3\0\0\0\2\0\0\0\1\207\0\0\0\0"
};


/* Defines */
#define PLUG_IN_NAME "plug_in_screenshot"
#define HELP_ID      "plug-in-screenshot"

#ifdef __GNUC__
#ifdef GDK_NATIVE_WINDOW_POINTER
#if GLIB_SIZEOF_VOID_P != 4
#warning window_id does not fit in PDB_INT32
#endif
#endif
#endif

typedef struct
{
  gboolean         root;
  guint            window_id;
  guint            select_delay;
  guint            grab_delay;
} ScreenShotValues;

static ScreenShotValues shootvals =
{
  FALSE,     /* root window  */
  0,         /* window ID    */
  0,         /* select delay */
  0,         /* grab delay   */
};


static void      query (void);
static void      run   (const gchar      *name,
			gint              nparams,
			const GimpParam  *param,
			gint             *nreturn_vals,
			GimpParam       **return_vals);

static GdkNativeWindow select_window  (GdkScreen       *screen);
static gint32          create_image   (const GdkPixbuf *pixbuf);

static void      shoot                (void);
static gboolean  shoot_dialog         (void);
static void      shoot_delay          (gint32     delay);
static gboolean  shoot_delay_callback (gpointer   data);


/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,	 /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

/* the image that will be returned */
static gint32           image_ID   = -1;

/* the screen on which we are running */
static GdkScreen       *cur_screen = NULL;

/* the window the user selected */
static GdkNativeWindow  selected_native;

/* Functions */

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_INT32, "root",      "Root window { TRUE, FALSE }" },
    { GIMP_PDB_INT32, "window_id", "Window id" }
  };

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Creates a screenshot of a single window or the whole screen",
                          "After a user specified time out the user selects a window and "
                          "another time out is started. At the end of the second time out "
                          "the window is grabbed and the image is loaded into The GIMP. "
                          "Alternatively the whole screen can be grabbed. When called "
                          "non-interactively it may grab the root window or use the "
                          "window-id passed as a parameter.",
			  "Sven Neumann <sven@gimp.org>, Henrik Brix Andersen <brix@gimp.org>",
			  "1998 - 2003",
			  "v0.9.7 (2003/11/15)",
			  N_("_Screen Shot..."),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Toolbox>/File/Acquire");
  /* gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/File/Acquire"); */

  gimp_plugin_icon_register (PLUG_IN_NAME,
                             GIMP_ICON_TYPE_INLINE_PIXBUF, screenshot_icon);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usually only
   * during non-interactive calling
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /* always return at least the status to the caller. */
  static GimpParam values[2];

  /* initialize the return of the status */
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      shootvals.window_id = 0;

     /* Get information from the dialog */
      if (!shoot_dialog ())
	status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3)
	{
	  shootvals.root         = param[1].data.d_int32;
	  shootvals.window_id    = param[2].data.d_int32;
          shootvals.select_delay = 0;
	  shootvals.grab_delay   = 0;
	}
      else
	status = GIMP_PDB_CALLING_ERROR;

      if (!gdk_init_check (NULL, NULL))
	status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (shootvals.grab_delay > 0)
	shoot_delay (shootvals.grab_delay);
      /* Run the main function */
      shoot ();

      status = (image_ID != -1) ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  /* Store variable states for next run */
	  gimp_set_data (PLUG_IN_NAME, &shootvals, sizeof (ScreenShotValues));
	  /* display the image */
	  gimp_display_new (image_ID);
	}
      /* set return values */
      *nreturn_vals = 2;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }

  values[0].data.d_status = status;
}

/* Allow the user to select a window with the mouse */

static GdkNativeWindow
select_window (GdkScreen *screen)
{
#if defined(GDK_WINDOWING_X11)
  /* X11 specific code */

#define MASK (ButtonPressMask | ButtonReleaseMask)

  Display    *x_dpy;
  Cursor      x_cursor;
  XEvent      x_event;
  Window      x_win;
  Window      x_root;
  gint        x_scr;
  gint        status;
  gint        buttons;

  x_dpy = GDK_SCREEN_XDISPLAY (screen);
  x_scr = GDK_SCREEN_XNUMBER (screen);

  x_win    = None;
  x_root   = RootWindow (x_dpy, x_scr);
  x_cursor = XCreateFontCursor (x_dpy, GDK_CROSSHAIR);
  buttons  = 0;

  status = XGrabPointer (x_dpy, x_root, False,
                         MASK, GrabModeSync, GrabModeAsync,
                         x_root, x_cursor, CurrentTime);

  if (status != GrabSuccess)
    {
      g_message (_("Error grabbing the pointer"));
      return 0;
    }

  while ((x_win == None) || (buttons != 0))
    {
      XAllowEvents (x_dpy, SyncPointer, CurrentTime);
      XWindowEvent (x_dpy, x_root, MASK, &x_event);

      switch (x_event.type)
        {
        case ButtonPress:
          if (x_win == None)
            {
              x_win = x_event.xbutton.subwindow;
              if (x_win == None)
                x_win = x_root;
            }
          buttons++;
          break;

        case ButtonRelease:
          if (buttons > 0)
            buttons--;
          break;

        default:
          g_assert_not_reached ();
        }
    }

  XUngrabPointer (x_dpy, CurrentTime);
  XFreeCursor (x_dpy, x_cursor);

  return x_win;
#elif defined(GDK_WINDOWING_WIN32)
  /* MS Windows specific code goes here (yet to be written) */

  /* basically the code should grab the pointer using a crosshair
     cursor, allow the user to click on a window and return the
     obtained HWND (as a GdkNativeWindow) - for more details consult
     the X11 specific code above */

  /* note to self: take a look at the winsnap plug-in for example
     code */

#ifdef __GNUC__
#warning Win32 screenshot window chooser not implemented yet
#else
#pragma message "Win32 screenshot window chooser not implemented yet"
#endif
  return 0;
#else /* GDK_WINDOWING_WIN32 */
#warning screenshot window chooser not implemented yet for this GDK backend
  return 0;
#endif
}

/* Create a GimpImage from a GdkPixbuf */

static gint32
create_image (const GdkPixbuf *pixbuf)
{
  GimpPixelRgn	rgn;
  GimpDrawable *drawable;
  gint32        image;
  gint32        layer;
  gdouble       xres, yres;
  gchar        *comment;
  gint          width, height;
  gint          rowstride;
  gint          bpp;
  gboolean      status;
  gchar        *pixels;
  gpointer      pr;

  status = gimp_progress_init (_("Loading Screen Shot..."));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);
  layer = gimp_layer_new (image, _("Screen Shot"),
                          width, height,
                          GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);

  gimp_image_add_layer (image, layer, 0);

  drawable = gimp_drawable_get (layer);

  gimp_pixel_rgn_init (&rgn, drawable, 0, 0, width, height, TRUE, FALSE);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  bpp       = gdk_pixbuf_get_n_channels (pixbuf);
  pixels    = gdk_pixbuf_get_pixels (pixbuf);

  g_assert (bpp == rgn.bpp);

  for (pr = gimp_pixel_rgns_register (1, &rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      const guchar *src;
      guchar       *dest;
      gint          y;

      src  = pixels + rgn.y * rowstride + rgn.x * bpp;
      dest = rgn.data;

      for (y = 0; y < rgn.h; y++)
        {
          memcpy (dest, src, rgn.w * rgn.bpp);

          src  += rowstride;
          dest += rgn.rowstride;
        }
    }

  gimp_drawable_detach (drawable);

  gimp_progress_update (1.0);

  gimp_get_monitor_resolution (&xres, &yres);
  gimp_image_set_resolution (image, xres, yres);

  comment = gimp_get_default_comment ();
  if (comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment", GIMP_PARASITE_PERSISTENT,
                                    strlen (comment) + 1, comment);

      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
      g_free (comment);
    }

  gimp_image_undo_enable (image);
  return image;
}

/* The main ScreenShot function */

static void
shoot (void)
{
  GdkWindow    *window;
  GdkPixbuf    *screenshot;
  GdkRectangle  rect;
  GdkRectangle  screen_rect;
  gint          x, y;

  /* use default screen if we are running non-interactively */
  if (cur_screen == NULL)
    cur_screen = gdk_screen_get_default ();

  screen_rect.x      = 0;
  screen_rect.y      = 0;
  screen_rect.width  = gdk_screen_get_width (cur_screen);
  screen_rect.height = gdk_screen_get_height (cur_screen);

  if (shootvals.root)
    {
      /* entire screen */
      window = gdk_screen_get_root_window (cur_screen);
    }
  else
    {
      GdkDisplay *display;

      display = gdk_screen_get_display (cur_screen);

      /* single window */
      if (shootvals.window_id)
        {
          window = gdk_window_foreign_new_for_display (display,
                                                       shootvals.window_id);
        }
      else
        {
          window = gdk_window_foreign_new_for_display (display,
                                                       selected_native);
        }
    }

  if (!window)
    {
      g_message (_("Specified window not found"));
      return;
    }

  gdk_window_get_origin (window, &x, &y);

  rect.x = x;
  rect.y = y;
  gdk_drawable_get_size (GDK_DRAWABLE (window), &rect.width, &rect.height);

  if (! gdk_rectangle_intersect (&rect, &screen_rect, &rect))
    return;

  screenshot = gdk_pixbuf_get_from_drawable (NULL, window,
                                             NULL,
                                             rect.x - x, rect.y - y, 0, 0,
                                             rect.width, rect.height);

  gdk_display_beep (gdk_screen_get_display (cur_screen));
  gdk_flush ();

  if (!screenshot)
    {
      g_message (_("Error obtaining Screen Shot"));
      return;
    }

  image_ID = create_image (screenshot);

  g_object_unref (screenshot);
}

/*  ScreenShot dialog  */

static gboolean
shoot_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *spinner;
  GdkPixbuf *pixbuf;
  GSList    *radio_group = NULL;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init ("screenshot", FALSE);

  dialog = gimp_dialog_new (_("Screen Shot"), "screenshot",
                            NULL, 0,
			    gimp_standard_help_func, HELP_ID,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    _("Grab"),        GTK_RESPONSE_OK,

			    NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  pixbuf = gdk_pixbuf_new_from_inline (-1, screenshot_icon, FALSE, NULL);
  if (pixbuf)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);

      g_object_unref (pixbuf);

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  /*  single window  */
  frame = gimp_frame_new (_("Grab"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("a _Single Window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), ! shootvals.root);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (FALSE));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.root);

  /*  select window delay  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("S_elect Window After"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (shootvals.select_delay, 0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.select_delay);

  label = gtk_label_new (_("Seconds Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  root window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("the _Whole Screen"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), shootvals.root);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (TRUE));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.root);

  gtk_widget_show (button);
  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /*  grab delay  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Grab _After"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (shootvals.grab_delay, 0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.grab_delay);

  label = gtk_label_new (_("Seconds Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      /* get the screen on which we are running */
      cur_screen = gtk_widget_get_screen (dialog);
    }

  gtk_widget_destroy (dialog);

  if (run)
   {
     if (!shootvals.root && !shootvals.window_id)
       {
         if (shootvals.select_delay > 0)
           shoot_delay (shootvals.select_delay);

         selected_native = select_window (cur_screen);
       }
    }

  return run;
}


/*  delay functions  */
void
shoot_delay (gint delay)
{
  g_timeout_add (1000, shoot_delay_callback, &delay);
  gtk_main ();
}

gboolean
shoot_delay_callback (gpointer data)
{
  gint *seconds_left = data;

  (*seconds_left)--;

  if (!*seconds_left)
    gtk_main_quit ();

  return *seconds_left;
}
