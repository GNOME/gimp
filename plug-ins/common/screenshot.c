/* GIMP - The GNU Image Manipulation Program
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

/*
 *  Screenshot plug-in
 *  Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 *  Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gdk/gdkkeysyms.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif /* HAVE_X11_EXTENSIONS_SHAPE_H */

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
  /* length: header (24) + pixel_data (1582) */
  "\0\0\6F"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (88) */
  "\0\0\0X"
  /* width (22) */
  "\0\0\0\26"
  /* height (22) */
  "\0\0\0\26"
  /* pixel_data: */
  "\213\0\0\0\0\1\242\242\242\5\203\242\242\242\31\221\0\0\0\0\2\27\27\26"
  "D\40\40\40\372\203)))\375\2\32\32\32\362\26\26\25""0\217\0\0\0\0\2\2"
  "\2\2\322\307\310\307\377\203\377\377\377\377\2\263\263\262\363\0\0\0"
  "\314\217\0\0\0\0\2\6\6\6\334\206\207\205\377\202\252\253\251\377\3\252"
  "\252\251\377ghe\376\1\1\1\320\217\0\0\0\0\2\11\11\11\346./-\345\202G"
  "HE\25\3JKH\32""01/\364\5\5\5\333\212\0\0\0\0\17\26\26\25\31\26\26\25"
  "0\26\26\25g\0\0\0\314+,+\331LMK\375EFD\362ffe\347iig\346lmk\346RSQ\362"
  "\13\13\13\360\0\0\0\314\2\2\2\321\26\26\25V\203\0\0\0\0#\26\26\25\31"
  "\26\26\25""0\26\26\25n\3\3\3\331\2\2\2\332JJI\355\215\215\215\372\246"
  "\246\246\347\267\270\266\362\177\202~\377BC\77\377TUR\377\\]Y\377gid"
  "\377[]X\377\204\206\204\374wxw\276\224\225\224\314LLK\343\26\26\25D\0"
  "\0\0\0\26\26\25D\3\3\3\341\17\17\17\373ghg\377\237\240\236\377\273\274"
  "\272\377\302\303\300\377\272\273\270\377\200\201\177\377zzz\377tws\377"
  "\220\223\217\377\221\225\221\377\224\227\223\377\202\226\232\226\377"
  "K\177\202}\377xyu\377\217\221\215\377\226\227\226\365\0\0\0\371\0\0\0"
  "\0\2\2\2\330\257\261\257\377\275\276\273\377\262\263\260\377UWT\377Q"
  "SP\377suq\377]^[\377\304\304\304\377\303\303\303\377kmi\377MNJ\377WZ"
  "X\377[`_\377aeb\377gid\377}\200{\377SUQ\377FGE\377\200\201\200\377\12"
  "\12\12\357\0\0\0\0\4\4\4\345xyv\377\241\242\236\377\210\212\205\377v"
  "xt\377\220\222\217\377GIF\377+,)\377```\377jji\377UXV\377y\204\210\377"
  "\203\215\220\377~\204\207\377flo\377PW\\\377SWU\377JLI\377452\377tut"
  "\377\12\12\12\357\0\0\0\0\4\4\4\344opm\377\221\223\217\377}\177{\377"
  "\215\217\213\377`b_\377()'\377*+)\377785\377VXV\377u~\202\377nsu\377"
  "VYZ\377OST\377OQS\377JOP\377^cd\377698\377!\"!\377llk\377\12\12\12\357"
  "\0\0\0\0\3\3\3\345npl\377\220\221\216\377\202UWS\377\24GIF\377\40!\37"
  "\377*+)\3779:7\377^ce\377NRT\377UXY\377(,.\377\22\24\25\377!$&\377IL"
  "M\3778=\77\377RVX\377''&\377klk\377\12\12\12\357\0\0\0\1\3\3\3\345no"
  "l\377\220\221\216\377\202UWS\377\24GIF\377\40!\37\377*+)\3778:8\377T"
  "XY\377289\377\23\25\26\377\16\16\16\377\1\1\1\377\2\2\2\377\24\25\25"
  "\377.34\377289\377011\377aa`\377\11\11\11\360\0\0\0\3\3\3\3\345nol\377"
  "\204\205\202\377\202UWS\377\24GIF\377\40!\37\377*+)\377\77BA\37728:\377"
  "\34\40!\377\31\32\32\377\377\377\377\377hhh\377\40\40\40\377\22\22\22"
  "\377.00\377.46\377:>\77\377`a`\377\11\11\10\361\0\0\0\7\3\3\3\345mok"
  "\377z{x\377\202UWS\377\24GIF\377\40!\37\377*+)\377HJI\377.46\377\25\27"
  "\30\377\40\40\40\377hhh\377\232\232\232\377}}}\377'''\3779;;\377/57\377"
  "599\377``_\377\11\11\10\362\0\0\0\15\3\3\3\345lnk\377rsp\377\202UWS\377"
  "\24GIF\377\40!\37\377,-*\377>@>\377.46\377\35\37\40\377\1\1\1\377888"
  "\377\214\214\214\377\213\213\213\377AAA\377JKK\377/57\3776:;\377WXV\377"
  "\6\6\6\364\0\0\0\21\0\0\0\371\201\202\177\377mok\377\202UWS\377iGIF\377"
  "MNM\377RRR\372ghg\364[`b\377+/0\377\17\17\17\377\"\"\"\377'''\377HHH"
  "\377\263\263\263\377SUU\377W\\^\377CFF\374<<<\376\20\20\17\317\0\0\0"
  "\20\24\24\23]\4\4\4\353\177\177~\375|}z\373}~|\374hih\3769:9\371\0\0"
  "\0\347\0\0\0\351\7\7\7\367.46\377256\377\40\40\40\377(((\377:::\377`"
  "ab\3779>\77\377\11\13\13\376\0\0\0\353\0\0\0\300\14\14\13H\0\0\0\15\0"
  "\0\0\4\23\23\22L\0\0\0\332\0\0\0\341\0\0\0\346\0\0\0\341\14\14\13{\0"
  "\0\0V\0\0\0a\0\0\0l\10\11\11\371\\^_\377LOP\377ILL\377LOQ\377QUV\377"
  "\16\17\17\377\34\37\40\244\0\0\0`\0\0\0""4\0\0\0\27\0\0\0\7\0\0\0\1\0"
  "\0\0\3\0\0\0\10\0\0\0\21\0\0\0\34\0\0\0'\0\0\0""0\0\0\0""6\0\0\0=\0\0"
  "\0A\0\0\0F\4\5\5\355\12\13\13\371\17\20\20\376\15\16\16\375\10\10\11"
  "\365\7\10\11j\0\0\0G\0\0\0-\0\0\0\30\0\0\0\13\0\0\0\4\0\0\0\0\0\0\0\1"
  "\0\0\0\2\0\0\0\4\0\0\0\11\0\0\0\16\0\0\0\23\0\0\0\31\0\0\0\35\0\0\0!"
  "\0\0\0#\0\0\0$\0\0\0%\0\0\0$\0\0\0&\0\0\0%\0\0\0\36\0\0\0\27\0\0\0\20"
  "\0\0\0\10\0\0\0\3\0\0\0\1\204\0\0\0\0\5\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0"
  "\5\0\0\0\6\202\0\0\0\10\11\0\0\0\11\0\0\0\7\0\0\0\10\0\0\0\5\0\0\0\7"
  "\0\0\0\6\0\0\0\5\0\0\0\3\0\0\0\1\202\0\0\0\0"
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

