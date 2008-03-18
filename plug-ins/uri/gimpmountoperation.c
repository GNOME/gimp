/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* GTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimpmountoperation.h"

#include "libgimp/stdplugins-intl.h"


/* GObject, GtkObject methods
 */
static void   gimp_mount_operation_set_property (GObject          *object,
                                                 guint             prop_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);
static void   gimp_mount_operation_get_property (GObject          *object,
                                                 guint             prop_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void   gimp_mount_operation_finalize     (GObject          *object);

/* GMountOperation methods
 */
static void   gimp_mount_operation_ask_password (GMountOperation *op,
                                                 const char      *message,
                                                 const char      *default_user,
                                                 const char      *default_domain,
                                                 GAskPasswordFlags flags);

static void   gimp_mount_operation_ask_question (GMountOperation *op,
                                                 const char      *message,
                                                 const char      *choices[]);

G_DEFINE_TYPE (GimpMountOperation, gimp_mount_operation, G_TYPE_MOUNT_OPERATION);

enum {
  PROP_0,
  PROP_PARENT,
  PROP_IS_SHOWING,
  PROP_SCREEN

};

struct GimpMountOperationPrivate {
  GtkWindow *parent_window;
  GtkDialog *dialog;
  GdkScreen *screen;

  /* for the ask-password dialog */
  GtkWidget *entry_container;
  GtkWidget *username_entry;
  GtkWidget *domain_entry;
  GtkWidget *password_entry;
  GtkWidget *anonymous_toggle;

  GAskPasswordFlags ask_flags;
  GPasswordSave     password_save;
  gboolean          anonymous;
};

static void
gimp_mount_operation_finalize (GObject *object)
{
  GimpMountOperation *operation;
  GimpMountOperationPrivate *priv;

  operation = GIMP_MOUNT_OPERATION (object);

  priv = operation->priv;

  if (priv->parent_window)
    g_object_unref (priv->parent_window);

  if (priv->screen)
    g_object_unref (priv->screen);

  G_OBJECT_CLASS (gimp_mount_operation_parent_class)->finalize (object);
}

static void
gimp_mount_operation_class_init (GimpMountOperationClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GMountOperationClass *mount_op_class;

  g_type_class_add_private (klass, sizeof (GimpMountOperationPrivate));

  object_class->finalize     = gimp_mount_operation_finalize;
  object_class->get_property = gimp_mount_operation_get_property;
  object_class->set_property = gimp_mount_operation_set_property;

  mount_op_class = G_MOUNT_OPERATION_CLASS (klass);
  mount_op_class->ask_password = gimp_mount_operation_ask_password;
  mount_op_class->ask_question = gimp_mount_operation_ask_question;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        "Parent",
                                                        "The parent window",
                                                        GTK_TYPE_WINDOW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_IS_SHOWING,
                                   g_param_spec_boolean ("is-showing",
                                                         "Is Showing",
                                                         "Are we showing a dialog",
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_SCREEN,
                                   g_param_spec_object ("screen",
                                                        "Screen",
                                                        "The screen where this window will be displayed.",
                                                        GTK_TYPE_WINDOW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

}


static void
gimp_mount_operation_set_property (GObject         *object,
                                   guint            prop_id,
                                   const GValue    *value,
                                   GParamSpec      *pspec)
{
  GimpMountOperation *operation;
  gpointer tmp;

  operation = GIMP_MOUNT_OPERATION (object);

  switch (prop_id)
    {
   case PROP_PARENT:
     tmp = g_value_get_object (value);
     gimp_mount_operation_set_parent (operation, tmp);
     break;

   case PROP_SCREEN:
      tmp = g_value_get_object (value);
      gimp_mount_operation_set_screen (operation, tmp);
     break;

   case PROP_IS_SHOWING:
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_mount_operation_get_property (GObject         *object,
                                   guint            prop_id,
                                   GValue          *value,
                                   GParamSpec      *pspec)
{
  GimpMountOperationPrivate *priv;
  GimpMountOperation *operation;

  operation = GIMP_MOUNT_OPERATION (object);
  priv = operation->priv;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, priv->parent_window);
      break;

    case PROP_IS_SHOWING:
      g_value_set_boolean (value, priv->dialog != NULL);
      break;

    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_mount_operation_init (GimpMountOperation *operation)
{
  operation->priv = G_TYPE_INSTANCE_GET_PRIVATE (operation,
                                                 GIMP_TYPE_MOUNT_OPERATION,
                                                 GimpMountOperationPrivate);
}

static void
remember_button_toggled (GtkWidget          *widget,
                         GimpMountOperation *operation)
{
  GimpMountOperationPrivate *priv = operation->priv;
  gpointer data;

  data = g_object_get_data (G_OBJECT (widget), "password-save");
  priv->password_save = GPOINTER_TO_INT (data);
}

static void
pw_dialog_got_response (GtkDialog          *dialog,
                        gint                response_id,
                        GimpMountOperation *mount_op)
{
  GimpMountOperationPrivate *priv;
  GMountOperation *op;

  priv = mount_op->priv;
  op = G_MOUNT_OPERATION (mount_op);

  if (response_id == GTK_RESPONSE_OK)
    {
      const char *text;

      if (priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
        g_mount_operation_set_anonymous (op, priv->anonymous);

      if (priv->username_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (priv->username_entry));
          g_mount_operation_set_username (op, text);
        }

      if (priv->domain_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (priv->domain_entry));
          g_mount_operation_set_domain (op, text);
        }

      if (priv->password_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (priv->password_entry));
          g_mount_operation_set_password (op, text);
        }

      if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
        g_mount_operation_set_password_save (op, priv->password_save);

      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (op), "is-showing");
  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (op);
}

static gboolean
entry_has_input (GtkWidget *entry_widget)
{
  const char *text;

  if (entry_widget == NULL)
    return TRUE;

  text = gtk_entry_get_text (GTK_ENTRY (entry_widget));

  return text != NULL && text[0] != '\0';
}

static gboolean
pw_dialog_input_is_valid (GimpMountOperation *operation)
{
  GimpMountOperationPrivate *priv = operation->priv;
  gboolean is_valid = TRUE;

  is_valid = entry_has_input (priv->username_entry) &&
             entry_has_input (priv->domain_entry) &&
             entry_has_input (priv->password_entry);

  return is_valid;
}

static void
pw_dialog_verify_input (GtkEditable        *editable,
                        GimpMountOperation *operation)
{
  GimpMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  is_valid = pw_dialog_input_is_valid (operation);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->dialog),
                                     GTK_RESPONSE_OK,
                                     is_valid);
}

static void
pw_dialog_anonymous_toggled (GtkWidget          *widget,
                             GimpMountOperation *operation)
{
  GimpMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  priv->anonymous = widget == priv->anonymous_toggle;

  if (priv->anonymous)
    is_valid = TRUE;
  else
    is_valid = pw_dialog_input_is_valid (operation);

  gtk_widget_set_sensitive (priv->entry_container, priv->anonymous == FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->dialog),
                                     GTK_RESPONSE_OK,
                                     is_valid);
}


