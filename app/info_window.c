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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "appenv.h"
#include "actionarea.h"
#include "colormaps.h"
#include "info_dialog.h"
#include "info_window.h"
#include "gdisplay.h"
#include "general.h"
#include "gximage.h"
#include "interface.h"

#include "libgimp/gimpintl.h"

#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;
struct _InfoWinData
{
  char dimensions_str[MAX_BUF];
  char scale_str[MAX_BUF];
  char color_type_str[MAX_BUF];
  char visual_class_str[MAX_BUF];
  char visual_depth_str[MAX_BUF];
  char shades_str[MAX_BUF];
  char resolution_str[MAX_BUF];
};

/*  The different classes of visuals  */
static char *visual_classes[] =
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
  sprintf (buf, "Using GdkRgb - we'll get back to you");
#if 0
  GtkPreviewInfo *info;

  info = gtk_preview_get_info ();

  switch (gimage_base_type (gdisp->gimage))
    {
    case GRAY:
      sprintf (buf, "%d", info->ngray_shades);
      break;
    case RGB:
      switch (gdisp->depth)
	{
	case 8 :
	  sprintf (buf, "%d / %d / %d",
		   info->nred_shades,
		   info->ngreen_shades,
		   info->nblue_shades);
	  break;
	case 15 : case 16 :
	  sprintf (buf, "%d / %d / %d",
		   (1 << (8 - info->visual->red_prec)),
		   (1 << (8 - info->visual->green_prec)),
		   (1 << (8 - info->visual->blue_prec)));
	  break;
	case 24 :
	  sprintf (buf, "256 / 256 / 256");
	  break;
	}
      break;

    case INDEXED:
      sprintf (buf, "%d", gdisp->gimage->num_cols);
      break;
    }
#endif
}

static void
info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
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

static ActionAreaItem action_items[] =
{
  { N_("Close"), info_window_close_callback, NULL, NULL },
};

InfoDialog *
info_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay *gdisp;
  InfoWinData *iwd;
  char * title, * title_buf;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;

  title = prune_filename (gimage_filename (gdisp->gimage));
  type = gimage_base_type (gdisp->gimage);

  /*  create the info dialog  */
  title_buf = g_strdup_printf (_("%s: Window Info"), title);
  info_win = info_dialog_new (title_buf);
  g_free (title_buf);

  iwd = (InfoWinData *) g_malloc (sizeof (InfoWinData));
  info_win->user_data = iwd;
  iwd->dimensions_str[0] = '\0';
  iwd->resolution_str[0] = '\0';
  iwd->scale_str[0] = '\0';
  iwd->color_type_str[0] = '\0';
  iwd->visual_class_str[0] = '\0';
  iwd->visual_depth_str[0] = '\0';
  iwd->shades_str[0] = '\0';

  /*  add the information fields  */
  info_dialog_add_field (info_win, _("Dimensions (w x h): "), iwd->dimensions_str, NULL, NULL);
  info_dialog_add_field (info_win, _("Resolution: "), iwd->resolution_str, NULL, NULL);
  info_dialog_add_field (info_win, _("Scale Ratio: "), iwd->scale_str, NULL, NULL);
  info_dialog_add_field (info_win, _("Display Type: "), iwd->color_type_str, NULL, NULL);
  info_dialog_add_field (info_win, _("Visual Class: "), iwd->visual_class_str, NULL, NULL);
  info_dialog_add_field (info_win, _("Visual Depth: "), iwd->visual_depth_str, NULL, NULL);
  if (type == RGB)
    info_dialog_add_field (info_win, _("Shades of Color: "), iwd->shades_str, NULL, NULL);
  else if (type == INDEXED)
    info_dialog_add_field (info_win, _("Shades: "), iwd->shades_str, NULL, NULL);
  else if (type == GRAY)
    info_dialog_add_field (info_win, _("Shades of Gray: "), iwd->shades_str, NULL, NULL);

  /*  update the fields  */
  info_window_update (info_win, gdisp_ptr);

  /* Create the action area  */
  action_items[0].user_data = info_win;
  build_action_area (GTK_DIALOG (info_win->shell), action_items, 1, 0);

  return info_win;
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
  GDisplay *gdisp;
  InfoWinData *iwd;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  /*  width and height  */
  g_snprintf (iwd->dimensions_str, MAX_BUF, "%d x %d",
	   (int) gdisp->gimage->width, (int) gdisp->gimage->height);

  /*  image resolution  */
  g_snprintf (iwd->resolution_str, MAX_BUF, "%g x %g dpi",
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
