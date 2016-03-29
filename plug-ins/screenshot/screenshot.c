/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 * Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 * Copyright 2012      Simone Karin Lehmann - OS X patches
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
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "screenshot.h"
#include "screenshot-osx.h"
#include "screenshot-gnome-shell.h"
#include "screenshot-x11.h"

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
#define PLUG_IN_ROLE   "gimp-screenshot"

#ifdef __GNUC__
#ifdef GDK_NATIVE_WINDOW_POINTER
#if GLIB_SIZEOF_VOID_P != 4
#warning window_id does not fit in PDB_INT32
#endif
#endif
#endif


static void                query              (void);
static void                run                (const gchar      *name,
                                               gint              nparams,
                                               const GimpParam  *param,
                                               gint             *nreturn_vals,
                                               GimpParam       **return_vals);

static GimpPDBStatusType   shoot              (GdkScreen        *screen,
                                               gint32           *image_ID);

static gboolean            shoot_dialog       (GdkScreen       **screen);
static gboolean            shoot_quit_timeout (gpointer          data);


/* Global Variables */

static ScreenshotBackend      backend      = SCREENSHOT_BACKEND_NONE;
static ScreenshotCapabilities capabilities = 0;

static ScreenshotValues shootvals =
{
  SHOOT_WINDOW, /* root window  */
#ifdef PLATFORM_OSX
  FALSE,
#else
  TRUE,         /* include WM decorations */
#endif
  0,            /* window ID    */
  0,            /* select delay */
  0,            /* coords of region dragged out by pointer */
  0,
  0,
  0,
  FALSE
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
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
    { GIMP_PDB_INT32, "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
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
                          "The plug-in takes screenshots of an "
                          "interactively selected window or of the desktop, "
                          "either the whole desktop or an interactively "
                          "selected region. When called non-interactively, it "
                          "may grab the root window or use the window-id "
                          "passed as a parameter.  The last four parameters "
                          "are optional and can be used to specify the corners "
                          "of the region to be grabbed."
#ifdef PLATFORM_OSX
                          "On Mac OS X, when called non-interactively, the plugin"
                          "only can take screenshots of the entire root window."
                          "Grabbing a window or a region is not supported"
                          "non-interactively. To grab a region or a particular"
                          "window, you need to use the interactive mode."
#endif
                          ,
                          "Sven Neumann <sven@gimp.org>, "
                          "Henrik Brix Andersen <brix@gimp.org>,"
                          "Simone Karin Lehmann",
                          "1998 - 2008",
                          "v1.1 (2008/04)",
                          N_("_Screenshot..."),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Create/Acquire");
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
  static GimpParam   values[2];
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GdkScreen         *screen   = NULL;
  gint32             image_ID;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

#ifdef PLATFORM_OSX
  if (! backend && screenshot_osx_available ())
    {
      backend      = SCREENSHOT_BACKEND_OSX;
      capabilities = screenshot_osx_get_capabilities ();
    }
#endif

  if (! backend && screenshot_gnome_shell_available ())
    {
      backend      = SCREENSHOT_BACKEND_GNOME_SHELL;
      capabilities = screenshot_gnome_shell_get_capabilities ();
    }

#ifdef GDK_WINDOWING_X11
  if (! backend && screenshot_x11_available ())
    {
      backend      = SCREENSHOT_BACKEND_X11;
      capabilities = screenshot_x11_get_capabilities ();
    }
#endif

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

      if (! (capabilities & SCREENSHOT_CAN_PICK_NONINTERACTIVELY))
        {
          if (shootvals.shoot_type == SHOOT_WINDOW ||
              shootvals.shoot_type == SHOOT_REGION)
            status = GIMP_PDB_CALLING_ERROR;
        }
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
      status = shoot (screen, &image_ID);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* Store variable states for next run */
          gimp_set_data (PLUG_IN_PROC, &shootvals, sizeof (ScreenshotValues));

          gimp_display_new (image_ID);

          /* Give some sort of feedback that the shot is done */
          if (shootvals.select_delay > 0)
            {
              gdk_display_beep (gdk_screen_get_display (screen));
              gdk_flush (); /* flush so the beep makes it to the server */
            }
        }

      /* set return values */
      *nreturn_vals = 2;

      values[1].type         = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }

  values[0].data.d_status = status;
}


/* The main Screenshot function */

static GimpPDBStatusType
shoot (GdkScreen *screen,
       gint32    *image_ID)
{
#ifdef PLATFORM_OSX
  if (backend == SCREENSHOT_BACKEND_OSX)
    return screenshot_osx_shoot (&shootvals, screen, image_ID);
#endif

  if (backend == SCREENSHOT_BACKEND_GNOME_SHELL)
    return screenshot_gnome_shell_shoot (&shootvals, screen, image_ID);

#ifdef GDK_WINDOWING_X11
  if (backend == SCREENSHOT_BACKEND_X11)
    return screenshot_x11_shoot (&shootvals, screen, image_ID);
#endif

  return GIMP_PDB_CALLING_ERROR; /* silence compiler */
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
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *notebook;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GtkWidget     *spinner;
  GdkPixbuf     *pixbuf;
  GSList        *radio_group = NULL;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Screenshot"), PLUG_IN_ROLE,
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

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);


  /*  single window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
                                               _("Take a screenshot of "
                                                 "a single _window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_WINDOW));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  window decorations  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_check_button_new_with_mnemonic (_("Include window _decoration"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    shootvals.decorate);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 24);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &shootvals.decorate);

      g_object_bind_property (button, "active",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_WINDOW);

  /*  whole screen  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
                                               _("Take a screenshot of "
                                                 "the entire _screen"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_ROOT));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  mouse pointer  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_POINTER)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_check_button_new_with_mnemonic (_("Include _mouse pointer"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    shootvals.show_cursor);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 24);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &shootvals.show_cursor);

      g_object_bind_property (button, "active",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_ROOT);

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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (shootvals.select_delay,
                        0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
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

static gboolean
shoot_quit_timeout (gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}
