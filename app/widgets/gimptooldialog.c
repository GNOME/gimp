/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooldialog.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpobject.h"
#include "core/gimptoolinfo.h"

#include "gimpdialogfactory.h"
#include "gimptooldialog.h"


G_DEFINE_TYPE (GimpToolDialog, gimp_tool_dialog, GIMP_TYPE_VIEWABLE_DIALOG)


static void
gimp_tool_dialog_class_init (GimpToolDialogClass *klass)
{
}

static void
gimp_tool_dialog_init (GimpToolDialog *dialog)
{
}


/**
 * gimp_tool_dialog_new:
 * @tool_info: a #GimpToolInfo
 * @parent:    the parent widget of this dialog or %NULL
 * @desc:      a string to use in the dialog header or %NULL to use the help
 *             field from #GimpToolInfo
 * @...:       a %NULL-terminated valist of button parameters as described in
 *             gtk_dialog_new_with_buttons().
 *
 * This function conveniently creates a #GimpViewableDialog using the
 * information stored in @tool_info. It also registers the tool with
 * the "toplevel" dialog factory.
 *
 * Return value: a new #GimpViewableDialog
 **/
GtkWidget *
gimp_tool_dialog_new (GimpToolInfo *tool_info,
                      GtkWidget    *parent,
                      const gchar  *desc,
                      ...)
{
  GtkWidget   *dialog;
  const gchar *stock_id;
  gchar       *identifier;
  va_list      args;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

  dialog = g_object_new (GIMP_TYPE_TOOL_DIALOG,
                         "title",        tool_info->blurb,
                         "role",         GIMP_OBJECT (tool_info)->name,
                         "help-func",    gimp_standard_help_func,
                         "help-id",      tool_info->help_id,
                         "stock-id",     stock_id,
                         "description",  desc ? desc : tool_info->help,
                         "parent",       parent,
                         NULL);

  va_start (args, desc);
  gimp_dialog_add_buttons_valist (GIMP_DIALOG (dialog), args);
  va_end (args);

  identifier = g_strconcat (GIMP_OBJECT (tool_info)->name, "-dialog", NULL);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   identifier,
                                   dialog);

  g_free (identifier);

  return dialog;
}
