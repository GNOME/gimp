#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include <libgimp/gimpui.h>

#include "FractalExplorer.h"
#include "Dialogs.h"
#include "Events.h"

#include "logo.h"

#include "libgimp/stdplugins-intl.h"


#ifdef G_OS_WIN32
#  include <io.h>
#  ifndef W_OK
#    define W_OK 2
#  endif
#  ifndef S_ISDIR
#    define S_ISDIR(m) ((m) & _S_IFDIR)
#  endif
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

static gdouble *gradient_samples = NULL;
static gchar   *gradient_name    = NULL;

/**********************************************************************
 FORWARD DECLARATIONS
 *********************************************************************/

static void explorer_logo_dialog (void);

/**********************************************************************
 CALLBACKS
 *********************************************************************/

static void
dialog_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  wint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
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
  wvals.cx = standardvals.cx;
  wvals.cy = standardvals.cy;

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
  set_cmap_preview();
  dialog_update_preview();
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

  xdifferenz = wvals.xmax-wvals.xmin;
  ydifferenz = wvals.ymax-wvals.ymin;
  wvals.xmin += 1.0/6.0*xdifferenz;
  wvals.ymin += 1.0/6.0*ydifferenz;
  wvals.xmax -= 1.0/6.0*xdifferenz;
  wvals.ymax -= 1.0/6.0*ydifferenz;
  zooms[zoomindex] = wvals;

  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}				/* dialog_step_in_callback */

static void
dialog_step_out_callback (GtkWidget *widget,
			  gpointer   data)
{
  double xdifferenz;
  double ydifferenz;

  if (zoomindex < zoommax)
    {
      zooms[zoomindex]=wvals;
      zoomindex++;
    }

  xdifferenz = wvals.xmax-wvals.xmin;
  ydifferenz = wvals.ymax-wvals.ymin;
  wvals.xmin -= 1.0/4.0*xdifferenz;
  wvals.ymin -= 1.0/4.0*ydifferenz;
  wvals.xmax += 1.0/4.0*xdifferenz;
  wvals.ymax += 1.0/4.0*ydifferenz;
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

  if (gradient_name == NULL)
    gradient_name = gimp_gradients_get_active ();

  gimp_gradient_get_gradient_data (gradient_name, &dummy, wvals.ncolors,
				   &gradient_samples);

  set_cmap_preview ();
  dialog_update_preview ();
}

