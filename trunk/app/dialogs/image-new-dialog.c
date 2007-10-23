/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimptemplateeditor.h"

#include "image-new-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1

typedef struct
{
  GtkWidget    *dialog;
  GtkWidget    *confirm_dialog;

  GtkWidget    *combo;
  GtkWidget    *editor;

  GimpContext  *context;
  GimpTemplate *template;
} ImageNewDialog;


/*  local function prototypes  */

static void   image_new_dialog_free      (ImageNewDialog *dialog);
static void   image_new_dialog_response  (GtkWidget      *widget,
                                          gint            response_id,
                                          ImageNewDialog *dialog);
static void   image_new_template_changed (GimpContext    *context,
                                          GimpTemplate   *template,
                                          ImageNewDialog *dialog);
static void   image_new_confirm_dialog   (ImageNewDialog *dialog);
static void   image_new_create_image     (ImageNewDialog *dialog);


/*  public functions  */

GtkWidget *
image_new_dialog_new (GimpContext *context)
{
  ImageNewDialog *dialog;
  GtkWidget      *main_vbox;
  GtkWidget      *table;
  GimpSizeEntry  *entry;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  dialog = g_slice_new0 (ImageNewDialog);

  dialog->context  = gimp_context_new (context->gimp, "image-new-dialog",
                                       context);
  dialog->template = g_object_new (GIMP_TYPE_TEMPLATE, NULL);

  dialog->dialog = gimp_dialog_new (_("Create a New Image"),
                                    "gimp-image-new",
                                    NULL, 0,
                                    gimp_standard_help_func, GIMP_HELP_FILE_NEW,

                                    GIMP_STOCK_RESET, RESPONSE_RESET,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                    NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_set_data_full (G_OBJECT (dialog->dialog),
                          "gimp-image-new-dialog", dialog,
                          (GDestroyNotify) image_new_dialog_free);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (image_new_dialog_response),
                    dialog);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  dialog->combo = g_object_new (GIMP_TYPE_CONTAINER_COMBO_BOX,
                                "container",         context->gimp->templates,
                                "context",           dialog->context,
                                "view-size",         16,
                                "view-border-width", 0,
                                "ellipsize",         PANGO_ELLIPSIZE_NONE,
                                "focus-on-click",    FALSE,
                                NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Template:"),  0.0, 0.5,
                             dialog->combo, 1, TRUE);

  g_signal_connect (dialog->context, "template-changed",
                    G_CALLBACK (image_new_template_changed),
                    dialog);

  /*  Template editor  */
  dialog->editor = gimp_template_editor_new (dialog->template, context->gimp,
                                             FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->editor, FALSE, FALSE, 0);
  gtk_widget_show (dialog->editor);

  entry = GIMP_SIZE_ENTRY (GIMP_TEMPLATE_EDITOR (dialog->editor)->size_se);
  gimp_size_entry_set_activates_default (entry, TRUE);
  gimp_size_entry_grab_focus (entry);

  image_new_template_changed (dialog->context,
                              gimp_context_get_template (dialog->context),
                              dialog);

  return dialog->dialog;
}

void
image_new_dialog_set (GtkWidget    *widget,
                      GimpImage    *image,
                      GimpTemplate *template)
{
  ImageNewDialog *dialog;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));
  g_return_if_fail (template == NULL || GIMP_IS_TEMPLATE (template));

  dialog = g_object_get_data (G_OBJECT (widget), "gimp-image-new-dialog");

  g_return_if_fail (dialog != NULL);

  gimp_context_set_template (dialog->context, template);

  if (! template)
    {
      template = gimp_image_new_get_last_template (dialog->context->gimp,
                                                   image);

      gimp_config_sync (G_OBJECT (template), G_OBJECT (dialog->template), 0);

      g_object_unref (template);
    }
}


/*  private functions  */

static void
image_new_dialog_free (ImageNewDialog *dialog)
{
  g_object_unref (dialog->context);
  g_object_unref (dialog->template);

  g_slice_free (ImageNewDialog, dialog);
}

static void
image_new_dialog_response (GtkWidget      *widget,
                           gint            response_id,
                           ImageNewDialog *dialog)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_config_sync (G_OBJECT (dialog->context->gimp->config->default_image),
                        G_OBJECT (dialog->template), 0);
      gimp_context_set_template (dialog->context, NULL);
      break;

    case GTK_RESPONSE_OK:
      if (dialog->template->initial_size >
          GIMP_GUI_CONFIG (dialog->context->gimp->config)->max_new_image_size)
        image_new_confirm_dialog (dialog);
      else
        image_new_create_image (dialog);
      break;

    default:
      gtk_widget_destroy (dialog->dialog);
      break;
    }
}

static void
image_new_template_changed (GimpContext    *context,
                            GimpTemplate   *template,
                            ImageNewDialog *dialog)
{
  gchar *comment = NULL;

  if (!template)
    return;

  if (!template->comment || !strlen (template->comment))
    comment = g_strdup (dialog->template->comment);

  gimp_config_sync (G_OBJECT (template), G_OBJECT (dialog->template), 0);

  if (comment)
    {
      g_object_set (dialog->template,
                    "comment", comment,
                    NULL);

      g_free (comment);
    }
}


/*  the confirm dialog  */

static void
image_new_confirm_response (GtkWidget      *dialog,
                            gint            response_id,
                            ImageNewDialog *data)
{
  gtk_widget_destroy (dialog);

  data->confirm_dialog = NULL;

  if (response_id == GTK_RESPONSE_OK)
    image_new_create_image (data);
  else
    gtk_widget_set_sensitive (data->dialog, TRUE);
}

static void
image_new_confirm_dialog (ImageNewDialog *data)
{
  GimpGuiConfig *config;
  GtkWidget     *dialog;
  gchar         *size;

  if (data->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (data->confirm_dialog));
      return;
    }

  data->confirm_dialog =
    dialog = gimp_message_dialog_new (_("Confirm Image Size"),
                                      GIMP_STOCK_WARNING,
                                      data->dialog,
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      gimp_standard_help_func, NULL,

                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                      NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (data->confirm_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (image_new_confirm_response),
                    data);

  size = gimp_memsize_to_string (data->template->initial_size);
  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("You are trying to create an image "
                                       "with a size of %s."), size);
  g_free (size);

  config = GIMP_GUI_CONFIG (data->context->gimp->config);
  size = gimp_memsize_to_string (config->max_new_image_size);
  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                              _("An image of the chosen size will use more "
                                "memory than what is configured as "
                                "\"Maximum Image Size\" in the Preferences "
                                "dialog (currently %s)."), size);
  g_free (size);

  gtk_widget_set_sensitive (data->dialog, FALSE);

  gtk_widget_show (dialog);
}

static void
image_new_create_image (ImageNewDialog *dialog)
{
  GimpTemplate *template = g_object_ref (dialog->template);
  GimpContext  *context  = g_object_ref (dialog->context);
  Gimp         *gimp     = dialog->context->gimp;

  gtk_widget_destroy (dialog->dialog);

  gimp_template_create_image (gimp, template, context);
  gimp_image_new_set_last_template (gimp, template);

  g_object_unref (template);
  g_object_unref (context);
}
