#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "FractalExplorer.h"
#include "Dialogs.h"

#include "logo.h"

#include "libgimp/stdplugins-intl.h"


#define RESPONSE_ABOUT 1


static gdouble 		*gradient_samples = NULL;
static gchar   		*gradient_name    = NULL;
static gboolean          ready_now = FALSE;
static gchar 		*tpath = NULL;
static DialogElements   *elements = NULL;
static GtkWidget        *cmap_preview;
static GtkWidget        *maindlg;

static explorer_vals_t	zooms[100];
static gint          	zoomindex = 1;
static gint          	zoommax = 1;

static gint 		 oldxpos = -1;
static gint 		 oldypos = -1;
static GdkCursor	*MyCursor;
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

static void load_file_selection_ok     (GtkWidget          *widget,
					GtkFileSelection   *fs);
static void file_selection_ok          (GtkWidget          *widget,
					GtkFileSelection   *fs);
static void create_load_file_selection (GtkWidget          *widget,
                                        GtkWidget          *dialog);
static void create_file_selection      (GtkWidget          *widget,
                                        GtkWidget          *dialog);

static void explorer_logo_dialog       (GtkWidget          *parent);

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
    case RESPONSE_ABOUT:
      explorer_logo_dialog (widget);
      break;

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
  if (zoomindex > 1)
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

  if (zoomindex < zoommax)
    {
      zooms[zoomindex]=wvals;
      zoomindex++;
    }

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

  if (zoomindex < zoommax)
    {
      zooms[zoomindex]=wvals;
      zoomindex++;
    }

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
  gimp_radio_button_update (widget, data);

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
  gint dummy;

  gimp_int_adjustment_update (adjustment, data);

  g_free (gradient_samples);

  if (! gradient_name)
    gradient_name = gimp_gradients_get_gradient ();

  gimp_gradients_get_gradient_data (gradient_name,
                                    wvals.ncolors,
                                    wvals.gradinvert,
                                    &dummy,
                                    &gradient_samples);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_gradient_select_callback (const gchar   *name,
				   gint           width,
				   const gdouble *gradient_data,
				   gboolean       dialog_closing,
				   gpointer       data)
{
  gint dummy;

  g_free (gradient_name);
  g_free (gradient_samples);

  gradient_name = g_strdup (name);

  gimp_gradients_get_gradient_data (gradient_name,
                                    wvals.ncolors,
                                    wvals.gradinvert,
                                    &dummy,
				    &gradient_samples);

  if (wvals.colormode == 1)
    {
      set_cmap_preview ();
      dialog_update_preview ();
    }
}

static void
preview_draw_crosshair (gint px, gint py)
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
  gint	   y;
  guchar  *p;

  p = wint.wimage;

  for (y = 0; y < preview_height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (wint.preview), p,
			    0, y, preview_width);
      p += preview_width * 3;
    }

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
  GdkDisplay *display;

  if (oldypos != -1)
    {
      preview_draw_crosshair (oldxpos, oldypos);
    }
  oldxpos = -1;
  oldypos = -1;

  preview_redraw ();

  display = gtk_widget_get_display (maindlg);
  MyCursor = gdk_cursor_new_for_display (display, GDK_TOP_LEFT_ARROW);
  gdk_window_set_cursor (maindlg->window, MyCursor);

  return TRUE;
}

