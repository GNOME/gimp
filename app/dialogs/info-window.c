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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-projection.h"
#include "core/gimpunit.h"

#include "file/file-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpviewabledialog.h"

#include "info-dialog.h"
#include "info-window.h"

#include "libgimp/gimpintl.h"


#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;

struct _InfoWinData
{
  GimpDisplay *gdisp;

  gchar        dimensions_str[MAX_BUF];
  gchar        real_dimensions_str[MAX_BUF];
  gchar        scale_str[MAX_BUF];
  gchar        color_type_str[MAX_BUF];
  gchar        visual_class_str[MAX_BUF];
  gchar        visual_depth_str[MAX_BUF];
  gchar        resolution_str[MAX_BUF];

  const gchar *unit_str;

  GtkWidget   *pos_labels[4];
  GtkWidget   *unit_labels[2];
  GtkWidget   *rgb_labels[4];
  GtkWidget   *hsv_labels[4];

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
info_window_close_callback (GtkWidget *widget,
			    gpointer   data)
{
  info_dialog_popdown ((InfoDialog *) data);
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
  GtkWidget   *hbox2;
  GtkWidget   *abox;
  GtkWidget   *table;
  GtkWidget   *label;
  GtkWidget   *image;
  GtkWidget   *sep;
  InfoWinData *iwd;
  gint         i;

  iwd = (InfoWinData *) info_win->user_data;

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

  /* cursor information */

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  image = gtk_image_new_from_stock (GIMP_STOCK_TOOL_MOVE, GTK_ICON_SIZE_BUTTON);
  gtk_frame_set_label_widget (GTK_FRAME (frame), image);
  gtk_widget_show (image);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (abox), table);
  gtk_widget_show (table);

