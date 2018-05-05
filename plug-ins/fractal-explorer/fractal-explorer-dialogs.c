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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


static gint              n_gradient_samples = 0;
static gdouble          *gradient_samples = NULL;
static gchar            *gradient_name    = NULL;
static gboolean          ready_now = FALSE;
static gchar            *tpath = NULL;
static DialogElements   *elements = NULL;
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

static void load_file_chooser_response (GtkFileChooser *chooser,
                                        gint            response_id,
                                        gpointer        data);
static void save_file_chooser_response (GtkFileChooser *chooser,
                                        gint            response_id,
                                        gpointer        data);
static void create_load_file_chooser   (GtkWidget      *widget,
                                        GtkWidget      *dialog);
static void create_save_file_chooser   (GtkWidget      *widget,
                                        GtkWidget      *dialog);

static void cmap_preview_size_allocate (GtkWidget      *widget,
                                        GtkAllocation  *allocation);

/**********************************************************************
 CALLBACKS
 *********************************************************************/

static void
dialog_response (GtkWidget *widget,
                 gint       response_id,
                 gpointer   data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      wint.run = TRUE;
      gtk_widget_destroy (widget);
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
dialog_reset_callback (GtkWidget *widget,
                       gpointer   data)
{
  wvals.xmin = standardvals.xmin;
  wvals.xmax = standardvals.xmax;
  wvals.ymin = standardvals.ymin;
  wvals.ymax = standardvals.ymax;
  wvals.iter = standardvals.iter;
  wvals.cx   = standardvals.cx;
  wvals.cy   = standardvals.cy;

  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
dialog_redraw_callback (GtkWidget *widget,
                        gpointer   data)
{
  gint alwaysprev = wvals.alwayspreview;

  wvals.alwayspreview = TRUE;
  set_cmap_preview ();
  dialog_update_preview ();
  wvals.alwayspreview = alwaysprev;
}

static void
dialog_undo_zoom_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (zoomindex > 0)
    {
      zooms[zoomindex] = wvals;
      zoomindex--;
      wvals = zooms[zoomindex];
      dialog_change_scale ();
      set_cmap_preview ();
      dialog_update_preview ();
    }
}

static void
dialog_redo_zoom_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (zoomindex < zoommax)
    {
      zoomindex++;
      wvals = zooms[zoomindex];
      dialog_change_scale ();
      set_cmap_preview ();
      dialog_update_preview ();
    }
}