static void
explorer_gradient_select_callback (gchar    *name,
				   gint      width,
				   gdouble  *gradient_data,
				   gint      dialog_closing,
				   gpointer  data)
{
  gint dummy;

  g_free (gradient_name);
  g_free (gradient_samples);

  gradient_name = g_strdup (name);

  gimp_gradient_get_gradient_data (gradient_name, &dummy, wvals.ncolors,
				   &gradient_samples);

  if (wvals.colormode == 1)
    {
      set_cmap_preview();
      dialog_update_preview(); 
    }
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
  gint     argc;
  gchar  **argv;
  guchar  *color_cube;
  GSList  *group = NULL;
  gint     i;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("fractalexplorer");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  plug_in_parse_fractalexplorer_path ();

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  wint.wimage = g_new (guchar, preview_width * preview_height * 3);
  elements    = g_new (DialogElements, 1);

  dialog = maindlg =
    gimp_dialog_new ("Fractal Explorer <Daniel Cotting/cotting@multimania.com>",
		     "fractalexplorer",
		     gimp_plugin_help_func, "filters/fractalexplorer.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("About"), explorer_logo_dialog,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("OK"), dialog_ok_callback,
		     NULL, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

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
  gtk_signal_connect (GTK_OBJECT (wint.preview), "button_press_event",
		      (GtkSignalFunc) preview_button_press_event,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (wint.preview), "button_release_event",
		      (GtkSignalFunc) preview_button_release_event,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (wint.preview), "motion_notify_event",
		      (GtkSignalFunc) preview_motion_notify_event,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (wint.preview), "leave_notify_event",
		      (GtkSignalFunc) preview_leave_notify_event,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (wint.preview), "enter_notify_event",
		      (GtkSignalFunc) preview_enter_notify_event,
		      NULL);
  gtk_widget_set_events (wint.preview, (GDK_BUTTON_PRESS_MASK |
					GDK_BUTTON_RELEASE_MASK |
					GDK_POINTER_MOTION_MASK |
					GDK_LEAVE_NOTIFY_MASK |
					GDK_ENTER_NOTIFY_MASK));
  gtk_widget_show (wint.preview);

  toggle = gtk_check_button_new_with_label (_("Realtime Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_toggle_update),
		      &wvals.alwayspreview);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.alwayspreview);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle, _("If you enable this option the preview "
				     "will be redrawn automatically"), NULL);

  button = gtk_button_new_with_label (_("Redraw"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_redraw_callback),
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

  button = gtk_button_new_with_label (_("Undo Zoom"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_undo_zoom_callback),
		      dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Undo last zoom"), NULL);

  button = gtk_button_new_with_label (_("Redo Zoom"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_redo_zoom_callback),
		      dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Redo last zoom"), NULL);

  button = gtk_button_new_with_label (_("Step In"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_step_in_callback),
		      dialog);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Step Out"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_step_out_callback),
		      dialog);
  gtk_widget_show (button);


  /*  Create notebook  */
  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (top_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  /*  "Parameters" page  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new (_("Parameters")));
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
			  _("XMIN:"), SCALE_WIDTH, 100,
			  wvals.xmin, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the first (minimal) x-coordinate "
			    "delimitation"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->xmin), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.xmin);

  elements->xmax =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("XMAX:"), SCALE_WIDTH, 100,
			  wvals.xmax, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the second (maximal) x-coordinate "
			    "delimitation"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->xmax), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.xmax);

  elements->ymin =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("YMIN:"), SCALE_WIDTH, 100,
			  wvals.ymin, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the first (minimal) y-coordinate "
			    "delimitation"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->ymin), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.ymin);

  elements->ymax =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			  _("YMAX:"), SCALE_WIDTH, 100,
			  wvals.ymax, -3, 3, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the second (maximal) y-coordinate "
			    "delimitation"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->ymax), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.ymax);

  elements->iter =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			  _("ITER:"), SCALE_WIDTH, 100,
			  wvals.iter, 0, 1000, 1, 10, 5,
			  TRUE, 0, 0,
			  _("Change the iteration value. The higher it "
			    "is, the more details will be calculated, "
			    "which will take more time"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->iter), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.iter);

  elements->cx =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
			  _("CX:"), SCALE_WIDTH, 100,
			  wvals.cx, -2.5, 2.5, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the CX value (changes aspect of "
			    "fractal, active with every fractal but "
			    "Mandelbrot and Sierpinski)"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->cx), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.cx);

  elements->cy =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 6,
			  _("CY:"), SCALE_WIDTH, 100,
			  wvals.cy, -2.5, 2.5, 0.001, 0.01, 5,
			  TRUE, 0, 0,
			  _("Change the CY value (changes aspect of "
			    "fractal, active with every fractal but "
			    "Mandelbrot and Sierpinski)"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->cy), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.cy);

  hbbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), hbbox, 0, 3, 7, 8);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_label (_("Load"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (create_load_file_selection),
		      dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Load a fractal from file"), NULL);

  button = gtk_button_new_with_label (_("Reset"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
		      GTK_SIGNAL_FUNC (dialog_reset_callback),
		      dialog);
  gtk_widget_show (button);
  gimp_help_set_help_data (button, _("Reset parameters to default values"),
			   NULL);

  button = gtk_button_new_with_label (_("Save"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (create_file_selection),
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
			   explorer_radio_update,
			   &wvals.fractaltype,
			   (gpointer) &wvals.fractaltype,

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
      gtk_object_ref (GTK_OBJECT (elements->type[i]));
      gtk_widget_hide (elements->type[i]);
      gtk_container_remove (GTK_CONTAINER (toggle_vbox), elements->type[i]);
      gtk_box_pack_start (GTK_BOX (toggle_vbox2), elements->type[i],
			  FALSE, FALSE, 0);
      gtk_widget_show (elements->type[i]);
      gtk_object_unref (GTK_OBJECT (elements->type[i]));
    }

  toggle_vbox3 = gtk_vbox_new (FALSE, 1);
  for (i = TYPE_MAN_O_WAR; i <= TYPE_SIERPINSKI; i++)
    {
      gtk_object_ref (GTK_OBJECT (elements->type[i]));
      gtk_widget_hide (elements->type[i]);
      gtk_container_remove (GTK_CONTAINER (toggle_vbox), elements->type[i]);
      gtk_box_pack_start (GTK_BOX (toggle_vbox3), elements->type[i],
			  FALSE, FALSE, 0);
      gtk_widget_show (elements->type[i]);
      gtk_object_unref (GTK_OBJECT (elements->type[i]));
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
			    gtk_label_new (_("Colors")));
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
			  _("Number of Colors:"), SCALE_WIDTH, 0,
			  wvals.ncolors, 2, MAXNCOLORS, 1, 10, 0,
			  TRUE, 0, 0,
			  _("Change the number of colors in the mapping"),
			  NULL);
  gtk_signal_connect (GTK_OBJECT (elements->ncol), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_number_of_colors_callback),
		      &wvals.ncolors);

  elements->useloglog = toggle =
    gtk_check_button_new_with_label (_("Use loglog Smoothing"));
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 0, 3, 1, 2);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) explorer_toggle_update,
		      &wvals.useloglog);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), wvals.useloglog);
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
  gtk_signal_connect (GTK_OBJECT (elements->red), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.redstretch);

  elements->green =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("Green:"), SCALE_WIDTH, 0,
			  wvals.greenstretch, 0, 1, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Change the intensity of the green channel"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->green), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
		      &wvals.greenstretch);

  elements->blue =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("Blue:"), SCALE_WIDTH, 0,
			  wvals.bluestretch, 0, 1, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("Change the intensity of the blue channel"), NULL);
  gtk_signal_connect (GTK_OBJECT (elements->blue), "value_changed",
		      GTK_SIGNAL_FUNC (explorer_double_adjustment_update),
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
				 explorer_radio_update,
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
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_toggle_update),
		      &wvals.redinvert);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
			   _("If you enable this option higher color values "
			     "will be swapped with lower ones and vice "
			     "versa"), NULL);

  /*  Greenmode radio frame  */
  frame = gimp_radio_group_new2 (TRUE, _("Green"),
				 explorer_radio_update,
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
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_toggle_update),
		      &wvals.greeninvert);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
			   _("If you enable this option higher color values "
			     "will be swapped with lower ones and vice "
			     "versa"), NULL);

  /*  Bluemode radio frame  */
  frame = gimp_radio_group_new2 (TRUE, _("Blue"),
				 explorer_radio_update,
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
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON( toggle), wvals.blueinvert);
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_toggle_update),
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
    gtk_radio_button_new_with_label (group, _("As Specified above"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_radio_update),
		      &wvals.colormode);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
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
				     _("Apply Active Gradient to Final Image"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (toggle), (gpointer) 1);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (explorer_radio_update),
		      &wvals.colormode);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				wvals.colormode == 1);
  gtk_widget_show (toggle);
  gimp_help_set_help_data (toggle,
			   _("Create a color-map using a gradient from "
			     "the gradient editor"), NULL);

  gradient_name = gimp_gradients_get_active ();
  gradient_samples = gimp_gradients_sample_uniform (wvals.ncolors);
  gradient = gimp_gradient_select_widget (_("FractalExplorer Gradient"),
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

    gtk_widget_set_usize (abox, xsize, ysize * 4);
  }
  gtk_box_pack_start (GTK_BOX (toggle_vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  cmap_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (cmap_preview), 32, 32);
  gtk_container_add (GTK_CONTAINER (abox), cmap_preview);
  gtk_widget_show (cmap_preview);

  frame = add_objects_list ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
			    gtk_label_new (_("Fractals")));
  gtk_widget_show (frame);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);

  /* Done */

  /* Popup for list area: Not yet fully implemented
     fractalexplorer_op_menu_create(maindlg);
  */

  gtk_widget_show (dialog);
  ready_now = TRUE;

  set_cmap_preview ();
  dialog_update_preview ();

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  if (the_tile != NULL)
    {
      gimp_tile_unref (the_tile, FALSE);
      the_tile = NULL;
    }

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
  guchar  *p;
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

  if ((ready_now) && (wvals.alwayspreview))
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

      p = wint.wimage;

      for (y = 0; y < preview_height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (wint.preview), p,
				0, y, preview_width);
	  p += preview_width * 3;
	}

      gtk_widget_draw (wint.preview, NULL);
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

  gtk_preview_size     (GTK_PREVIEW (cmap_preview), xsize, ysize * 4);
  gtk_widget_set_usize (GTK_WIDGET (cmap_preview),  xsize, ysize * 4);
    
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

  gtk_widget_draw (cmap_preview, NULL);

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
explorer_logo_dialog (void)
{
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
  guchar *temp;
  guchar *temp2;
  guchar *datapointer;
  gint    y;
  gint    x;

  if (logodlg)
    {
      gdk_window_raise (logodlg->window);
      return;
    }

  xdlg = logodlg =
    gimp_dialog_new (_("About"), "fractalexplorer",
		     gimp_plugin_help_func, "filters/fractalexplorer.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), gtk_widget_destroy,
		     NULL, 1, NULL, TRUE, TRUE,

		     NULL);

  gtk_signal_connect (GTK_OBJECT (xdlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
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
      gtk_preview_draw_row(GTK_PREVIEW(xpreview),
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

void
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

/**********************************************************************
 FUNCTION: save_callback
 *********************************************************************/

void
save_callback (void)
{
  FILE  *fp;
  gchar *savename;
  gchar *message;

  savename = filename;

  fp = fopen (savename, "wt+");

  if (!fp) 
    {
      message = g_strconcat (_("Error opening: %s"), 
			     "\n",
			     _("Could not save."), 
			     savename);
      g_message (message);
      g_free (message);
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
    g_message (_("Failed to write file\n"));
  fclose (fp);
}

/**********************************************************************
 FUNCTION: file_selection_ok
 *********************************************************************/

void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs,
		   gpointer          data)
{
  gchar       *filenamebuf;
  struct stat  filestat;
  gint         err;

  filenamebuf = gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs));

  /* Get the name */
  if (strlen (filenamebuf) == 0)
    {
      g_message (_("Save: No filename given"));
      return;
    }

  /* Check if directory exists */
  err = stat (filenamebuf, &filestat);

  if (!err && S_ISDIR (filestat.st_mode))
    {
      /* Can't save to directory */
      g_message (_("Save: Can't save to a directory"));
      return;
    }

  filename = g_strdup (filenamebuf);
  save_callback ();
  gtk_widget_destroy (GTK_WIDGET (fs));
}

/**********************************************************************
 FUNCTION: load_file_selection_ok
**********************************************************************/

void
load_file_selection_ok (GtkWidget        *w,
			GtkFileSelection *fs,
			gpointer          data)
{
  struct stat filestat;
  gint        err;

  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs)));

  err = stat (filename, &filestat);

  if (!err && S_ISREG (filestat.st_mode))
    {
      explorer_load ();
    }

  gtk_widget_destroy (GTK_WIDGET (fs));
  gtk_widget_show (maindlg);
  dialog_change_scale ();
  set_cmap_preview ();
  dialog_update_preview ();
}