static void
pw_dialog_cycle_focus (GtkWidget          *widget,
                       GimpMountOperation *operation)
{
  GimpMountOperationPrivate *priv;
  GtkWidget *next_widget = NULL;

  priv = operation->priv;

  if (widget == priv->username_entry)
    {
      if (priv->domain_entry != NULL)
        next_widget = priv->domain_entry;
      else if (priv->password_entry != NULL)
        next_widget = priv->password_entry;
    }
  else if (widget == priv->domain_entry && priv->password_entry)
    next_widget = priv->password_entry;

  if (next_widget)
    gtk_widget_grab_focus (next_widget);
  else if (pw_dialog_input_is_valid (operation))
    gtk_window_activate_default (GTK_WINDOW (priv->dialog));
}

static GtkWidget *
table_add_entry (GtkWidget  *table,
                 int         row,
                 const char *label_text,
                 const char *value,
                 gpointer    user_data)
{
  GtkWidget *entry;
  GtkWidget *label;

  label = gtk_label_new_with_mnemonic (label_text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  entry = gtk_entry_new ();

  if (value)
    gtk_entry_set_text (GTK_ENTRY (entry), value);

  gtk_table_attach (GTK_TABLE (table), label,
                    0, 1, row, row + 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), entry,
                             1, 2, row, row + 1);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (pw_dialog_verify_input), user_data);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (pw_dialog_cycle_focus), user_data);

  return entry;
}