static gboolean
preview_enter_notify_event (GtkWidget      *widget,
			    GdkEventButton *event)
{
  GdkDisplay *display;

  display = gtk_widget_get_display (maindlg);
  MyCursor = gdk_cursor_new_for_display (display, GDK_TCROSS);
  gdk_window_set_cursor (maindlg->window, MyCursor);

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

	  zooms[zoomindex] = wvals;
	  zoomindex++;
	  if (zoomindex > 99)
	    zoomindex = 99;
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
  GtkWidget *hbbox;
  GtkWidget *frame;
  GtkWidget *pframe;
  GtkWidget *toggle;
  GtkWidget *toggle_vbox;
  GtkWidget *toggle_vbox2;
  GtkWidget *toggle_vbox3;
  GtkWidget *notebook;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *gradient;
  GtkWidget *sep;
  gchar   *gradient_name;
  GSList  *group = NULL;
  gint     i;

  gimp_ui_init ("fractalexplorer", TRUE);

  fractalexplorer_path = gimp_plug_in_get_path ("fractalexplorer-path",
                                                "fractalexplorer");

  wint.wimage = g_new (guchar, preview_width * preview_height * 3);
  elements    = g_new (DialogElements, 1);

  dialog = maindlg =
    gimp_dialog_new ("Fractal Explorer", "fractalexplorer",
                     NULL, 0,
		     gimp_standard_help_func, "filters/fractalexplorer.html",

		     _("About"),       RESPONSE_ABOUT,
		     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                     NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response),
                    NULL);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  top_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (top_hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), top_hbox,
		      FALSE, FALSE, 0);
  gtk_widget_show (top_hbox);

  left_vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (top_hbox), left_vbox, FALSE, FALSE, 0);
  gtk_widget_show (left_vbox);

  /*  Preview  */
  frame = gtk_frame_new (_("Preview"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (left_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  wint.preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (wint.preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (pframe), wint.preview);

  g_signal_connect (wint.preview, "button_press_event",
                    G_CALLBACK (preview_button_press_event),
                    NULL);
  g_signal_connect (wint.preview, "button_release_event",
                    G_CALLBACK (preview_button_release_event),
		    NULL);
  g_signal_connect (wint.preview, "motion_notify_event",
                    G_CALLBACK (preview_motion_notify_event),
                    NULL);
  g_signal_connect (wint.preview, "leave_notify_event",
                    G_CALLBACK (preview_leave_notify_event),
                    NULL);
  g_signal_connect (wint.preview, "enter_notify_event",
                    G_CALLBACK (preview_enter_notify_event),
                    NULL);

  gtk_widget_set_events (wint.preview, (GDK_BUTTON_PRESS_MASK |
					GDK_BUTTON_RELEASE_MASK |
					GDK_POINTER_MOTION_MASK |
					GDK_LEAVE_NOTIFY_MASK |
					GDK_ENTER_NOTIFY_MASK));
  gtk_widget_show (wint.preview);

  toggle = gtk_check_button_new_with_label (_("Realtime Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.alwayspreview);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.alwayspreview);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("If you enable this option the preview "
				     "will be redrawn automatically"), NULL);

  button = gtk_button_new_with_label (_("Redraw"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_redraw_callback),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Redraw preview"), NULL);

  /*  Zoom Options  */
  frame = gtk_frame_new (_("Zoom Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_end (GTK_BOX (left_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Undo last zoom"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_undo_zoom_callback),
                    dialog);

  button = gtk_button_new_from_stock (GTK_STOCK_REDO);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Redo last zoom"), NULL);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_redo_zoom_callback),
                    dialog);

  button = gtk_button_new_from_stock (GTK_STOCK_ZOOM_IN);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_step_in_callback),
                    dialog);

  button = gtk_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_step_out_callback),
                    dialog);


  /*  Create notebook  */
  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (top_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  /*  "Parameters" page  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic (_("_Parameters")));
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Fractal Parameters"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (8, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 6, 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  elements->xmin =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("XMIN:"), SCALE_WIDTH, 10,
			  wvals.xmin, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the first (minimal) x-coordinate "
			    "delimitation"), NULL);
  g_signal_connect (elements->xmin, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.xmin);

  elements->xmax =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("XMAX:"), SCALE_WIDTH, 10,
			  wvals.xmax, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the second (maximal) x-coordinate "
			    "delimitation"), NULL);
  g_signal_connect (elements->xmax, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.xmax);

  elements->ymin =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("YMIN:"), SCALE_WIDTH, 10,
			  wvals.ymin, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the first (minimal) y-coordinate "
			    "delimitation"), NULL);
  g_signal_connect (elements->ymin, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.ymin);

  elements->ymax =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			  _("YMAX:"), SCALE_WIDTH, 10,
			  wvals.ymax, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the second (maximal) y-coordinate "
			    "delimitation"), NULL);
  g_signal_connect (elements->ymax, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.ymax);

  elements->iter =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			  _("ITER:"), SCALE_WIDTH, 10,
			  wvals.iter, 0, 1000, 1, 10, 5,
			  TRUE, 0, 0,
			  _("Change the iteration value. The higher it "
			    "is, the more details will be calculated, "
			    "which will take more time"), NULL);
  g_signal_connect (elements->iter, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.iter);

  elements->cx =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
			  _("CX:"), SCALE_WIDTH, 10,
			  wvals.cx, -2.5, 2.5, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the CX value (changes aspect of "
			    "fractal, active with every fractal but "
			    "Mandelbrot and Sierpinski)"), NULL);
  g_signal_connect (elements->cx, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.cx);

  elements->cy =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 6,
			  _("CY:"), SCALE_WIDTH, 10,
			  wvals.cy, -2.5, 2.5, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the CY value (changes aspect of "
			    "fractal, active with every fractal but "
			    "Mandelbrot and Sierpinski)"), NULL);
  g_signal_connect (elements->cy, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.cy);

  hbbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), hbbox, 0, 3, 7, 8);
  gtk_widget_show (hbbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_load_file_selection),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Load a fractal from file"), NULL);

  button = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (dialog_reset_callback),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Reset parameters to default values"),
			   NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (create_file_selection),
                    dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Save active fractal to file"), NULL);

  /*  Fractal type toggle box  */
  frame = gtk_frame_new (_("Fractal Type"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  toggle_vbox =
    gimp_radio_group_new2 (FALSE, NULL,
			   G_CALLBACK (explorer_radio_update),
			   &wvals.fractaltype,
			   (gpointer) wvals.fractaltype,

			   _("Mandelbrot"), (gpointer) TYPE_MANDELBROT,
			   &(elements->type[TYPE_MANDELBROT]),
			   _("Julia"), (gpointer) TYPE_JULIA,
			   &(elements->type[TYPE_JULIA]),
			   _("Barnsley 1"), (gpointer) TYPE_BARNSLEY_1,
			   &(elements->type[TYPE_BARNSLEY_1]),
			   _("Barnsley 2"), (gpointer) TYPE_BARNSLEY_2,
			   &(elements->type[TYPE_BARNSLEY_2]),
			   _("Barnsley 3"), (gpointer) TYPE_BARNSLEY_3,
			   &(elements->type[TYPE_BARNSLEY_3]),
			   _("Spider"), (gpointer) TYPE_SPIDER,
			   &(elements->type[TYPE_SPIDER]),
			   _("Man'o'war"), (gpointer) TYPE_MAN_O_WAR,
			   &(elements->type[TYPE_MAN_O_WAR]),
			   _("Lambda"), (gpointer) TYPE_LAMBDA,
			   &(elements->type[TYPE_LAMBDA]),
			   _("Sierpinski"), (gpointer) TYPE_SIERPINSKI,
			   &(elements->type[TYPE_SIERPINSKI]),

			   NULL);

  toggle_vbox2 = gtk_vbox_new (FALSE, 1);
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

  toggle_vbox3 = gtk_vbox_new (FALSE, 1);
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

  gtk_container_set_border_width (GTK_CONTAINER (toggle_vbox), 0);
  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox);

  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox2, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox2);

  gtk_box_pack_start (GTK_BOX (hbox), toggle_vbox3, FALSE, FALSE, 0);
  gtk_widget_show (toggle_vbox3);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  /*  Color page  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic (_("Co_lors")));
  gtk_widget_show (vbox);

  /*  Number of Colors frame  */
  frame = gtk_frame_new (_("Number of Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  elements->ncol =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Number of colors:"), SCALE_WIDTH, 0,
			  wvals.ncolors, 2, MAXNCOLORS, 1, 10, 0,
			  TRUE, 0, 0,
			  _("Change the number of colors in the mapping"),
			  NULL);
  g_signal_connect (elements->ncol, "value_changed",
                    G_CALLBACK (explorer_number_of_colors_callback),
                    &wvals.ncolors);

  elements->useloglog = toggle =
    gtk_check_button_new_with_label (_("Use loglog smoothing"));
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 3, 1, 2);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (explorer_toggle_update),
                    &wvals.useloglog);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), wvals.useloglog);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("Use log log smoothing to eliminate "
				     "\"banding\" in the result"), NULL);

  /*  Color Density frame  */
  frame = gtk_frame_new (_("Color Density"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  elements->red =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Red:"), SCALE_WIDTH, 0,
			  wvals.redstretch, 0, 1, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Change the intensity of the red channel"), NULL);
  g_signal_connect (elements->red, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.redstretch);

  elements->green =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("Green:"), SCALE_WIDTH, 0,
			  wvals.greenstretch, 0, 1, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Change the intensity of the green channel"), NULL);
  g_signal_connect (elements->green, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.greenstretch);

  elements->blue =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("Blue:"), SCALE_WIDTH, 0,
			  wvals.bluestretch, 0, 1, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Change the intensity of the blue channel"), NULL);
  g_signal_connect (elements->blue, "value_changed",
                    G_CALLBACK (explorer_double_adjustment_update),
                    &wvals.bluestretch);

  /*  Color Function frame  */
  frame = gtk_frame_new (_("Color Function"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  /*  Redmode radio frame  */
  frame = gimp_radio_group_new2 (TRUE, _("Red"),
				 G_CALLBACK (explorer_radio_update),
				 &wvals.redmode, (gpointer) wvals.redmode,

				 _("Sine"), (gpointer) SINUS,
				 &elements->redmode[SINUS],
				 _("Cosine"), (gpointer) COSINUS,
				 &elements->redmode[COSINUS],
				 _("None"), (gpointer) NONE,
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

  toggle_vbox = GTK_BIN (frame)->child;

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
  frame = gimp_radio_group_new2 (TRUE, _("Green"),
				 G_CALLBACK (explorer_radio_update),
				 &wvals.greenmode, (gpointer) wvals.greenmode,

				 _("Sine"), (gpointer) SINUS,
				 &elements->greenmode[SINUS],
				 _("Cosine"), (gpointer) COSINUS,
				 &elements->greenmode[COSINUS],
				 _("None"), (gpointer) NONE,
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

  toggle_vbox = GTK_BIN (frame)->child;

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
  frame = gimp_radio_group_new2 (TRUE, _("Blue"),
				 G_CALLBACK (explorer_radio_update),
				 &wvals.bluemode, (gpointer) wvals.bluemode,

				 _("Sine"), (gpointer) SINUS,
				 &elements->bluemode[SINUS],
				 _("Cosine"), (gpointer) COSINUS,
				 &elements->bluemode[COSINUS],
				 _("None"), (gpointer) NONE,
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

  toggle_vbox = GTK_BIN (frame)->child;

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
  frame = gtk_frame_new (_("Color Mode"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (toggle_vbox), 2);
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

  hbox = gtk_hbox_new (FALSE, 6);
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

  gradient_name = gimp_gradients_get_gradient ();
  gradient_samples = gimp_gradients_sample_uniform (wvals.ncolors,
                                                    wvals.gradinvert);
  gradient = gimp_gradient_select_widget_new (_("FractalExplorer Gradient"),
                                              gradient_name,
                                              explorer_gradient_select_callback,
                                              NULL);
  g_free (gradient_name);
  gtk_box_pack_start (GTK_BOX (hbox), gradient, FALSE, FALSE, 0);
  gtk_widget_show (gradient);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (toggle_vbox), sep, FALSE, FALSE, 1);
  gtk_widget_show (sep);

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

  cmap_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (cmap_preview), 32, 32);
  gtk_container_add (GTK_CONTAINER (abox), cmap_preview);
  gtk_widget_show (cmap_preview);

  frame = add_objects_list ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame,
			    gtk_label_new_with_mnemonic (_("_Fractals")));
  gtk_widget_show (frame);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 1);

  gtk_widget_show (dialog);
  ready_now = TRUE;

  set_cmap_preview ();
  dialog_update_preview ();

  gtk_main ();
  gdk_flush ();

  g_free (wint.wimage);

  return wint.run;
}


/**********************************************************************
 FUNCTION: dialog_update_preview
 *********************************************************************/

void
dialog_update_preview (void)
{
  gdouble  left;
  gdouble  right;
  gdouble  bottom;
  gdouble  top;
  gdouble  dx;
  gdouble  dy;
  gdouble  cx;
  gdouble  cy;
  gint     px;
  gint     py;
  gint     xcoord;
  gint     ycoord;
  gint     iteration;
  guchar  *p_ul;
  gdouble  a;
  gdouble  b;
  gdouble  x;
  gdouble  y;
  gdouble  oldx;
  gdouble  oldy;
  gdouble  foldxinitx;
  gdouble  foldxinity;
  gdouble  tempsqrx;
  gdouble  tempsqry;
  gdouble  tmpx = 0;
  gdouble  tmpy = 0;
  gdouble  foldyinitx;
  gdouble  foldyinity;
  gdouble  adjust;
  gdouble  xx = 0;
  gint     zaehler;
  gint     color;
  gint     useloglog;

  if (NULL == wint.preview)
    return;

  if (ready_now && wvals.alwayspreview)
    {
      left = sel_x1;
      right = sel_x2 - 1;
      bottom = sel_y2 - 1;
      top = sel_y1;
      dx = (right - left) / (preview_width - 1);
      dy = (bottom - top) / (preview_height - 1);

      xmin = wvals.xmin;
      xmax = wvals.xmax;
      ymin = wvals.ymin;
      ymax = wvals.ymax;
      cx = wvals.cx;
      cy = wvals.cy;
      xbild = preview_width;
      ybild = preview_height;
      xdiff = (xmax - xmin) / xbild;
      ydiff = (ymax - ymin) / ybild;

      py = 0;

      p_ul = wint.wimage;
      iteration = (int) wvals.iter;
      useloglog = wvals.useloglog;
      for (ycoord = 0; ycoord < preview_height; ycoord++)
	{
	  px = 0;

	  for (xcoord = 0; xcoord < preview_width; xcoord++)
	    {
	      a = (double) xmin + (double) xcoord *xdiff;
	      b = (double) ymin + (double) ycoord *ydiff;

	      if (wvals.fractaltype != TYPE_MANDELBROT)
		{
		  tmpx = x = a;
		  tmpy = y = b;
		}
	      else
		{
		  x = 0;
		  y = 0;
		}
	      for (zaehler = 0;
		   (zaehler < iteration) && ((x * x + y * y) < 4);
		   zaehler++)
		{
		  oldx = x;
		  oldy = y;

		  switch (wvals.fractaltype)
		    {
		    case TYPE_MANDELBROT:
		      xx = x * x - y * y + a;
		      y = 2.0 * x * y + b;
		      break;

		    case TYPE_JULIA:
		      xx = x * x - y * y + cx;
		      y = 2.0 * x * y + cy;
		      break;

		    case TYPE_BARNSLEY_1:
		      foldxinitx = oldx * cx;
		      foldyinity = oldy * cy;
		      foldxinity = oldx * cy;
		      foldyinitx = oldy * cx;
		      /* orbit calculation */
		      if (oldx >= 0)
			{
		          xx = (foldxinitx - cx - foldyinity);
                          y  = (foldyinitx - cy + foldxinity);
			}
		      else
			{
		          xx = (foldxinitx + cx - foldyinity);
		          y  = (foldyinitx + cy + foldxinity);
			}
		      break;

		    case TYPE_BARNSLEY_2:
		      foldxinitx = oldx * cx;
		      foldyinity = oldy * cy;
		      foldxinity = oldx * cy;
		      foldyinitx = oldy * cx;
		      /* orbit calculation */
		      if (foldxinity + foldyinitx >= 0)
                        {
			  xx = foldxinitx - cx - foldyinity;
			  y  = foldyinitx - cy + foldxinity;
                        }
		      else
		        {
			  xx = foldxinitx + cx - foldyinity;
			  y  = foldyinitx + cy + foldxinity;
                        }
		      break;

		    case TYPE_BARNSLEY_3:
		      foldxinitx  = oldx * oldx;
		      foldyinity  = oldy * oldy;
		      foldxinity  = oldx * oldy;
		      /* orbit calculation */
		      if(oldx > 0)
			{
			  xx = foldxinitx - foldyinity - 1.0;
			  y  = foldxinity * 2;
			}
		      else
			{
			  xx = foldxinitx - foldyinity -1.0 + cx * oldx;
			  y  = foldxinity * 2;
			  y += cy * oldx;
			}
		      break;

		    case TYPE_SPIDER:
		      /* { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
		      xx = x*x - y*y + tmpx + cx;
		      y = 2 * oldx * oldy + tmpy +cy;
		      tmpx = tmpx/2 + xx;
		      tmpy = tmpy/2 + y;
		      break;

		    case TYPE_MAN_O_WAR:
		      xx = x*x - y*y + tmpx + cx;
		      y = 2.0 * x * y + tmpy + cy;
		      tmpx = oldx;
		      tmpy = oldy;
		      break;

		    case TYPE_LAMBDA:
		      tempsqrx = x * x;
		      tempsqry = y * y;
		      tempsqrx = oldx - tempsqrx + tempsqry;
		      tempsqry = -(oldy * oldx);
		      tempsqry += tempsqry + oldy;
		      xx = cx * tempsqrx - cy * tempsqry;
		      y = cx * tempsqry + cy * tempsqrx;
		      break;

		    case TYPE_SIERPINSKI:
		      xx = oldx + oldx;
		      y = oldy + oldy;
		      if (oldy > .5)
			y = y - 1;
		      else if (oldx > .5)
			xx = xx - 1;
		      break;

		    default:
		      break;
		    }

		  x = xx;
		}

	      if (useloglog)
		{
		  adjust = log (log (x * x + y * y) / 2) / log (2);
		}
	      else
		{
		  adjust = 0.0;
		}
	      color = (int) (((zaehler - adjust) *
			      (wvals.ncolors - 1)) / iteration);
	      p_ul[0] = colormap[color][0];
	      p_ul[1] = colormap[color][1];
	      p_ul[2] = colormap[color][2];
	      p_ul += 3;
	      px += 1;
	    } /* for */
	  py += 1;
	} /* for */

      preview_redraw ();
      gdk_flush ();
    }
}

/**********************************************************************
 FUNCTION: set_cmap_preview()
 *********************************************************************/

void
set_cmap_preview (void)
{
  gint    i;
  gint    x;
  gint    y;
  gint    j;
  guchar *b;
  guchar  c[GR_WIDTH * 3];
  gint    xsize, ysize;

  if (NULL == cmap_preview)
    return;

  make_color_map ();

  for (ysize = 1; ysize * ysize * ysize < wvals.ncolors; ysize++) /**/;
  xsize = wvals.ncolors / ysize;
  while (xsize * ysize < wvals.ncolors) xsize++;
  b = g_new (guchar, xsize * 3);

  gtk_preview_size (GTK_PREVIEW (cmap_preview), xsize, ysize * 4);
  gtk_widget_set_size_request (GTK_WIDGET (cmap_preview), xsize, ysize * 4);

  for (y = 0; y < ysize*4; y += 4)
    {
      for (x = 0; x < xsize; x++)
	{
	  i = x + (y / 4) * xsize;
	  if (i > wvals.ncolors)
	    {
	      for (j = 0; j < 3; j++)
		b[x * 3 + j] = 0;
	    }
	  else
	    {
	      for (j = 0; j < 3; j++)
		b[x * 3 + j] = colormap[i][j];
	    }
	}

      gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y, xsize);
      gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y + 1, xsize);
      gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y + 2, xsize);
      gtk_preview_draw_row (GTK_PREVIEW (cmap_preview), b, 0, y + 3, xsize);
    }

  for (x = 0; x < GR_WIDTH; x++)
    {
      for (j = 0; j < 3; j++)
	c[x * 3 + j] = colormap[(int)((float)x/(float)GR_WIDTH*256.0)][j];
    }

  gtk_widget_queue_draw (cmap_preview);

  g_free (b);
}

/**********************************************************************
 FUNCTION: make_color_map()
 *********************************************************************/

void
make_color_map (void)
{
  gint     i;
  gint     j;
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
    gradient_samples = gimp_gradients_sample_uniform (wvals.ncolors,
                                                      wvals.gradinvert);

  redstretch   = wvals.redstretch * 127.5;
  greenstretch = wvals.greenstretch * 127.5;
  bluestretch  = wvals.bluestretch * 127.5;

  for (i = 0; i < wvals.ncolors; i++)
    if (wvals.colormode == 1)
      {
	for (j = 0; j < 3; j++)
	  colormap[i][j] = (int) (gradient_samples[i * 4 + j] * 255.0);
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

	colormap[i][0] = r;
	colormap[i][1] = gr;
	colormap[i][2] = bl;
      }
}

/**********************************************************************
 FUNCTION: explorer_logo_dialog
 *********************************************************************/

static void
explorer_logo_dialog (GtkWidget *parent)
{
  static GtkWidget *logodlg;
  GtkWidget *xdlg;
  GtkWidget *xlabel = NULL;
  GtkWidget *xlogo_box;
  GtkWidget *xpreview;
  GtkWidget *xframe;
  GtkWidget *xframe2;
  GtkWidget *xframe3;
  GtkWidget *xvbox;
  GtkWidget *xhbox;
  GtkWidget *vpaned;
  guchar    *temp;
  guchar    *temp2;
  guchar    *datapointer;
  gint       y;
  gint       x;

  if (logodlg)
    {
      gtk_window_present (GTK_WINDOW (logodlg));
      return;
    }

  xdlg = logodlg =
    gimp_dialog_new (_("About"), "fractalexplorer",
                     parent, 0,
		     gimp_standard_help_func, "filters/fractalexplorer.html",

		     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

		     NULL);



  g_signal_connect (xdlg, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);
  g_signal_connect (xdlg, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &logodlg);

  xframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (xframe), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (xframe), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (xdlg)->vbox), xframe, TRUE, TRUE, 0);

  xvbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (xvbox), 4);
  gtk_container_add (GTK_CONTAINER (xframe), xvbox);

  /*  The logo frame & drawing area  */
  xhbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (xvbox), xhbox, FALSE, TRUE, 0);

  xlogo_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (xhbox), xlogo_box, FALSE, FALSE, 0);

  xframe2 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(xframe2), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(xlogo_box), xframe2, FALSE, FALSE, 0);

  xpreview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (xpreview), logo_width, logo_height);
  temp = g_malloc ((logo_width + 10) * 3);
  datapointer = header_data + logo_width * logo_height - 1;

  for (y = 0; y < logo_height; y++)
    {
      temp2 = temp;
      for (x = 0; x < logo_width; x++)
	{
	  HEADER_PIXEL(datapointer, temp2);
	  temp2 += 3;
	}

      gtk_preview_draw_row (GTK_PREVIEW (xpreview),
			    temp,
			    0, y, logo_width);
    }

  g_free (temp);
  gtk_container_add (GTK_CONTAINER (xframe2), xpreview);
  gtk_widget_show (xpreview);
  gtk_widget_show (xframe2);
  gtk_widget_show (xlogo_box);
  gtk_widget_show (xhbox);

  xhbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (xvbox), xhbox, TRUE, TRUE, 0);

  vpaned = gtk_vpaned_new ();
  gtk_box_pack_start (GTK_BOX (xhbox), vpaned, TRUE, TRUE, 0);
  gtk_widget_show (vpaned);

  xframe3 = gtk_frame_new (NULL);
  gtk_paned_add1 (GTK_PANED (vpaned), xframe3);
  gtk_widget_show (xframe3);

  xlabel = gtk_label_new ("\nCotting Software Productions\n"
			  "Quellenstrasse 10\n"
			  "CH-8005 Zuerich (Switzerland)\n\n"
			  "cotting@multimania.com\n"
			  "http://www.multimania.com/cotting\n\n"
			  "Fractal Chaos Explorer\nPlug-In for the GIMP\n"
			  "Version 2.00 Beta 2 (Multilingual)\n");
  gtk_container_add (GTK_CONTAINER (xframe3),  xlabel);
  gtk_widget_show (xlabel);

  xframe3 = gtk_frame_new (NULL);
  gtk_paned_add2 (GTK_PANED (vpaned), xframe3);
  gtk_widget_show (xframe3);

  xlabel = gtk_label_new ("\nContains code from:\n\n"
			  "Daniel Cotting\n<cotting@mygale.org>\n"
			  "Peter Kirchgessner\n<Pkirchg@aol.com>\n"
			  "Scott Draves\n<spot@cs.cmu.edu>\n"
			  "Andy Thomas\n<alt@picnic.demon.co.uk>\n"
			  "and the GIMP distribution.\n");
  gtk_container_add (GTK_CONTAINER (xframe3),  xlabel);
  gtk_widget_show (xlabel);

  gtk_widget_show (xhbox);

  gtk_widget_show (xvbox);
  gtk_widget_show (xframe);
  gtk_widget_show (xdlg);
}

