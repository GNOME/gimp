/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptemplate.h"

#include "widgets/gimptemplateeditor.h"
#include "widgets/gimpviewabledialog.h"

#include "template-options-dialog.h"

#include "gimp-intl.h"


typedef struct _TemplateOptionsDialog TemplateOptionsDialog;

struct _TemplateOptionsDialog
{
  GimpTemplate                *template;
  GimpContext                 *context;
  GimpTemplateOptionsCallback  callback;
  gpointer                     user_data;

  GtkWidget                   *editor;
};


/*  local function prototypes  */

static void   template_options_dialog_free     (TemplateOptionsDialog *private);
static void   template_options_dialog_response (GtkWidget             *dialog,
                                                gint                   response_id,
                                                TemplateOptionsDialog *private);


/*  public function  */

GtkWidget *
template_options_dialog_new (GimpTemplate *template,
                             GimpContext  *context,
                             GtkWidget    *parent,
                             const gchar  *title,
                             const gchar  *role,
                             const gchar  *icon_name,
                             const gchar  *desc,
                             const gchar  *help_id,
                             GimpTemplateOptionsCallback  callback,
                             gpointer                     user_data)
{
  TemplateOptionsDialog *private;
  GtkWidget             *dialog;
  GimpViewable          *viewable = NULL;
  GtkWidget             *vbox;

  g_return_val_if_fail (template == NULL || GIMP_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (TemplateOptionsDialog);

  private->template  = template;
  private->context   = context;
  private->callback  = callback;
  private->user_data = user_data;

  if (template)
    {
      viewable = GIMP_VIEWABLE (template);
      template = gimp_config_duplicate (GIMP_CONFIG (template));
    }
  else
    {
      template =
        gimp_config_duplicate (GIMP_CONFIG (context->gimp->config->default_image));
      viewable = GIMP_VIEWABLE (template);

      gimp_object_set_static_name (GIMP_OBJECT (template), _("Unnamed"));
    }

  dialog = gimp_viewable_dialog_new (g_list_prepend (NULL, viewable), context,
                                     title, role, icon_name, desc,
                                     parent,
                                     gimp_standard_help_func, help_id,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_OK"),     GTK_RESPONSE_OK,

                                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) template_options_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (template_options_dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  private->editor = gimp_template_editor_new (template, context->gimp, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), private->editor, FALSE, FALSE, 0);
  gtk_widget_show (private->editor);

  g_object_unref (template);

  return dialog;
}


/*  private functions  */

static void
template_options_dialog_free (TemplateOptionsDialog *private)
{
  g_slice_free (TemplateOptionsDialog, private);
}

static void
template_options_dialog_response (GtkWidget             *dialog,
                                  gint                   response_id,
                                  TemplateOptionsDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpTemplateEditor *editor = GIMP_TEMPLATE_EDITOR (private->editor);

      private->callback (dialog,
                         private->template,
                         gimp_template_editor_get_template (editor),
                         private->context,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}
