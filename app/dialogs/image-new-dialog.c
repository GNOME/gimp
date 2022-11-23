/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmatemplate.h"

#include "widgets/ligmacontainercombobox.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmatemplateeditor.h"
#include "widgets/ligmawidgets-utils.h"

#include "image-new-dialog.h"

#include "ligma-intl.h"


#define RESPONSE_RESET 1

typedef struct
{
  GtkWidget    *dialog;
  GtkWidget    *confirm_dialog;

  GtkWidget    *combo;
  GtkWidget    *editor;

  LigmaContext  *context;
  LigmaTemplate *template;
} ImageNewDialog;


/*  local function prototypes  */

static void   image_new_dialog_free      (ImageNewDialog *private);
static void   image_new_dialog_response  (GtkWidget      *widget,
                                          gint            response_id,
                                          ImageNewDialog *private);
static void   image_new_template_changed (LigmaContext    *context,
                                          LigmaTemplate   *template,
                                          ImageNewDialog *private);
static void   image_new_confirm_dialog   (ImageNewDialog *private);
static void   image_new_create_image     (ImageNewDialog *private);


/*  public functions  */

GtkWidget *
image_new_dialog_new (LigmaContext *context)
{
  ImageNewDialog *private;
  GtkWidget      *dialog;
  GtkWidget      *main_vbox;
  GtkWidget      *hbox;
  GtkWidget      *label;
  LigmaSizeEntry  *entry;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  private = g_slice_new0 (ImageNewDialog);

  private->context  = ligma_context_new (context->ligma, "image-new-dialog",
                                        context);
  private->template = g_object_new (LIGMA_TYPE_TEMPLATE, NULL);

  private->dialog = dialog =
    ligma_dialog_new (_("Create a New Image"),
                     "ligma-image-new",
                     NULL, 0,
                     ligma_standard_help_func, LIGMA_HELP_FILE_NEW,

                     _("_Reset"),  RESPONSE_RESET,
                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_OK"),     GTK_RESPONSE_OK,

                     NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_set_data_full (G_OBJECT (dialog),
                          "ligma-image-new-dialog", private,
                          (GDestroyNotify) image_new_dialog_free);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (image_new_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  The template combo  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Template:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  private->combo = g_object_new (LIGMA_TYPE_CONTAINER_COMBO_BOX,
                                 "container",         context->ligma->templates,
                                 "context",           private->context,
                                 "view-size",         16,
                                 "view-border-width", 0,
                                 "ellipsize",         PANGO_ELLIPSIZE_NONE,
                                 "focus-on-click",    FALSE,
                                 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), private->combo, TRUE, TRUE, 0);
  gtk_widget_show (private->combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), private->combo);

  g_signal_connect (private->context, "template-changed",
                    G_CALLBACK (image_new_template_changed),
                    private);

  /*  Template editor  */
  private->editor = ligma_template_editor_new (private->template, context->ligma,
                                              FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), private->editor, FALSE, FALSE, 0);
  gtk_widget_show (private->editor);

  entry = LIGMA_SIZE_ENTRY (ligma_template_editor_get_size_se (LIGMA_TEMPLATE_EDITOR (private->editor)));
  ligma_size_entry_set_activates_default (entry, TRUE);
  ligma_size_entry_grab_focus (entry);

  image_new_template_changed (private->context,
                              ligma_context_get_template (private->context),
                              private);

  return dialog;
}

void
image_new_dialog_set (GtkWidget    *dialog,
                      LigmaImage    *image,
                      LigmaTemplate *template)
{
  ImageNewDialog *private;

  g_return_if_fail (LIGMA_IS_DIALOG (dialog));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));
  g_return_if_fail (template == NULL || LIGMA_IS_TEMPLATE (template));

  private = g_object_get_data (G_OBJECT (dialog), "ligma-image-new-dialog");

  g_return_if_fail (private != NULL);

  ligma_context_set_template (private->context, template);

  if (! template)
    {
      template = ligma_image_new_get_last_template (private->context->ligma,
                                                   image);

      image_new_template_changed (private->context, template, private);

      g_object_unref (template);
    }
}


/*  private functions  */

static void
image_new_dialog_free (ImageNewDialog *private)
{
  g_object_unref (private->context);
  g_object_unref (private->template);

  g_slice_free (ImageNewDialog, private);
}

