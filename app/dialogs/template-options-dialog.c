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


static void  template_options_dialog_free (TemplateOptionsDialog *dialog);


TemplateOptionsDialog *
template_options_dialog_new (GimpTemplate *template,
                             GimpContext  *context,
                             GtkWidget    *parent,
                             const gchar  *title,
                             const gchar  *role,
                             const gchar  *stock_id,
                             const gchar  *desc,
                             const gchar  *help_id)
{
  TemplateOptionsDialog *options;
  GimpViewable          *viewable = NULL;
  GtkWidget             *vbox;

  g_return_val_if_fail (template == NULL || GIMP_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  options = g_slice_new0 (TemplateOptionsDialog);

  options->gimp     = context->gimp;
  options->template = template;

  if (template)
    {
      viewable = GIMP_VIEWABLE (template);
      template = gimp_config_duplicate (GIMP_CONFIG (template));
    }
  else
    {
      template =
        gimp_config_duplicate (GIMP_CONFIG (options->gimp->config->default_image));

      gimp_object_set_static_name (GIMP_OBJECT (template), _("Unnamed"));
    }

  options->dialog =
    gimp_viewable_dialog_new (viewable, context,
                              title, role, stock_id, desc,
                              parent,
                              gimp_standard_help_func, help_id,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (options->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (options->dialog),
                     (GWeakNotify) template_options_dialog_free, options);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  options->editor = gimp_template_editor_new (template, options->gimp, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), options->editor, FALSE, FALSE, 0);
  gtk_widget_show (options->editor);

  g_object_unref (template);

  return options;
}

static void
template_options_dialog_free (TemplateOptionsDialog *dialog)
{
  g_slice_free (TemplateOptionsDialog, dialog);
}
