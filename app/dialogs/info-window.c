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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-projection.h"
#include "core/gimpimage-unit.h"
#include "core/gimptemplate.h"
#include "core/gimpunit.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpcolorframe.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "info-dialog.h"
#include "info-window.h"

#include "gimp-intl.h"


#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;

struct _InfoWinData
{
  GimpDisplay *gdisp;

  gchar        dimensions_str[MAX_BUF];
  gchar        real_dimensions_str[MAX_BUF];
  gchar        scale_str[MAX_BUF];
  gchar        num_layers_str[MAX_BUF];
  gchar        memsize_str[MAX_BUF];
  gchar        color_type_str[MAX_BUF];
  gchar        visual_class_str[MAX_BUF];
  gchar        visual_depth_str[MAX_BUF];
  gchar        resolution_str[MAX_BUF];

  GtkWidget   *pixel_labels[2];
  GtkWidget   *unit_labels[2];

  GtkWidget   *frame1;
  GtkWidget   *frame2;

  gboolean     showing_extended;
};


/*  The different classes of visuals  */
static const gchar *visual_classes[] =
{
  N_("Static Gray"),
  N_("Grayscale"),
  N_("Static Color"),
  N_("Pseudo Color"),
  N_("True Color"),
  N_("Direct Color"),
};


static void
info_window_response (GtkWidget  *widget,
                      gint        response_id,
                      InfoDialog *info_win)
{
  info_dialog_hide (info_win);
}

static void
info_window_page_switch (GtkWidget       *widget,
			 GtkNotebookPage *page,
			 gint             page_num,
                         InfoDialog      *info_win)
{
  InfoWinData *iwd;

  iwd = (InfoWinData *) info_win->user_data;

  iwd->showing_extended = (page_num == 1);
}


/*  displays information:
 *    cursor pos
 *    cursor pos in real units
 *    color under cursor
 *  Can't we find a better place for this than in the image window? (Ralf)
 */

static void
info_window_create_extended (InfoDialog *info_win,
                             Gimp       *gimp)
{
  GtkWidget   *hbox;
  GtkWidget   *frame;
  GtkWidget   *table;
  GtkWidget   *vbox;
  InfoWinData *iwd;

  iwd = (InfoWinData *) info_win->user_data;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (info_win->info_notebook),
			    vbox, gtk_label_new (_("Extended")));
  gtk_widget_show (vbox);


  /* cursor information */

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  iwd->pixel_labels[0] = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (iwd->pixel_labels[0]), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X:"), 1.0, 0.5,
                             iwd->pixel_labels[0], 1, FALSE);

  iwd->pixel_labels[1] = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (iwd->pixel_labels[1]), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y:"), 1.0, 0.5,
                             iwd->pixel_labels[1], 1, FALSE);

  frame = gtk_frame_new (_("Units"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  iwd->unit_labels[0] = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (iwd->unit_labels[0]), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X:"), 1.0, 0.5,
                             iwd->unit_labels[0], 1, FALSE);

  iwd->unit_labels[1] = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (iwd->unit_labels[1]), 0.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y:"), 1.0, 0.5,
                             iwd->unit_labels[1], 1, FALSE);


  /* color information */

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  iwd->frame1 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (iwd->frame1),
                             GIMP_COLOR_FRAME_MODE_PIXEL);
  gtk_box_pack_start (GTK_BOX (hbox), iwd->frame1, TRUE, TRUE, 0);
  gtk_widget_show (iwd->frame1);

  iwd->frame2 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (iwd->frame2),
                             GIMP_COLOR_FRAME_MODE_RGB);
  gtk_box_pack_start (GTK_BOX (hbox), iwd->frame2, TRUE, TRUE, 0);
  gtk_widget_show (iwd->frame2);


  /* Set back to first page */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (info_win->info_notebook), 0);

  g_signal_connect (info_win->info_notebook, "switch_page",
		    G_CALLBACK (info_window_page_switch),
		    info_win);
}


