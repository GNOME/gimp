/**********************************************************************
   GIMP - The GNU Image Manipulation Program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *********************************************************************/

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fractal-explorer.h"
#include "fractal-explorer-dialogs.h"

#include "libgimp/stdplugins-intl.h"


#define ZOOM_UNDO_SIZE 100


static GeglColor       **gradient_samples = NULL;
static GimpGradient     *gradient = NULL;
static gboolean          ready_now = FALSE;
static gchar            *tpath = NULL;
static GtkWidget        *cmap_preview;
static GtkWidget        *maindlg;

static explorer_vals_t  zooms[ZOOM_UNDO_SIZE];
static gint             zoomindex = 0;
static gint             zoommax = 0;

static gint              oldxpos = -1;
static gint              oldypos = -1;
static gdouble           x_press = -1.0;
static gdouble           y_press = -1.0;

static explorer_vals_t standardvals =
{
  0,
  -2.0,
  2.0,
  -1.5,
  1.5,
  50.0,
  -0.75,
  -0.2,
  0,
  1.0,
  1.0,
  1.0,
  1,
  1,
  0,
  0,
  0,
  0,
  1,
  256,
  0
};

/**********************************************************************
 FORWARD DECLARATIONS
 *********************************************************************/

static void update_previews            (GimpProcedureConfig *config);
static void load_file_chooser_response (GtkFileChooser      *chooser,
                                        gint                 response_id,
                                        gpointer             data);
static void save_file_chooser_response (GtkFileChooser      *chooser,
                                        gint                 response_id,
                                        gpointer             data);
static void create_load_file_chooser   (GtkWidget           *widget,
                                        GtkWidget           *dialog);
static void create_save_file_chooser   (GtkWidget           *widget,
                                        GtkWidget           *dialog);

static void cmap_preview_size_allocate (GtkWidget           *widget,
                                        GtkAllocation       *allocation,
                                        GimpProcedureConfig *config);

static void set_grad_data_cache        (GimpGradient        *in_gradient,
                                        gint                 n_colors);

/**********************************************************************
 CALLBACKS
 *********************************************************************/

/* Update both previews: image and colormap
 * Handler for notify signals, and also called directly.
 */
static void
update_previews (GimpProcedureConfig *config)
{
  set_cmap_preview (config);
  dialog_update_preview (config);
}

static void
dialog_redraw_callback (GtkWidget *widget,
                        gpointer   data)
{
  GimpProcedureConfig *config     = (GimpProcedureConfig *) data;
  gint                 alwaysprev = wvals.alwayspreview;

  wvals.alwayspreview = TRUE;
  update_previews (config);
  wvals.alwayspreview = alwaysprev;
}

static void
dialog_undo_zoom_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  if (zoomindex > 0)
    {
      g_object_get (config,
                    "xmin", &wvals.xmin,
                    "xmax", &wvals.xmax,
                    "ymin", &wvals.ymin,
                    "ymax", &wvals.ymax,
                    NULL);

      zooms[zoomindex] = wvals;
      zoomindex--;
      wvals = zooms[zoomindex];

      g_object_set (config,
                    "xmin", wvals.xmin,
                    "xmax", wvals.xmax,
                    "ymin", wvals.ymin,
                    "ymax", wvals.ymax,
                    NULL);

      update_previews (config);
    }
}

static void
dialog_redo_zoom_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  if (zoomindex < zoommax)
    {
      zoomindex++;
      wvals = zooms[zoomindex];
      g_object_set (config,
                    "xmin", wvals.xmin,
                    "xmax", wvals.xmax,
                    "ymin", wvals.ymin,
                    "ymax", wvals.ymax,
                    NULL);

      update_previews (config);
    }
}

static void
dialog_step_in_callback (GtkWidget *widget,
                         gpointer   data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;
  gdouble              xdifferenz;
  gdouble              ydifferenz;
  gdouble              xmin;
  gdouble              xmax;
  gdouble              ymin;
  gdouble              ymax;

  g_object_get (config,
                "xmin", &xmin,
                "xmax", &xmax,
                "ymin", &ymin,
                "ymax", &ymax,
                NULL);

  if (zoomindex < ZOOM_UNDO_SIZE - 1)
    {
      g_object_get (config,
                    "xmin", &wvals.xmin,
                    "xmax", &wvals.xmax,
                    "ymin", &wvals.ymin,
                    "ymax", &wvals.ymax,
                    NULL);

      zooms[zoomindex] = wvals;
      zoomindex++;
    }
  zoommax = zoomindex;

  xdifferenz =  xmax - xmin;
  ydifferenz =  ymax - ymin;
  xmin += 1.0 / 6.0 * xdifferenz;
  ymin += 1.0 / 6.0 * ydifferenz;
  xmax -= 1.0 / 6.0 * xdifferenz;
  ymax -= 1.0 / 6.0 * ydifferenz;

  wvals.xmin = xmin;
  wvals.xmax = xmax;
  wvals.ymin = ymin;
  wvals.ymax = ymax;
  zooms[zoomindex] = wvals;

  g_object_set (config,
                "xmin", xmin,
                "xmax", xmax,
                "ymin", ymin,
                "ymax", ymax,
                NULL);

  update_previews (config);
}

