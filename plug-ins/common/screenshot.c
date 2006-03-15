/*
 *  Screenshot plug-in
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gdk/gdkkeysyms.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>

#ifdef HAVE_X11_XMU_WINUTIL_H
#include <X11/Xmu/WinUtil.h>
#endif /* HAVE_X11_XMU_WINUTIL_H */

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
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (1653) */
  "\0\0\6\215"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (96) */
  "\0\0\0`"
  /* width (24) */
  "\0\0\0\30"
  /* height (24) */
  "\0\0\0\30"
  /* pixel_data: */
  "\273\0\0\0\0\7kkk\17\250\250\250\316\265\265\265\377\276\276\276\377"
  "\273\273\273\377\253\253\253\316rrr\17\221\0\0\0\0\7\210\210\210F\322"
  "\322\322\364\370\370\370\377\376\376\376\377\370\370\370\377\325\325"
  "\325\370\220\220\220F\221\0\0\0\0\7www\250\256\256\256\377\300\300\300"
  "\377\310\310\310\377\303\303\303\377\260\260\260\377\204\204\204\253"
  "\220\0\0\0\0\11\35\35\35\17""111\354<<<\334LLL\201PPPrLLL\201>>>\323"
  ";;;\344'''\17\217\0\0\0\0\11\2\2\2E\40\40\40\377***\316RRR\200QQQ\200"
  "PPP\200$$$\316\34\34\34\377\4\4\4E\215\0\0\0\0\5\0\0\0\12\0\0\0'\0\0"
  "\0\207777\365ppp\377\203zzz\377\6ppp\377000\366\0\0\0\217\0\0\0'\0\0"
  "\0\\\0\0\0""4\204\0\0\0\0\13\0\0\0\6\0\0\0/\2\2\2`\1\1\0\222\"\"\"\251"
  "111\324FFF\342DDD\343PPP\312\216\216\216\377\244\244\244\377\204\260"
  "\260\260\377\6\222\222\222\377DDD\364888\355'''\342\37\37\37\322\0\0"
  "\0\3\202\0\0\0\0\14\0\0\0h988\377ddc\377feb\377edb\377kjg\377{{z\377"
  "nnn\377\344\344\344\377\231\231\231\377\256\256\256\377\212\212\212\377"
  "\202\203\203\203\377\202\201\201\201\377\6\226\226\226\377xxx\377www"
  "\377yyy\377MMM\366\12\12\11E\202\0\0\0\0\1LKH\377\202yxu\377\5ffd\377"
  ",,,\377a`^\377FFD\377ZZZ\377\202\220\220\220\377\4\240\240\240\377\217"
  "\216\216\377\215\214\212\377nmj\377\202ffd\377\6nnm\377\212\212\212\377"
  "lll\377fff\377kkk\377\6\6\6q\202\0\0\0\0\26>=:\377\232\232\226\377\222"
  "\221\215\377\226\225\224\377\206\205\202\377\\\\Z\377((&\377ooo\377e"
  "ee\377\232\232\232\377\206\206\205\377prp\377add\377X\\\\\377RVV\377"
  "ORR\377BEA\377[]Y\377hhh\377~~~\377}}}\377\0\0\0\200\202\0\0\0\0\26>"
  "=8\377\203\201x\377sph\377gd]\377XVP\377;:6\37700.\377qqq\377nnn\377"
  "\222\222\221\377`cc\377W\\\\\377DHI\377FJL\377DIL\377<AC\377151\377/"
  "1+\377OOL\377zzz\377~~~\377\0\0\0\200\202\0\0\0\0\26=<8\377\203\200x"
  "\377sph\377hf^\377YVP\377=;7\377543\377mmm\377\200\200\200\377Z]]\377"
  "KQQ\377NQR\377RTR\377)+,\377%((\377MOP\377PSU\377042\377+*$\377rrr\377"
  "|||\377\0\0\0\200\202\0\0\0\0\26=;7\377\202\200x\377rog\377ge^\377XV"
  "O\377<:6\377443\377qqq\377^`a\377PUW\377RVT\377698\377443\377,,,\377"
  "\34\34\34\377\32\32\32\3777;;\377\77DG\377'(\"\377``\\\377{{{\377\0\0"
  "\0\200\202\0\0\0\0\177<;7\377\202\177w\377rog\377fd]\377WUN\377<:6\377"
  "442\377kkk\377VXY\377<BB\377VYY\377887\377\224\224\224\377\265\265\265"
  "\377XXX\377;;;\377*++\377JPR\377)-,\377DC\77\377yyy\377\0\0\0\201\0\0"
  "\0\1\0\0\0\2<;6\377\202\177w\377qnf\377fc\\\377VTN\377<:6\377442\377"
  "mmm\377\\^^\377@EF\377KOL\377\36\36\36\377xxx\377\316\316\316\377\237"
  "\237\237\377YYY\377:::\377NRU\377)-/\377ZZV\377vvv\377\0\0\0\206\0\0"
  "\0\6\0\0\0\10<:6\377\200~v\377qnf\377eb\\\377VTN\377;95\377442\377nn"
  "n\377XYZ\3779@@\3776<<\377\20\21\20\377...\377ggg\377\204\204\204\377"
  "\206\206\206\377BBB\377EKN\377(,.\377hih\377jjj\377\0\0\0\215\0\0\0\16"
  "\0\0\0\20<:6\377\200~v\377pnf\377dc[\377USM\377;95\377432\377ttt\377"
  "ijj\377*./\377<DG\377$(&\377###\377@@@\377RRR\377JJJ\377JLP\377<BG\377"
  "),,\377xxx\377BBB\371\0\0\0b\0\0\0\16\0\0\0\17\"\"\"\255XVP\371`^W\376"
  "^\\U\377NKF\377,+(\363\0\0\0\310\0\0\0\274\0\0\0\276\20\22\22\3457>A"
  "\377UXX\377045\377024\3778<=\377BFH\377SWY\377158\377\13\15\15\307\0"
  "\0\0\251\0\0\0e\0\0\0\32\0\0\0\10\0\0\0\6\0\0\0\30\0\0\0G\0\0\0e\0\0"
  "\0|\15\14\13o\5\5\4X\0\0\0F'\0\0\0J\0\0\0V\2\2\2q\24\26\27\324<AD\377"
  "PUX\377@FJ\377RVY\377WZ\\\377379\377\17\20\21\262\0\0\0Q\0\0\0""3\0\0"
  "\0\32\0\0\0\11\0\0\0\2\0\0\0\1\0\0\0\4\0\0\0\10\0\0\0\12\0\0\0\13\0\0"
  "\0\15\0\0\0\20\0\0\0\22\0\0\0\26\0\0\0$\0\0\0""2\1\1\1D\11\12\12\212"
  "$')\306\"%'\360&),\343\33\35\36\300\12\13\13l\0\0\0""6\0\0\0\"\0\0\0"
  "\20\0\0\0\6\0\0\0\1\210\0\0\0\0\16\0\0\0\1\0\0\0\2\0\0\0\10\0\0\0\16"
  "\0\0\0\25\0\0\0\34\0\0\0\"\0\0\0#\0\0\0\40\0\0\0\34\0\0\0\27\0\0\0\17"
  "\0\0\0\7\0\0\0\2\215\0\0\0\0\3\0\0\0\2\0\0\0\4\0\0\0\6\202\0\0\0\11\4"
  "\0\0\0\7\0\0\0\5\0\0\0\3\0\0\0\1\205\0\0\0\0"
};


