/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * URI plug-in, GIO backend
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gio/gio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "uri-backend.h"

#include "libgimp/stdplugins-intl.h"


typedef enum
{
  DOWNLOAD,
  UPLOAD
} Mode;


static GType      mount_operation_get_type (void) G_GNUC_CONST;

static gchar    * get_protocols (void);
static gboolean   copy_uri      (const gchar  *src_uri,
                                 const gchar  *dest_uri,
                                 Mode          mode,
                                 GimpRunMode   run_mode,
                                 GError      **error);

static gchar *supported_protocols = NULL;


gboolean
uri_backend_init (const gchar  *plugin_name,
                  gboolean      run,
                  GimpRunMode   run_mode,
                  GError      **error)
{
  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (plugin_name, FALSE);
    }

  return TRUE;
}

void
uri_backend_shutdown (void)
{
}

const gchar *
uri_backend_get_load_help (void)
{
  return "Loads a file using the GIO Virtual File System";
}

const gchar *
uri_backend_get_save_help (void)
{
  return "Saves a file using the GIO Virtual File System";
}

const gchar *
uri_backend_get_load_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
}

const gchar *
uri_backend_get_save_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
}

gboolean
uri_backend_load_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar *dest_uri = g_filename_to_uri (tmpname, NULL, error);

  if (dest_uri)
    {
      gboolean success = copy_uri (uri, dest_uri, DOWNLOAD, run_mode, error);

      g_free (dest_uri);

      return success;
    }

  return FALSE;
}

gboolean
uri_backend_save_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar *src_uri = g_filename_to_uri (tmpname, NULL, error);

  if (src_uri)
    {
      gboolean success = copy_uri (src_uri, uri, UPLOAD, run_mode, error);

      g_free (src_uri);

      return success;
    }

  return FALSE;
}


/*  private functions  */

static gchar *
get_protocols (void)
{
  const gchar * const *schemes;
  GString             *string = g_string_new (NULL);
  gint                 i;

  schemes = g_vfs_get_supported_uri_schemes (g_vfs_get_default ());

  for (i = 0; schemes[i]; i++)
    {
      if (string->len > 0)
        g_string_append_c (string, ',');

      g_string_append (string, schemes[i]);
      g_string_append_c (string, ':');
    }

  return g_string_free (string, FALSE);
}

static void
uri_progress_callback (goffset  current_num_bytes,
                       goffset  total_num_bytes,
                       gpointer user_data)
{
  Mode mode = GPOINTER_TO_INT (user_data);

  if (total_num_bytes > 0)
    {
      const gchar *format;
      gchar       *done  = gimp_memsize_to_string (current_num_bytes);
      gchar       *total = gimp_memsize_to_string (total_num_bytes);

      switch (mode)
        {
        case DOWNLOAD:
          format = _("Downloading image (%s of %s)");
          break;
        case UPLOAD:
          format = _("Uploading image (%s of %s)");
          break;
        default:
          g_assert_not_reached ();
        }

      gimp_progress_set_text_printf (format, done, total);
      gimp_progress_update (current_num_bytes / total_num_bytes);

      g_free (total);
      g_free (done);
    }
  else
    {
      const gchar *format;
      gchar       *done = gimp_memsize_to_string (current_num_bytes);

      switch (mode)
        {
        case DOWNLOAD:
          format = _("Downloaded %s of image data");
          break;
        case UPLOAD:
          format = _("Uploaded %s of image data");
          break;
        default:
          g_assert_not_reached ();
        }

      gimp_progress_set_text_printf (format, done);
      gimp_progress_pulse ();

      g_free (done);
    }
}

static void
mount_volume_ready (GFile         *file,
                    GAsyncResult  *res,
                    GError       **error)
{
  g_file_mount_enclosing_volume_finish (file, res, error);

  gtk_main_quit ();
}

