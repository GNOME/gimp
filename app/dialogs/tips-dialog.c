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


static GtkWidget * tips_button_new     (const gchar *label,
                                        gboolean     back);
static void        tips_set_labels     (GimpTip     *tip);
static void        tips_dialog_destroy (GtkWidget   *widget,
                                        gpointer     data);
static void        tips_show_previous  (GtkWidget   *widget,
                                        gpointer     data);
static void        tips_show_next      (GtkWidget   *widget,
                                        gpointer     data);


static GtkWidget *tips_dialog   = NULL;
static GtkWidget *welcome_label = NULL;
static GtkWidget *thetip_label  = NULL;
static GList     *tips          = NULL;
static GList     *current_tip   = NULL;
static gint       tips_count    = 0;


GtkWidget *
tips_dialog_create (Gimp *gimp)
{
  GimpGuiConfig *config;
  GtkWidget     *vbox;
  GtkWidget     *vbox2;
  GtkWidget     *hbox;
  GtkWidget     *bbox;
  GtkWidget     *button;
  GtkWidget     *image;

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

  tips_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (tips_dialog),
                            GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_role (GTK_WINDOW (tips_dialog), "gimp-tip-of-the-day");
  gtk_window_set_title (GTK_WINDOW (tips_dialog), _("GIMP Tip of the Day"));
  gtk_window_set_position (GTK_WINDOW (tips_dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (tips_dialog), TRUE);

  g_signal_connect (tips_dialog, "delete-event",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  g_signal_connect (tips_dialog, "destroy",
                    G_CALLBACK (tips_dialog_destroy),
                    config);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (tips_dialog), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
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

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_prop_check_button_new (G_OBJECT (config), "show-tips",
                                       _("Show tip next time GIMP starts"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_window_set_default (GTK_WINDOW (tips_dialog), button);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            tips_dialog);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (bbox), 6);
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = tips_button_new (_("_Previous Tip"), TRUE);
  gtk_widget_set_sensitive (button, (tips_count > 1));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (tips_show_previous),
                    NULL);

  button = tips_button_new (_("_Next Tip"), FALSE);
  gtk_widget_set_sensitive (button, (tips_count > 1));
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);

  gtk_widget_grab_focus (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (tips_show_next),
                    NULL);

  gimp_help_connect (tips_dialog, gimp_standard_help_func,
                     GIMP_HELP_TIPS_DIALOG, NULL);

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


static GtkWidget *
tips_button_new (const gchar *text,
                 gboolean     back)
{
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *hbox;

  hbox = gtk_hbox_new (FALSE, 2);

  label = gtk_label_new_with_mnemonic (text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  if (back)
    gtk_box_pack_end (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  else
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_widget_show (label);

  image = gtk_image_new_from_stock (back ?
                                    GTK_STOCK_GO_BACK : GTK_STOCK_GO_FORWARD,
                                    GTK_ICON_SIZE_BUTTON);

  if (back)
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  else
    gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  gtk_widget_show (image);

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show (hbox);

  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);

  return button;
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

static void
tips_show_previous (GtkWidget *widget,
                    gpointer  data)
{
  current_tip = current_tip->prev ? current_tip->prev : g_list_last (tips);

  tips_set_labels (current_tip->data);
}

static void
tips_show_next (GtkWidget *widget,
                gpointer   data)
{
  current_tip = current_tip->next ? current_tip->next : tips;

  tips_set_labels (current_tip->data);
}