/**********************************************************************
 FUNCTION: dialog_change_scale
 *********************************************************************/

void
dialog_change_scale (void)
{
  ready_now = FALSE;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->xmin), wvals.xmin);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->xmax), wvals.xmax);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->ymin), wvals.ymin);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->ymax), wvals.ymax);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->iter), wvals.iter);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->cx),   wvals.cx);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->cy),   wvals.cy);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->red),  wvals.redstretch);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->green),wvals.greenstretch);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (elements->blue), wvals.bluestretch);

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
  /* Save options */

  fprintf (fp, "fractaltype: %i\n", wvals.fractaltype);
  fprintf (fp, "xmin: %0.15f\n", wvals.xmin);
  fprintf (fp, "xmax: %0.15f\n", wvals.xmax);
  fprintf (fp, "ymin: %0.15f\n", wvals.ymin);
  fprintf (fp, "ymax: %0.15f\n", wvals.ymax);
  fprintf (fp, "iter: %0.15f\n", wvals.iter);
  fprintf (fp, "cx: %0.15f\n", wvals.cx);
  fprintf (fp, "cy: %0.15f\n", wvals.cy);
  fprintf (fp, "redstretch: %0.15f\n", wvals.redstretch * 128.0);
  fprintf (fp, "greenstretch: %0.15f\n", wvals.greenstretch * 128.0);
  fprintf (fp, "bluestretch: %0.15f\n", wvals.bluestretch * 128.0);
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
  FILE  *fp;
  gchar *savename;

  savename = filename;

  fp = fopen (savename, "wt+");

  if (!fp)
    {
      g_message (_("Can't open '%s' for writing:\n%s"),
                 savename, g_strerror (errno));
      return;
    }

  /* Write header out */
  fputs (FRACTAL_HEADER, fp);
  fputs ("#**********************************************************************\n", fp);
  fputs ("# This is a data file for the Fractal Explorer plug-in for the GIMP   *\n", fp);
  fputs ("# Get the plug-in at              http://www.multimania.com/cotting   *\n", fp);
  fputs ("#**********************************************************************\n", fp);

  save_options (fp);

  if (ferror (fp))
    g_message (_("Failed to write '%s':\n%s"),
               savename, g_strerror (ferror (fp)));

  fclose (fp);
}

