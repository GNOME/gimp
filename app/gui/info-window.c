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
#include "gimpui.h"
#include "gximage.h"
#include "info_dialog.h"
#include "info_window.h"
#include "interface.h"
#include "scroll.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpunit.h"
#include "pixmaps/dropper.xpm"

#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;

struct _InfoWinData
{
  gchar      dimensions_str[MAX_BUF];
  gchar      scale_str[MAX_BUF];
  gchar      color_type_str[MAX_BUF];
  gchar      visual_class_str[MAX_BUF];
  gchar      visual_depth_str[MAX_BUF];
  gchar      shades_str[MAX_BUF];
  gchar      resolution_str[MAX_BUF];
  gchar      unit_str[MAX_BUF];

  void      *gdisp_ptr; /* I'a not happy 'bout this one */

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


static void
get_shades (GDisplay *gdisp,
	    char     *buf)
{
  g_snprintf (buf, MAX_BUF, "Using GdkRgb - we'll get back to you");
#if 0
  GtkPreviewInfo *info;

  info = gtk_preview_get_info ();

  switch (gimage_base_type (gdisp->gimage))
    {
    case GRAY:
      g_snprintf (buf, MAX_BUF, "%d", info->ngray_shades);
      break;
    case RGB:
      switch (gdisp->depth)
	{
	case 8 :
	  g_snprintf (buf, MAX_BUF, "%d / %d / %d",
		      info->nred_shades,
		      info->ngreen_shades,
		      info->nblue_shades);
	  break;
	case 15 : case 16 :
	  g_snprintf (buf, MAX_BUF, "%d / %d / %d",
		      (1 << (8 - info->visual->red_prec)),
		      (1 << (8 - info->visual->green_prec)),
		      (1 << (8 - info->visual->blue_prec)));
	  break;
	case 24 :
	  g_snprintf (buf, MAX_BUF, "256 / 256 / 256");
	  break;
	}
      break;

    case INDEXED:
      g_snprintf (buf, MAX_BUF, "%d", gdisp->gimage->num_cols);
      break;
    }
#endif
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
info_window_image_preview_book (InfoDialog *info_win)
{
  GtkWidget *hbox1;
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *table2;
  GtkWidget *labelBvalue;
  GtkWidget *labelGvalue;
  GtkWidget *labelRvalue;
  GtkWidget *labelAvalue;
  GtkWidget *labelB;
  GtkWidget *labelG;
  GtkWidget *labelR;
  GtkWidget *labelA;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle  *style;

  InfoWinData *iwd;

  iwd = (InfoWinData *) info_win->user_data;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_widget_show (alignment);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox1), alignment, TRUE, TRUE, 0);