InfoDialog *
info_window_create (GimpDisplay *gdisp)
{
  InfoDialog  *info_win;
  InfoWinData *iwd;
  gint         type;

  type = gimp_image_base_type (gdisp->gimage);

  info_win = info_dialog_notebook_new (GIMP_VIEWABLE (gdisp->gimage),
                                       _("Info Window"), "gimp-info-window",
                                       GIMP_STOCK_INFO,
                                       _("Image Information"),
                                       gdisp->shell,
				       gimp_standard_help_func,
				       GIMP_HELP_INFO_DIALOG);

  gtk_dialog_add_button (GTK_DIALOG (info_win->shell),
                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect (info_win->shell, "response",
                    G_CALLBACK (info_window_response),
                    info_win);

  iwd = g_new0 (InfoWinData, 1);
  iwd->gdisp = gdisp;
  info_win->user_data = iwd;

  /*  add the information fields  */
  info_dialog_add_label (info_win, _("Pixel Dimensions:"),
			 iwd->dimensions_str);
  info_dialog_add_label (info_win, _("Print Size:"),
			 iwd->real_dimensions_str);
  info_dialog_add_label (info_win, _("Resolution:"),
			iwd->resolution_str);
  info_dialog_add_label (info_win, _("Scale Ratio:"),
			 iwd->scale_str);
  info_dialog_add_label (info_win, _("Number of Layers:"),
			 iwd->num_layers_str);
  info_dialog_add_label (info_win, _("Size in Memory:"),
			 iwd->memsize_str);
  info_dialog_add_label (info_win, _("Display Type:"),
			 iwd->color_type_str);
  info_dialog_add_label (info_win, _("Visual Class:"),
			 iwd->visual_class_str);
  info_dialog_add_label (info_win, _("Visual Depth:"),
			 iwd->visual_depth_str);

  /*  Add extra tabs  */
  info_window_create_extended (info_win, gdisp->gimage->gimp);

  return info_win;
}

static InfoDialog *info_window_auto = NULL;

static void
info_window_change_display (GimpContext *context,
			    GimpDisplay *newdisp,
			    gpointer     data)
{
  GimpDisplay *gdisp = newdisp;
  GimpDisplay *old_gdisp;
  GimpImage   *gimage;
  InfoWinData *iwd;

  iwd = (InfoWinData *) info_window_auto->user_data;

  old_gdisp = (GimpDisplay *) iwd->gdisp;

  if (!info_window_auto || gdisp == old_gdisp || !gdisp)
    return;

  gimage = gdisp->gimage;

  if (gimage && gimp_container_have (context->gimp->images,
				     GIMP_OBJECT (gimage)))
    {
      iwd->gdisp = gdisp;
      info_window_update (gdisp);
    }
}

void
info_window_follow_auto (Gimp *gimp)
{
  GimpContext *context;
  GimpDisplay *gdisp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  context = gimp_get_user_context (gimp);

  gdisp = gimp_context_get_display (context);

  if (! gdisp)
    return;

  if (! info_window_auto)
    {
      info_window_auto = info_window_create (gdisp);

      g_signal_connect (context, "display_changed",
			G_CALLBACK (info_window_change_display),
			NULL);

      info_window_update (gdisp);
    }

  info_dialog_present (info_window_auto);
}


/* Updates all extended information. */

void
info_window_update_extended (GimpDisplay *gdisp,
                             gdouble      tx,
                             gdouble      ty)
{
  InfoDialog  *info_win;
  InfoWinData *iwd;
  guchar      *color;

  info_win = GIMP_DISPLAY_SHELL (gdisp->shell)->info_dialog;

  if (! info_win && info_window_auto != NULL)
    info_win = info_window_auto;

  if (! info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  if (info_window_auto)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (info_window_auto->shell),
                                         GIMP_VIEWABLE (gdisp->gimage));
    }

  if (iwd->gdisp != gdisp)
    {
      iwd->gdisp = gdisp;

      info_window_update (gdisp);
    }

  if (! iwd || ! iwd->showing_extended)
    return;

  /* fill in position information */

  if (tx < 0.0 && ty < 0.0)
    {
      gtk_label_set_text (GTK_LABEL (iwd->pixel_labels[0]), _("n/a"));
      gtk_label_set_text (GTK_LABEL (iwd->pixel_labels[1]), _("n/a"));
      gtk_label_set_text (GTK_LABEL (iwd->unit_labels[0]),  _("n/a"));
      gtk_label_set_text (GTK_LABEL (iwd->unit_labels[1]),  _("n/a"));
    }
  else
    {
      gdouble      unit_factor;
      gint         unit_digits;
      const gchar *unit_str;
      gchar        format_buf[32];
      gchar        buf[32];

      unit_factor = gimp_image_unit_get_factor (gdisp->gimage);
      unit_digits = gimp_image_unit_get_digits (gdisp->gimage);
      unit_str    = gimp_image_unit_get_abbreviation (gdisp->gimage);

      g_snprintf (buf, sizeof (buf), "%d", (gint) tx);
      gtk_label_set_text (GTK_LABEL (iwd->pixel_labels[0]), buf);

      g_snprintf (buf, sizeof (buf), "%d", (gint) ty);
      gtk_label_set_text (GTK_LABEL (iwd->pixel_labels[1]), buf);

      g_snprintf (format_buf, sizeof (format_buf),
		  "%%.%df %s", unit_digits, unit_str);

      g_snprintf (buf, sizeof (buf), format_buf,
		  tx * unit_factor / gdisp->gimage->xresolution);
      gtk_label_set_text (GTK_LABEL (iwd->unit_labels[0]), buf);

      g_snprintf (buf, sizeof (buf), format_buf,
		  ty * unit_factor / gdisp->gimage->yresolution);
      gtk_label_set_text (GTK_LABEL (iwd->unit_labels[1]), buf);

    }

  /* fill in color information */
  color = gimp_image_projection_get_color_at (gdisp->gimage, tx, ty);

  if (! color || (tx < 0.0 && ty < 0.0))
    {
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (iwd->frame1));
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (iwd->frame2));
    }
  else
    {
      GimpImageType sample_type;
      GimpRGB       rgb;

      sample_type = gimp_image_projection_type (gdisp->gimage);

      gimp_rgba_set_uchar (&rgb,
                           color[RED_PIX],
                           color[GREEN_PIX],
                           color[BLUE_PIX],
                           color[ALPHA_PIX]);

      gimp_color_frame_set_color (GIMP_COLOR_FRAME (iwd->frame1),
                                  sample_type, &rgb, -1);
      gimp_color_frame_set_color (GIMP_COLOR_FRAME (iwd->frame2),
                                  sample_type, &rgb, -1);

      g_free (color);
    }
}