static void
dialog_step_out_callback (GtkWidget *widget,
                          gpointer   data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;
  gdouble              xdifferenz;
  gdouble              ydifferenz;
  gdouble              xmin;
  gdouble              xmax;
  gdouble              ymin;
  gdouble              ymax;

  g_object_get (config,
                "xmin", &xmin,
                "xmax", &xmax,
                "ymin", &ymin,
                "ymax", &ymax,
                NULL);

  if (zoomindex < ZOOM_UNDO_SIZE - 1)
    {
      g_object_get (config,
                    "xmin", &wvals.xmin,
                    "xmax", &wvals.xmax,
                    "ymin", &wvals.ymin,
                    "ymax", &wvals.ymax,
                    NULL);

      zooms[zoomindex] = wvals;
      zoomindex++;
    }
  zoommax = zoomindex;

  xdifferenz =  xmax - xmin;
  ydifferenz =  ymax - ymin;
  xmin -= 1.0 / 4.0 * xdifferenz;
  ymin -= 1.0 / 4.0 * ydifferenz;
  xmax += 1.0 / 4.0 * xdifferenz;
  ymax += 1.0 / 4.0 * ydifferenz;

  wvals.xmin = xmin;
  wvals.xmax = xmax;
  wvals.ymin = ymin;
  wvals.ymax = ymax;
  zooms[zoomindex] = wvals;

  g_object_set (config,
                "xmin", xmin,
                "xmax", xmax,
                "ymin", ymin,
                "ymax", ymax,
                NULL);

  update_previews (config);
}

static void
explorer_toggle_update (GtkWidget *widget,
                        gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  update_previews (wvals.config);
}

static void
explorer_radio_update (GtkWidget *widget,
                       gpointer   data)
{
  gboolean c_sensitive;
  gint     fractal_type;

  fractal_type =
    gimp_procedure_config_get_choice_id (wvals.config, "fractal-type");

  switch (fractal_type)
    {
    case TYPE_MANDELBROT:
    case TYPE_SIERPINSKI:
      c_sensitive = FALSE;
      break;

    default:
      c_sensitive = TRUE;
      break;
    }

  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (maindlg),
                                       "cx",
                                       c_sensitive, NULL, NULL, FALSE);
  gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (maindlg),
                                       "cy",
                                       c_sensitive, NULL, NULL, FALSE);

  update_previews (wvals.config);
}

static void
explorer_number_of_colors_callback (GimpProcedureConfig *config)
{
  gint n_colors;

  g_object_get (config,
                "n-colors", &n_colors,
                NULL);
  set_grad_data_cache (gradient, n_colors);

  update_previews (config);
}

/* Same signature as all GimpResourceSelectButton */
static void
explorer_gradient_select_callback (gpointer      data,  /* config */
                                   GimpGradient *gradient,
                                   gboolean      dialog_closing)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;
  gint n_colors;
  gint color_mode;

  g_object_get (config,
                "n-colors",   &n_colors,
                NULL);
  color_mode = gimp_procedure_config_get_choice_id (config, "color-mode");
  set_grad_data_cache (gradient, n_colors);

  if (color_mode == 1)
    {
      update_previews (config);
    }
}

static void
preview_draw_crosshair (gint px,
                        gint py)
{
  gint     x, y;
  guchar  *p_ul;

  p_ul = wint.wimage + 3 * (preview_width * py + 0);

  for (x = 0; x < preview_width; x++)
    {
      p_ul[0] ^= 254;
      p_ul[1] ^= 254;
      p_ul[2] ^= 254;
      p_ul += 3;
    }

  p_ul = wint.wimage + 3 * (preview_width * 0 + px);

  for (y = 0; y < preview_height; y++)
    {
      p_ul[0] ^= 254;
      p_ul[1] ^= 254;
      p_ul[2] ^= 254;
      p_ul += 3 * preview_width;
    }
}

static void
preview_redraw (void)
{
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (wint.preview),
                          0, 0, preview_width, preview_height,
                          GIMP_RGB_IMAGE,
                          wint.wimage, preview_width * 3);

  gtk_widget_queue_draw (wint.preview);
}

static gboolean
preview_button_press_event (GtkWidget      *widget,
                            GdkEventButton *event)
{
  if (event->button == 1)
    {
      x_press = event->x;
      y_press = event->y;
      xbild = preview_width;
      ybild = preview_height;
      xdiff = (xmax - xmin) / xbild;
      ydiff = (ymax - ymin) / ybild;

      preview_draw_crosshair (x_press, y_press);
      preview_redraw ();
    }
  return TRUE;
}

static gboolean
preview_motion_notify_event (GtkWidget      *widget,
                             GdkEventButton *event)
{
  if (oldypos != -1)
    {
      preview_draw_crosshair (oldxpos, oldypos);
    }

  oldxpos = event->x;
  oldypos = event->y;

  if ((oldxpos >= 0.0) &&
      (oldypos >= 0.0) &&
      (oldxpos < preview_width) &&
      (oldypos < preview_height))
    {
      preview_draw_crosshair (oldxpos, oldypos);
    }
  else
    {
      oldypos = -1;
      oldxpos = -1;
    }

  preview_redraw ();

  return TRUE;
}

static gboolean
preview_leave_notify_event (GtkWidget      *widget,
                            GdkEventButton *event)
{
  if (oldypos != -1)
    {
      preview_draw_crosshair (oldxpos, oldypos);
    }
  oldxpos = -1;
  oldypos = -1;

  preview_redraw ();

  gdk_window_set_cursor (gtk_widget_get_window (maindlg), NULL);

  return TRUE;
}

static gboolean
preview_enter_notify_event (GtkWidget      *widget,
                            GdkEventButton *event)
{
  static GdkCursor *cursor = NULL;

  if (! cursor)
    {
      GdkDisplay *display = gtk_widget_get_display (maindlg);

      cursor = gdk_cursor_new_for_display (display, GDK_TCROSS);

    }

  gdk_window_set_cursor (gtk_widget_get_window (maindlg), cursor);

  return TRUE;
}