static void
dialog_step_in_callback (GtkWidget *widget,
                         gpointer   data)
{
  double xdifferenz;
  double ydifferenz;

  if (zoomindex < ZOOM_UNDO_SIZE - 1)
    {
      zooms[zoomindex]=wvals;
      zoomindex++;
    }
  zoommax = zoomindex;

  xdifferenz =  wvals.xmax - wvals.xmin;
  ydifferenz =  wvals.ymax - wvals.ymin;
  wvals.xmin += 1.0 / 6.0 * xdifferenz;
  wvals.ymin += 1.0 / 6.0 * ydifferenz;
  wvals.xmax -= 1.0 / 6.0 * xdifferenz;
  wvals.ymax -= 1.0 / 6.0 * ydifferenz;
  zooms[zoomindex] = wvals;

  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
dialog_step_out_callback (GtkWidget *widget,
                          gpointer   data)
{
  gdouble xdifferenz;
  gdouble ydifferenz;

  if (zoomindex < ZOOM_UNDO_SIZE - 1)
    {
      zooms[zoomindex]=wvals;
      zoomindex++;
    }
  zoommax = zoomindex;

  xdifferenz =  wvals.xmax - wvals.xmin;
  ydifferenz =  wvals.ymax - wvals.ymin;
  wvals.xmin -= 1.0 / 4.0 * xdifferenz;
  wvals.ymin -= 1.0 / 4.0 * ydifferenz;
  wvals.xmax += 1.0 / 4.0 * xdifferenz;
  wvals.ymax += 1.0 / 4.0 * ydifferenz;
  zooms[zoomindex] = wvals;

  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_toggle_update (GtkWidget *widget,
                        gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_radio_update  (GtkWidget *widget,
                        gpointer   data)
{
  gboolean c_sensitive;

  gimp_radio_button_update (widget, data);

  switch (wvals.fractaltype)
    {
    case TYPE_MANDELBROT:
    case TYPE_SIERPINSKI:
      c_sensitive = FALSE;
      break;

    default:
      c_sensitive = TRUE;
      break;
    }

  gimp_scale_entry_set_sensitive (elements->cx, c_sensitive);
  gimp_scale_entry_set_sensitive (elements->cy, c_sensitive);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_double_adjustment_update (GtkAdjustment *adjustment,
                                   gpointer       data)
{
  gimp_double_adjustment_update (adjustment, data);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_number_of_colors_callback (GtkAdjustment *adjustment,
                                    gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  g_free (gradient_samples);

  if (! gradient_name)
    gradient_name = gimp_context_get_gradient ();

  gimp_gradient_get_uniform_samples (gradient_name,
                                     wvals.ncolors,
                                     wvals.gradinvert,
                                     &n_gradient_samples,
                                     &gradient_samples);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_gradient_select_callback (GimpGradientSelectButton *gradient_button,
                                   const gchar              *name,
                                   gint                      width,
                                   const gdouble            *gradient_data,
                                   gboolean                  dialog_closing,
                                   gpointer                  data)
{
  g_free (gradient_name);
  g_free (gradient_samples);

  gradient_name = g_strdup (name);

  gimp_gradient_get_uniform_samples (gradient_name,
                                     wvals.ncolors,
                                     wvals.gradinvert,
                                     &n_gradient_samples,
                                     &gradient_samples);

  if (wvals.colormode == 1)
    {
      set_cmap_preview ();
      dialog_update_preview ();
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
                              GdkEventButton *event)
{
  gdouble l_xmin;
  gdouble l_xmax;
  gdouble l_ymin;
  gdouble l_ymax;

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
          l_xmin = (wvals.xmin +
                    (wvals.xmax - wvals.xmin) * (x_press / preview_width));
          l_xmax = (wvals.xmin +
                    (wvals.xmax - wvals.xmin) * (x_release / preview_width));
          l_ymin = (wvals.ymin +
                    (wvals.ymax - wvals.ymin) * (y_press / preview_height));
          l_ymax = (wvals.ymin +
                    (wvals.ymax - wvals.ymin) * (y_release / preview_height));

          if (zoomindex < ZOOM_UNDO_SIZE - 1)
            {
              zooms[zoomindex] = wvals;
              zoomindex++;
            }
          zoommax = zoomindex;
          wvals.xmin = l_xmin;
          wvals.xmax = l_xmax;
          wvals.ymin = l_ymin;
          wvals.ymax = l_ymax;
          dialog_change_scale ();
          dialog_update_preview ();
          oldypos = oldxpos = -1;
        }
    }

  return TRUE;
}

/**********************************************************************
 FUNCTION: explorer_dialog
 *********************************************************************/

gint
explorer_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *top_hbox;
  GtkWidget *left_vbox;
  GtkWidget *abox;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *toggle_vbox;
  GtkWidget *toggle_vbox2;
  GtkWidget *toggle_vbox3;
  GtkWidget *notebook;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *button;
  GtkWidget *gradient;
  gchar     *path;
  gchar     *gradient_name;
  GSList    *group = NULL;
  gint       i;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  path = gimp_gimprc_query ("fractalexplorer-path");

  if (path)
    {
      fractalexplorer_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }
  else
    {
      gchar *gimprc    = gimp_personal_rc_file ("gimprc");
      gchar *full_path = gimp_config_build_data_path ("fractalexplorer");
      gchar *esc_path  = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 "fractalexplorer-path",
                 "fractalexplorer-path",
                 esc_path, gimp_filename_to_utf8 (gimprc));

      g_free (gimprc);
      g_free (esc_path);
    }

  wint.wimage = g_new (guchar, preview_width * preview_height * 3);
  elements    = g_new (DialogElements, 1);

  dialog = maindlg =
    gimp_dialog_new (_("Fractal Explorer"), PLUG_IN_ROLE,
                     NULL, 0,
                     gimp_standard_help_func, PLUG_IN_PROC,

                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_OK"),     GTK_RESPONSE_OK,

                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response),
                    NULL);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  top_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_vexpand (top_hbox, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (top_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      top_hbox, FALSE, FALSE, 0);
  gtk_widget_show (top_hbox);

  left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (top_hbox), left_vbox, FALSE, FALSE, 0);
  gtk_widget_show (left_vbox);

  /*  Preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (left_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  wint.preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (wint.preview, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), wint.preview);

  g_signal_connect (wint.preview, "button-press-event",
                    G_CALLBACK (preview_button_press_event),
                    NULL);
  g_signal_connect (wint.preview, "button-release-event",
                    G_CALLBACK (preview_button_release_event),
                    NULL);
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

  toggle = gtk_check_button_new_with_label (_("Realtime preview"));
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
                    dialog);
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
                    dialog);

  button = gtk_button_new_with_mnemonic (_("Zoom _Out"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_step_out_callback),
                    dialog);

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
                    dialog);

  button = gtk_button_new_with_mnemonic (_("_Redo"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Redo last zoom change"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_redo_zoom_callback),
                    dialog);

  /*  Create notebook  */
  notebook = gtk_notebook_new ();
  gtk_widget_set_halign (notebook, GTK_ALIGN_START);
  gtk_widget_set_hexpand (notebook, FALSE);
  gtk_box_pack_start (GTK_BOX (top_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  /*  "Parameters" page  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new_with_mnemonic (_("_Parameters")));
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Fractal Parameters"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  elements->xmin =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                               _("Left:"), SCALE_WIDTH, 10,
                               wvals.xmin, -3, 3, 0.001, 0.01, 5,
                               TRUE, 0, 0, NULL, NULL);
  g_signal_connect (elements->xmin, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.xmin);

  elements->xmax =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 1,
                               _("Right:"), SCALE_WIDTH, 10,
                               wvals.xmax, -3, 3, 0.001, 0.01, 5,
                               TRUE, 0, 0, NULL, NULL);
  g_signal_connect (elements->xmax, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.xmax);

  elements->ymin =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 2,
                               _("Top:"), SCALE_WIDTH, 10,
                               wvals.ymin, -3, 3, 0.001, 0.01, 5,
                               TRUE, 0, 0, NULL, NULL);
  g_signal_connect (elements->ymin, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.ymin);

  elements->ymax =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 3,
                               _("Bottom:"), SCALE_WIDTH, 10,
                               wvals.ymax, -3, 3, 0.001, 0.01, 5,
                               TRUE, 0, 0, NULL, NULL);
  g_signal_connect (elements->ymax, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.ymax);

  elements->iter =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 4,
                               _("Iterations:"), SCALE_WIDTH, 10,
                               wvals.iter, 1, 1000, 1, 10, 0,
                               TRUE, 0, 0,
                               _("The higher the number of iterations, "
                                 "the more details will be calculated"), NULL);
  g_signal_connect (elements->iter, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.iter);

  elements->cx =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 5,
                               _("CX:"), SCALE_WIDTH, 10,
                               wvals.cx, -2.5, 2.5, 0.001, 0.01, 5,
                               TRUE, 0, 0,
                               _("Changes aspect of fractal"), NULL);
  g_signal_connect (elements->cx, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.cx);

  elements->cy =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 6,
                               _("CY:"), SCALE_WIDTH, 10,
                               wvals.cy, -2.5, 2.5, 0.001, 0.01, 5,
                               TRUE, 0, 0,
                               _("Changes aspect of fractal"), NULL);
  g_signal_connect (elements->cy, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.cy);

  bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top (bbox, 12);
  gtk_box_set_homogeneous (GTK_BOX (bbox), TRUE);
  gtk_grid_attach (GTK_GRID (grid), bbox, 0, 7, 3, 1);
  gtk_widget_show (bbox);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_load_file_chooser),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Load a fractal from file"), NULL);

  button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_reset_callback),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Reset parameters to default values"),
                           NULL);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_box_pack_start (GTK_BOX (bbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_save_file_chooser),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Save active fractal to file"), NULL);

  /*  Fractal type toggle box  */
  frame = gimp_frame_new (_("Fractal Type"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  toggle_vbox =
    gimp_int_radio_group_new (FALSE, NULL,
                              G_CALLBACK (explorer_radio_update),
                              &wvals.fractaltype, wvals.fractaltype,

                              _("Mandelbrot"), TYPE_MANDELBROT,
                              &(elements->type[TYPE_MANDELBROT]),
                              _("Julia"),      TYPE_JULIA,
                              &(elements->type[TYPE_JULIA]),
                              _("Barnsley 1"), TYPE_BARNSLEY_1,
                              &(elements->type[TYPE_BARNSLEY_1]),
                              _("Barnsley 2"), TYPE_BARNSLEY_2,
                              &(elements->type[TYPE_BARNSLEY_2]),
                              _("Barnsley 3"), TYPE_BARNSLEY_3,
                              &(elements->type[TYPE_BARNSLEY_3]),
                              _("Spider"),     TYPE_SPIDER,
                              &(elements->type[TYPE_SPIDER]),
                              _("Man'o'war"),  TYPE_MAN_O_WAR,
                              &(elements->type[TYPE_MAN_O_WAR]),
                              _("Lambda"),     TYPE_LAMBDA,
                              &(elements->type[TYPE_LAMBDA]),
                              _("Sierpinski"), TYPE_SIERPINSKI,
                              &(elements->type[TYPE_SIERPINSKI]),

                              NULL);

  toggle_vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  for (i = TYPE_BARNSLEY_2; i <= TYPE_SPIDER; i++)
    {
      g_object_ref (elements->type[i]);

      gtk_widget_hide (elements->type[i]);
      gtk_container_remove (GTK_CONTAINER (toggle_vbox), elements->type[i]);
      gtk_box_pack_start (GTK_BOX (toggle_vbox2), elements->type[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (elements->type[i]);

      g_object_unref (elements->type[i]);
    }

  toggle_vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  for (i = TYPE_MAN_O_WAR; i <= TYPE_SIERPINSKI; i++)
    {
      g_object_ref (elements->type[i]);

      gtk_widget_hide (elements->type[i]);
      gtk_container_remove (GTK_CONTAINER (toggle_vbox), elements->type[i]);
      gtk_box_pack_start (GTK_BOX (toggle_vbox3), elements->type[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (elements->type[i]);

      g_object_unref (elements->type[i]);
    }

  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox);

  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox2, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox2);

  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox3, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox3);

  /*  Color page  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new_with_mnemonic (_("Co_lors")));
  gtk_widget_show (vbox);

  /*  Number of Colors frame  */
  frame = gimp_frame_new (_("Number of Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  elements->ncol =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                               _("Number of colors:"), SCALE_WIDTH, 0,
                               wvals.ncolors, 2, MAXNCOLORS, 1, 10, 0,
                               TRUE, 0, 0,
                               _("Change the number of colors in the mapping"),
                               NULL);
  g_signal_connect (elements->ncol, "value-changed",
                    G_CALLBACK (explorer_number_of_colors_callback),
                    &wvals.ncolors);

  elements->useloglog = toggle =
    gtk_check_button_new_with_label (_("Use loglog smoothing"));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 1, 3, 1);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.useloglog);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.useloglog);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("Use log log smoothing to eliminate "
                                     "\"banding\" in the result"), NULL);

  /*  Color Density frame  */
  frame = gimp_frame_new (_("Color Density"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  elements->red =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                               _("Red:"), SCALE_WIDTH, 0,
                               wvals.redstretch, 0, 1, 0.01, 0.1, 2,
                               TRUE, 0, 0,
                               _("Change the intensity of the red channel"), NULL);
  g_signal_connect (elements->red, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.redstretch);

  elements->green =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 1,
                               _("Green:"), SCALE_WIDTH, 0,
                               wvals.greenstretch, 0, 1, 0.01, 0.1, 2,
                               TRUE, 0, 0,
                               _("Change the intensity of the green channel"), NULL);
  g_signal_connect (elements->green, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.greenstretch);

  elements->blue =
    gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 2,
                               _("Blue:"), SCALE_WIDTH, 0,
                               wvals.bluestretch, 0, 1, 0.01, 0.1, 2,
                               TRUE, 0, 0,
                               _("Change the intensity of the blue channel"), NULL);
  g_signal_connect (elements->blue, "value-changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.bluestretch);

  /*  Color Function frame  */
  frame = gimp_frame_new (_("Color Function"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  /*  Redmode radio frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Red"),
                                    G_CALLBACK (explorer_radio_update),
                                    &wvals.redmode, wvals.redmode,

                                    _("Sine"),                    SINUS,
                                    &elements->redmode[SINUS],
                                    _("Cosine"),                  COSINUS,
                                    &elements->redmode[COSINUS],
                                    C_("color-function", "None"), NONE,
                                    &elements->redmode[NONE],

                                    NULL);
  gimp_help_set_help_data (elements->redmode[SINUS],
                           _("Use sine-function for this color component"),
                           NULL);
  gimp_help_set_help_data (elements->redmode[COSINUS],
                           _("Use cosine-function for this color "
                             "component"), NULL);
  gimp_help_set_help_data (elements->redmode[NONE],
                           _("Use linear mapping instead of any "
                             "trigonometrical function for this color "
                             "channel"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_bin_get_child (GTK_BIN (frame));

  elements->redinvert = toggle =
    gtk_check_button_new_with_label (_("Inversion"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.redinvert);
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.redinvert);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
                           _("If you enable this option higher color values "
                             "will be swapped with lower ones and vice "
                             "versa"), NULL);

  /*  Greenmode radio frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Green"),
                                    G_CALLBACK (explorer_radio_update),
                                    &wvals.greenmode, wvals.greenmode,

                                    _("Sine"),                    SINUS,
                                    &elements->greenmode[SINUS],
                                    _("Cosine"),                  COSINUS,
                                    &elements->greenmode[COSINUS],
                                    C_("color-function", "None"), NONE,
                                    &elements->greenmode[NONE],

                                    NULL);
  gimp_help_set_help_data (elements->greenmode[SINUS],
                           _("Use sine-function for this color component"),
                           NULL);
  gimp_help_set_help_data (elements->greenmode[COSINUS],
                           _("Use cosine-function for this color "
                             "component"), NULL);
  gimp_help_set_help_data (elements->greenmode[NONE],
                           _("Use linear mapping instead of any "
                             "trigonometrical function for this color "
                             "channel"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_bin_get_child (GTK_BIN (frame));

  elements->greeninvert = toggle =
    gtk_check_button_new_with_label (_("Inversion"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.greeninvert);
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.greeninvert);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
                           _("If you enable this option higher color values "
                             "will be swapped with lower ones and vice "
                             "versa"), NULL);

  /*  Bluemode radio frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Blue"),
                                    G_CALLBACK (explorer_radio_update),
                                    &wvals.bluemode, wvals.bluemode,

                                    _("Sine"),                    SINUS,
                                    &elements->bluemode[SINUS],
                                    _("Cosine"),                  COSINUS,
                                    &elements->bluemode[COSINUS],
                                    C_("color-function", "None"), NONE,
                                    &elements->bluemode[NONE],

                                    NULL);
  gimp_help_set_help_data (elements->bluemode[SINUS],
                           _("Use sine-function for this color component"),
                           NULL);
  gimp_help_set_help_data (elements->bluemode[COSINUS],
                           _("Use cosine-function for this color "
                             "component"), NULL);
  gimp_help_set_help_data (elements->bluemode[NONE],
                           _("Use linear mapping instead of any "
                             "trigonometrical function for this color "
                             "channel"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_bin_get_child (GTK_BIN (frame));

  elements->blueinvert = toggle =
    gtk_check_button_new_with_label (_("Inversion"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON( toggle), wvals.blueinvert);
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.blueinvert);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
                           _("If you enable this option higher color values "
                             "will be swapped with lower ones and vice "
                             "versa"), NULL);

  /*  Colormode toggle box  */
  frame = gimp_frame_new (_("Color Mode"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
  gtk_widget_show (toggle_vbox);

  toggle = elements->colormode[0] =
    gtk_radio_button_new_with_label (group, _("As specified above"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (0));
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_radio_update),
                    &wvals.colormode);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                wvals.colormode == 0);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
                           _("Create a color-map with the options you "
                             "specified above (color density/function). The "
                             "result is visible in the preview image"), NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (toggle_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = elements->colormode[1] =
    gtk_radio_button_new_with_label (group,
                                     _("Apply active gradient to final image"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (1));
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_radio_update),
                    &wvals.colormode);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                wvals.colormode == 1);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
                           _("Create a color-map using a gradient from "
                             "the gradient editor"), NULL);

  gradient_name = gimp_context_get_gradient ();

  gimp_gradient_get_uniform_samples (gradient_name,
                                     wvals.ncolors,
                                     wvals.gradinvert,
                                     &n_gradient_samples,
                                     &gradient_samples);

  gradient = gimp_gradient_select_button_new (_("FractalExplorer Gradient"),
                                              gradient_name);
  g_signal_connect (gradient, "gradient-set",
                    G_CALLBACK (explorer_gradient_select_callback), NULL);
  g_free (gradient_name);
  gtk_box_pack_start (GTK_BOX (hbox), gradient, FALSE, FALSE, 0);
  gtk_widget_show (gradient);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  {
    gint xsize, ysize;

    for (ysize = 1; ysize * ysize * ysize < 8192; ysize++) /**/;
    xsize = wvals.ncolors / ysize;
    while (xsize * ysize < 8192) xsize++;

    gtk_widget_set_size_request (abox, xsize, ysize * 4);
  }
  gtk_box_pack_start (GTK_BOX (toggle_vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  cmap_preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (cmap_preview, 32, 32);
  gtk_container_add (GTK_CONTAINER (abox), cmap_preview);
  g_signal_connect (cmap_preview, "size-allocate",
                    G_CALLBACK (cmap_preview_size_allocate), NULL);
  gtk_widget_show (cmap_preview);

  frame = add_objects_list ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
                            gtk_label_new_with_mnemonic (_("_Fractals")));
  gtk_widget_show (frame);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

  gtk_widget_show (dialog);
  ready_now = TRUE;

  set_cmap_preview ();
  dialog_update_preview ();

  gtk_main ();

  g_free (wint.wimage);

  return wint.run;
}


/**********************************************************************
 FUNCTION: dialog_update_preview
 *********************************************************************/

void
dialog_update_preview (void)
{
  gint     ycoord;
  guchar  *p_ul;

  if (NULL == wint.preview)
    return;

  if (ready_now && wvals.alwayspreview)
    {
      xmin = wvals.xmin;
      xmax = wvals.xmax;
      ymin = wvals.ymin;
      ymax = wvals.ymax;
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
                               3);
          p_ul += preview_width * 3;
        }

      preview_redraw ();
    }
}

/**********************************************************************
 FUNCTION: cmap_preview_size_allocate()
 *********************************************************************/

static void
cmap_preview_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  gint             i;
  gint             x;
  gint             y;
  gint             j;
  guchar          *b;
  GimpPreviewArea *preview = GIMP_PREVIEW_AREA (widget);

  b = g_new (guchar, allocation->width * allocation->height * 3);

  for (y = 0; y < allocation->height; y++)
    {
      for (x = 0; x < allocation->width; x++)
        {
          i = x + (y / 4) * allocation->width;
          if (i > wvals.ncolors)
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
set_cmap_preview (void)
{
  gint    xsize, ysize;

  if (NULL == cmap_preview)
    return;

  make_color_map ();

  for (ysize = 1; ysize * ysize * ysize < wvals.ncolors; ysize++)
    /**/;
  xsize = wvals.ncolors / ysize;
  while (xsize * ysize < wvals.ncolors)
    xsize++;

  gtk_widget_set_size_request (cmap_preview, xsize, ysize * 4);
}

/**********************************************************************
 FUNCTION: make_color_map()
 *********************************************************************/

void
make_color_map (void)
{
  gint     i;
  gint     r;
  gint     gr;
  gint     bl;
  gdouble  redstretch;
  gdouble  greenstretch;
  gdouble  bluestretch;
  gdouble  pi = atan (1) * 4;

  /*  get gradient samples if they don't exist -- fixes gradient color
   *  mode for noninteractive use (bug #103470).
   */
  if (gradient_samples == NULL)
    {
      gchar *gradient_name = gimp_context_get_gradient ();

      gimp_gradient_get_uniform_samples (gradient_name,
                                         wvals.ncolors,
                                         wvals.gradinvert,
                                         &n_gradient_samples,
                                         &gradient_samples);

      g_free (gradient_name);
    }

  redstretch   = wvals.redstretch * 127.5;
  greenstretch = wvals.greenstretch * 127.5;
  bluestretch  = wvals.bluestretch * 127.5;

  for (i = 0; i < wvals.ncolors; i++)
    if (wvals.colormode == 1)
      {
        colormap[i].r = (guchar)(gradient_samples[i * 4] * 255.9);
        colormap[i].g = (guchar)(gradient_samples[i * 4 + 1] * 255.9);
        colormap[i].b = (guchar)(gradient_samples[i * 4 + 2] * 255.9);
      }
    else
      {
        double x = (i*2.0) / wvals.ncolors;
        r = gr = bl = 0;

        switch (wvals.redmode)
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

        switch (wvals.greenmode)
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

        switch (wvals.bluemode)
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

        if (wvals.redinvert)
          r = 255 - r;

        if (wvals.greeninvert)
          gr = 255 - gr;

        if (wvals.blueinvert)
          bl = 255 - bl;

        colormap[i].r = r;
        colormap[i].g = gr;
        colormap[i].b = bl;
      }
}

/**********************************************************************
 FUNCTION: dialog_change_scale
 *********************************************************************/

void
dialog_change_scale (void)
{
  ready_now = FALSE;

  gtk_adjustment_set_value (elements->xmin, wvals.xmin);
  gtk_adjustment_set_value (elements->xmax, wvals.xmax);
  gtk_adjustment_set_value (elements->ymin, wvals.ymin);
  gtk_adjustment_set_value (elements->ymax, wvals.ymax);
  gtk_adjustment_set_value (elements->iter, wvals.iter);
  gtk_adjustment_set_value (elements->cx,   wvals.cx);
  gtk_adjustment_set_value (elements->cy,   wvals.cy);

  gtk_adjustment_set_value (elements->red,  wvals.redstretch);
  gtk_adjustment_set_value (elements->green,wvals.greenstretch);
  gtk_adjustment_set_value (elements->blue, wvals.bluestretch);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (elements->type[wvals.fractaltype]), TRUE);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (elements->redmode[wvals.redmode]), TRUE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (elements->greenmode[wvals.greenmode]), TRUE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (elements->bluemode[wvals.bluemode]), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (elements->redinvert),
                                wvals.redinvert);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (elements->greeninvert),
                                wvals.greeninvert);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (elements->blueinvert),
                                wvals.blueinvert);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (elements->colormode[wvals.colormode]), TRUE);

  ready_now = TRUE;
}