static void
gimp_mount_operation_ask_password (GMountOperation   *mount_op,
                                   const char        *message,
                                   const char        *default_user,
                                   const char        *default_domain,
                                   GAskPasswordFlags  flags)
{
  GimpMountOperation *operation;
  GimpMountOperationPrivate *priv;
  GtkWidget *widget;
  GtkDialog *dialog;
  GtkWindow *window;
  GtkWidget *entry_container;
  GtkWidget *hbox, *main_vbox, *vbox, *icon;
  GtkWidget *table;
  GtkWidget *message_label;
  gboolean   can_anonymous;
  guint      rows;

  operation = GIMP_MOUNT_OPERATION (mount_op);
  priv = operation->priv;

  priv->ask_flags = flags;

  widget = gtk_dialog_new ();
  dialog = GTK_DIALOG (widget);
  window = GTK_WINDOW (widget);

  priv->dialog = dialog;

  /* Set the dialog up with HIG properties */
  gtk_dialog_set_has_separator (dialog, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (dialog->action_area), 6);

  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_icon_name (window, GTK_STOCK_DIALOG_AUTHENTICATION);

  gtk_dialog_add_buttons (dialog,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          _("Co_nnect"), GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

  /* Build contents */
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, TRUE, TRUE, 0);

  icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION,
                                   GTK_ICON_SIZE_DIALOG);

  gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

  main_vbox = gtk_vbox_new (FALSE, 18);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

  message_label = gtk_label_new (message);
  gtk_misc_set_alignment (GTK_MISC (message_label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (message_label), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (message_label),
                      FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

  can_anonymous = flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED;

  if (can_anonymous)
    {
      GtkWidget *anon_box;
      GtkWidget *choice;
      GSList    *group;

      anon_box = gtk_vbox_new (FALSE, 6);
      gtk_box_pack_start (GTK_BOX (vbox), anon_box,
                          FALSE, FALSE, 0);

      choice = gtk_radio_button_new_with_mnemonic (NULL, _("Connect _anonymously"));
      gtk_box_pack_start (GTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
      priv->anonymous_toggle = choice;

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (choice));
      choice = gtk_radio_button_new_with_mnemonic (group, _("Connect as u_ser:"));
      gtk_box_pack_start (GTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
    }

  rows = 0;

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    rows++;

  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    rows++;

  if (flags &G_ASK_PASSWORD_NEED_DOMAIN)
    rows++;

  /* The table that holds the entries */
  entry_container = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);

  gtk_alignment_set_padding (GTK_ALIGNMENT (entry_container),
                             0, 0, can_anonymous ? 12 : 0, 0);

  gtk_box_pack_start (GTK_BOX (vbox), entry_container,
                      FALSE, FALSE, 0);
  priv->entry_container = entry_container;

  table = gtk_table_new (rows, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (entry_container), table);

  rows = 0;

  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    priv->username_entry = table_add_entry (table, rows++, _("_Username:"),
                                            default_user, operation);

  if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
    priv->domain_entry = table_add_entry (table, rows++, _("_Domain:"),
                                          default_domain, operation);

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      priv->password_entry = table_add_entry (table, rows++, _("_Password:"),
                                              NULL, operation);
      gtk_entry_set_visibility (GTK_ENTRY (priv->password_entry), FALSE);
    }

   if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
    {
      GtkWidget  *choice;
      GtkWidget  *remember_box;
      GSList     *group;

      remember_box = gtk_vbox_new (FALSE, 6);
      gtk_box_pack_start (GTK_BOX (vbox), remember_box,
                          FALSE, FALSE, 0);

      choice = gtk_radio_button_new_with_mnemonic (NULL, _("_Forget password immediately"));
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_NEVER));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      gtk_box_pack_start (GTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (choice));
      choice = gtk_radio_button_new_with_mnemonic (group, _("_Remember password until you logout"));
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_FOR_SESSION));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      gtk_box_pack_start (GTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (choice));
      choice = gtk_radio_button_new_with_mnemonic (group, _("_Remember forever"));
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_PERMANENTLY));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      gtk_box_pack_start (GTK_BOX (remember_box), choice, FALSE, FALSE, 0);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (pw_dialog_got_response), operation);

  if (can_anonymous)
    gtk_widget_set_sensitive (priv->entry_container, FALSE);
  else if (! pw_dialog_input_is_valid (operation))
    gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, FALSE);

  g_object_notify (G_OBJECT (operation), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    gtk_window_set_screen (GTK_WINDOW (dialog), priv->screen);

  gtk_widget_show_all (GTK_WIDGET (dialog));

  g_object_ref (operation);
}