/* Defines */
#define PLUG_IN_PROC   "plug-in-screenshot"
#define PLUG_IN_BINARY "screenshot"

#ifdef __GNUC__
#ifdef GDK_NATIVE_WINDOW_POINTER
#if GLIB_SIZEOF_VOID_P != 4
#warning window_id does not fit in PDB_INT32
#endif
#endif
#endif

typedef enum
{
  SHOOT_ROOT,
  SHOOT_REGION,
  SHOOT_WINDOW
} ShootType;

typedef struct
{
  ShootType  shoot_type;
  gboolean   decorate;
  guint      window_id;
  guint      select_delay;
  gint       x1;
  gint       y1;
  gint       x2;
  gint       y2;
} ScreenshotValues;

static ScreenshotValues shootvals =
{
  SHOOT_WINDOW, /* root window  */
  TRUE,         /* include WM decorations */
  0,            /* window ID    */
  0,            /* select delay */
  0,            /* coords of region dragged out by pointer */
  0,
  0,
  0
};


static void      query (void);
static void      run   (const gchar      *name,
			gint              nparams,
			const GimpParam  *param,
			gint             *nreturn_vals,
			GimpParam       **return_vals);

static GdkNativeWindow select_window  (GdkScreen        *screen);
static gint32          create_image   (const GdkPixbuf  *pixbuf);

