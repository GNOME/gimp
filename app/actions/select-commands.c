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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimpselection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gui/dialogs.h"

#include "actions.h"
#include "select-commands.h"

#include "gimp-intl.h"


#define return_if_no_display(gdisp,data) \
  gdisp = action_data_get_display (data); \
  if (! gdisp) \
    return

#define return_if_no_image(gimage,data) \
  gimage = action_data_get_image (data); \
  if (! gimage) \
    return


/*  local functions  */

static void   gimp_image_mask_feather_callback (GtkWidget   *widget,
                                                gdouble      size,
                                                GimpUnit     unit,
                                                gpointer     data);
static void   gimp_image_mask_border_callback  (GtkWidget   *widget,
                                                gdouble      size,
                                                GimpUnit     unit,
                                                gpointer     data);
static void   gimp_image_mask_grow_callback    (GtkWidget   *widget,
                                                gdouble      size,
                                                GimpUnit     unit,
                                                gpointer     data);
static void   gimp_image_mask_shrink_callback  (GtkWidget   *widget,
                                                gdouble      size,
                                                GimpUnit     unit,
                                                gpointer     data);


/*  local variables  */

static gdouble   selection_feather_radius    = 5.0;
static gint      selection_border_radius     = 5;
static gint      selection_grow_pixels       = 1;
static gint      selection_shrink_pixels     = 1;
static gboolean  selection_shrink_edge_lock  = FALSE;


void
select_invert_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_channel_invert (gimp_image_get_mask (gimage), TRUE);
  gimp_image_flush (gimage);
}

void
select_all_cmd_callback (GtkAction *action,
			 gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_channel_all (gimp_image_get_mask (gimage), TRUE);
  gimp_image_flush (gimage);
}

void
select_none_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_channel_clear (gimp_image_get_mask (gimage), NULL, TRUE);
  gimp_image_flush (gimage);
}

void
select_from_vectors_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage *gimage;
  GimpVectors *vectors;
  return_if_no_image (gimage, data);

  vectors = gimp_image_get_active_vectors (gimage);
  if (!vectors)
    return;

  gimp_channel_select_vectors (gimp_image_get_mask (gimage),
                               _("Path to Selection"),
                               vectors,
                               GIMP_CHANNEL_OP_REPLACE,
                               TRUE, FALSE, 0, 0);
  gimp_image_flush (gimage);
}

void
select_float_cmd_callback (GtkAction *action,
			   gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_selection_float (gimp_image_get_mask (gimage),
                        gimp_image_active_drawable (gimage),
                        gimp_get_user_context (gimage->gimp),
                        TRUE, 0, 0);
  gimp_image_flush (gimage);
}

void
select_feather_cmd_callback (GtkAction *action,
			     gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_size_box (_("Feather Selection"),
                              gdisp->shell,
			      gimp_standard_help_func,
			      GIMP_HELP_SELECTION_FEATHER,
			      _("Feather selection by"),
			      selection_feather_radius, 0, 32767, 3,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
			      G_OBJECT (gdisp->gimage), "disconnect",
			      gimp_image_mask_feather_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_sharpen_cmd_callback (GtkAction *action,
			     gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_channel_sharpen (gimp_image_get_mask (gimage), TRUE);
  gimp_image_flush (gimage);
}

void
select_shrink_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *shrink_dialog;
  GtkWidget   *edge_lock;
  return_if_no_display (gdisp, data);

  shrink_dialog =
    gimp_query_size_box (_("Shrink Selection"),
                         gdisp->shell,
			 gimp_standard_help_func,
			 GIMP_HELP_SELECTION_SHRINK,
			 _("Shrink selection by"),
			 selection_shrink_pixels, 1, 32767, 0,
			 gdisp->gimage->unit,
			 MIN (gdisp->gimage->xresolution,
			      gdisp->gimage->yresolution),
			 GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
			 G_OBJECT (gdisp->gimage), "disconnect",
			 gimp_image_mask_shrink_callback, gdisp->gimage);

  edge_lock = gtk_check_button_new_with_label (_("Shrink from image border"));

  gtk_box_pack_start (GTK_BOX (GIMP_QUERY_BOX_VBOX (shrink_dialog)), edge_lock,
                      FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (shrink_dialog), "edge_lock_toggle", edge_lock);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (edge_lock),
				! selection_shrink_edge_lock);
  gtk_widget_show (edge_lock);

  gtk_widget_show (shrink_dialog);
}

void
select_grow_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_size_box (_("Grow Selection"),
                              gdisp->shell,
			      gimp_standard_help_func,
			      GIMP_HELP_SELECTION_GROW,
			      _("Grow selection by"),
			      selection_grow_pixels, 1, 32767, 0,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
			      G_OBJECT (gdisp->gimage), "disconnect",
			      gimp_image_mask_grow_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_border_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_size_box (_("Border Selection"),
                              gdisp->shell,
			      gimp_standard_help_func,
			      GIMP_HELP_SELECTION_BORDER,
			      _("Border selection by"),
			      selection_border_radius, 1, 32767, 0,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      GIMP_DISPLAY_SHELL (gdisp->shell)->dot_for_dot,
			      G_OBJECT (gdisp->gimage), "disconnect",
			      gimp_image_mask_border_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_save_cmd_callback (GtkAction *action,
			  gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_selection_save (gimp_image_get_mask (gdisp->gimage));
  gimp_image_flush (gdisp->gimage);

  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (gdisp->shell),
                                    "gimp-channel-list", -1);
}


/*  private functions  */

static void
gimp_image_mask_feather_callback (GtkWidget *widget,
                                  gdouble    size,
                                  GimpUnit   unit,
                                  gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_feather_radius = size;

  radius_x = radius_y = selection_feather_radius;

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimp_channel_feather (gimp_image_get_mask (gimage), radius_x, radius_y, TRUE);
  gimp_image_flush (gimage);
}

static void
gimp_image_mask_border_callback (GtkWidget *widget,
                                 gdouble    size,
                                 GimpUnit   unit,
                                 gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_border_radius = ROUND (size);

  radius_x = radius_y = selection_border_radius;

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimp_channel_border (gimp_image_get_mask (gimage), radius_x, radius_y, TRUE);
  gimp_image_flush (gimage);
}

static void
gimp_image_mask_grow_callback (GtkWidget *widget,
                               gdouble    size,
                               GimpUnit   unit,
                               gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_grow_pixels = ROUND (size);

  radius_x = radius_y = selection_grow_pixels;

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimp_channel_grow (gimp_image_get_mask (gimage), radius_x, radius_y, TRUE);
  gimp_image_flush (gimage);
}

static void
gimp_image_mask_shrink_callback (GtkWidget *widget,
                                 gdouble    size,
                                 GimpUnit   unit,
                                 gpointer   data)
{
  GimpImage *gimage;
  gint       radius_x;
  gint       radius_y;

  gimage = GIMP_IMAGE (data);

  selection_shrink_pixels = ROUND (size);

  radius_x = radius_y = selection_shrink_pixels;

  selection_shrink_edge_lock =
    ! GTK_TOGGLE_BUTTON (g_object_get_data (G_OBJECT (widget),
					    "edge_lock_toggle"))->active;

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimp_channel_shrink (gimp_image_get_mask (gimage), radius_x, radius_y,
                       selection_shrink_edge_lock, TRUE);
  gimp_image_flush (gimage);
}