static GdkNativeWindow   select_window (GdkScreen    *screen);
static gint32            create_image  (GdkPixbuf    *pixbuf,
                                        GdkRegion    *shape,
                                        const gchar  *name);

static gint32     shoot                (GdkScreen    *screen);
static gboolean   shoot_dialog         (GdkScreen   **screen);
static void       shoot_delay          (gint32        delay);
static gboolean   shoot_delay_callback (gpointer      data);
static gboolean   shoot_quit_timeout   (gpointer      data);


/* Global Variables */
const GimpPlugInInfo PLUG_IN_INFO =
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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode",  "Interactive, non-interactive"     },
    { GIMP_PDB_INT32, "root",      "Root window { TRUE, FALSE }"      },
    { GIMP_PDB_INT32, "window-id", "Window id"                        },
    { GIMP_PDB_INT32, "x1",        "(optional) Region left x coord"   },
    { GIMP_PDB_INT32, "y1",        "(optional) Region top y coord"    },
    { GIMP_PDB_INT32, "x2",        "(optional) Region right x coord"  },
    { GIMP_PDB_INT32, "y2",        "(optional) Region bottom y coord" }
  };

  static const GimpParamDef return_vals[] =
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
			  "1998 - 2007",
			  "v1.0 (2007/05)",
			  N_("_Screenshot..."),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Toolbox>/File/Acquire");
  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Acquire");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_INLINE_PIXBUF,
                             screenshot_icon);
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
	status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3)
	{
          gboolean do_root = param[1].data.d_int32;

          shootvals.shoot_type   = do_root ? SHOOT_ROOT : SHOOT_WINDOW;
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
      else
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

      if (shootvals.shoot_type != SHOOT_ROOT && ! shootvals.window_id)
        {
          shootvals.window_id = select_window (screen);

          if (! shootvals.window_id)
            status = GIMP_PDB_CANCEL;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      image_ID = shoot (screen);

      if (image_ID == -1)
        status = GIMP_PDB_EXECUTION_ERROR;
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

      values[1].type         = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }

  values[0].data.d_status = status;
}