static gint32    shoot                (GdkScreen        *screen);
static gboolean  shoot_dialog         (GdkScreen       **screen);
static void      shoot_delay          (gint32            delay);
static gboolean  shoot_delay_callback (gpointer          data);
static gboolean  shoot_quit_timeout   (gpointer          data);


/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,	 /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


/* Functions */

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode",  "Interactive, non-interactive"     },
    { GIMP_PDB_INT32, "root",      "Root window { TRUE, FALSE }"      },
    { GIMP_PDB_INT32, "window-id", "Window id"                        },
    { GIMP_PDB_INT32, "x1",        "(optional) Region left x coord"   },
    { GIMP_PDB_INT32, "y1",        "(optional) Region top y coord"    },
    { GIMP_PDB_INT32, "x2",        "(optional) Region right x coord"  },
    { GIMP_PDB_INT32, "y2",        "(optional) Region bottom y coord" }
  };

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  N_("Create an image from an area of the screen"),
                          "The plug-in allows to take screenshots of a an "
                          "interactively selected window or of the desktop, "
                          "either the whole desktop or an interactively "
                          "selected region. When called non-interactively, it "
                          "may grab the root window or use the window-id "
                          "passed as a parameter.  The last four parameters "
                          "are optional and can be used to specify the corners "
                          "of the region to be grabbed.",
			  "Sven Neumann <sven@gimp.org>, "
                          "Henrik Brix Andersen <brix@gimp.org>",
			  "1998 - 2003",
			  "v0.9.7 (2003/11/15)",
			  N_("_Screenshot..."),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Toolbox>/File/Acquire");
  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Acquire");

  gimp_plugin_icon_register (PLUG_IN_PROC,
                             GIMP_ICON_TYPE_INLINE_PIXBUF, screenshot_icon);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpRunMode        run_mode = param[0].data.d_int32;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GdkScreen         *screen   = NULL;
  gint32             image_ID;

  static GimpParam   values[2];

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
      gimp_get_data (PLUG_IN_PROC, &shootvals);
      shootvals.window_id = 0;

     /* Get information from the dialog */
      if (! shoot_dialog (&screen))
	status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3)
	{
          gboolean do_root = param[1].data.d_int32;
          if (do_root)
            shootvals.shoot_type = SHOOT_ROOT;
          else
            shootvals.shoot_type = SHOOT_WINDOW;
	  shootvals.window_id    = param[2].data.d_int32;
          shootvals.select_delay = 0;
	}
      else if (nparams == 7)
	{
	  shootvals.shoot_type   = SHOOT_REGION;
	  shootvals.window_id    = param[2].data.d_int32;
          shootvals.select_delay = 0;
          shootvals.x1           = param[3].data.d_int32;
          shootvals.y1           = param[4].data.d_int32;
          shootvals.x2           = param[5].data.d_int32;
          shootvals.y2           = param[6].data.d_int32;
	}

        {
          status = GIMP_PDB_CALLING_ERROR;
        }

      if (! gdk_init_check (NULL, NULL))
	status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_PROC, &shootvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (shootvals.select_delay > 0)
	shoot_delay (shootvals.select_delay);

      image_ID = shoot (screen);

      status = (image_ID != -1) ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  /* Store variable states for next run */
	  gimp_set_data (PLUG_IN_PROC, &shootvals, sizeof (ScreenshotValues));

	  gimp_display_new (image_ID);
	}

      /* set return values */
      *nreturn_vals = 2;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }

  values[0].data.d_status = status;
}


