/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh
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

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolordisplayeditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-filter-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GimpDisplayShell      *shell;
  GtkWidget             *dialog;

  GimpColorDisplayStack *old_stack;
} ColorDisplayDialog;


/*  local function prototypes  */

static void gimp_display_shell_filter_dialog_response (GtkWidget          *widget,
                                                       gint                response_id,
                                                       ColorDisplayDialog *cdd);

static void gimp_display_shell_filter_dialog_free     (ColorDisplayDialog *cdd);


/*  public functions  */

GtkWidget *
gimp_display_shell_filter_dialog_new (GimpDisplayShell *shell)
{
  ColorDisplayDialog *cdd;
  GimpImage          *image;
  GtkWidget          *editor;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  image = shell->display->image;

  cdd = g_slice_new0 (ColorDisplayDialog);

  cdd->shell  = shell;
  cdd->dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                                          gimp_get_user_context (image->gimp),
                                          _("Color Display Filters"),
                                          "gimp-display-filters",
                                          GIMP_STOCK_DISPLAY_FILTER,
                                          _("Configure Color Display Filters"),
                                          GTK_WIDGET (cdd->shell),
                                          gimp_standard_help_func,
                                          GIMP_HELP_DISPLAY_FILTER_DIALOG,

                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                          NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (cdd->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (cdd->dialog), TRUE);

  g_object_weak_ref (G_OBJECT (cdd->dialog),
                     (GWeakNotify) gimp_display_shell_filter_dialog_free, cdd);

  g_signal_connect (cdd->dialog, "response",
                    G_CALLBACK (gimp_display_shell_filter_dialog_response),
                    cdd);

  if (shell->filter_stack)
    {
      cdd->old_stack = gimp_color_display_stack_clone (shell->filter_stack);

      g_object_weak_ref (G_OBJECT (cdd->dialog),
                         (GWeakNotify) g_object_unref, cdd->old_stack);
    }
  else
    {
      GimpColorDisplayStack *stack = gimp_color_display_stack_new ();

      gimp_display_shell_filter_set (shell, stack);
      g_object_unref (stack);
    }

  editor = gimp_color_display_editor_new (shell->filter_stack);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cdd->dialog)->vbox), editor);
  gtk_widget_show (editor);

  return cdd->dialog;
}


/*  private functions  */

static void
gimp_display_shell_filter_dialog_response (GtkWidget          *widget,
                                           gint                response_id,
                                           ColorDisplayDialog *cdd)
{
  if (response_id != GTK_RESPONSE_OK)
    gimp_display_shell_filter_set (cdd->shell, cdd->old_stack);

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
}

static void
gimp_display_shell_filter_dialog_free (ColorDisplayDialog *cdd)
{
  g_slice_free (ColorDisplayDialog, cdd);
}
