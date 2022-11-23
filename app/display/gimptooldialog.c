/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooldialog.c
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "core/ligmaobject.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmadialogfactory.h"

#include "ligmadisplayshell.h"
#include "ligmatooldialog.h"


typedef struct _LigmaToolDialogPrivate LigmaToolDialogPrivate;

struct _LigmaToolDialogPrivate
{
  LigmaDisplayShell *shell;
};

#define GET_PRIVATE(dialog) ((LigmaToolDialogPrivate *) ligma_tool_dialog_get_instance_private ((LigmaToolDialog *) (dialog)))


static void   ligma_tool_dialog_dispose     (GObject          *object);

static void   ligma_tool_dialog_shell_unmap (LigmaDisplayShell *shell,
                                            LigmaToolDialog   *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolDialog, ligma_tool_dialog,
                            LIGMA_TYPE_VIEWABLE_DIALOG)


static void
ligma_tool_dialog_class_init (LigmaToolDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_tool_dialog_dispose;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "LigmaToolDialog");
}

static void
ligma_tool_dialog_init (LigmaToolDialog *dialog)
{
}

static void
ligma_tool_dialog_dispose (GObject *object)
{
  LigmaToolDialogPrivate *private = GET_PRIVATE (object);

  if (private->shell)
    {
      g_object_remove_weak_pointer (G_OBJECT (private->shell),
                                    (gpointer) &private->shell);
      private->shell = NULL;
    }

  G_OBJECT_CLASS (ligma_tool_dialog_parent_class)->dispose (object);
}


/**
 * ligma_tool_dialog_new: (skip)
 * @tool_info: a #LigmaToolInfo
 * @desc:      a string to use in the dialog header or %NULL to use the help
 *             field from #LigmaToolInfo
 * @...:       a %NULL-terminated valist of button parameters as described in
 *             gtk_dialog_new_with_buttons().
 *
 * This function conveniently creates a #LigmaViewableDialog using the
 * information stored in @tool_info. It also registers the tool with
 * the "toplevel" dialog factory.
 *
 * Returns: a new #LigmaViewableDialog
 **/
GtkWidget *
ligma_tool_dialog_new (LigmaToolInfo *tool_info,
                      GdkMonitor   *monitor,
                      const gchar  *title,
                      const gchar  *description,
                      const gchar  *icon_name,
                      const gchar  *help_id,
                      ...)
{
  GtkWidget *dialog;
  gchar     *identifier;
  va_list    args;
  gboolean   use_header_bar;

  g_return_val_if_fail (LIGMA_IS_TOOL_INFO (tool_info), NULL);

  if (! title)
    title = tool_info->label;

  if (! description)
    description = tool_info->tooltip;

  if (! help_id)
    help_id = tool_info->help_id;

  if (! icon_name)
    icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (LIGMA_TYPE_TOOL_DIALOG,
                         "title",          title,
                         "role",           ligma_object_get_name (tool_info),
                         "description",    description,
                         "icon-name",      icon_name,
                         "help-func",      ligma_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                          NULL);

  va_start (args, help_id);
  ligma_dialog_add_buttons_valist (LIGMA_DIALOG (dialog), args);
  va_end (args);

  identifier = g_strconcat (ligma_object_get_name (tool_info), "-dialog", NULL);

  ligma_dialog_factory_add_foreign (ligma_dialog_factory_get_singleton (),
                                   identifier,
                                   dialog,
                                   monitor);

  g_free (identifier);

  return dialog;
}

void
ligma_tool_dialog_set_shell (LigmaToolDialog   *tool_dialog,
                            LigmaDisplayShell *shell)
{
  LigmaToolDialogPrivate *private = GET_PRIVATE (tool_dialog);

  g_return_if_fail (LIGMA_IS_TOOL_DIALOG (tool_dialog));
  g_return_if_fail (shell == NULL || LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell == private->shell)
    return;

  if (private->shell)
    {
      g_object_remove_weak_pointer (G_OBJECT (private->shell),
                                    (gpointer) &private->shell);
      g_signal_handlers_disconnect_by_func (private->shell,
                                            ligma_tool_dialog_shell_unmap,
                                            tool_dialog);

      gtk_window_set_transient_for (GTK_WINDOW (tool_dialog), NULL);
    }

  private->shell = shell;

  if (private->shell)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

      gtk_window_set_transient_for (GTK_WINDOW (tool_dialog),
                                    GTK_WINDOW (toplevel));

      g_signal_connect_object (private->shell, "unmap",
                               G_CALLBACK (ligma_tool_dialog_shell_unmap),
                               tool_dialog, 0);
      g_object_add_weak_pointer (G_OBJECT (private->shell),
                                 (gpointer) &private->shell);
    }
}


/*  private functions  */

static void
ligma_tool_dialog_shell_unmap (LigmaDisplayShell *shell,
                              LigmaToolDialog   *dialog)
{
  /*  the dialog being mapped while the shell is being unmapped
   *  happens when switching images in LigmaImageWindow
   */
  if (gtk_widget_get_mapped (GTK_WIDGET (dialog)))
    g_signal_emit_by_name (dialog, "close");
}