static gboolean
mount_enclosing_volume (GFile   *file,
                        GError **error)
{
  GMountOperation *operation;

  operation = g_object_new (mount_operation_get_type (), NULL);

  g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE,
                                 operation,
                                 NULL,
                                 (GAsyncReadyCallback) mount_volume_ready,
                                 error);
  gtk_main ();

  g_object_unref (operation);

  return (*error == NULL);
}

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          Mode          mode,
          GimpRunMode   run_mode,
          GError      **error)
{
  GVfs     *vfs = g_vfs_get_default ();
  GFile    *src_file;
  GFile    *dest_file;
  gboolean  success;

  vfs = g_vfs_get_default ();

  if (! g_vfs_is_active (vfs))
    {
      g_set_error (error, 0, 0, "Initialization of GVfs failed");
      return FALSE;
    }

  src_file  = g_vfs_get_file_for_uri (vfs, src_uri);
  dest_file = g_vfs_get_file_for_uri (vfs, dest_uri);

  gimp_progress_init (_("Connecting to server"));

  success = g_file_copy (src_file, dest_file, 0, NULL,
                         uri_progress_callback, GINT_TO_POINTER (mode),
                         error);

  if (! success &&
      run_mode == GIMP_RUN_INTERACTIVE &&
      (*error)->domain == G_IO_ERROR   &&
      (*error)->code   == G_IO_ERROR_NOT_MOUNTED)
    {
      g_clear_error (error);

      if (mount_enclosing_volume (mode == DOWNLOAD ? src_file : dest_file,
                                  error))
        {
          success = g_file_copy (src_file, dest_file, 0, NULL,
                                 uri_progress_callback, GINT_TO_POINTER (mode),
                                 error);

        }
    }

  g_object_unref (src_file);
  g_object_unref (dest_file);

  return success;
}


/*************************************************************
 * The code below is partly copied from eel-mount-operation.c.
 * It should become obsolete with the next GTK+ release.
 *************************************************************/

#define MOUNT_OPERATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), mount_operation_get_type (), MountOperation))


typedef struct _MountOperation  MountOperation;
typedef GMountOperationClass    MountOperationClass;

struct _MountOperation
{
  GMountOperation  parent_instance;

  GtkWidget       *username_entry;
  GtkWidget       *anon_toggle;
  GtkWidget       *domain_entry;
  GtkWidget       *password_entry;
};

static void mount_operation_ask_password (GMountOperation   *operation,
                                          const char        *message,
                                          const char        *default_user,
                                          const char        *default_domain,
                                          GAskPasswordFlags  flags);
static void mount_operation_ask_question (GMountOperation   *operation,
                                          const char        *message,
                                          const char        *choices[]);


G_DEFINE_TYPE (MountOperation, mount_operation, G_TYPE_MOUNT_OPERATION)


static void
mount_operation_class_init (MountOperationClass *klass)
{
  GMountOperationClass *operation_class = G_MOUNT_OPERATION_CLASS (klass);

  operation_class->ask_password = mount_operation_ask_password;
  operation_class->ask_question = mount_operation_ask_question;
}

static void
mount_operation_init (MountOperation *operation)
{
}

static void
mount_operation_password_response (GtkWidget      *dialog,
                                   gint            response_id,
                                   MountOperation *operation)
{
  const gchar *text;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      if (operation->username_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (operation->username_entry));
          g_mount_operation_set_username (G_MOUNT_OPERATION (operation), text);
        }

      if (operation->domain_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (operation->domain_entry));
          g_mount_operation_set_domain (G_MOUNT_OPERATION (operation), text);
        }

      if (operation->password_entry)
        {
          text = gtk_entry_get_text (GTK_ENTRY (operation->password_entry));
          g_mount_operation_set_password (G_MOUNT_OPERATION (operation), text);
        }

      g_mount_operation_set_password_save (G_MOUNT_OPERATION (operation),
                                           G_PASSWORD_SAVE_NEVER);

      g_mount_operation_reply (G_MOUNT_OPERATION (operation),
                               G_MOUNT_OPERATION_HANDLED);
      break;

    case GTK_RESPONSE_CANCEL:
      g_mount_operation_reply (G_MOUNT_OPERATION (operation),
                               G_MOUNT_OPERATION_ABORTED);
      break;
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (operation);
}

static void
mount_operation_anon_toggled (GtkToggleButton *button,
                              MountOperation  *operation)
{
  if (operation->username_entry)
    gtk_widget_set_sensitive (operation->username_entry,
                              ! gtk_toggle_button_get_active (button));
}

