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

#include "appenv.h"
#include "colormaps.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpset.h"
#include "gimpui.h"
#include "gximage.h"
#include "info_dialog.h"
#include "info_window.h"
#include "scroll.h"
#include "tools.h"

#include "libgimp/gimpunit.h"

#include "libgimp/gimpintl.h"

#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;

struct _InfoWinData
{
  gchar      dimensions_str[MAX_BUF];
  gchar      real_dimensions_str[MAX_BUF];
  gchar      scale_str[MAX_BUF];
  gchar      color_type_str[MAX_BUF];
  gchar      visual_class_str[MAX_BUF];
  gchar      visual_depth_str[MAX_BUF];
  gchar      resolution_str[MAX_BUF];
  gchar      unit_str[MAX_BUF];

  GDisplay  *gdisp;

  GtkWidget *labelBvalue;
  GtkWidget *labelGvalue;
  GtkWidget *labelRvalue;
  GtkWidget *labelAvalue;

  gboolean   showingPreview;
};

/*  The different classes of visuals  */
static gchar *visual_classes[] =
{
  N_("Static Gray"),
  N_("Grayscale"),
  N_("Static Color"),
  N_("Pseudo Color"),
  N_("True Color"),
  N_("Direct Color"),
};

static gchar * info_window_title (GDisplay *gdisp);


static void
info_window_image_rename_callback (GimpImage *gimage,
				   gpointer   data)
{
  InfoDialog *id;
  gchar *title;
  GDisplay * gdisp;
  InfoWinData *iwd;

  id = (InfoDialog *) data;

  iwd = (InfoWinData *) id->user_data;

  gdisp = (GDisplay *) iwd->gdisp;

  title = info_window_title(gdisp);
  gtk_window_set_title (GTK_WINDOW (id->shell), title);
  g_free (title);
}

static void
info_window_close_callback (GtkWidget *widget,
			    gpointer   data)
{
  info_dialog_popdown ((InfoDialog *) data);
}

static void
info_window_page_switch (GtkWidget       *widget, 
			 GtkNotebookPage *page, 
			 gint             page_num)
{
  InfoDialog *info_win;
  InfoWinData *iwd;
  
  info_win = (InfoDialog *) gtk_object_get_user_data (GTK_OBJECT (widget));
  iwd = (InfoWinData *) info_win->user_data;

  /* Only deal with the second page */
  if (page_num != 1)
    {
      iwd->showingPreview = FALSE;
      return;
    }

  iwd->showingPreview = TRUE;
}

