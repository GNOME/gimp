/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgui.c
 * Copyright (C) 2013  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpoverlaybox.h"
#include "widgets/gimpoverlaydialog.h"

#include "gimpdisplayshell.h"
#include "gimptooldialog.h"
#include "gimptoolgui.h"


typedef struct _GimpToolGuiPrivate GimpToolGuiPrivate;

struct _GimpToolGuiPrivate
{
  GimpToolInfo     *tool_info;
  gchar            *description;

  gboolean          overlay;

  GimpDisplayShell *shell;

  GtkWidget        *dialog;
  GtkWidget        *vbox;
};

#define GET_PRIVATE(gui) G_TYPE_INSTANCE_GET_PRIVATE (gui, \
                                                      GIMP_TYPE_TOOL_GUI, \
                                                      GimpToolGuiPrivate)


static void   gimp_tool_gui_dispose  (GObject *object);
static void   gimp_tool_gui_finalize (GObject *object);


G_DEFINE_TYPE (GimpToolGui, gimp_tool_gui, GIMP_TYPE_OBJECT)


static void
gimp_tool_gui_class_init (GimpToolGuiClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_tool_gui_dispose;
  object_class->finalize = gimp_tool_gui_finalize;

  g_type_class_add_private (klass, sizeof (GimpToolGuiPrivate));
}

static void
gimp_tool_gui_init (GimpToolGui *gui)
{
  GimpToolGuiPrivate *private = GET_PRIVATE (gui);

  private->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  g_object_ref_sink (private->vbox);
}

static void
gimp_tool_gui_dispose (GObject *object)
{
  GimpToolGuiPrivate *private = GET_PRIVATE (object);

  if (private->tool_info)
    {
      g_object_unref (private->tool_info);
      private->tool_info = NULL;
    }

  if (private->shell)
    gimp_tool_gui_set_shell (GIMP_TOOL_GUI (object), NULL);

  if (private->vbox)
    {
      g_object_unref (private->vbox);
      private->vbox = NULL;
    }

  if (private->dialog)
    {
      if (private->overlay)
        g_object_unref (private->dialog);
      else
        gtk_widget_destroy (private->dialog);

      private->dialog = NULL;
    }

  G_OBJECT_CLASS (gimp_tool_gui_parent_class)->dispose (object);
}

static void
gimp_tool_gui_finalize (GObject *object)
{
  GimpToolGuiPrivate *private = GET_PRIVATE (object);

  if (private->description)
    {
      g_free (private->description);
      private->description = NULL;
    }

  G_OBJECT_CLASS (gimp_tool_gui_parent_class)->finalize (object);
}


/**
 * gimp_tool_gui_new:
 * @tool_info:   a #GimpToolInfo
 * @description: a string to use in the gui header or %NULL to use the help
 *               field from #GimpToolInfo
 * @...:         a %NULL-terminated valist of button parameters as described in
 *               gtk_gui_new_with_buttons().
 *
 * This function creates a #GimpToolGui using the information stored
 * in @tool_info.
 *
 * Return value: a new #GimpToolGui
 **/
GimpToolGui *
gimp_tool_gui_new (GimpToolInfo *tool_info,
                   const gchar  *description,
                   gboolean      overlay,
                   ...)
{
  GimpToolGui        *gui;
  GimpToolGuiPrivate *private;
  va_list             args;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);

  gui = g_object_new (GIMP_TYPE_TOOL_GUI, NULL);

  private = GET_PRIVATE (gui);

  private->tool_info   = g_object_ref (tool_info);
  private->description = g_strdup (description);
  private->overlay     = overlay;

  if (overlay)
    {
      private->dialog = gimp_overlay_dialog_new (tool_info, description, NULL);
      g_object_ref_sink (private->dialog);

      va_start (args, overlay);
      gimp_overlay_dialog_add_buttons_valist (GIMP_OVERLAY_DIALOG (private->dialog),
                                              args);
      va_end (args);

      gtk_container_set_border_width (GTK_CONTAINER (private->dialog), 6);

      gtk_container_set_border_width (GTK_CONTAINER (private->vbox), 0);
      gtk_container_add (GTK_CONTAINER (private->dialog), private->vbox);
      gtk_widget_show (private->vbox);
    }
  else
    {
      private->dialog = gimp_tool_dialog_new (tool_info, description, NULL);

      va_start (args, overlay);
      gimp_dialog_add_buttons_valist (GIMP_DIALOG (private->dialog), args);
      va_end (args);

      gtk_container_set_border_width (GTK_CONTAINER (private->vbox), 6);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (private->dialog))),
                          private->vbox, TRUE, TRUE, 0);
      gtk_widget_show (private->vbox);
    }

  return gui;
}