static void
image_new_dialog_response (GtkWidget      *dialog,
                           gint            response_id,
                           ImageNewDialog *private)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      ligma_config_sync (G_OBJECT (private->context->ligma->config->default_image),
                        G_OBJECT (private->template), 0);
      ligma_context_set_template (private->context, NULL);
      break;

    case GTK_RESPONSE_OK:
      if (ligma_template_get_initial_size (private->template) >
          LIGMA_GUI_CONFIG (private->context->ligma->config)->max_new_image_size)
        image_new_confirm_dialog (private);
      else
        image_new_create_image (private);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
image_new_template_changed (LigmaContext    *context,
                            LigmaTemplate   *template,
                            ImageNewDialog *private)
{
  LigmaTemplateEditor *editor;
  GtkWidget          *chain;
  gdouble             xres, yres;
  gchar              *comment;

  if (! template)
    return;

  editor = LIGMA_TEMPLATE_EDITOR (private->editor);
  chain  = ligma_template_editor_get_resolution_chain (editor);

  xres = ligma_template_get_resolution_x (template);
  yres = ligma_template_get_resolution_y (template);

  ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain),
                                ABS (xres - yres) < LIGMA_MIN_RESOLUTION);

  comment = (gchar *) ligma_template_get_comment (template);

  if (! comment || ! strlen (comment))
    comment = g_strdup (ligma_template_get_comment (private->template));
  else
    comment = NULL;

  /*  make sure the resolution values are copied first (see bug #546924)  */
  ligma_config_sync (G_OBJECT (template), G_OBJECT (private->template),
                    LIGMA_TEMPLATE_PARAM_COPY_FIRST);
  ligma_config_sync (G_OBJECT (template), G_OBJECT (private->template), 0);

  if (comment)
    {
      g_object_set (private->template,
                    "comment", comment,
                    NULL);

      g_free (comment);
    }
}


/*  the confirm dialog  */

static void
image_new_confirm_response (GtkWidget      *dialog,
                            gint            response_id,
                            ImageNewDialog *private)
{
  gtk_widget_destroy (dialog);

  private->confirm_dialog = NULL;

  if (response_id == GTK_RESPONSE_OK)
    image_new_create_image (private);
  else
    gtk_widget_set_sensitive (private->dialog, TRUE);
}

static void
image_new_confirm_dialog (ImageNewDialog *private)
{
  LigmaGuiConfig *config;
  GtkWidget     *dialog;
  gchar         *size;

  if (private->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (private->confirm_dialog));
      return;
    }

  private->confirm_dialog =
    dialog = ligma_message_dialog_new (_("Confirm Image Size"),
                                      LIGMA_ICON_DIALOG_WARNING,
                                      private->dialog,
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      ligma_standard_help_func, NULL,

                                      _("_Cancel"), GTK_RESPONSE_CANCEL,
                                      _("_OK"),     GTK_RESPONSE_OK,

                                      NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (private->confirm_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (image_new_confirm_response),
                    private);

  size = g_format_size (ligma_template_get_initial_size (private->template));
  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                     _("You are trying to create an image "
                                       "with a size of %s."), size);
  g_free (size);

  config = LIGMA_GUI_CONFIG (private->context->ligma->config);
  size = g_format_size (config->max_new_image_size);
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                              _("An image of the chosen size will use more "
                                "memory than what is configured as "
                                "\"Maximum new image size\" in the Preferences "
                                "dialog (currently %s)."), size);
  g_free (size);

  gtk_widget_set_sensitive (private->dialog, FALSE);

  gtk_widget_show (dialog);
}

static void
image_new_create_image (ImageNewDialog *private)
{
  LigmaTemplate *template = g_object_ref (private->template);
  Ligma         *ligma     = private->context->ligma;
  LigmaImage    *image;

  gtk_widget_hide (private->dialog);

  image = ligma_image_new_from_template (ligma, template,
                                        ligma_get_user_context (ligma));
  ligma_create_display (ligma, image, ligma_template_get_unit (template), 1.0,
                       G_OBJECT (ligma_widget_get_monitor (private->dialog)));
  g_object_unref (image);

  gtk_widget_destroy (private->dialog);

  ligma_image_new_set_last_template (ligma, template);

  g_object_unref (template);
}