  label = gtk_label_new (_("X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  for (i = 0; i < 4; i++)
    {
      iwd->pos_labels[i] = label = gtk_label_new (_("N/A"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (label);

      if (i & 1)
        {
          iwd->unit_labels[i/2] = label = gtk_label_new (NULL);
          gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
          gtk_table_attach (GTK_TABLE (table), label, 2, 3, i, i + 1,
                            GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
          gtk_widget_show (label);
        }
    }

  /* color information */

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  image = gtk_image_new_from_stock (GIMP_STOCK_TOOL_COLOR_PICKER,
                                    GTK_ICON_SIZE_BUTTON);
  gtk_frame_set_label_widget (GTK_FRAME (frame), image);
  gtk_widget_show (image);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);
  gtk_widget_show (hbox2);

  /* RGB */

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Red */
  label = gtk_label_new (_("R:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Green */
  label = gtk_label_new (_("G:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Blue */
  label = gtk_label_new (_("B:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Alpha */
  label = gtk_label_new (_("A:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  for (i = 0; i < 4; i++)
    {
      iwd->rgb_labels[i] = label = gtk_label_new (_("N/A"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  /* HSV */

  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Hue */
  label = gtk_label_new (_("H:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Saturation */
  label = gtk_label_new (_("S:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Value */
  label = gtk_label_new (_("V:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Alpha */
  label = gtk_label_new (_("A:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  for (i = 0; i < 4; i++)
    {
      iwd->hsv_labels[i] = label = gtk_label_new (_("N/A"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  gtk_notebook_append_page (GTK_NOTEBOOK (info_win->info_notebook),
			    hbox, gtk_label_new (_("Extended")));
  gtk_widget_show (hbox);


  /* Set back to first page */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (info_win->info_notebook), 0);

  g_signal_connect (G_OBJECT (info_win->info_notebook), "switch_page",
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
                                       _("Info Window"), "info_window",
                                       GIMP_STOCK_INFO,
                                       _("Image Information"),
				       gimp_standard_help_func,
				       "dialogs/info_window.html");

  /*  create the action area  */
  gimp_dialog_create_action_area (GIMP_DIALOG (info_win->shell),

				  GTK_STOCK_CLOSE, info_window_close_callback,
				  info_win, NULL, NULL, TRUE, FALSE,

				  NULL);

  iwd = g_new0 (InfoWinData, 1);
  iwd->gdisp = gdisp;
  info_win->user_data = iwd;

  /*  add the information fields  */
  info_dialog_add_label (info_win, _("Dimensions (W x H):"),
			 iwd->dimensions_str);
  info_dialog_add_label (info_win, '\0',
			 iwd->real_dimensions_str);
  info_dialog_add_label (info_win, _("Resolution:"),
			iwd->resolution_str);
  info_dialog_add_label (info_win, _("Scale Ratio:"),
			 iwd->scale_str);
  info_dialog_add_label (info_win, _("Display Type:"),
			 iwd->color_type_str);
  info_dialog_add_label (info_win, _("Visual Class:"),
			 iwd->visual_class_str);
  info_dialog_add_label (info_win, _("Visual Depth:"),
			 iwd->visual_depth_str);

  /*  Add extra tabs  */
  info_window_create_extended (info_win, gdisp->gimage->gimp);

  /*  update the fields  */
  info_window_update (gdisp);

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

      g_signal_connect (G_OBJECT (context), "display_changed",
			G_CALLBACK (info_window_change_display), 
			NULL);

      info_window_update (gdisp);
    }

  info_dialog_popup (info_window_auto);
}

 
/* Updates all extended information. */

void  
info_window_update_extended (GimpDisplay *gdisp,
                             gdouble      tx,
                             gdouble      ty)
{
  InfoWinData   *iwd;
  gdouble        unit_factor;
  gint           unit_digits;
  gchar          format_buf[32];
  gchar          buf[32];
  guchar        *color;
  GimpImageType  sample_type;
  InfoDialog    *info_win;
  gboolean       force_update = FALSE;
  gint           i;
 
  info_win = GIMP_DISPLAY_SHELL (gdisp->shell)->info_dialog;

  if (! info_win && info_window_auto != NULL)
    info_win = info_window_auto;

  if (! info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  if (iwd->gdisp != gdisp)
    force_update = TRUE;

  iwd->gdisp = gdisp;

  if (force_update)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (info_window_auto->shell),
                                         GIMP_VIEWABLE (gdisp->gimage));

      info_window_update (gdisp);
    }

  if (! iwd || ! iwd->showing_extended)
    return;

  /* fill in position information */

  if (tx < 0.0 && ty < 0.0)
    {
      iwd->unit_str = NULL;
      for (i = 0; i < 4; i++)
        gtk_label_set_text (GTK_LABEL (iwd->pos_labels[i]), _("N/A"));
      for (i = 0; i < 2; i++)
        gtk_label_set_text (GTK_LABEL (iwd->unit_labels[i]), NULL);
    }
  else
    {
      /*  width and height  */
      unit_factor = _gimp_unit_get_factor (gdisp->gimage->gimp,
					   gdisp->gimage->unit);
      unit_digits = _gimp_unit_get_digits (gdisp->gimage->gimp,
					   gdisp->gimage->unit);

      if (iwd->unit_str != _gimp_unit_get_abbreviation (gdisp->gimage->gimp,
							gdisp->gimage->unit))
        {
          iwd->unit_str = _gimp_unit_get_abbreviation (gdisp->gimage->gimp,
						       gdisp->gimage->unit);

          gtk_label_set_text (GTK_LABEL (iwd->unit_labels[0]), iwd->unit_str);
          gtk_label_set_text (GTK_LABEL (iwd->unit_labels[1]), iwd->unit_str);
        }

      g_snprintf (format_buf, sizeof (format_buf),
		  "%%.%df", unit_digits);

      g_snprintf (buf, sizeof (buf), "%d", (gint) tx);
      gtk_label_set_text (GTK_LABEL (iwd->pos_labels[0]), buf);
      
      g_snprintf (buf, sizeof (buf), format_buf,
		  tx * unit_factor / gdisp->gimage->xresolution);
      gtk_label_set_text (GTK_LABEL (iwd->pos_labels[1]), buf);

      g_snprintf (buf, sizeof (buf), "%d", (gint) ty);
      gtk_label_set_text (GTK_LABEL (iwd->pos_labels[2]), buf);

      g_snprintf (buf, sizeof (buf), format_buf,
		  ty * unit_factor / gdisp->gimage->yresolution);
      gtk_label_set_text (GTK_LABEL (iwd->pos_labels[3]), buf);
      
    }

  /* fill in color information */
  if (! (color = gimp_image_projection_get_color_at (gdisp->gimage, tx, ty))
      || (tx < 0.0 && ty < 0.0))
    {
      for (i = 0; i < 4; i++)
        {
          gtk_label_set_text (GTK_LABEL (iwd->rgb_labels[i]), _("N/A"));
          gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[i]), _("N/A"));
        }
    }
  else
    {
      GimpRGB rgb;
      GimpHSV hsv;

      sample_type = gimp_image_projection_type (gdisp->gimage);

      for (i = RED_PIX;
           i <= (GIMP_IMAGE_TYPE_HAS_ALPHA (sample_type) ? 
                 ALPHA_PIX : BLUE_PIX);
           i++)
        {
          g_snprintf (buf, sizeof (buf), "%d", (gint) color[i]);
          gtk_label_set_text (GTK_LABEL (iwd->rgb_labels[i]), buf);

          if (i == ALPHA_PIX)
            gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[i]), buf);
        }
      
      if (i == ALPHA_PIX)
        {
          gtk_label_set_text (GTK_LABEL (iwd->rgb_labels[i]), _("N/A"));
          gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[i]), _("N/A"));
        }

      gimp_rgb_set_uchar (&rgb,
                          color[RED_PIX],
                          color[GREEN_PIX],
                          color[BLUE_PIX]);
      gimp_rgb_to_hsv (&rgb, &hsv);

      g_snprintf (buf, sizeof (buf), "%d", (gint) (hsv.h * 360.999));
      gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[0]), buf);

      g_snprintf (buf, sizeof (buf), "%d", (gint) (hsv.s * 100.999));
      gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[1]), buf);

      g_snprintf (buf, sizeof (buf), "%d", (gint) (hsv.v * 100.999));
      gtk_label_set_text (GTK_LABEL (iwd->hsv_labels[2]), buf);

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
  InfoWinData      *iwd;
  gint              type;
  gdouble           unit_factor;
  gint              unit_digits;
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

  /*  width and height  */
  unit_factor = _gimp_unit_get_factor (gdisp->gimage->gimp,
				       gdisp->gimage->unit);
  unit_digits = _gimp_unit_get_digits (gdisp->gimage->gimp,
				       gdisp->gimage->unit);

  g_snprintf (iwd->dimensions_str, MAX_BUF,
	      _("%d x %d pixels"),
	      (int) gdisp->gimage->width,
	      (int) gdisp->gimage->height);
  g_snprintf (format_buf, sizeof (format_buf),
	      "%%.%df x %%.%df %s",
	      unit_digits + 1, unit_digits + 1,
	      _gimp_unit_get_plural (gdisp->gimage->gimp,
				     gdisp->gimage->unit));
  g_snprintf (iwd->real_dimensions_str, MAX_BUF, format_buf,
	      gdisp->gimage->width * unit_factor / gdisp->gimage->xresolution,
	      gdisp->gimage->height * unit_factor / gdisp->gimage->yresolution);

  /*  image resolution  */
  g_snprintf (iwd->resolution_str, MAX_BUF, _("%g x %g dpi"),
	      gdisp->gimage->xresolution,
	      gdisp->gimage->yresolution);

  /*  user zoom ratio  */
  g_snprintf (iwd->scale_str, MAX_BUF, "%d:%d",
	      SCALEDEST (shell), SCALESRC (shell));

  type = gimp_image_base_type (gdisp->gimage);

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
                  _("Indexed Color"), gdisp->gimage->num_cols, _("colors"));
      break;
    }

  {
    GdkVisual *visual;

    visual = gdk_rgb_get_visual ();

    /*  visual class  */
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s",
                gettext (visual_classes[visual->type]));
  
    /*  visual depth  */
    g_snprintf (iwd->visual_depth_str, MAX_BUF, "%d", visual->depth);
  }

  info_dialog_update (info_win);
}