/* Allow the user to select a window or a region with the mouse */

#ifdef GDK_WINDOWING_X11

static GdkNativeWindow
select_window_x11 (GdkScreen *screen)
{
  Display      *x_dpy       = GDK_SCREEN_XDISPLAY (screen);
  gint          x_scr       = GDK_SCREEN_XNUMBER (screen);
  Window        x_root      = RootWindow (x_dpy, x_scr);
  Window        x_win       = None;
  GC            x_gc          = None;
  Cursor        x_cursor    = XCreateFontCursor (x_dpy, GDK_CROSSHAIR);
  GdkKeymapKey *keys        = NULL;
  gint          status;
  gint          i, num_keys;
  gint          buttons     = 0;
  gint          mask        = ButtonPressMask | ButtonReleaseMask;
  gboolean      cancel      = FALSE;

  if (shootvals.shoot_type == SHOOT_REGION)
    mask |= PointerMotionMask;

  status = XGrabPointer (x_dpy, x_root, False,
                         mask, GrabModeSync, GrabModeAsync,
                         x_root, x_cursor, CurrentTime);

  if (status != GrabSuccess)
    {
      gint  x, y;
      guint xmask;

      /* if we can't grab the pointer, return the window under the pointer */
      XQueryPointer (x_dpy, x_root, &x_root, &x_win, &x, &y, &x, &y, &xmask);

      if (x_win == None || x_win == x_root)
        g_message (_("Error selecting the window"));
    }

  if (shootvals.shoot_type == SHOOT_REGION)
    {
      XGCValues gc_values;

      gc_values.function           = GXxor;
      gc_values.plane_mask         = AllPlanes;
      gc_values.foreground         = WhitePixel (x_dpy, x_scr);
      gc_values.background         = BlackPixel (x_dpy, x_scr);
      gc_values.line_width         = 0;
      gc_values.line_style         = LineSolid;
      gc_values.fill_style         = FillSolid;
      gc_values.cap_style          = CapButt;
      gc_values.join_style         = JoinMiter;
      gc_values.graphics_exposures = FALSE;
      gc_values.clip_x_origin      = 0;
      gc_values.clip_y_origin      = 0;
      gc_values.clip_mask          = None;
      gc_values.subwindow_mode     = IncludeInferiors;

      x_gc = XCreateGC (x_dpy, x_root,
                        GCFunction | GCPlaneMask | GCForeground | GCLineWidth |
                        GCLineStyle | GCCapStyle | GCJoinStyle |
                        GCGraphicsExposures | GCBackground | GCFillStyle |
                        GCClipXOrigin | GCClipYOrigin | GCClipMask |
                        GCSubwindowMode,
                        &gc_values);
    }

  if (gdk_keymap_get_entries_for_keyval (NULL, GDK_Escape, &keys, &num_keys))
    {
      gdk_error_trap_push ();

#define X_GRAB_KEY(index, modifiers) \
      XGrabKey (x_dpy, keys[index].keycode, modifiers, x_root, False, \
                GrabModeAsync, GrabModeAsync)

      for (i = 0; i < num_keys; i++)
        {
          X_GRAB_KEY (i, 0);
          X_GRAB_KEY (i, LockMask);            /* CapsLock              */
          X_GRAB_KEY (i, Mod2Mask);            /* NumLock               */
          X_GRAB_KEY (i, Mod5Mask);            /* ScrollLock            */
          X_GRAB_KEY (i, LockMask | Mod2Mask); /* CapsLock + NumLock    */
          X_GRAB_KEY (i, LockMask | Mod5Mask); /* CapsLock + ScrollLock */
          X_GRAB_KEY (i, Mod2Mask | Mod5Mask); /* NumLock  + ScrollLock */
          X_GRAB_KEY (i, LockMask | Mod2Mask | Mod5Mask); /* all        */
        }

#undef X_GRAB_KEY

      gdk_flush ();
      gdk_error_trap_pop ();
    }

  while (! cancel && ((x_win == None) || (buttons != 0)))
    {
      XEvent x_event;
      gint   x, y, w, h;

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
                x_win = XmuClientWindow (x_dpy, x_win);
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
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);

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
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);

              shootvals.x2 = x_event.xmotion.x_root;
              shootvals.y2 = x_event.xmotion.y_root;

              x = MIN (shootvals.x1, shootvals.x2);
              y = MIN (shootvals.y1, shootvals.y2);
              w = ABS (shootvals.x2 - shootvals.x1);
              h = ABS (shootvals.y2 - shootvals.y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);
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
#define X_UNGRAB_KEY(index, modifiers) \
      XUngrabKey (x_dpy, keys[index].keycode, modifiers, x_root)

      for (i = 0; i < num_keys; i++)
        {
          X_UNGRAB_KEY (i, 0);
          X_UNGRAB_KEY (i, LockMask);            /* CapsLock              */
          X_UNGRAB_KEY (i, Mod2Mask);            /* NumLock               */
          X_UNGRAB_KEY (i, Mod5Mask);            /* ScrollLock            */
          X_UNGRAB_KEY (i, LockMask | Mod2Mask); /* CapsLock + NumLock    */
          X_UNGRAB_KEY (i, LockMask | Mod5Mask); /* CapsLock + ScrollLock */
          X_UNGRAB_KEY (i, Mod2Mask | Mod5Mask); /* NumLock  + ScrollLock */
          X_UNGRAB_KEY (i, LockMask | Mod2Mask | Mod5Mask); /* all        */
        }