void
gimp_tool_gui_set_description (GimpToolGui *gui,
                               const gchar *description)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (description == private->description)
    return;

  g_free (private->description);
  private->description = g_strdup (description);

  if (! description)
    description = private->tool_info->help;

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      g_object_set (private->dialog, "description", description, NULL);
    }
}

void
gimp_tool_gui_set_shell (GimpToolGui      *gui,
                         GimpDisplayShell *shell)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));
  g_return_if_fail (shell == NULL || GIMP_IS_DISPLAY_SHELL (shell));

  private = GET_PRIVATE (gui);

  if (shell == private->shell)
    return;

  if (! private->overlay)
    {
      gimp_tool_dialog_set_shell (GIMP_TOOL_DIALOG (private->dialog), shell);
    }

  private->shell = shell;
}

void
gimp_tool_gui_set_viewable (GimpToolGui  *gui,
                            GimpViewable *viewable)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private = GET_PRIVATE (gui);

  if (! private->overlay)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (private->dialog),
                                         viewable,
                                         GIMP_CONTEXT (private->tool_info->tool_options));
    }
}

GtkWidget *
gimp_tool_gui_get_dialog (GimpToolGui *gui)
{
  g_return_val_if_fail (GIMP_IS_TOOL_GUI (gui), NULL);

  return GET_PRIVATE (gui)->dialog;
}

GtkWidget *
gimp_tool_gui_get_vbox (GimpToolGui *gui)
{
  g_return_val_if_fail (GIMP_IS_TOOL_GUI (gui), NULL);

  return GET_PRIVATE (gui)->vbox;
}

void
gimp_tool_gui_show (GimpToolGui *gui)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  g_return_if_fail (private->shell != NULL);

  if (private->overlay)
    {
      if (! gtk_widget_get_parent (private->dialog))
        {
          gimp_overlay_box_add_child (GIMP_OVERLAY_BOX (private->shell->canvas),
                                      private->dialog, 1.0, 0.0);
          gtk_widget_show (private->dialog);
        }
    }
  else
    {
      if (gtk_widget_get_visible (private->dialog))
        gdk_window_show (gtk_widget_get_window (private->dialog));
      else
        gtk_widget_show (private->dialog);
    }
}

void
gimp_tool_gui_hide (GimpToolGui *gui)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      if (gtk_widget_get_parent (private->dialog))
        {
          gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (private->dialog)),
                                private->dialog);
          gtk_widget_hide (private->dialog);
        }
    }
  else
    {
      if (gimp_dialog_factory_from_widget (private->dialog, NULL))
        {
          gimp_dialog_factory_hide_dialog (private->dialog);
        }
      else
        {
          gtk_widget_hide (private->dialog);
        }
    }
}

void
gimp_tool_gui_set_default_response (GimpToolGui *gui,
                                    gint         response_id)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      gtk_dialog_set_default_response (GTK_DIALOG (private->dialog),
                                       response_id);
    }
}

void
gimp_tool_gui_set_response_sensitive (GimpToolGui *gui,
                                      gint         response_id,
                                      gboolean     sensitive)
{
  GimpToolGuiPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (private->dialog),
                                         response_id, sensitive);
    }
}

void
gimp_tool_gui_set_alternative_button_order (GimpToolGui *gui,
                                            ...)
{
  GimpToolGuiPrivate *private;
  va_list             args;
  gint                response_id;
  GList              *id_list = NULL;
  GList              *list;
  gint               *ids;
  gint                n_ids;
  gint                i;

  g_return_if_fail (GIMP_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  va_start (args, gui);

  for (response_id = va_arg (args, gint);
       response_id != -1;
       response_id = va_arg (args, gint))
    {
      id_list = g_list_append (id_list, GINT_TO_POINTER (response_id));
    }

  va_end (args);

  n_ids = g_list_length (id_list);
  ids   = g_new0 (gint, n_ids);

  for (list = id_list, i = 0; list; list = g_list_next (list), i++)
    {
      ids[i] = GPOINTER_TO_INT (list->data);
    }

  g_list_free (id_list);

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      gtk_dialog_set_alternative_button_order_from_array (GTK_DIALOG (private->dialog),
                                                          n_ids, ids);
    }

  g_free (ids);
}