/* Allow the user to select a window or a region with the mouse */

#ifdef GDK_WINDOWING_X11

static GdkNativeWindow
select_window_x11 (GdkScreen *screen)
{
  Display      *x_dpy;
  Cursor        x_cursor;
  XEvent        x_event;
  Window        x_win;
  Window        x_root;
  XGCValues     gc_values;
  GC            gc          = NULL;
  GdkKeymapKey *keys        = NULL;
  gint          x_scr;
  gint          status;
  gint          buttons;
  gint          mask        = ButtonPressMask | ButtonReleaseMask;
  gint          x, y, w, h;
  gint          num_keys;
  gboolean      cancel      = FALSE;

  x_dpy = GDK_SCREEN_XDISPLAY (screen);
  x_scr = GDK_SCREEN_XNUMBER (screen);

  x_win    = None;
  x_root   = RootWindow (x_dpy, x_scr);
  x_cursor = XCreateFontCursor (x_dpy, GDK_CROSSHAIR);
  buttons  = 0;

  if (shootvals.shoot_type == SHOOT_REGION)
    {
      mask |= PointerMotionMask;

      gc_values.function   = GXxor;
      gc_values.plane_mask = AllPlanes;
      gc_values.foreground = WhitePixel (x_dpy, x_scr);
      gc_values.background = BlackPixel (x_dpy, x_scr);
      gc_values.line_width = 0;
      gc_values.line_style = LineSolid;
      gc_values.fill_style = FillSolid;
      gc_values.cap_style  = CapButt;
      gc_values.join_style = JoinMiter;
      gc_values.graphics_exposures = FALSE;
      gc_values.clip_x_origin = 0;
      gc_values.clip_y_origin = 0;
      gc_values.clip_mask  = None;
      gc_values.subwindow_mode = IncludeInferiors;

      gc = XCreateGC (x_dpy, x_root,
                      GCFunction | GCPlaneMask | GCForeground | GCLineWidth |
                      GCLineStyle | GCCapStyle | GCJoinStyle |
                      GCGraphicsExposures | GCBackground | GCFillStyle |
                      GCClipXOrigin | GCClipYOrigin | GCClipMask |
                      GCSubwindowMode,
                      &gc_values);
    }

  status = XGrabPointer (x_dpy, x_root, False,
                         mask, GrabModeSync, GrabModeAsync,
                         x_root, x_cursor, CurrentTime);

  if (status != GrabSuccess)
    {
      g_message (_("Error grabbing the pointer"));
      return 0;
    }

  if (gdk_keymap_get_entries_for_keyval (NULL, GDK_Escape, &keys, &num_keys))
    {
      gdk_error_trap_push ();
      XGrabKey (x_dpy, keys[0].keycode, AnyModifier, x_root, False,
                GrabModeAsync, GrabModeAsync);
      gdk_flush ();
      gdk_error_trap_pop ();
    }

  while (! cancel && ((x_win == None) || (buttons != 0)))
    {
      XAllowEvents (x_dpy, SyncPointer, CurrentTime);
      XWindowEvent (x_dpy, x_root, mask | KeyPressMask, &x_event);

      switch (x_event.type)
        {
        case ButtonPress:
          if (x_win == None)
            {
              x_win = x_event.xbutton.subwindow;
              if (x_win == None)
                x_win = x_root;
#ifdef HAVE_X11_XMU_WINUTIL_H
              else if (! shootvals.decorate)
                {
                  x_win = XmuClientWindow (x_dpy, x_win);
                }
#endif

              shootvals.x2 = shootvals.x1 = x_event.xbutton.x_root;
              shootvals.y2 = shootvals.y1 = x_event.xbutton.y_root;
            }
          buttons++;
          break;

        case ButtonRelease:
          if (buttons > 0)
            buttons--;
          if (! buttons && shootvals.shoot_type == SHOOT_REGION)
            {
              x = MIN (shootvals.x1, shootvals.x2);
              y = MIN (shootvals.y1, shootvals.y2);
              w = ABS (shootvals.x2 - shootvals.x1);
              h = ABS (shootvals.y2 - shootvals.y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, gc, x, y, w, h);

              shootvals.x2 = x_event.xbutton.x_root;
              shootvals.y2 = x_event.xbutton.y_root;
            }
          break;

        case MotionNotify:
          if (buttons > 0)
            {
              x = MIN (shootvals.x1, shootvals.x2);
              y = MIN (shootvals.y1, shootvals.y2);
              w = ABS (shootvals.x2 - shootvals.x1);
              h = ABS (shootvals.y2 - shootvals.y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, gc, x, y, w, h);

              shootvals.x2 = x_event.xmotion.x_root;
              shootvals.y2 = x_event.xmotion.y_root;

              x = MIN (shootvals.x1, shootvals.x2);
              y = MIN (shootvals.y1, shootvals.y2);
              w = ABS (shootvals.x2 - shootvals.x1);
              h = ABS (shootvals.y2 - shootvals.y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, gc, x, y, w, h);
            }
          break;

        case KeyPress:
          {
            guint *keyvals;
            gint   n;

            if (gdk_keymap_get_entries_for_keycode (NULL, x_event.xkey.keycode,
                                                    NULL, &keyvals, &n))
              {
                gint i;

                for (i = 0; i < n && ! cancel; i++)
                  if (keyvals[i] == GDK_Escape)
                    cancel = TRUE;

                g_free (keyvals);
              }
          }
          break;

        default:
          break;
        }
    }

  if (keys)
    {
      XUngrabKey (x_dpy, keys[0].keycode, AnyModifier, x_root);
      g_free (keys);
    }

  XUngrabPointer (x_dpy, CurrentTime);
  XFreeCursor (x_dpy, x_cursor);

  return x_win;
}