static void
info_window_create_extended (InfoDialog *info_win)
{
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *pixmap;

  InfoWinData *iwd;

  iwd = (InfoWinData *) info_win->user_data;

  hbox = gtk_hbox_new (FALSE, 0);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  gtk_widget_show (alignment);

  frame = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_container_add (GTK_CONTAINER (alignment), frame); 
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  pixmap = gtk_pixmap_new (tool_get_pixmap (COLOR_PICKER),
			   tool_get_mask (COLOR_PICKER));
  gtk_table_attach (GTK_TABLE (table), pixmap, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
  gtk_widget_show (pixmap);

  label = gtk_label_new (_("R:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("G:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("B:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("A:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  iwd->labelRvalue = label = gtk_label_new (_("N/A"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  iwd->labelGvalue = label = gtk_label_new (_("N/A"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  iwd->labelBvalue = label = gtk_label_new (_("N/A"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  iwd->labelAvalue = label = gtk_label_new (_("N/A"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);


  gtk_notebook_append_page (GTK_NOTEBOOK(info_win->info_notebook),
			    hbox, gtk_label_new (_("Extended")));
  gtk_widget_show (hbox);

  /* Set back to first page */
  gtk_notebook_set_page (GTK_NOTEBOOK(info_win->info_notebook), 0);

  gtk_object_set_user_data (GTK_OBJECT (info_win->info_notebook),
			    (gpointer)info_win);

  gtk_signal_connect (GTK_OBJECT (info_win->info_notebook), "switch_page",
		      GTK_SIGNAL_FUNC (info_window_page_switch), NULL);
}

/*  displays information:
 *    image name
 *    image width, height
 *    zoom ratio
 *    image color type
 *    Display info:
 *      visual class
 *      visual depth
 */

InfoDialog *
info_window_create (GDisplay *gdisp)
{
  InfoDialog *info_win;
  InfoWinData *iwd;
  gchar *title, *title_buf;
  gint type;

  title = g_basename (gimage_filename (gdisp->gimage));
  type = gimage_base_type (gdisp->gimage);

  /*  create the info dialog  */
  title_buf = info_window_title (gdisp);
  info_win = info_dialog_notebook_new (title_buf,
				       gimp_standard_help_func,
				       "dialogs/info_window.html");
  g_free (title_buf);

  /*  create the action area  */
  gimp_dialog_create_action_area (GTK_DIALOG (info_win->shell),

				  _("Close"), info_window_close_callback,
				  info_win, NULL, NULL, TRUE, FALSE,

				  NULL);

  iwd = g_new (InfoWinData, 1);
  info_win->user_data = iwd;

  iwd->dimensions_str[0]       = '\0';
  iwd->real_dimensions_str[0]  = '\0';
  iwd->resolution_str[0]       = '\0';
  iwd->scale_str[0]            = '\0';
  iwd->color_type_str[0]       = '\0';
  iwd->visual_class_str[0]     = '\0';
  iwd->visual_depth_str[0]     = '\0';
  iwd->gdisp                   = gdisp;
  iwd->showingPreview          = FALSE;

  /*  add the information fields  */
  info_dialog_add_label (info_win, _("Dimensions (w x h):"),
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
  /*  update the fields  */
  /*gdisp->window_info_dialog = info_win;*/
  info_window_update (gdisp);

  /*  Add extra tabs  */
  info_window_create_extended (info_win);

  /*  keep track of image name changes  */
  gtk_signal_connect (GTK_OBJECT (gdisp->gimage), "rename",
		      GTK_SIGNAL_FUNC (info_window_image_rename_callback),
		      info_win);

  return info_win;
}

static InfoDialog *info_window_auto = NULL;

static gchar *
info_window_title (GDisplay *gdisp)
{
  gchar *title;
  gchar *title_buf;
  
  title = g_basename (gimage_filename (gdisp->gimage));
  
  /*  create the info dialog  */
  title_buf = g_strdup_printf (_("Info: %s-%d.%d"), 
			       title,
			       pdb_image_to_id (gdisp->gimage),
			       gdisp->instance);

  return title_buf;
}

static void
info_window_change_display (GimpContext *context, /* NOT USED */
			    GDisplay    *newdisp,
			    gpointer     data /* Not used */)
{
  GDisplay * gdisp = newdisp;
  GDisplay * old_gdisp;
  GimpImage * gimage;
  InfoWinData *iwd;

  iwd = (InfoWinData *) info_window_auto->user_data;

  old_gdisp = (GDisplay *) iwd->gdisp;

  if (!info_window_auto || gdisp == old_gdisp || !gdisp)
    {
      return;
    }

  gimage = gdisp->gimage;

  if (gimage && gimp_set_have (image_context, gimage))
    {
      iwd->gdisp = gdisp;
      info_window_update(gdisp);
    }
}

void
info_window_follow_auto (void)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active (); 
  
  if (!gdisp) 
    return;

  if(!info_window_auto)
    {
      info_window_auto = info_window_create ((void *) gdisp);
      gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "display_changed",
			  GTK_SIGNAL_FUNC (info_window_change_display), NULL);
      info_window_update(gdisp); /* Update to include the info */
    }

  info_dialog_popup (info_window_auto);
  /*
  iwd = (NavWinData *)nav_window_auto->user_data;
  gtk_widget_set_sensitive(nav_window_auto->vbox,TRUE);
  iwd->frozen = FALSE;
  */
}

void  
info_window_update_RGB (GDisplay *gdisp,
			gdouble   tx,
			gdouble   ty)
{
  InfoWinData *iwd;
  gchar buff[4];
  guchar *color;
  GimpImageType sample_type;
  InfoDialog *info_win = gdisp->window_info_dialog;
  gboolean force_update = FALSE;

  if (!info_win && info_window_auto != NULL)
    {
      info_win = info_window_auto;
    }

  if (!info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  if(iwd->gdisp != gdisp)
    force_update = TRUE;

  iwd->gdisp = gdisp;

  if(force_update == TRUE)
    {
      gchar       *title_buf;
      info_window_update(gdisp);
      title_buf = info_window_title(gdisp);
      
      gtk_window_set_title (GTK_WINDOW (info_window_auto->shell), title_buf);
      
      g_free (title_buf);
    }


  if (!iwd || iwd->showingPreview == FALSE)
    return;

  /* gimage_active_drawable (gdisp->gimage) */
  if (!(color = gimp_image_get_color_at (gdisp->gimage, tx, ty))
      || (tx <  0.0 && ty < 0.0))
    {
      gtk_label_set_text (GTK_LABEL (iwd->labelRvalue), _("N/A"));
      gtk_label_set_text (GTK_LABEL (iwd->labelGvalue), _("N/A"));
      gtk_label_set_text (GTK_LABEL (iwd->labelBvalue), _("N/A"));
      gtk_label_set_text (GTK_LABEL (iwd->labelAvalue), _("N/A"));

      return;
    }

  g_snprintf (buff, sizeof (buff), "%d", (gint) color[RED_PIX]);
  gtk_label_set_text (GTK_LABEL (iwd->labelRvalue), buff);

  g_snprintf (buff, sizeof (buff), "%d", (gint) color[GREEN_PIX]);
  gtk_label_set_text (GTK_LABEL (iwd->labelGvalue), buff);

  g_snprintf (buff, sizeof (buff), "%d", (gint) color[BLUE_PIX]);
  gtk_label_set_text (GTK_LABEL (iwd->labelBvalue), buff);

  sample_type = gimp_image_composite_type (gdisp->gimage);

  if (TYPE_HAS_ALPHA (sample_type))
    {
      g_snprintf (buff, sizeof (buff), "%d", (gint) color[ALPHA_PIX]);
      gtk_label_set_text (GTK_LABEL (iwd->labelAvalue), buff);
    }
  else
    gtk_label_set_text (GTK_LABEL (iwd->labelAvalue), _("N/A"));

  g_free(color);
}

void
info_window_free (InfoDialog *info_win)
{
  InfoWinData *iwd;
  extern gint gimage_image_count (void);

  if (!info_win && info_window_auto)
    {
      gtk_widget_set_sensitive (info_window_auto->vbox, FALSE);
      return;
    }

  if (!info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  gtk_signal_disconnect_by_data (GTK_OBJECT (iwd->gdisp->gimage), info_win);

  g_free (iwd);
  info_dialog_free (info_win);
}

void
info_window_update (GDisplay *gdisp)
{
  InfoWinData *iwd;
  gint         type;
  gdouble      unit_factor;
  gint         unit_digits;
  gchar        format_buf[32];
  InfoDialog *info_win = gdisp->window_info_dialog;

  if (!info_win && info_window_auto != NULL)
    info_win = info_window_auto;

  if (!info_win)
    return;

  iwd = (InfoWinData *) info_win->user_data;

  /* Make it sensitive... */
  if (info_window_auto)
    gtk_widget_set_sensitive (info_window_auto->vbox, TRUE);

  /* If doing info_window_auto then return if this display
   * is not the one we are showing.
   */

  if (info_window_auto && iwd->gdisp != gdisp)
    return;

  /*  width and height  */
  unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
  unit_digits = gimp_unit_get_digits (gdisp->gimage->unit);
  g_snprintf (iwd->dimensions_str, MAX_BUF,
	      _("%d x %d pixels"),
	      (int) gdisp->gimage->width,
	      (int) gdisp->gimage->height);
  g_snprintf (format_buf, sizeof (format_buf),
	      "%%.%df x %%.%df %s",
	      unit_digits + 1, unit_digits + 1,
	      gimp_unit_get_plural (gdisp->gimage->unit));
  g_snprintf (iwd->real_dimensions_str, MAX_BUF, format_buf,
	      gdisp->gimage->width * unit_factor / gdisp->gimage->xresolution,
	      gdisp->gimage->height * unit_factor / gdisp->gimage->yresolution);

  /*  image resolution  */
  g_snprintf (iwd->resolution_str, MAX_BUF, _("%g x %g dpi"),
	      gdisp->gimage->xresolution,
	      gdisp->gimage->yresolution);

  /*  user zoom ratio  */
  g_snprintf (iwd->scale_str, MAX_BUF, "%d:%d",
	   SCALEDEST (gdisp), SCALESRC (gdisp));

  type = gimage_base_type (gdisp->gimage);

  /*  color type  */
  if (type == RGB)
    g_snprintf (iwd->color_type_str, MAX_BUF, "%s", _("RGB Color"));
  else if (type == GRAY)
    g_snprintf (iwd->color_type_str, MAX_BUF, "%s", _("Grayscale"));
  else if (type == INDEXED)
    g_snprintf (iwd->color_type_str, MAX_BUF, "%s (%d %s)", 
		_("Indexed Color"), gdisp->gimage->num_cols, _("colors"));

  /*  visual class  */
  if (type == RGB ||
      type == INDEXED)
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s", gettext (visual_classes[g_visual->type]));
  else if (type == GRAY)
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s", gettext (visual_classes[g_visual->type]));

  /*  visual depth  */
  g_snprintf (iwd->visual_depth_str, MAX_BUF, "%d", gdisp->depth);

  info_dialog_update (info_win);
}
