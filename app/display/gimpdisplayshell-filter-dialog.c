/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh
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

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "core/ligma.h"
#include "core/ligmaviewable.h"

#include "widgets/ligmacolordisplayeditor.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-filter.h"
#include "ligmadisplayshell-filter-dialog.h"

#include "ligma-intl.h"


typedef struct
{
  LigmaDisplayShell      *shell;
  GtkWidget             *dialog;

  LigmaColorDisplayStack *old_stack;
} ColorDisplayDialog;


/*  local function prototypes  */

static void ligma_display_shell_filter_dialog_response (GtkWidget          *widget,
                                                       gint                response_id,
                                                       ColorDisplayDialog *cdd);

static void ligma_display_shell_filter_dialog_free     (ColorDisplayDialog *cdd);


/*  public functions  */

GtkWidget *
ligma_display_shell_filter_dialog_new (LigmaDisplayShell *shell)
{
  LigmaImage          *image;
  ColorDisplayDialog *cdd;
  GtkWidget          *editor;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  image = ligma_display_get_image (shell->display);

  cdd = g_slice_new0 (ColorDisplayDialog);

  cdd->shell  = shell;
  cdd->dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, image),
                                          ligma_get_user_context (shell->display->ligma),
                                          _("Color Display Filters"),
                                          "ligma-display-filters",
                                          LIGMA_ICON_DISPLAY_FILTER,
                                          _("Configure Color Display Filters"),
                                          GTK_WIDGET (cdd->shell),
                                          ligma_standard_help_func,
                                          LIGMA_HELP_DISPLAY_FILTER_DIALOG,

                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                          _("_OK"),     GTK_RESPONSE_OK,

                                          NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (cdd->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (cdd->dialog), TRUE);

  g_object_weak_ref (G_OBJECT (cdd->dialog),
                     (GWeakNotify) ligma_display_shell_filter_dialog_free, cdd);

  g_signal_connect (cdd->dialog, "response",
                    G_CALLBACK (ligma_display_shell_filter_dialog_response),
                    cdd);

  if (shell->filter_stack)
    {
      cdd->old_stack = ligma_color_display_stack_clone (shell->filter_stack);

      g_object_weak_ref (G_OBJECT (cdd->dialog),
                         (GWeakNotify) g_object_unref, cdd->old_stack);
    }
  else
    {
      LigmaColorDisplayStack *stack = ligma_color_display_stack_new ();

      ligma_display_shell_filter_set (shell, stack);
      g_object_unref (stack);
    }

  editor = ligma_color_display_editor_new (shell->display->ligma,
                                          shell->filter_stack,
                                          ligma_display_shell_get_color_config (shell),
                                          LIGMA_COLOR_MANAGED (shell));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (cdd->dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  return cdd->dialog;
}


/*  private functions  */

static void
ligma_display_shell_filter_dialog_response (GtkWidget          *widget,
                                           gint                response_id,
                                           ColorDisplayDialog *cdd)
{
  if (response_id != GTK_RESPONSE_OK)
    ligma_display_shell_filter_set (cdd->shell, cdd->old_stack);

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
}

static void
ligma_display_shell_filter_dialog_free (ColorDisplayDialog *cdd)
{
  g_slice_free (ColorDisplayDialog, cdd);
}