#endif


#ifdef GDK_WINDOWING_WIN32

static GdkNativeWindow
select_window_win32 (GdkScreen *screen)
{
  /* MS Windows specific code goes here (yet to be written) */

  /* basically the code should grab the pointer using a crosshair
   * cursor, allow the user to click on a window and return the
   * obtained HWND (as a GdkNativeWindow) - for more details consult
   * the X11 specific code above
   */

  /* note to self: take a look at the winsnap plug-in for example code */

#ifdef __GNUC__
#warning Win32 screenshot window chooser not implemented yet
#else
#pragma message("Win32 screenshot window chooser not implemented yet")
#endif

  return 0;
}

#endif


static GdkNativeWindow
select_window (GdkScreen *screen)
{
#if defined(GDK_WINDOWING_X11)
  return select_window_x11 (screen);
#elif defined(GDK_WINDOWING_WIN32)
  return select_window_win32 (screen);
#else
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
  guchar       *pixels;
  gpointer      pr;

  status = gimp_progress_init (_("Importing screenshot"));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);
  layer = gimp_layer_new (image, _("Screenshot"),
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

      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    g_utf8_strlen (comment, -1) + 1,
                                    comment);

      gimp_image_parasite_attach (image, parasite);
      gimp_parasite_free (parasite);
      g_free (comment);
    }

  gimp_image_undo_enable (image);

  return image;
}

/* The main Screenshot function */