/**********************************************************************
 FUNCTION: save_options
 *********************************************************************/

static void
save_options (FILE * fp)
{
  gchar buf[64];

  /* Save options */

  fprintf (fp, "fractaltype: %i\n", wvals.fractaltype);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.xmin);
  fprintf (fp, "xmin: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.xmax);
  fprintf (fp, "xmax: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.ymin);
  fprintf (fp, "ymin: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.ymax);
  fprintf (fp, "ymax: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.iter);
  fprintf (fp, "iter: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.cx);
  fprintf (fp, "cx: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.cy);
  fprintf (fp, "cy: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.redstretch * 128.0);
  fprintf (fp, "redstretch: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.greenstretch * 128.0);
  fprintf (fp, "greenstretch: %s\n", buf);

  g_ascii_formatd (buf, sizeof (buf), "%0.15f", wvals.bluestretch * 128.0);
  fprintf (fp, "bluestretch: %s\n", buf);

  fprintf (fp, "redmode: %i\n", wvals.redmode);
  fprintf (fp, "greenmode: %i\n", wvals.greenmode);
  fprintf (fp, "bluemode: %i\n", wvals.bluemode);
  fprintf (fp, "redinvert: %i\n", wvals.redinvert);
  fprintf (fp, "greeninvert: %i\n", wvals.greeninvert);
  fprintf (fp, "blueinvert: %i\n", wvals.blueinvert);
  fprintf (fp, "colormode: %i\n", wvals.colormode);
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
  if (response_id == GTK_RESPONSE_OK)
    {
      filename = gtk_file_chooser_get_filename (chooser);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          explorer_load ();
        }

      gtk_widget_show (maindlg);
      dialog_change_scale ();
      set_cmap_preview ();
      dialog_update_preview ();
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static void
create_load_file_chooser (GtkWidget *widget,
                          GtkWidget *dialog)
{
  static GtkWidget *window = NULL;

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

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (window),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      file_chooser_set_default_folder (GTK_FILE_CHOOSER (window));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);
      g_signal_connect (window, "response",
                        G_CALLBACK (load_file_chooser_response),
                        window);
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

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (window),
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
  xxx->opts = standardvals;
  xxx->opts.gradinvert    = FALSE;

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
  if (load_options (current_obj,fp))
    {
      g_message (_("'%s' is corrupt. Line %d Option section incorrect"),
                 gimp_filename_to_utf8 (filename), line_no);
      fclose (fp);
      return;
    }

  wvals = current_obj->opts;

  fclose (fp);
}