#undef X_UNGRAB_KEY

      g_free (keys);
    }

  if (status == GrabSuccess)
    XUngrabPointer (x_dpy, CurrentTime);

  XFreeCursor (x_dpy, x_cursor);

  if (x_gc != None)
    XFreeGC (x_dpy, x_gc);

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

static gchar *
window_get_utf8_property (GdkDisplay      *display,
                          GdkNativeWindow  window,
                          const gchar     *name)
{
  gchar   *retval = NULL;

#if defined(GDK_WINDOWING_X11)
  Atom     utf8_string;
  Atom     type   = None;
  guchar  *val    = NULL;
  gulong   nitems = 0;
  gulong   after  = 0;
  gint     format = 0;

  utf8_string = gdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING");

  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), window,
                      gdk_x11_get_xatom_by_name_for_display (display, name),
                      0, G_MAXLONG, False, utf8_string,
                      &type, &format, &nitems, &after, &val);

  if (type != utf8_string || format != 8 || nitems == 0)
    {
      if (val)
        XFree (val);
      return NULL;
    }

  if (g_utf8_validate ((const gchar *) val, nitems, NULL))
    retval = g_strndup ((const gchar *) val, nitems);

  XFree (val);

#endif
  return retval;
}

static gchar *
window_get_title (GdkDisplay      *display,
                  GdkNativeWindow  window)
{
#if defined(GDK_WINDOWING_X11)
#ifdef HAVE_X11_XMU_WINUTIL_H
  window = XmuClientWindow (GDK_DISPLAY_XDISPLAY (display), window);
#endif

  return window_get_utf8_property (display, window, "_NET_WM_NAME");
#else
  return NULL;
#endif
}

static GdkRegion *
window_get_shape (GdkScreen       *screen,
                  GdkNativeWindow  window)
{
  GdkRegion  *shape = NULL;

#if defined(GDK_WINDOWING_X11) && defined(HAVE_X11_EXTENSIONS_SHAPE_H)
  XRectangle *rects;
  gint        rect_count;
  gint        rect_order;

  rects = XShapeGetRectangles (GDK_SCREEN_XDISPLAY (screen), window,
                               ShapeBounding,
                               &rect_count, &rect_order);

  if (rects)
    {
      if (rect_count > 1)
        {
          gint i;

          shape = gdk_region_new ();

          for (i = 0; i < rect_count; i++)
            {
              GdkRectangle rect = { rects[i].x,
                                    rects[i].y,
                                    rects[i].width,
                                    rects[i].height };

              gdk_region_union_with_rect (shape, &rect);
            }
        }

      XFree (rects);
    }
#endif

  return shape;
}

static void
image_select_shape (gint32     image,
                    GdkRegion *shape)
{
  GdkRectangle *rects;
  gint          num_rects;
  gint          i;

  gimp_selection_none (image);

  gdk_region_get_rectangles (shape, &rects, &num_rects);

  for (i = 0; i < num_rects; i++)
    {
      gimp_rect_select (image,
                        rects[i].x, rects[i].y, rects[i].width, rects[i].height,
                        GIMP_CHANNEL_OP_ADD, FALSE, 0);
    }

  g_free (rects);

  gimp_selection_invert (image);
}


/* Create a GimpImage from a GdkPixbuf */