static gint32
shoot (GdkScreen *screen)
{
  GdkWindow    *window;
  GdkPixbuf    *screenshot;
  GdkRectangle  rect;
  GdkRectangle  screen_rect;
  gint32        image;
  gint          x, y;

  /* use default screen if we are running non-interactively */
  if (screen == NULL)
    screen = gdk_screen_get_default ();

  screen_rect.x      = 0;
  screen_rect.y      = 0;
  screen_rect.width  = gdk_screen_get_width (screen);
  screen_rect.height = gdk_screen_get_height (screen);


  if (shootvals.shoot_type == SHOOT_REGION)
    {
      rect.x = MIN (shootvals.x1, shootvals.x2);
      rect.y = MIN (shootvals.y1, shootvals.y2);
      rect.width  = ABS (shootvals.x2 - shootvals.x1);
      rect.height = ABS (shootvals.y2 - shootvals.y1);
    }
  else
    {
      if (shootvals.shoot_type == SHOOT_ROOT)
        {
          window = gdk_screen_get_root_window (screen);
        }
      else
        {
          GdkDisplay *display = gdk_screen_get_display (screen);

          window = gdk_window_foreign_new_for_display (display,
                                                       shootvals.window_id);
        }

      if (! window)
        {
          g_message (_("Specified window not found"));
          return -1;
        }

      gdk_drawable_get_size (GDK_DRAWABLE (window), &rect.width, &rect.height);
      gdk_window_get_origin (window, &x, &y);

      rect.x = x;
      rect.y = y;
    }

  window = gdk_screen_get_root_window (screen);
  gdk_window_get_origin (window, &x, &y);

  if (! gdk_rectangle_intersect (&rect, &screen_rect, &rect))
    return -1;

  screenshot = gdk_pixbuf_get_from_drawable (NULL, window,
                                             NULL,
                                             rect.x - x, rect.y - y, 0, 0,
                                             rect.width, rect.height);

  gdk_display_beep (gdk_screen_get_display (screen));
  gdk_flush ();

  if (!screenshot)
    {
      g_message (_("There was an error taking the screenshot."));
      return -1;
    }

  image = create_image (screenshot);

  g_object_unref (screenshot);

  return image;
}

/*  Screenshot dialog  */

static gboolean
shoot_dialog (GdkScreen **screen)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *spinner;
  GdkPixbuf *pixbuf;
  GSList    *radio_group = NULL;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Screenshot"), PLUG_IN_BINARY,
                            NULL, 0,
			    gimp_standard_help_func, PLUG_IN_PROC,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

			    NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("_Grab"), GTK_RESPONSE_OK);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  pixbuf = gdk_pixbuf_new_from_inline (-1, screenshot_icon, FALSE, NULL);
  if (pixbuf)
    {
      gtk_button_set_image (GTK_BUTTON (button),
                            gtk_image_new_from_pixbuf (pixbuf));
      g_object_unref (pixbuf);
    }

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (_("Area"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);


  /*  single window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("Take a screenshot of "
                                                 "a single _window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_WINDOW);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_WINDOW));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.shoot_type);

#ifdef HAVE_X11_XMU_WINUTIL_H

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_check_button_new_with_mnemonic (_("Include window _decoration"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), shootvals.decorate);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 24);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (button), "set_sensitive", toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &shootvals.decorate);

#endif /* HAVE_X11_XMU_WINUTIL_H */


  /*  whole screen  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("Take a screenshot of "
                                                 "the entire _screen"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_ROOT);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_ROOT));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.shoot_type);


  /*  dragged region  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("Select a _region to grab"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_REGION);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("If enabled, you can use the mouse to "
                                     "select a rectangular region of the "
                                     "screen."), NULL);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_REGION));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.shoot_type);


  /*  grab delay  */
  frame = gimp_frame_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* this string is part of "Wait [spinbutton] seconds before grabbing" */
  label = gtk_label_new_with_mnemonic (_("W_ait"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinner = gimp_spin_button_new (&adj, shootvals.select_delay,
                                  0.0, 100.0, 1.0, 5.0, 0.0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.select_delay);

  /* this string is part of "Wait [spinbutton] seconds before grabbing" */
  label = gtk_label_new (_("seconds before grabbing"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gimp_help_set_help_data (spinner, _("The number of seconds to wait after "
                                      "selecting the window or region and "
                                      "actually taking the screenshot."), NULL);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      /* get the screen on which we are running */
      *screen = gtk_widget_get_screen (dialog);
    }

  gtk_widget_destroy (dialog);

  if (run)
    {
      /*  A short timeout to give the server a chance to
       *  redraw the area that was obscured by our dialog.
       */
      g_timeout_add (100, shoot_quit_timeout, NULL);
      gtk_main ();

      if (shootvals.shoot_type != SHOOT_ROOT && ! shootvals.window_id)
        {
          shootvals.window_id = select_window (*screen);

          if (! shootvals.window_id)
            return FALSE;
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

static gboolean
shoot_quit_timeout (gpointer data)
{
  gtk_main_quit ();
  return FALSE;
}
