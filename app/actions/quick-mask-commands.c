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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-qmask.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "qmask-commands.h"

#include "gimp-intl.h"


typedef struct _EditQmaskOptions EditQmaskOptions;

struct _EditQmaskOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *color_panel;

  GimpImage *gimage;
};


/*  local function prototypes  */

static void   qmask_channel_query       (GimpDisplayShell *shell);
static void   qmask_query_response      (GtkWidget        *widget,
                                         gint              response_id,
                                         EditQmaskOptions *options);
static void   qmask_query_scale_update  (GtkAdjustment    *adjustment,
                                         gpointer          data);
static void   qmask_query_color_changed (GimpColorButton  *button,
                                         gpointer          data);


/*  public functionss */

void
qmask_toggle_cmd_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  if (GTK_CHECK_MENU_ITEM (widget)->active !=
      gimp_image_get_qmask_state (shell->gdisp->gimage))
    {
      gimp_image_set_qmask_state (shell->gdisp->gimage,
                                  GTK_CHECK_MENU_ITEM (widget)->active);

      gimp_image_flush (shell->gdisp->gimage);
    }
}

void
qmask_invert_cmd_callback (GtkWidget *widget,
                           gpointer   data,
                           guint      action)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  if (GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (action != shell->gdisp->gimage->qmask_inverted)
        {
          gimp_image_qmask_invert (shell->gdisp->gimage);

          if (gimp_image_get_qmask_state (shell->gdisp->gimage))
            gimp_image_flush (shell->gdisp->gimage);
        }
    }
}

void
qmask_configure_cmd_callback (GtkWidget *widget,
                              gpointer   data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  qmask_channel_query (shell);
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
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (options->color_panel),
                                gimp_get_user_context (options->gimage->gimp));

  /*  The dialog  */
  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (shell->gdisp->gimage),
                              _("Qmask Attributes"), "gimp-qmask-edit",
                              GIMP_STOCK_QMASK_ON,
                              _("Edit QuickMask Attributes"),
                              GTK_WIDGET (shell),
                              gimp_standard_help_func,
                              GIMP_HELP_QMASK_EDIT,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (qmask_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free, options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
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

  g_signal_connect (opacity_scale_data, "value_changed",
		    G_CALLBACK (qmask_query_scale_update),
		    options->color_panel);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (options->color_panel, "color_changed",
		    G_CALLBACK (qmask_query_color_changed),
		    opacity_scale_data);

  gtk_widget_show (options->query_box);
}

static void
qmask_query_response (GtkWidget        *widget,
                      gint              response_id,
                      EditQmaskOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpChannel *channel = gimp_image_get_qmask (options->gimage);
      GimpRGB      color;

      if (options->gimage && channel)
        {
          gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
                                       &color);

          if (gimp_rgba_distance (&color, &channel->color) > 0.0001)
            {
              gimp_channel_set_color (channel, &color, TRUE);

              gimp_image_flush (options->gimage);
            }
        }

      /* update the qmask color no matter what */
      options->gimage->qmask_color = color;
    }

  gtk_widget_destroy (options->query_box);
}

static void
qmask_query_scale_update (GtkAdjustment *adjustment,
			  gpointer       data)
{
  GimpRGB color;

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