static void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{
  const gchar *filenamebuf;

  filenamebuf = gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs));

  /* Get the name */
  if (!filenamebuf || strlen (filenamebuf) == 0)
    {
      g_message (_("Save: No filename given"));
      return;
    }

  if (g_file_test (filenamebuf, G_FILE_TEST_IS_DIR))
    {
      /* Can't save to directory */
      g_message (_("Save: Can't save to a folder."));
      return;
    }

  filename = g_strdup (filenamebuf);
  save_callback ();
  gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
load_file_selection_ok (GtkWidget        *w,
			GtkFileSelection *fs)
{
  filename =
    g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));

  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      explorer_load ();
    }

  gtk_widget_destroy (GTK_WIDGET (fs));
  gtk_widget_show (maindlg);
  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

static void
create_load_file_selection (GtkWidget *widget,
                            GtkWidget *dialog)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      window = gtk_file_selection_new (_("Load Fractal Parameters"));
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);

      gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (dialog));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);

      g_signal_connect (GTK_FILE_SELECTION (window)->ok_button, "clicked",
                        G_CALLBACK (load_file_selection_ok),
                        window);

      g_signal_connect_swapped (GTK_FILE_SELECTION (window)->cancel_button,
                                "clicked",
                                G_CALLBACK (gtk_widget_destroy),
                                window);
    }

  gtk_widget_show (window);
}

