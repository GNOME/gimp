/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpiconpicker.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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
#include "core/gimplist.h"
#include "core/gimpcontext.h"
#include "core/gimptemplate.h"

#include "gimpiconpicker.h"
#include "gimpviewablebutton.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_STOCK_ID
};


typedef struct _GimpIconPickerPrivate GimpIconPickerPrivate;

struct _GimpIconPickerPrivate
{
  Gimp          *gimp;

  gchar         *stock_id;

  GimpContainer *stock_id_container;
  GimpContext   *stock_id_context;
};

#define GET_PRIVATE(picker) \
        G_TYPE_INSTANCE_GET_PRIVATE (picker, \
                                     GIMP_TYPE_ICON_PICKER, \
                                     GimpIconPickerPrivate)


static void    gimp_icon_picker_constructed  (GObject        *object);
static void    gimp_icon_picker_finalize     (GObject        *object);
static void    gimp_icon_picker_set_property (GObject        *object,
                                              guint           property_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void    gimp_icon_picker_get_property (GObject        *object,
                                              guint           property_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);

static void gimp_icon_picker_icon_changed    (GimpContext    *context,
                                              GimpTemplate   *template,
                                              GimpIconPicker *picker);


G_DEFINE_TYPE (GimpIconPicker, gimp_icon_picker, GTK_TYPE_BOX)

#define parent_class gimp_icon_picker_parent_class


static void
gimp_icon_picker_class_init (GimpIconPickerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_icon_picker_constructed;
  object_class->finalize     = gimp_icon_picker_finalize;
  object_class->set_property = gimp_icon_picker_set_property;
  object_class->get_property = gimp_icon_picker_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_STOCK_ID,
                                   g_param_spec_string ("stock-id", NULL, NULL,
                                                        "gimp-toilet-paper",
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_type_class_add_private (object_class, sizeof (GimpIconPickerPrivate));
}

static void
gimp_icon_picker_init (GimpIconPicker *picker)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (picker),
                                  GTK_ORIENTATION_HORIZONTAL);
}

static void
gimp_icon_picker_constructed (GObject *object)
{
  GimpIconPicker        *picker  = GIMP_ICON_PICKER (object);
  GimpIconPickerPrivate *private = GET_PRIVATE (object);
  GtkWidget             *button;
  GSList                *stock_list;
  GSList                *list;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (private->gimp));

  private->stock_id_container = gimp_list_new (GIMP_TYPE_TEMPLATE, FALSE);
  private->stock_id_context = gimp_context_new (private->gimp, "foo", NULL);

  g_signal_connect (private->stock_id_context, "template-changed",
                    G_CALLBACK (gimp_icon_picker_icon_changed),
                    picker);

  stock_list = gtk_stock_list_ids ();

  for (list = stock_list; list; list = g_slist_next (list))
    {
      GimpObject *object = g_object_new (GIMP_TYPE_TEMPLATE,
                                         "name",     list->data,
                                         "stock-id", list->data,
                                         NULL);

      gimp_container_add (private->stock_id_container, object);
      g_object_unref (object);

      if (private->stock_id && strcmp (list->data, private->stock_id) == 0)
        gimp_context_set_template (private->stock_id_context,
                                   GIMP_TEMPLATE (object));
    }

  g_slist_free_full (stock_list, (GDestroyNotify) g_free);

  button = gimp_viewable_button_new (private->stock_id_container,
                                     private->stock_id_context,
                                     GIMP_VIEW_TYPE_LIST,
                                     GIMP_VIEW_SIZE_SMALL,
                                     GIMP_VIEW_SIZE_SMALL, 0,
                                     NULL, NULL, NULL, NULL);
  gimp_viewable_button_set_view_type (GIMP_VIEWABLE_BUTTON (button),
                                      GIMP_VIEW_TYPE_GRID);
  gtk_box_pack_start (GTK_BOX (picker), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
gimp_icon_picker_finalize (GObject *object)
{
  GimpIconPickerPrivate *private = GET_PRIVATE (object);

  if (private->stock_id)
    {
      g_free (private->stock_id);
      private->stock_id = NULL;
    }

  if (private->stock_id_container)
    {
      g_object_unref (private->stock_id_container);
      private->stock_id_container = NULL;
    }

  if (private->stock_id_context)
    {
      g_object_unref (private->stock_id_context);
      private->stock_id_context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_icon_picker_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpIconPickerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value); /* don't ref */
      break;

    case PROP_STOCK_ID:
      gimp_icon_picker_set_stock_id (GIMP_ICON_PICKER (object),
                                     g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_icon_picker_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  GimpIconPickerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

    case PROP_STOCK_ID:
      g_value_set_string (value, private->stock_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_icon_picker_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_ICON_PICKER,
                       "gimp", gimp,
                       NULL);
}

const gchar *
gimp_icon_picker_get_stock_id (GimpIconPicker *picker)
{
  g_return_val_if_fail (GIMP_IS_ICON_PICKER (picker), NULL);

  return GET_PRIVATE (picker)->stock_id;
}

void
gimp_icon_picker_set_stock_id (GimpIconPicker *picker,
                               const gchar    *stock_id)
{
  GimpIconPickerPrivate *private;

  g_return_if_fail (GIMP_IS_ICON_PICKER (picker));
  g_return_if_fail (stock_id != NULL);

  private = GET_PRIVATE (picker);

  g_free (private->stock_id);
  private->stock_id = g_strdup (stock_id);

  if (private->stock_id_container)
    {
      GimpObject *object;

      object = gimp_container_get_child_by_name (private->stock_id_container,
                                                 stock_id);

      if (object)
        gimp_context_set_template (private->stock_id_context,
                                   GIMP_TEMPLATE (object));
    }

  g_object_notify (G_OBJECT (picker), "stock-id");
}


/*  private functions  */

static void
gimp_icon_picker_icon_changed (GimpContext    *context,
                               GimpTemplate   *template,
                               GimpIconPicker *picker)
{
  gimp_icon_picker_set_stock_id (picker, gimp_object_get_name (template));
}
