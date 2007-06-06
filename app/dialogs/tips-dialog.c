/* GIMP - The GNU Image Manipulation Program
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
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimphelp-ids.h"

#include "tips-dialog.h"
#include "tips-parser.h"

#include "gimp-intl.h"


enum
{
  RESPONSE_PREVIOUS = 1,
  RESPONSE_NEXT     = 2
};

static void  tips_set_labels      (GimpTip   *tip);
static void  tips_dialog_response (GtkWidget *dialog,
                                   gint       response);
static void  tips_dialog_destroy  (GtkWidget *widget,
                                   gpointer   data);


static GtkWidget *tips_dialog   = NULL;
static GtkWidget *welcome_label = NULL;
static GtkWidget *thetip_label  = NULL;
static GList     *tips          = NULL;
static GList     *current_tip   = NULL;


GtkWidget *
tips_dialog_create (Gimp *gimp)
{
  GimpGuiConfig *config;
  GtkWidget     *vbox;
  GtkWidget     *vbox2;
  GtkWidget     *hbox;
  GtkWidget     *button;
  GtkWidget     *image;
  gint           tips_count;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (!tips)
    {
      GError *error = NULL;
      gchar  *filename;

      filename = g_build_filename (gimp_data_directory (), "tips",
                                   "gimp-tips.xml", NULL);

      tips = gimp_tips_from_file (filename, &error);

      if (! tips)
        {
          GimpTip *tip;

          if (error->code == G_FILE_ERROR_NOENT)
            {
              tip = gimp_tip_new ("<b>%s</b>",
                                  _("Your GIMP tips file appears to be missing!"));
              gimp_tip_set (tip,
                            _("There should be a file called '%s'. "
                              "Please check your installation."), filename);
            }
          else
            {
              tip = gimp_tip_new ("<b>%s</b>",
                                  _("The GIMP tips file could not be parsed!"));
              gimp_tip_set (tip, "%s", error->message);
            }

          tips = g_list_prepend (tips, tip);
          g_error_free (error);
        }
      else if (error)
        {
          g_printerr ("Error while parsing '%s': %s\n",
                      filename, error->message);
          g_error_free (error);
        }

      g_free (filename);
    }

  tips_count = g_list_length (tips);

  config = GIMP_GUI_CONFIG (gimp->config);

  if (config->last_tip >= tips_count || config->last_tip < 0)
    config->last_tip = 0;

  current_tip = g_list_nth (tips, config->last_tip);

  if (tips_dialog)
    return tips_dialog;

  tips_dialog = gimp_dialog_new (_("GIMP Tip of the Day"),
                                 "gimp-tip-of-the-day",
                                 NULL, 0, NULL, NULL,
                                 NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (tips_dialog),
                                  _("_Previous Tip"), RESPONSE_PREVIOUS);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
                                                  GTK_ICON_SIZE_BUTTON));

  button = gtk_dialog_add_button (GTK_DIALOG (tips_dialog),
                                  _("_Next Tip"), RESPONSE_NEXT);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
                                                  GTK_ICON_SIZE_BUTTON));

  button = gtk_dialog_add_button (GTK_DIALOG (tips_dialog),
                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (tips_dialog),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_PREVIOUS,
                                           RESPONSE_NEXT,
                                           -1);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (tips_dialog),
                                     RESPONSE_NEXT, tips_count > 1);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (tips_dialog),
                                     RESPONSE_PREVIOUS, tips_count > 1);

  g_signal_connect (tips_dialog, "response",
                    G_CALLBACK (tips_dialog_response),
                    NULL);
  g_signal_connect (tips_dialog, "destroy",
                    G_CALLBACK (tips_dialog_destroy),
                    config);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tips_dialog)->vbox),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  welcome_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (welcome_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (welcome_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (welcome_label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox2), welcome_label, FALSE, FALSE, 0);

  thetip_label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (thetip_label), TRUE);
  gtk_label_set_justify (GTK_LABEL (thetip_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (thetip_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (thetip_label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox2), thetip_label, TRUE, TRUE, 0);
  gtk_widget_show (thetip_label);

  gtk_container_set_focus_chain (GTK_CONTAINER (vbox2), NULL);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  image = gtk_image_new_from_stock (GIMP_STOCK_INFO, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (vbox2), image, TRUE, FALSE, 0);
  gtk_widget_show (image);

  button = gimp_prop_check_button_new (G_OBJECT (config), "show-tips",
                                       _("Show tip next time GIMP starts"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  tips_set_labels (current_tip->data);

  return tips_dialog;
}

static void
tips_dialog_destroy (GtkWidget *widget,
                     gpointer   data)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (data);

  /* the last-shown-tip is saved in sessionrc */
  config->last_tip = g_list_position (tips, current_tip);

  tips_dialog = NULL;
  current_tip = NULL;

  gimp_tips_free (tips);
  tips = NULL;
}

static void
tips_dialog_response (GtkWidget *dialog,
                      gint       response)
{
  switch (response)
    {
    case RESPONSE_PREVIOUS:
      current_tip = current_tip->prev ? current_tip->prev : g_list_last (tips);
      tips_set_labels (current_tip->data);
      break;

    case RESPONSE_NEXT:
      current_tip = current_tip->next ? current_tip->next : tips;
      tips_set_labels (current_tip->data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
tips_set_labels (GimpTip *tip)
{
  g_return_if_fail (tip != NULL);

  if (tip->welcome)
    gtk_widget_show (welcome_label);
  else
    gtk_widget_hide (welcome_label);

  gtk_label_set_markup (GTK_LABEL (welcome_label), tip->welcome);
  gtk_label_set_markup (GTK_LABEL (thetip_label), tip->thetip);
}