static void
question_dialog_button_clicked (GtkDialog       *dialog,
                                gint             button_number,
                                GMountOperation *op)
{
  GimpMountOperationPrivate *priv;
  GimpMountOperation *operation;

  operation = GIMP_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);
 
  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (op);
}

static void
gimp_mount_operation_ask_question (GMountOperation *op,
                                   const char      *message,
                                   const char      *choices[])
{
  GimpMountOperationPrivate *priv;
  GtkWidget  *dialog;
  const char *secondary = NULL;
  char       *primary;
  int        count, len = 0;

  g_return_if_fail (GIMP_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (choices != NULL);

  priv = GIMP_MOUNT_OPERATION (op)->priv;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  dialog = gtk_message_dialog_new (priv->parent_window, 0,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE, "%s",
                                   primary != NULL ? primary : message);
  g_free (primary);

  if (secondary)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

  /* First count the items in the list then
   * add the buttons in reverse order */

  while (choices[len] != NULL)
    len++;

  for (count = len - 1; count >= 0; count--)
    gtk_dialog_add_button (GTK_DIALOG (dialog), choices[count], count);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (question_dialog_button_clicked), op);

  priv->dialog = GTK_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    gtk_window_set_screen (GTK_WINDOW (dialog), priv->screen);

  gtk_widget_show (dialog);
  g_object_ref (op);
}

GMountOperation *
gimp_mount_operation_new (GtkWindow *parent)
{
  GMountOperation *mount_operation;

  mount_operation = g_object_new (GIMP_TYPE_MOUNT_OPERATION,
                                  "parent", parent, NULL);

  return mount_operation;
}

gboolean
gimp_mount_operation_is_showing (GimpMountOperation *op)
{
  g_return_val_if_fail (GIMP_IS_MOUNT_OPERATION (op), FALSE);

  return op->priv->dialog != NULL;
}

void
gimp_mount_operation_set_parent (GimpMountOperation *op,
                                 GtkWindow          *parent)
{
  GimpMountOperationPrivate *priv;

  g_return_if_fail (GIMP_IS_MOUNT_OPERATION (op));
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));

  priv = op->priv;

  if (priv->parent_window == parent)
    return;

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            gtk_widget_destroyed,
                                            &priv->parent_window);
      priv->parent_window = NULL;
    }

  if (parent)
    {
      priv->parent_window = g_object_ref (parent);

      g_signal_connect (parent, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &priv->parent_window);

      if (priv->dialog)
        gtk_window_set_transient_for (GTK_WINDOW (priv->dialog), parent);
    }

  g_object_notify (G_OBJECT (op), "parent");
}

GtkWindow *
gimp_mount_operation_get_parent (GimpMountOperation *op)
{
  g_return_val_if_fail (GIMP_IS_MOUNT_OPERATION (op), NULL);

  return op->priv->parent_window;
}

void
gimp_mount_operation_set_screen (GimpMountOperation *op,
                                 GdkScreen          *screen)
{
  GimpMountOperationPrivate *priv;

  g_return_if_fail (GIMP_IS_MOUNT_OPERATION (op));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  priv = op->priv;

  if (priv->screen == screen)
    return;

  if (priv->screen)
    g_object_unref (priv->screen);

  priv->screen = g_object_ref (screen);

  if (priv->dialog)
    gtk_window_set_screen (GTK_WINDOW (priv->dialog), screen);

  g_object_notify (G_OBJECT (op), "screen");
}

GdkScreen *
gimp_mount_operation_get_screen (GimpMountOperation *op)
{
  GimpMountOperationPrivate *priv;

  g_return_val_if_fail (GIMP_IS_MOUNT_OPERATION (op), NULL);

  priv = op->priv;

  if (priv->dialog)
    return gtk_window_get_screen (GTK_WINDOW (priv->dialog));
  else if (priv->parent_window)
    return gtk_window_get_screen (GTK_WINDOW (priv->parent_window));
  else if (priv->screen)
    return priv->screen;
  else
    return gdk_screen_get_default ();
}