static gboolean
preview_button_release_event (GtkWidget      *widget,
                              GdkEventButton *event,
                              gpointer        data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;
  gdouble l_xmin;
  gdouble l_xmax;
  gdouble l_ymin;
  gdouble l_ymax;
  gdouble xmin;
  gdouble xmax;
  gdouble ymin;
  gdouble ymax;

  g_object_get (config,
                "xmin", &xmin,
                "xmax", &xmax,
                "ymin", &ymin,
                "ymax", &ymax,
                NULL);

  if (event->button == 1)
    {
      gdouble x_release, y_release;

      x_release = event->x;
      y_release = event->y;

      if ((x_press >= 0.0) && (y_press >= 0.0) &&
          (x_release >= 0.0) && (y_release >= 0.0) &&
          (x_press < preview_width) && (y_press < preview_height) &&
          (x_release < preview_width) && (y_release < preview_height))
        {
          l_xmin = (xmin + (xmax -xmin) * (x_press / preview_width));
          l_xmax = (xmin + (xmax - xmin) * (x_release / preview_width));
          l_ymin = (ymin + (ymax - ymin) * (y_press / preview_height));
          l_ymax = (ymin + (ymax - ymin) * (y_release / preview_height));

          if (zoomindex < ZOOM_UNDO_SIZE - 1)
            {
              wvals.xmin = l_xmin;
              wvals.xmax = l_xmax;
              wvals.ymin = l_ymin;
              wvals.ymax = l_ymax;
              zooms[zoomindex] = wvals;
              zoomindex++;
            }
          zoommax = zoomindex;
          g_object_set (config,
                "xmin", l_xmin,
                "xmax", l_xmax,
                "ymin", l_ymin,
                "ymax", l_ymax,
                NULL);

          dialog_update_preview (config);
          oldypos = oldxpos = -1;
        }
    }

  return TRUE;
}

/**********************************************************************
 FUNCTION: explorer_dialog
 *********************************************************************/

