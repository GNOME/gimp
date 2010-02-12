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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpmarshal.h"

#include "gimpdeviceeditor.h"
#include "gimpdeviceinfo.h"
#include "gimpdeviceinfoeditor.h"
#include "gimpdevices.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP
};


typedef struct _GimpDeviceEditorPrivate GimpDeviceEditorPrivate;

struct _GimpDeviceEditorPrivate
{
  Gimp      *gimp;

  GtkWidget *notebook;
};


#define GIMP_DEVICE_EDITOR_GET_PRIVATE(editor) \
        G_TYPE_INSTANCE_GET_PRIVATE (editor, \
                                     GIMP_TYPE_DEVICE_EDITOR, \
                                     GimpDeviceEditorPrivate)


static GObject * gimp_device_editor_constructor    (GType                  type,
                                                    guint                  n_params,
                                                    GObjectConstructParam *params);
static void      gimp_device_editor_set_property   (GObject               *object,
                                                    guint                  property_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void      gimp_device_editor_get_property   (GObject               *object,
                                                    guint                  property_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);

static void      gimp_device_editor_screen_changed (GtkWidget             *widget,
                                                    GdkScreen             *previous_screen);


G_DEFINE_TYPE (GimpDeviceEditor, gimp_device_editor, GTK_TYPE_HBOX)

#define parent_class gimp_device_editor_parent_class


static void
gimp_device_editor_class_init (GimpDeviceEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructor    = gimp_device_editor_constructor;
  object_class->set_property   = gimp_device_editor_set_property;
  object_class->get_property   = gimp_device_editor_get_property;

  widget_class->screen_changed = gimp_device_editor_screen_changed;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GimpDeviceEditorPrivate));
}

static void
gimp_device_editor_init (GimpDeviceEditor *editor)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  private->notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (private->notebook), GTK_POS_LEFT);
  gtk_box_pack_start (GTK_BOX (editor), private->notebook, TRUE, TRUE, 0);
  gtk_widget_show (private->notebook);
}

static GObject *
gimp_device_editor_constructor (GType                   type,
                                guint                   n_params,
                                GObjectConstructParam  *params)
{
  GObject                 *object;
  GimpDeviceEditor        *editor;
  GimpDeviceEditorPrivate *private;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor  = GIMP_DEVICE_EDITOR (object);
  private = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);

  g_assert (GIMP_IS_GIMP (private->gimp));

  return object;
}

static void
gimp_device_editor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_editor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDeviceEditorPrivate *private = GIMP_DEVICE_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_editor_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous_screen)
{
  GimpDeviceEditor        *editor       = GIMP_DEVICE_EDITOR (widget);
  GimpDeviceEditorPrivate *private      = GIMP_DEVICE_EDITOR_GET_PRIVATE (editor);
  GdkDevice               *core_pointer = NULL;
  GList                   *devices      = NULL;
  GList                   *list;

  if (gtk_widget_has_screen (widget))
    {
      GdkDisplay *display = gtk_widget_get_display (widget);

      devices      = gdk_display_list_devices (display);
      core_pointer = gdk_display_get_core_pointer (display);
    }

  gtk_container_foreach (GTK_CONTAINER (private->notebook),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (g_list_length (devices) <= 1)
    return;

  for (list = devices; list; list = g_list_next (list))
    {
      GdkDevice *device = list->data;

      if (device != core_pointer)
        {
          GimpDeviceInfo *info = gimp_device_info_get_by_device (device);
          GtkWidget      *widget;

          widget = gimp_device_info_editor_new (info);
          gtk_container_set_border_width (GTK_CONTAINER (widget), 12);

          gtk_notebook_append_page (GTK_NOTEBOOK (private->notebook),
                                    widget,
                                    gtk_label_new (gimp_object_get_name (info)));
          gtk_widget_show (widget);
        }
    }
}

GtkWidget *
gimp_device_editor_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_EDITOR,
                       "gimp", gimp,
                       NULL);
}