static gint32
create_image (GdkPixbuf   *pixbuf,
              GdkRegion   *shape,
              const gchar *name)
{
  gint32     image;
  gint32     layer;
  gdouble    xres, yres;
  gchar     *comment;
  gint       width, height;
  gboolean   status;

  status = gimp_progress_init (_("Importing screenshot"));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);

  gimp_get_monitor_resolution (&xres, &yres);
  gimp_image_set_resolution (image, xres, yres);

  comment = gimp_get_default_comment ();
  if (comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment", GIMP_PARASITE_PERSISTENT,
                                    strlen (comment) + 1, comment);

      gimp_image_parasite_attach (image, parasite);
      gimp_parasite_free (parasite);

      g_free (comment);
    }

  layer = gimp_layer_new_from_pixbuf (image,
                                      name ? name : _("Screenshot"),
                                      pixbuf,
                                      100, GIMP_NORMAL_MODE, 0.0, 1.0);
  gimp_image_add_layer (image, layer, 0);

  if (shape && ! gdk_region_empty (shape))
    {
      image_select_shape (image, shape);

      if (! gimp_selection_is_empty (image))
        {
          gimp_layer_add_alpha (layer);
          gimp_edit_clear (layer);
          gimp_selection_none (image);
        }
    }

  gimp_image_undo_enable (image);

  return image;
}


/* The main Screenshot function */

static gint32
shoot (GdkScreen *screen)
{
  GdkDisplay   *display;
  GdkWindow    *window;
  GdkPixbuf    *screenshot;
  GdkRegion    *shape = NULL;
  GdkRectangle  rect;
  GdkRectangle  screen_rect;
  gchar        *name  = NULL;
  gint32        image;
  gint          screen_x;
  gint          screen_y;
  gint          x, y;

  /* use default screen if we are running non-interactively */
  if (screen == NULL)
    screen = gdk_screen_get_default ();

  display = gdk_screen_get_display (screen);

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

  if (! gdk_rectangle_intersect (&rect, &screen_rect, &rect))
    return -1;

  window = gdk_screen_get_root_window (screen);
  gdk_window_get_origin (window, &screen_x, &screen_y);

  screenshot = gdk_pixbuf_get_from_drawable (NULL, window, NULL,
                                             rect.x - screen_x,
                                             rect.y - screen_y,
                                             0, 0, rect.width, rect.height);

  gdk_display_beep (display);
  gdk_flush ();

  if (! screenshot)
    {
      g_message (_("There was an error taking the screenshot."));
      return -1;
    }

  if (shootvals.shoot_type == SHOOT_WINDOW)
    {
      name = window_get_title (display, shootvals.window_id);

      shape = window_get_shape (screen, shootvals.window_id);

      if (shape)
        gdk_region_offset (shape, x - rect.x, y - rect.y);
    }

  image = create_image (screenshot, shape, name);

  g_object_unref (screenshot);

  if (shape)
    gdk_region_destroy (shape);

  g_free (name);

  return image;
}

/*  Screenshot dialog  */

static void
shoot_dialog_add_hint (GtkNotebook *notebook,
                       ShootType    type,
                       const gchar *hint)
{
  GtkWidget *label;

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   hint,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);

  gtk_notebook_insert_page (notebook, label, NULL, type);
  gtk_widget_show (label);
}

static void
shoot_radio_button_toggled (GtkWidget *widget,
                            GtkWidget *notebook)
{
  gimp_radio_button_update (widget, &shootvals.shoot_type);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), shootvals.shoot_type);
}

static gboolean
shoot_dialog (GdkScreen **screen)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *notebook;
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
                                  _("S_nap"), GTK_RESPONSE_OK);

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

  /*  Hints  */
  notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                           "show-border", FALSE,
                           "show-tabs",   FALSE,
                           NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_ROOT,
                         _("After the delay, the screenshot is taken."));
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_REGION,
                         _("After the delay, drag your mouse to select "
                           "the region for the screenshot."));
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_WINDOW,
                         _("At the end of the delay, click in a window "
                           "to snap it."));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), shootvals.shoot_type);

  /*  Area  */
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
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

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
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  dragged region  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("Select a _region to grab"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_REGION);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_REGION));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  Delay  */
  frame = gimp_frame_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  spinner = gimp_spin_button_new (&adj, shootvals.select_delay,
                                  0.0, 100.0, 1.0, 5.0, 0.0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.select_delay);

  /* this is the unit label of a spinbutton */
  label = gtk_label_new (_("seconds"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

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
    }

  return run;
}


/*  delay functions  */

static void
shoot_delay (gint delay)
{
  g_timeout_add (1000, shoot_delay_callback, &delay);
  gtk_main ();
}

static gboolean
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