gint
explorer_dialog (GimpProcedure       *procedure,
                 GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GtkWidget *top_hbox;
  GtkWidget *left_vbox;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *gradient_button;
  gchar     *path;
  gint       n_colors;

  gimp_ui_init (PLUG_IN_BINARY);

  path = gimp_gimprc_query ("fractalexplorer-path");

  if (path)
    {
      fractalexplorer_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }
  else
    {
      GFile *gimprc    = gimp_directory_file ("gimprc", NULL);
      gchar *full_path = gimp_config_build_data_path ("fractalexplorer");
      gchar *esc_path  = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 "fractalexplorer-path",
                 "fractalexplorer-path",
                 esc_path, gimp_file_get_utf8_name (gimprc));

      g_object_unref (gimprc);
      g_free (esc_path);
    }

  wint.wimage = g_new (guchar, preview_width * preview_height * 3);

  dialog = maindlg = gimp_procedure_dialog_new (procedure,
                                                GIMP_PROCEDURE_CONFIG (config),
                                                _("Fractal Explorer"));

  top_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_vexpand (top_hbox, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (top_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      top_hbox, FALSE, FALSE, 0);
  gtk_widget_show (top_hbox);

  left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_show (left_vbox);

  /*  Preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (left_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_widget_set_halign (frame, GTK_ALIGN_START);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  wint.preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (wint.preview, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), wint.preview);

  g_signal_connect (wint.preview, "button-press-event",
                    G_CALLBACK (preview_button_press_event),
                    NULL);
  g_signal_connect (wint.preview, "button-release-event",
                    G_CALLBACK (preview_button_release_event),
                    config);
  g_signal_connect (wint.preview, "motion-notify-event",
                    G_CALLBACK (preview_motion_notify_event),
                    NULL);
  g_signal_connect (wint.preview, "leave-notify-event",
                    G_CALLBACK (preview_leave_notify_event),
                    NULL);
  g_signal_connect (wint.preview, "enter-notify-event",
                    G_CALLBACK (preview_enter_notify_event),
                    NULL);

  gtk_widget_set_events (wint.preview, (GDK_BUTTON_PRESS_MASK |
                                        GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK |
                                        GDK_LEAVE_NOTIFY_MASK |
                                        GDK_ENTER_NOTIFY_MASK));
  gtk_widget_show (wint.preview);

  toggle = gtk_check_button_new_with_mnemonic (_("Re_altime preview"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.alwayspreview);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                wvals.alwayspreview);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("If enabled the preview will "
                                     "be redrawn automatically"), NULL);

  button = gtk_button_new_with_mnemonic (_("R_edraw preview"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_redraw_callback),
                    config);
  gtk_widget_show (button);

  /*  Zoom Options  */
  frame = gimp_frame_new (_("Zoom"));
  gtk_box_pack_start (GTK_BOX (left_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (bbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("Zoom _In"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_step_in_callback),
                    config);

  button = gtk_button_new_with_mnemonic (_("Zoom _Out"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_step_out_callback),
                    config);

  bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (bbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("_Undo"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Undo last zoom change"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_undo_zoom_callback),
                    config);

  button = gtk_button_new_with_mnemonic (_("_Redo"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Redo last zoom change"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_redo_zoom_callback),
                    config);

  /*  Create notebook  */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "parameter-tab", _("_Parameters"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "color-tab", _("_Colors"), FALSE, TRUE);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "fractal-tab", _("_Fractals"), FALSE, TRUE);

  /*  "Parameters" page  */
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "xmin", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "xmax", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "ymin", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "ymax", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "iter", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "cx", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "cy", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "parameter-input-label", _("Fractal Parameters"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "parameter-input-box",
                                  "xmin", "xmax", "ymin", "ymax",
                                  "iter", "cx", "cy",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "parameter-input-frame",
                                    "parameter-input-label", FALSE,
                                    "parameter-input-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "parameter-type-label", _("Fractal Type"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "parameter-type-box",
                                  "fractal-type",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "parameter-type-frame",
                                    "parameter-type-label", FALSE,
                                    "parameter-type-box");

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "parameter-box",
                                         "parameter-input-frame",
                                         "parameter-type-frame",
                                         NULL);

  bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top (bbox, 12);
  gtk_box_set_homogeneous (GTK_BOX (bbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, TRUE, TRUE, 2);
  gtk_box_reorder_child (GTK_BOX (vbox), bbox, 1);
  gtk_widget_set_visible (bbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_load_file_chooser),
                    dialog);
  gtk_widget_set_visible (button, TRUE);
  gimp_help_set_help_data (button, _("Load a fractal from file"), NULL);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_save_file_chooser),
                    dialog);
  gtk_widget_set_visible (button, TRUE);
  gimp_help_set_help_data (button, _("Save active fractal to file"), NULL);

  g_signal_connect (config, "notify::xmin",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::xmax",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::ymin",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::ymax",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::iter",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::cx",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::cy",
                    G_CALLBACK (update_previews),
                    config);

  g_signal_connect (config, "notify::fractal-type",
                    G_CALLBACK (explorer_radio_update),
                    config);
  explorer_radio_update (NULL, NULL);

  /* Colors Page */
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "n-colors", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "num-colors-label", _("Number of Colors"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "num-colors-box",
                                      "n-colors",
                                      "use-loglog-smoothing",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "num-colors-frame",
                                    "num-colors-label", FALSE,
                                    "num-colors-box");

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "red-stretch", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "green-stretch", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                        "blue-stretch", 1.0);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "color-density-label", _("Color Density"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "color-density-box",
                                      "red-stretch",
                                      "green-stretch",
                                      "blue-stretch",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "color-density-frame",
                                    "color-density-label", FALSE,
                                    "color-density-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "color-function-label", _("Color Function"),
                                   FALSE, FALSE);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "red-function-label", _("Red"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "red-function-box",
                                      "red-mode",
                                      "red-invert",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "red-function-frame",
                                    "red-function-label", FALSE,
                                    "red-function-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "green-function-label", _("Green"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "green-function-box",
                                      "green-mode",
                                      "green-invert",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "green-function-frame",
                                    "green-function-label", FALSE,
                                    "green-function-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "blue-function-label", _("Blue"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                      "blue-function-box",
                                      "blue-mode",
                                      "blue-invert",
                                      NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "blue-function-frame",
                                    "blue-function-label", FALSE,
                                    "blue-function-box");

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "function-hbox",
                                         "red-function-frame",
                                         "green-function-frame",
                                         "blue-function-frame",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "color-function-frame",
                                    "color-function-label", FALSE,
                                    "function-hbox");

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "color-mode", GIMP_TYPE_INT_RADIO_FRAME);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "color-mode-hbox",
                                         "color-mode",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  g_object_get (config,
                "n-colors", &n_colors,
                NULL);
  /* Pass NULL to set local gradient data from context. */
  set_grad_data_cache (NULL, n_colors);

  gradient_button = gimp_gradient_chooser_new (_("FractalExplorer Gradient"), NULL, gradient);
  g_signal_connect_swapped (gradient_button, "resource-set",
                            G_CALLBACK (explorer_gradient_select_callback), config);
  gtk_box_pack_start (GTK_BOX (hbox), gradient_button, TRUE, TRUE, 0);
  gtk_widget_set_visible (gradient_button, TRUE);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "color-box",
                                         "num-colors-frame",
                                         "color-density-frame",
                                         "color-function-frame",
                                         "color-mode-hbox",
                                         NULL);

  cmap_preview = gimp_preview_area_new ();
  gtk_widget_set_halign (cmap_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (cmap_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (cmap_preview, 32, 32);
  gtk_box_pack_start (GTK_BOX (vbox), cmap_preview, TRUE, TRUE, 0);
  g_signal_connect (cmap_preview, "size-allocate",
                    G_CALLBACK (cmap_preview_size_allocate), config);
  gtk_widget_set_visible (cmap_preview, TRUE);

  g_signal_connect (config, "notify::n-colors",
                    G_CALLBACK (explorer_number_of_colors_callback),
                    config);
  g_signal_connect (config, "notify::use-loglog-smoothing",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::red-stretch",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::blue-stretch",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::green-stretch",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::red-mode",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::green-mode",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::blue-mode",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::red-invert",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::green-invert",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::blue-invert",
                    G_CALLBACK (update_previews),
                    config);
  g_signal_connect (config, "notify::color-mode",
                    G_CALLBACK (update_previews),
                    config);
  /* Fractal Presets */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "preset-label", _("Presets"),
                                   FALSE, FALSE);
  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "fractal-box",
                                         "preset-label",
                                         NULL);
  frame = add_objects_list ();
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 2);
  gtk_widget_set_visible (frame, TRUE);

  /* Creating layout */
  gimp_procedure_dialog_fill_notebook (GIMP_PROCEDURE_DIALOG (dialog),
                                       "main-notebook",
                                       "parameter-tab", "parameter-box",
                                       "color-tab", "color-box",
                                       "fractal-tab", "fractal-box",
                                       NULL);

  top_hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                             "top-hbox",
                                             "main-notebook",
                                             NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (top_hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_vexpand (top_hbox, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (top_hbox), 12);
  gtk_box_pack_start (GTK_BOX (top_hbox), left_vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (top_hbox), left_vbox, 0);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "top-hbox", NULL);

  gtk_widget_show (dialog);
  ready_now = TRUE;

  update_previews (config);

  wint.run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  g_free (wint.wimage);

  return wint.run;
}