void
info_window_free (InfoDialog *info_win)
{
  InfoWinData *iwd;

  if (! info_win && info_window_auto)
    {
      gtk_widget_set_sensitive (info_window_auto->vbox, FALSE);
      return;
    }

  if (! info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  g_free (iwd);
  info_dialog_free (info_win);
}

void
info_window_update (GimpDisplay *gdisp)
{
  GimpDisplayShell *shell;
  GimpImage        *gimage;
  InfoWinData      *iwd;
  gint              type;
  gdouble           unit_factor;
  gint              unit_digits;
  GimpUnit          res_unit;
  gdouble           res_unit_factor;
  gchar             format_buf[32];
  InfoDialog       *info_win;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  info_win = shell->info_dialog;

  if (! info_win && info_window_auto != NULL)
    info_win = info_window_auto;

  if (! info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  if (info_window_auto)
    gtk_widget_set_sensitive (info_window_auto->vbox, TRUE);

  /* If doing info_window_auto then return if this display
   * is not the one we are showing.
   */
  if (info_window_auto && iwd->gdisp != gdisp)
    return;

  gimage = gdisp->gimage;

  /*  width and height  */
  unit_factor = gimp_image_unit_get_factor (gimage);
  unit_digits = gimp_image_unit_get_digits (gimage);

  g_snprintf (iwd->dimensions_str, MAX_BUF, _("%d x %d pixels"),
	      gimage->width, gimage->height);
  g_snprintf (format_buf, sizeof (format_buf), "%%.%df x %%.%df %s",
	      unit_digits + 1, unit_digits + 1,
              gimp_image_unit_get_plural (gimage));
  g_snprintf (iwd->real_dimensions_str, MAX_BUF, format_buf,
	      gimage->width  * unit_factor / gimage->xresolution,
	      gimage->height * unit_factor / gimage->yresolution);

  /*  image resolution  */
  res_unit = gimage->gimp->config->default_image->resolution_unit;
  res_unit_factor = _gimp_unit_get_factor (gimage->gimp, res_unit);

  g_snprintf (format_buf, sizeof (format_buf), _("pixels/%s"),
              _gimp_unit_get_abbreviation (gimage->gimp, res_unit));
  g_snprintf (iwd->resolution_str, MAX_BUF, _("%g x %g %s"),
	      gimage->xresolution / res_unit_factor,
	      gimage->yresolution / res_unit_factor,
              res_unit == GIMP_UNIT_INCH ? _("dpi") : format_buf);

  /*  user zoom ratio  */
  g_snprintf (iwd->scale_str, MAX_BUF, "%.2f", shell->scale * 100);

  /*  number of layers  */
  g_snprintf (iwd->num_layers_str, MAX_BUF, "%d",
              gimp_container_num_children (gimage->layers));

  /*  size in memory  */
  {
    GimpObject *object = GIMP_OBJECT (gimage);
    gchar      *str;

    str = gimp_memsize_to_string (gimp_object_get_memsize (object, NULL));

    g_snprintf (iwd->memsize_str, MAX_BUF, "%s", str);

    g_free (str);
  }

  type = gimp_image_base_type (gimage);

  /*  color type  */
  switch (type)
    {
    case GIMP_RGB:
      g_snprintf (iwd->color_type_str, MAX_BUF, "%s", _("RGB Color"));
      break;
    case GIMP_GRAY:
      g_snprintf (iwd->color_type_str, MAX_BUF, "%s", _("Grayscale"));
      break;
    case GIMP_INDEXED:
      g_snprintf (iwd->color_type_str, MAX_BUF, "%s (%d %s)",
                  _("Indexed Color"), gimage->num_cols, _("colors"));
      break;
    }

  {
    GdkScreen *screen;
    GdkVisual *visual;

    screen = gtk_widget_get_screen (GTK_WIDGET (shell));
    visual = gdk_screen_get_rgb_visual (screen);

    /*  visual class  */
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s",
                gettext (visual_classes[visual->type]));

    /*  visual depth  */
    g_snprintf (iwd->visual_depth_str, MAX_BUF, "%d", visual->depth);
  }

  info_dialog_update (info_win);
}