static void
create_file_selection (GtkWidget *widget,
                       GtkWidget *dialog)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      window = gtk_file_selection_new (_("Save Fractal Parameters"));
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);

      gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (dialog));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &window);

      g_signal_connect (GTK_FILE_SELECTION (window)->ok_button, "clicked",
                        G_CALLBACK (file_selection_ok),
                        window);

      g_signal_connect_swapped (GTK_FILE_SELECTION(window)->cancel_button,
                                "clicked",
                                G_CALLBACK (gtk_widget_destroy),
                                window);
    }

  if (tpath)
    {
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (window), tpath);
    }
  else if (fractalexplorer_path)
    {
      GList *path_list;
      gchar *dir;

      path_list = gimp_path_parse (fractalexplorer_path, 16, FALSE, NULL);

      dir = gimp_path_get_user_writable_dir (path_list);

      if (!dir)
	dir = g_strdup (gimp_directory ());

      gtk_file_selection_set_filename (GTK_FILE_SELECTION (window), dir);

      g_free (dir);
      gimp_path_free (path_list);
    }
  else
    {
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (window), "/tmp");
    }

  gtk_widget_show (window);
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
      sscanf (load_buf, "%s %s", str_buf, opt_buf);

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
  fp = fopen (filename, "rt");

  if (!fp)
    {
      g_message (_("Can't open '%s':\n%s"), filename, g_strerror (errno));
      return;
    }
  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (FRACTAL_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("'%s'\nis not a FractalExplorer file"), filename);
      return;
    }
  if (load_options (current_obj,fp))
    {
      g_message (_("'%s' is corrupt.\nLine %d Option section incorrect"),
		 filename, line_no);
      return;
    }

  wvals = current_obj->opts;

  fclose (fp);
}