/**********************************************************************
 FUNCTION: dialog_update_preview
 *********************************************************************/

void
dialog_update_preview (GimpProcedureConfig *config)
{
  gint     ycoord;
  guchar  *p_ul;

  if (NULL == wint.preview)
    return;

  if (ready_now && wvals.alwayspreview)
    {
      g_object_get (config,
                    "xmin", &xmin,
                    "xmax", &xmax,
                    "ymin", &ymin,
                    "ymax", &ymax,
                    NULL);
      xbild = preview_width;
      ybild = preview_height;
      xdiff = (xmax - xmin) / xbild;
      ydiff = (ymax - ymin) / ybild;

      p_ul = wint.wimage;

      for (ycoord = 0; ycoord < preview_height; ycoord++)
        {
          explorer_render_row (NULL,
                               p_ul,
                               ycoord,
                               preview_width,
                               3,
                               config);
          p_ul += preview_width * 3;
        }

      preview_redraw ();
    }
}

/**********************************************************************
 FUNCTION: cmap_preview_size_allocate()
 *********************************************************************/

/* Handler for size-allocate.
 * This:
 *    - transfers internal model colormap to the widget
 *    - redraws.
 */
static void
cmap_preview_size_allocate (GtkWidget           *widget,
                            GtkAllocation       *allocation,
                            GimpProcedureConfig *config)
{
  gint             i;
  gint             x;
  gint             y;
  gint             j;
  gint             n_colors;
  guchar          *b;
  GimpPreviewArea *preview = GIMP_PREVIEW_AREA (widget);

  g_object_get (config,
                "n-colors", &n_colors,
                NULL);

  b = g_new (guchar, allocation->width * allocation->height * 3);

  for (y = 0; y < allocation->height; y++)
    {
      for (x = 0; x < allocation->width; x++)
        {
          i = x + (y / 4) * allocation->width;
          if (i > n_colors)
            {
              for (j = 0; j < 3; j++)
                b[(y*allocation->width + x) * 3 + j] = 0;
            }
          else
            {
              b[(y*allocation->width + x) * 3]     = colormap[i].r;
              b[(y*allocation->width + x) * 3 + 1] = colormap[i].g;
              b[(y*allocation->width + x) * 3 + 2] = colormap[i].b;
            }
        }
    }
  gimp_preview_area_draw (preview,
                          0, 0, allocation->width, allocation->height,
                          GIMP_RGB_IMAGE, b, allocation->width*3);
  gtk_widget_queue_draw (cmap_preview);

  g_free (b);

}

/**********************************************************************
 FUNCTION: set_cmap_preview()
 *********************************************************************/

void
set_cmap_preview (GimpProcedureConfig *config)
{
  gint xsize;
  gint ysize;
  gint n_colors;

  g_object_get (config,
                "n-colors", &n_colors,
                NULL);

  if (NULL == cmap_preview)
    return;

  /* Change the internal model.  Does not write to widget's model. */
  make_color_map (config);

  for (ysize = 1; ysize * ysize * ysize < n_colors; ysize++)
    /**/;
  xsize = n_colors / ysize;
  while (xsize * ysize < n_colors)
    xsize++;

  /* Set minimum size. Initialize to 0, 0 to force a redraw. */
  gtk_widget_set_size_request (cmap_preview, 0, 0);
  gtk_widget_set_size_request (cmap_preview, xsize, ysize * 4);
}

/**********************************************************************
 FUNCTION: make_color_map()
 *********************************************************************/