  table2 = gtk_table_new (5, 2, TRUE);
  gtk_container_border_width (GTK_CONTAINER (table2), 2);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame), table2);
  gtk_container_add (GTK_CONTAINER (alignment), frame); 

  labelAvalue = gtk_label_new (_("N/A"));
  gtk_widget_show (labelAvalue);
  gtk_table_attach (GTK_TABLE (table2), labelAvalue, 1, 2, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelBvalue = gtk_label_new (_("N/A"));
  gtk_widget_show (labelBvalue);
  gtk_table_attach (GTK_TABLE (table2), labelBvalue, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelGvalue = gtk_label_new (_("N/A"));
  gtk_widget_show (labelGvalue);
  gtk_table_attach (GTK_TABLE (table2), labelGvalue, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelRvalue = gtk_label_new (_("N/A"));
  gtk_widget_show (labelRvalue);
  gtk_table_attach (GTK_TABLE (table2), labelRvalue, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelA = gtk_label_new (_("A:"));
  gtk_widget_show (labelA);
  gtk_table_attach (GTK_TABLE (table2), labelA, 0, 1, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelB = gtk_label_new (_("B:"));
  gtk_widget_show (labelB);
  gtk_table_attach (GTK_TABLE (table2), labelB, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelG = gtk_label_new (_("G:"));
  gtk_widget_show (labelG);
  gtk_table_attach (GTK_TABLE (table2), labelG, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  labelR = gtk_label_new (_("R:"));
  gtk_widget_show (labelR);
  gtk_table_attach (GTK_TABLE (table2), labelR, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_realize(info_win->shell);
  style = gtk_widget_get_style (info_win->shell);

  pixmap = gdk_pixmap_create_from_xpm_d (info_win->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 dropper_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);

  gtk_table_attach (GTK_TABLE (table2), pixmapwid, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (pixmapwid);

  gtk_notebook_append_page(GTK_NOTEBOOK(info_win->info_notebook),
			   hbox1,
			   gtk_label_new (_("Extended")));

  /* Set back to first page */
  gtk_notebook_set_page(GTK_NOTEBOOK(info_win->info_notebook),0);

  gtk_object_set_user_data(GTK_OBJECT (info_win->info_notebook),
			   (gpointer)info_win);

  gtk_signal_connect (GTK_OBJECT (info_win->info_notebook), "switch_page",
		      GTK_SIGNAL_FUNC (info_window_page_switch), NULL);

  iwd->labelBvalue = labelBvalue;
  iwd->labelGvalue = labelGvalue;
  iwd->labelRvalue = labelRvalue;
  iwd->labelAvalue = labelAvalue;
}

/*  displays information:
 *    image name
 *    image width, height
 *    zoom ratio
 *    image color type
 *    Display info:
 *      visual class
 *      visual depth
 *      shades of color/gray
 */

InfoDialog *
info_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay *gdisp;
  InfoWinData *iwd;
  char * title, * title_buf;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;

  title = g_basename (gimage_filename (gdisp->gimage));
  type = gimage_base_type (gdisp->gimage);

  /*  create the info dialog  */
  title_buf = g_strdup_printf (_("%s: Window Info"), title);
  info_win = info_dialog_notebook_new (title_buf,
				       gimp_standard_help_func,
				       "dialogs/info_window.html");
  g_free (title_buf);

  /*  create the action area  */
  gimp_dialog_create_action_area (GTK_DIALOG (info_win->shell),

				  _("Close"), info_window_close_callback,
				  info_win, NULL, TRUE, FALSE,

				  NULL);

  iwd = g_new (InfoWinData, 1);
  info_win->user_data = iwd;
  iwd->dimensions_str[0] = '\0';
  iwd->resolution_str[0] = '\0';
  iwd->unit_str[0] = '\0';
  iwd->scale_str[0] = '\0';
  iwd->color_type_str[0] = '\0';
  iwd->visual_class_str[0] = '\0';
  iwd->visual_depth_str[0] = '\0';
  iwd->shades_str[0] = '\0';
  iwd->showingPreview = FALSE;

  /*  add the information fields  */
  info_dialog_add_label (info_win, _("Dimensions (w x h):"),
			 iwd->dimensions_str);
  info_dialog_add_label (info_win, _("Resolution:"),
			iwd->resolution_str);
  info_dialog_add_label (info_win, _("Unit:"),
			 iwd->unit_str);
  info_dialog_add_label (info_win, _("Scale Ratio:"),
			 iwd->scale_str);
  info_dialog_add_label (info_win, _("Display Type:"),
			 iwd->color_type_str);
  info_dialog_add_label (info_win, _("Visual Class:"),
			 iwd->visual_class_str);
  info_dialog_add_label (info_win, _("Visual Depth:"),
			 iwd->visual_depth_str);
  if (type == RGB)
    info_dialog_add_label (info_win, _("Shades of Color:"),
			   iwd->shades_str);
  else if (type == INDEXED)
    info_dialog_add_label (info_win, _("Shades:"),
			   iwd->shades_str);
  else if (type == GRAY)
    info_dialog_add_label (info_win, _("Shades of Gray:"),
			   iwd->shades_str);

  /*  update the fields  */
  info_window_update (info_win, gdisp_ptr);

  /* Add extra tabs */
  info_window_image_preview_book (info_win);

  return info_win;
}

void  
info_window_update_RGB  (InfoDialog  *info_win,
			 void        *gdisp_ptr,
			 gdouble      tx,
			 gdouble      ty)
{
  GDisplay    *gdisp;
  InfoWinData *iwd;
  char buff[5];
  guchar *color;
  gint has_alpha;
  gint sample_type;

  if(!info_win)
    return;

  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  if(!iwd || iwd->showingPreview == FALSE)
    return;

  /* gimage_active_drawable (gdisp->gimage) */
  if (!(color = gimp_image_get_color_at(gdisp->gimage, tx, ty))
      || (tx <  0.0 && ty < 0.0))
    {
      g_snprintf(buff,5,"%4s","N/A");
      gtk_label_set_text(GTK_LABEL(iwd->labelBvalue),buff);
      gtk_label_set_text(GTK_LABEL(iwd->labelGvalue),buff);
      gtk_label_set_text(GTK_LABEL(iwd->labelRvalue),buff);
      gtk_label_set_text(GTK_LABEL(iwd->labelAvalue),buff);

      return;
    }
  
  g_snprintf(buff,5,"%4d",(gint)color[BLUE_PIX]);
  gtk_label_set_text(GTK_LABEL(iwd->labelBvalue),buff);
  g_snprintf(buff,5,"%4d",(gint)color[GREEN_PIX]);
  gtk_label_set_text(GTK_LABEL(iwd->labelGvalue),buff);
  g_snprintf(buff,5,"%4d",(gint)color[RED_PIX]);
  gtk_label_set_text(GTK_LABEL(iwd->labelRvalue),buff);

  sample_type = gimp_image_composite_type (gdisp->gimage);
  has_alpha = TYPE_HAS_ALPHA (sample_type);

  if(has_alpha)
    g_snprintf(buff,5,"%4d",(gint)color[ALPHA_PIX]);
  else
    g_snprintf(buff,5,"%4s","N/A");

  gtk_label_set_text(GTK_LABEL(iwd->labelAvalue),buff);

  g_free(color);
}

void
info_window_free (InfoDialog *info_win)
{
  g_free (info_win->user_data);
  info_dialog_free (info_win);
}

void
info_window_update (InfoDialog *info_win,
		    void       *gdisp_ptr)
{
  GDisplay    *gdisp;
  InfoWinData *iwd;
  int          type;
  gdouble      unit_factor;
  gint         unit_digits;
  gchar        format_buf[32];

  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  /*  width and height  */
  unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
  unit_digits = gimp_unit_get_digits (gdisp->gimage->unit);
  g_snprintf (format_buf, sizeof (format_buf),
	      _("%%d x %%d pixels (%%.%df x %%.%df %s)"),
	      unit_digits + 1, unit_digits + 1,
	      gimp_unit_get_symbol (gdisp->gimage->unit));
  g_snprintf (iwd->dimensions_str, MAX_BUF, format_buf,
	      (int) gdisp->gimage->width,
	      (int) gdisp->gimage->height,
	      gdisp->gimage->width * unit_factor / gdisp->gimage->xresolution,
	      gdisp->gimage->height * unit_factor / gdisp->gimage->yresolution);

  /*  image resolution  */
  g_snprintf (iwd->resolution_str, MAX_BUF, "%g x %g dpi",
	      gdisp->gimage->xresolution,
	      gdisp->gimage->yresolution);

  /*  image unit  */
  g_snprintf (iwd->unit_str, MAX_BUF, "%s",
	      gimp_unit_get_plural (gdisp->gimage->unit));

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
    g_snprintf (iwd->color_type_str, MAX_BUF, "%s", _("Indexed Color"));

  /*  visual class  */
  if (type == RGB ||
      type == INDEXED)
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s", visual_classes[g_visual->type]);
  else if (type == GRAY)
    g_snprintf (iwd->visual_class_str, MAX_BUF, "%s", visual_classes[g_visual->type]);

  /*  visual depth  */
  g_snprintf (iwd->visual_depth_str, MAX_BUF, "%d", gdisp->depth);

  /*  pure color shades  */
  get_shades (gdisp, iwd->shades_str);

  info_dialog_update (info_win);
}