/**********************************************************************
 FUNCTION: create_load_file_selection
 *********************************************************************/

void
create_load_file_selection (void)
{
  static GtkWidget *window = NULL;

  /* Load a single object */
  if (!window)
    {
      window = gtk_file_selection_new (_("Load Fractal Parameters"));
      gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &window);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (load_file_selection_ok),
			  (gpointer) window);
      gimp_help_set_help_data (GTK_FILE_SELECTION(window)->ok_button, _("Click here to load your file"), NULL);

      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
				 "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_destroy),
				 GTK_OBJECT (window));
      gimp_help_set_help_data (GTK_FILE_SELECTION(window)->cancel_button, _("Click here to cancel load procedure"), NULL);
    }

  if (!GTK_WIDGET_VISIBLE (window))
    gtk_widget_show (window);
}

/**********************************************************************
 FUNCTION: create_file_selection
 *********************************************************************/

void
create_file_selection (void)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      window = gtk_file_selection_new (_("Save Fractal Parameters"));
      gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &window);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (file_selection_ok),
			  (gpointer) window);
      gimp_help_set_help_data (GTK_FILE_SELECTION(window)->ok_button,
			       _("Click here to save your file"), NULL);

      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(window)->cancel_button),
				 "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_destroy),
				 GTK_OBJECT (window));
      gimp_help_set_help_data (GTK_FILE_SELECTION (window)->cancel_button,
			       _("Click here to cancel save procedure"), NULL);
    }
  if (tpath)
    {
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (window), tpath);
    }
  else if (fractalexplorer_path_list)
    {
      gchar *dir;

      dir = gimp_path_get_user_writable_dir (fractalexplorer_path_list);

      if (!dir)
	dir = g_strdup (gimp_directory ());

      gtk_file_selection_set_filename (GTK_FILE_SELECTION (window), dir);

      g_free (dir);
    }
  else
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (window),"/tmp");

  if (!GTK_WIDGET_VISIBLE (window))
    gtk_widget_show (window);
}