void
make_color_map (GimpProcedureConfig *config)
{
  const Babl *colormap_format = babl_format ("R'G'B' u8");
  gint        i;
  gint        r;
  gint        gr;
  gint        bl;
  gdouble     redstretch;
  gdouble     greenstretch;
  gdouble     bluestretch;
  gdouble     pi = atan (1) * 4;
  gint        n_colors;
  gint        color_mode;
  gint        red_mode;
  gint        green_mode;
  gint        blue_mode;
  gboolean    red_invert;
  gboolean    green_invert;
  gboolean    blue_invert;

  g_object_get (config,
                "n-colors",      &n_colors,
                "red-stretch",   &redstretch,
                "green-stretch", &greenstretch,
                "blue-stretch",  &bluestretch,
                "red-invert",    &red_invert,
                "green-invert",  &green_invert,
                "blue-invert",   &blue_invert,
                NULL);
  red_mode    = gimp_procedure_config_get_choice_id (config, "red-mode");
  green_mode  = gimp_procedure_config_get_choice_id (config, "green-mode");
  blue_mode   = gimp_procedure_config_get_choice_id (config, "blue-mode");
  color_mode  = gimp_procedure_config_get_choice_id (config, "color-mode");

  /*  get gradient samples if they don't exist -- fixes gradient color
   *  mode for noninteractive use (bug #103470).
   */
  if (gradient_samples == NULL)
    {
      GimpGradient *gradient = gimp_context_get_gradient ();

      gradient_samples = gimp_gradient_get_uniform_samples (gradient,
                                                            n_colors, FALSE);
    }

  redstretch   *= 127.5;
  greenstretch *= 127.5;
  bluestretch  *= 127.5;

  for (i = 0; i < n_colors; i++)
    if (color_mode == 1)
      {
        gegl_color_get_pixel (gradient_samples[i], colormap_format, (void *) &(colormap[i]));
      }
    else
      {
        double x = (i*2.0) / n_colors;
        r = gr = bl = 0;

        switch (red_mode)
          {
          case SINUS:
            r = (int) redstretch *(1.0 + sin((x - 1) * pi));
            break;
          case COSINUS:
            r = (int) redstretch *(1.0 + cos((x - 1) * pi));
            break;
          case NONE:
            r = (int)(redstretch *(x));
            break;
          default:
            break;
          }

        switch (green_mode)
          {
          case SINUS:
            gr = (int) greenstretch *(1.0 + sin((x - 1) * pi));
            break;
          case COSINUS:
            gr = (int) greenstretch *(1.0 + cos((x - 1) * pi));
            break;
          case NONE:
            gr = (int)(greenstretch *(x));
            break;
          default:
            break;
          }

        switch (blue_mode)
          {
          case SINUS:
            bl = (int) bluestretch * (1.0 + sin ((x - 1) * pi));
            break;
          case COSINUS:
            bl = (int) bluestretch * (1.0 + cos ((x - 1) * pi));
            break;
          case NONE:
            bl = (int) (bluestretch * x);
            break;
          default:
            break;
          }

        r  = MIN (r,  255);
        gr = MIN (gr, 255);
        bl = MIN (bl, 255);

        if (red_invert)
          r = 255 - r;

        if (green_invert)
          gr = 255 - gr;

        if (blue_invert)
          bl = 255 - bl;

        colormap[i].r = r;
        colormap[i].g = gr;
        colormap[i].b = bl;
      }
}

/**********************************************************************
 FUNCTION: save_options
 *********************************************************************/

