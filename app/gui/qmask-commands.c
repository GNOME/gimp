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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-qmask.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "qmask-commands.h"

#include "libgimp/gimpintl.h"


typedef struct _EditQmaskOptions EditQmaskOptions;

struct _EditQmaskOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *color_panel;

  GimpImage *gimage;
};


/*  local function prototypes  */

static void   qmask_channel_query         (GimpDisplayShell *shell);
static void   qmask_query_ok_callback     (GtkWidget        *widget, 
                                           gpointer          client_data);
static void   qmask_query_cancel_callback (GtkWidget        *widget, 
                                           gpointer          client_data);
static void   qmask_query_scale_update    (GtkAdjustment    *adjustment,
                                           gpointer          data);
static void   qmask_query_color_changed   (GimpColorButton  *button,
                                           gpointer          data);


/*  public functionss */

void
qmask_toggle_cmd_callback (GtkWidget *widget,
                           gpointer   data,
                           guint      action)
{
  GimpDisplayShell *shell;

  shell = (GimpDisplayShell *) gimp_widget_get_callback_context (widget);

  if (! shell)
    return;

  gimp_image_set_qmask_state (shell->gdisp->gimage,
                              GTK_CHECK_MENU_ITEM (widget)->active);

  gdisplays_flush ();
}

void
qmask_invert_cmd_callback (GtkWidget *widget,
                           gpointer   data,
                           guint      action)
{
  GimpDisplayShell *shell;

  shell = (GimpDisplayShell *) gimp_widget_get_callback_context (widget);

  if (! shell)
    return;

  if (GTK_CHECK_MENU_ITEM (widget)->active)
    {
      gimp_image_qmask_invert (shell->gdisp->gimage);

      if (shell->gdisp->gimage->qmask_state)
        gdisplays_flush ();
    }
}

void
qmask_configure_cmd_callback (GtkWidget *widget,
                              gpointer   data,
                              guint      action)
{
  GimpDisplayShell *shell;

  shell = (GimpDisplayShell *) gimp_widget_get_callback_context (widget);

  if (! shell)
    return;

  qmask_channel_query (shell);
}

void
qmask_menu_update (GtkItemFactory *factory,
                   gpointer        data)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (data);

#define SET_ACTIVE(menu,active) \
        gimp_item_factory_set_active (factory, "/" menu, (active))
#define SET_COLOR(menu,color) \
        gimp_item_factory_set_color (factory, "/" menu, (color), FALSE)

  SET_ACTIVE ("QMask Active", shell->gdisp->gimage->qmask_state);

  if (shell->gdisp->gimage->qmask_inverted)
    SET_ACTIVE ("Mask Selected Areas", TRUE);
  else
    SET_ACTIVE ("Mask Unselected Areas", TRUE);

  SET_COLOR ("Configure Color and Opacity...",
             &shell->gdisp->gimage->qmask_color);

#undef SET_SENSITIVE
#undef SET_COLOR
}


/*  private functions  */

static void
qmask_channel_query (GimpDisplayShell *shell)
{
  EditQmaskOptions *options;
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *opacity_scale;
  GtkObject        *opacity_scale_data;

  /*  the new options structure  */
  options = g_new0 (EditQmaskOptions, 1);

  options->gimage      = shell->gdisp->gimage;
  options->color_panel = gimp_color_panel_new (_("Edit Qmask Color"),
					       &options->gimage->qmask_color,
					       GIMP_COLOR_AREA_LARGE_CHECKS, 
					       48, 64);

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Edit Qmask Attributes"), "edit_qmask_attributes",
		     gimp_standard_help_func,
		     "dialogs/edit_qmask_attributes.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, qmask_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, qmask_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,

		     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
                     hbox);
  gtk_widget_show (hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  The table  */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The opacity scale  */
  opacity_scale_data =
    gtk_adjustment_new (options->gimage->qmask_color.a * 100.0, 
			0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_widget_set_size_request (opacity_scale, 100, -1);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Mask Opacity:"), 1.0, 1.0,
			     opacity_scale, 1, FALSE);

  g_signal_connect (G_OBJECT (opacity_scale_data), "value_changed",
		    G_CALLBACK (qmask_query_scale_update),
		    options->color_panel);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (G_OBJECT (options->color_panel), "color_changed",
		    G_CALLBACK (qmask_query_color_changed),
		    opacity_scale_data);		      

  gtk_widget_show (options->query_box);
}

static void 
qmask_query_ok_callback (GtkWidget *widget, 
                         gpointer   data) 
{
  EditQmaskOptions *options;
  GimpChannel      *channel;
  GimpRGB           color;

  options = (EditQmaskOptions *) data;

  channel = gimp_image_get_channel_by_name (options->gimage, "Qmask");

  if (options->gimage && channel)
    {
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
				   &color);

      if (gimp_rgba_distance (&color, &channel->color) > 0.0001)
	{
	  gimp_channel_set_color (channel, &color);

	  gdisplays_flush ();
	}
    }

  /* update the qmask color no matter what */
  options->gimage->qmask_color = color;

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
qmask_query_cancel_callback (GtkWidget *widget,
				  gpointer   data)
{
  EditQmaskOptions *options;

  options = (EditQmaskOptions *) data;

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void 
qmask_query_scale_update (GtkAdjustment *adjustment, 
			  gpointer       data)
{
  GimpRGB  color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (data), &color);
  gimp_rgb_set_alpha (&color, adjustment->value / 100.0);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (data), &color);
}

static void
qmask_query_color_changed (GimpColorButton *button,
                           gpointer         data)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (data);
  GimpRGB        color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adj, color.a * 100.0);
}