static void
mount_operation_ask_password (GMountOperation   *operation,
                              const char        *message,
                              const char        *default_user,
                              const char        *default_domain,
                              GAskPasswordFlags  flags)
{
  MountOperation *mount = MOUNT_OPERATION (operation);
  GtkWidget      *dialog;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *image;
  GtkWidget      *table;
  GtkWidget      *focus = NULL;
  gint            row   = 0;

  dialog = gtk_dialog_new_with_buttons (_("Enter Password"),
                                        NULL, 0,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        _("_Log In"),     GTK_RESPONSE_OK,
                                        NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (mount_operation_password_response),
                    operation);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  if (message)
    {
      GtkWidget *label = gtk_label_new (message);

      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);

      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  table = gtk_table_new (0, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    {
      mount->username_entry = gtk_entry_new ();

      if (default_user)
        gtk_entry_set_text (GTK_ENTRY (mount->username_entry), default_user);
      else
        focus = mount->username_entry;

      gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                 _("_User:"), 0.0, 0.5,
                                 mount->username_entry, 1, FALSE);
    }
  else
    {
      mount->username_entry = NULL;
    }

  if (flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
    {
      mount->anon_toggle =
        gtk_check_button_new_with_mnemonic (_("Log in _anonymously"));

      g_signal_connect (mount->anon_toggle, "toggled",
                        G_CALLBACK (mount_operation_anon_toggled),
                        mount);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mount->anon_toggle),
                                    TRUE);

      gtk_table_attach (GTK_TABLE (table), mount->anon_toggle,
                        1, 2, row, row + 1,
                        GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (mount->anon_toggle);
      row++;
    }
  else
    {
      mount->anon_toggle = NULL;
    }

  if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
    {
      mount->domain_entry = gtk_entry_new ();

      if (default_domain)
        gtk_entry_set_text (GTK_ENTRY (mount->domain_entry), default_domain);
      else if (! focus)
        focus = mount->domain_entry;

      gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                 _("_Domain:"), 0.0, 0.5,
                                 mount->domain_entry, 1, FALSE);
    }
  else
    {
      mount->domain_entry = NULL;
    }

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      mount->password_entry = gtk_entry_new ();
      gtk_entry_set_visibility (GTK_ENTRY (mount->password_entry), FALSE);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                 _("_Password:"), 0.0, 0.5,
                                 mount->password_entry, 1, FALSE);
      if (! focus)
        focus = mount->password_entry;

      if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
        {
          /* FIXME: add check buttons for this */
        }
    }
  else
    {
      mount->password_entry = NULL;
    }

  if (focus)
    gtk_widget_grab_focus (focus);

  gimp_window_set_transient (GTK_WINDOW (dialog));
  gtk_widget_show (dialog);

  g_object_ref (operation);
}

static void
mount_operation_question_response (GtkWidget      *dialog,
                                   gint            response_id,
                                   MountOperation *operation)
{
  if (response_id >= 0)
    {
      g_mount_operation_set_choice (G_MOUNT_OPERATION (operation), response_id);
      g_mount_operation_reply (G_MOUNT_OPERATION (operation),
                               G_MOUNT_OPERATION_HANDLED);
    }
  else
    {
      g_mount_operation_reply (G_MOUNT_OPERATION (operation),
                               G_MOUNT_OPERATION_ABORTED);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (operation);
}

static void
mount_operation_ask_question (GMountOperation *operation,
                              const char      *message,
                              const char      *choices[])

{
  GtkWidget   *dialog;
  gchar       *primary;
  const gchar *secondary = NULL;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, strlen (message) - strlen (primary));
    }

  dialog = gtk_message_dialog_new (NULL,
                                   0, GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE, "%s",
                                   primary != NULL ? primary : message);
  g_free (primary);

  if (secondary)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

  if (choices)
    {
      gint i, len;

      /* First count the items in the list then
       * add the buttons in reverse order */
      for (len = 0; choices[len] != NULL; len++)
        ;

      for (i = len - 1; i >= 0; i--)
        gtk_dialog_add_button (GTK_DIALOG (dialog), choices[i], i);
    }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (mount_operation_question_response),
                    operation);

  gimp_window_set_transient (GTK_WINDOW (dialog));
  gtk_widget_show (GTK_WIDGET (dialog));

  g_object_ref (operation);
}