static void
save_options (FILE * fp)
{
  gchar    buf[64];
  gint     fractal_type;
  gint     red_mode;
  gint     green_mode;
  gint     blue_mode;
  gint     xmin;
  gint     xmax;
  gint     ymin;
  gint     ymax;
  gint     iter;
  gint     cx;
  gint     cy;
  gdouble  red_stretch;
  gdouble  green_stretch;
  gdouble  blue_stretch;
  gboolean red_invert;
  gboolean green_invert;
  gboolean blue_invert;
  gint     color_mode;

  g_object_get (wvals.config,
                "xmin",          &xmin,
                "xmax",          &xmax,
                "ymin",          &ymin,
                "ymax",          &ymax,
                "iter",          &iter,
                "cx",            &cx,
                "cy",            &cy,
                "red-stretch",   &red_stretch,
                "green-stretch", &green_stretch,
                "blue-stretch",  &blue_stretch,
                "red-invert",    &red_invert,
                "green-invert",  &green_invert,
                "blue-invert",   &blue_invert,
                NULL);
  fractal_type =
    gimp_procedure_config_get_choice_id (wvals.config, "fractal-type");
  red_mode = gimp_procedure_config_get_choice_id (wvals.config, "red-mode");
  green_mode = gimp_procedure_config_get_choice_id (wvals.config, "green-mode");
  blue_mode = gimp_procedure_config_get_choice_id (wvals.config, "blue-mode");
  color_mode = gimp_procedure_config_get_choice_id (wvals.config, "color-mode");

  /* Save options */

  fprintf (fp, "fractaltype: %i\n", fractal_type);

  g_ascii_dtostr (buf, sizeof (buf), xmin);
  fprintf (fp, "xmin: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), xmax);
  fprintf (fp, "xmax: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), ymin);
  fprintf (fp, "ymin: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), ymax);
  fprintf (fp, "ymax: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), iter);
  fprintf (fp, "iter: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), cx);
  fprintf (fp, "cx: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), cy);
  fprintf (fp, "cy: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), red_stretch * 128.0);
  fprintf (fp, "redstretch: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), green_stretch * 128.0);
  fprintf (fp, "greenstretch: %s\n", buf);

  g_ascii_dtostr (buf, sizeof (buf), blue_stretch * 128.0);
  fprintf (fp, "bluestretch: %s\n", buf);

  fprintf (fp, "redmode: %i\n", red_mode);
  fprintf (fp, "greenmode: %i\n", green_mode);
  fprintf (fp, "bluemode: %i\n", blue_mode);
  fprintf (fp, "redinvert: %i\n", red_invert);
  fprintf (fp, "greeninvert: %i\n", green_invert);
  fprintf (fp, "blueinvert: %i\n", blue_invert);
  fprintf (fp, "colormode: %i\n", color_mode);
  fputs ("#**********************************************************************\n", fp);
  fprintf(fp, "<EOF>\n");
  fputs ("#**********************************************************************\n", fp);
}

static void
save_callback (void)
{
  FILE        *fp;
  const gchar *savename = filename;

  fp = g_fopen (savename, "wt+");

  if (!fp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (savename), g_strerror (errno));
      return;
    }

  /* Write header out */
  fputs (FRACTAL_HEADER, fp);
  fputs ("#**********************************************************************\n", fp);
  fputs ("# This is a data file for the Fractal Explorer plug-in for GIMP       *\n", fp);
  fputs ("#**********************************************************************\n", fp);

  save_options (fp);

  if (ferror (fp))
    g_message (_("Could not write '%s': %s"),
               gimp_filename_to_utf8 (savename), g_strerror (ferror (fp)));

  fclose (fp);
}

static void
save_file_chooser_response (GtkFileChooser *chooser,
                            gint            response_id,
                            gpointer        data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (chooser);

      save_callback ();
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static void
file_chooser_set_default_folder (GtkFileChooser *chooser)
{
  GList *path_list;
  gchar *dir;

  if (! fractalexplorer_path)
    return;

  path_list = gimp_path_parse (fractalexplorer_path, 256, FALSE, NULL);

  dir = gimp_path_get_user_writable_dir (path_list);

  if (! dir)
    dir = g_strdup (gimp_directory ());

  gtk_file_chooser_set_current_folder (chooser, dir);

  g_free (dir);
  gimp_path_free (path_list);
}

static void
load_file_chooser_response (GtkFileChooser *chooser,
                            gint            response_id,
                            gpointer        data)
{
  GimpProcedureConfig *config = (GimpProcedureConfig *) data;

  if (response_id == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (chooser);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          explorer_load ();
        }

      gtk_widget_show (maindlg);
      update_previews (config);
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static void
create_load_file_chooser (GtkWidget *widget,
                          GtkWidget *dialog)
{
  static GtkWidget    *window = NULL;
  GimpProcedureConfig *config;

  g_object_get (GIMP_PROCEDURE_DIALOG (dialog),
                "config", &config,
                NULL);

  if (!window)
    {
      window =
        gtk_file_chooser_dialog_new (_("Load Fractal Parameters"),
                                     GTK_WINDOW (dialog),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (window),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      file_chooser_set_default_folder (GTK_FILE_CHOOSER (window));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);
      g_signal_connect (window, "response",
                        G_CALLBACK (load_file_chooser_response),
                        config);
    }

  gtk_window_present (GTK_WINDOW (window));
}

static void
create_save_file_chooser (GtkWidget *widget,
                          GtkWidget *dialog)
{
  static GtkWidget *window = NULL;

  if (! window)
    {
      window =
        gtk_file_chooser_dialog_new (_("Save Fractal Parameters"),
                                     GTK_WINDOW (dialog),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (window),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (window),
                                                      TRUE);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);
      g_signal_connect (window, "response",
                        G_CALLBACK (save_file_chooser_response),
                        window);
    }

  if (tpath)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (window), tpath);
    }
  else
    {
      file_chooser_set_default_folder (GTK_FILE_CHOOSER (window));
    }

  gtk_window_present (GTK_WINDOW (window));
}

/* Set cache of gradient and samples from it.
 *
 * The cache is in 2 global variables:
 * gradient, gradient_samples.
 * This keeps the three variables coherent.
 *
 * !!! There is one other writer of gradient_samples, see below.
 * For NON-INTERACTIVE use, global gradient can remain NULL
 * while gradient_samples is not.
 *
 * When in_gradient is NULL, uses gradient from context.
 *
 * In some cases, the gradient is a reference to a proxy
 * received from a temporary procedure callback.
 * Since we are keeping a reference, and can pass it to a gradient chooser,
 * the plugin machinery must not destroy the proxy object until this plugin exits.
 */
static void
set_grad_data_cache (GimpGradient *in_gradient,
                     gint          n_colors)
{
  /* Set the global gradient. */
  if (in_gradient == NULL)
    gradient = gimp_context_get_gradient ();
  else
    gradient = in_gradient;

  /* Refresh the global cache of samples. */
  if (gradient_samples != NULL)
    gimp_color_array_free (gradient_samples);

  gradient_samples = gimp_gradient_get_uniform_samples (gradient,
                                                        n_colors, FALSE);
}

gchar*
get_line (gchar *buf,
          gint   s,
          FILE  *from,
          gint   init)
{
  gint   slen;
  gchar *ret;

  if (init)
    line_no = 1;
  else
    line_no++;

  do
    {
      ret = fgets (buf, s, from);
    }
  while (!ferror (from) && buf[0] == '#');

  slen = strlen (buf);

  /* The last newline is a pain */
  if (slen > 0)
    buf[slen - 1] = '\0';

  if (ferror (from))
    {
      g_warning ("Error reading file");
      return NULL;
    }

  return ret;
}

gint
load_options (fractalexplorerOBJ *xxx,
              FILE               *fp)
{
  gchar load_buf[MAX_LOAD_LINE];
  gchar str_buf[MAX_LOAD_LINE];
  gchar opt_buf[MAX_LOAD_LINE];

  /*  default values  */
  xxx->opts            = standardvals;
  xxx->opts.gradinvert = FALSE;

  get_line (load_buf, MAX_LOAD_LINE, fp, 0);

  while (!feof (fp) && strcmp (load_buf, "<EOF>"))
    {
      /* Get option name */
      sscanf (load_buf, "%255s %255s", str_buf, opt_buf);

      if (!strcmp (str_buf, "fractaltype:"))
        {
          gint sp = 0;

          sp = atoi (opt_buf);
          if (sp < 0)
            return -1;
          xxx->opts.fractaltype = sp;
        }
      else if (!strcmp (str_buf, "xmin:"))
        {
          xxx->opts.xmin = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp (str_buf, "xmax:"))
        {
          xxx->opts.xmax = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp(str_buf, "ymin:"))
        {
          xxx->opts.ymin = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp (str_buf, "ymax:"))
        {
          xxx->opts.ymax = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp(str_buf, "redstretch:"))
        {
          gdouble sp = g_ascii_strtod (opt_buf, NULL);
          xxx->opts.redstretch = sp / 128.0;
        }
      else if (!strcmp(str_buf, "greenstretch:"))
        {
          gdouble sp = g_ascii_strtod (opt_buf, NULL);
          xxx->opts.greenstretch = sp / 128.0;
        }
      else if (!strcmp (str_buf, "bluestretch:"))
        {
          gdouble sp = g_ascii_strtod (opt_buf, NULL);
          xxx->opts.bluestretch = sp / 128.0;
        }
      else if (!strcmp (str_buf, "iter:"))
        {
          xxx->opts.iter = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp(str_buf, "cx:"))
        {
          xxx->opts.cx = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp (str_buf, "cy:"))
        {
          xxx->opts.cy = g_ascii_strtod (opt_buf, NULL);
        }
      else if (!strcmp(str_buf, "redmode:"))
        {
          xxx->opts.redmode = atoi (opt_buf);
        }
      else if (!strcmp(str_buf, "greenmode:"))
        {
          xxx->opts.greenmode = atoi (opt_buf);
        }
      else if (!strcmp(str_buf, "bluemode:"))
        {
          xxx->opts.bluemode = atoi (opt_buf);
        }
      else if (!strcmp (str_buf, "redinvert:"))
        {
          xxx->opts.redinvert = atoi (opt_buf);
        }
      else if (!strcmp (str_buf, "greeninvert:"))
        {
          xxx->opts.greeninvert = atoi (opt_buf);
        }
      else if (!strcmp(str_buf, "blueinvert:"))
        {
          xxx->opts.blueinvert = atoi (opt_buf);
        }
      else if (!strcmp (str_buf, "colormode:"))
        {
          xxx->opts.colormode = atoi (opt_buf);
        }

      get_line (load_buf, MAX_LOAD_LINE, fp, 0);
    }

  return 0;
}

void
explorer_load (void)
{
  FILE  *fp;
  gchar  load_buf[MAX_LOAD_LINE];

  g_assert (filename != NULL);

  fp = g_fopen (filename, "rt");

  if (!fp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return;
    }
  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (FRACTAL_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("'%s' is not a FractalExplorer file"),
                 gimp_filename_to_utf8 (filename));
      fclose (fp);
      return;
    }
  if (load_options (current_obj, fp))
    {
      g_message (_("'%s' is corrupt. Line %d Option section incorrect"),
                 gimp_filename_to_utf8 (filename), line_no);
      fclose (fp);
      return;
    }

  g_object_set (wvals.config,
                "xmin",          current_obj->opts.xmin,
                "xmax",          current_obj->opts.xmax,
                "ymin",          current_obj->opts.ymin,
                "ymax",          current_obj->opts.ymax,
                "iter",          current_obj->opts.iter,
                "cx",            current_obj->opts.cx,
                "cy",            current_obj->opts.cy,
                "red-stretch",   current_obj->opts.redstretch,
                "green-stretch", current_obj->opts.greenstretch,
                "blue-stretch",  current_obj->opts.bluestretch,
                "color-mode",    (current_obj->opts.colormode == 0) ?
                                 "colormap" : "gradient",
                "red-invert",    current_obj->opts.redinvert,
                "green-invert",  current_obj->opts.greeninvert,
                "blue-invert",   current_obj->opts.blueinvert,
                NULL);

  switch (current_obj->opts.fractaltype)
    {
    case 0:
      g_object_set (wvals.config, "fractal-type", "mandelbrot", NULL);
      break;

    case 1:
      g_object_set (wvals.config, "fractal-type", "julia", NULL);
      break;

    case 2:
      g_object_set (wvals.config, "fractal-type", "barnsley-1", NULL);
      break;

    case 3:
      g_object_set (wvals.config, "fractal-type", "barnsley-2", NULL);
      break;

    case 4:
      g_object_set (wvals.config, "fractal-type", "barnsley-3", NULL);
      break;

    case 5:
      g_object_set (wvals.config, "fractal-type", "spider", NULL);
      break;

    case 6:
      g_object_set (wvals.config, "fractal-type", "man-o-war", NULL);
      break;

    case 7:
      g_object_set (wvals.config, "fractal-type", "lambda", NULL);
      break;

    case 8:
      g_object_set (wvals.config, "fractal-type", "sierpinski", NULL);
      break;

    default:
      break;
    }
  switch (current_obj->opts.redmode)
    {
    case 0:
      g_object_set (wvals.config, "red-mode", "red-sin", NULL);
      break;

    case 1:
      g_object_set (wvals.config, "red-mode", "red-cos", NULL);
      break;

    case 2:
      g_object_set (wvals.config, "red-mode", "red-none", NULL);
      break;

    default:
      break;
    }
  switch (current_obj->opts.greenmode)
    {
    case 0:
      g_object_set (wvals.config, "green-mode", "green-sin", NULL);
      break;

    case 1:
      g_object_set (wvals.config, "green-mode", "green-cos", NULL);
      break;

    case 2:
      g_object_set (wvals.config, "green-mode", "green-none", NULL);
      break;

    default:
      break;
    }
  switch (current_obj->opts.bluemode)
    {
    case 0:
      g_object_set (wvals.config, "blue-mode", "blue-sin", NULL);
      break;

    case 1:
      g_object_set (wvals.config, "blue-mode", "blue-cos", NULL);
      break;

    case 2:
      g_object_set (wvals.config, "blue-mode", "blue-none", NULL);
      break;

    default:
      break;
    }

  fclose (fp);
}