/**********************************************************************
 FUNCTION: get_line
 *********************************************************************/

gchar *
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

/**********************************************************************
 FUNCTION: load_options
 *********************************************************************/

gint
load_options (fractalexplorerOBJ *xxx,
	      FILE               *fp)
{
  gchar load_buf[MAX_LOAD_LINE];
  gchar str_buf[MAX_LOAD_LINE];
  gchar opt_buf[MAX_LOAD_LINE];

  /*  default values  */
  xxx->opts.fractaltype   =   0;
  xxx->opts.xmin          =  -2.0;
  xxx->opts.xmax          =   2.0;
  xxx->opts.ymin          =  -1.5;
  xxx->opts.ymax          =   1.5;
  xxx->opts.iter          =  50.0;
  xxx->opts.cx            =  -0.75;
  xxx->opts.cy            =  -0.2;
  xxx->opts.colormode     =   0;
  xxx->opts.redstretch    =   1.0;
  xxx->opts.greenstretch  =   1.0;
  xxx->opts.bluestretch   =   1.0;
  xxx->opts.redmode       =   1;
  xxx->opts.greenmode     =   1;
  xxx->opts.bluemode      =   1;
  xxx->opts.redinvert     =   0;
  xxx->opts.greeninvert   =   0;
  xxx->opts.blueinvert    =   0;
  xxx->opts.alwayspreview =   1;
  xxx->opts.ncolors       = 256;  /*  not saved  */

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
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.xmin = sp;
	}
      else if (!strcmp (str_buf, "xmax:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.xmax = sp;
	}
      else if (!strcmp(str_buf, "ymin:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.ymin = sp;
	}
      else if (!strcmp (str_buf, "ymax:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.ymax = sp;
	}
      else if (!strcmp(str_buf, "redstretch:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.redstretch = sp / 128.0;
	}
      else if (!strcmp(str_buf, "greenstretch:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.greenstretch = sp / 128.0;
	}
      else if (!strcmp (str_buf, "bluestretch:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.bluestretch = sp / 128.0;
	}
      else if (!strcmp (str_buf, "iter:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.iter = sp;
	}
      else if (!strcmp(str_buf, "cx:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.cx = sp;
	}
      else if (!strcmp (str_buf, "cy:"))
	{
	  gdouble sp = 0;

	  sp = atof (opt_buf);
	  xxx->opts.cy = sp;
	}
      else if (!strcmp(str_buf, "redmode:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.redmode = sp;
	}
      else if (!strcmp(str_buf, "greenmode:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.greenmode = sp;
	}
      else if (!strcmp(str_buf, "bluemode:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.bluemode = sp;
	}
      else if (!strcmp (str_buf, "redinvert:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.redinvert = sp;
	}
      else if (!strcmp (str_buf, "greeninvert:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.greeninvert = sp;
	}
      else if (!strcmp(str_buf, "blueinvert:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.blueinvert = sp;
	}
      else if (!strcmp (str_buf, "colormode:"))
	{
	  gint sp = 0;

	  sp = atoi (opt_buf);
	  xxx->opts.colormode = sp;
	}

      get_line (load_buf, MAX_LOAD_LINE, fp, 0);
    }

  return 0;
}

/**********************************************************************
 FUNCTION: explorer_load
 *********************************************************************/

void
explorer_load (void)
{
  FILE  *fp;
  gchar  load_buf[MAX_LOAD_LINE];

  g_assert (filename != NULL); 
  fp = fopen (filename, "rt");

  if (!fp)
    {
      g_warning ("Error opening: %s", filename);
      return;
    }
  get_line (load_buf, MAX_LOAD_LINE, fp, 1);

  if (strncmp (FRACTAL_HEADER, load_buf, strlen (load_buf)))
    {
      g_message (_("File '%s' is not a FractalExplorer file"), filename);
      return;
    }
  if (load_options (current_obj,fp))
    {
      g_message (_("File '%s' is corrupt.\nLine %d Option section incorrect"), 
		 filename, line_no);
      return;
    }

  wvals = current_obj->opts;
  fclose (fp);
}
