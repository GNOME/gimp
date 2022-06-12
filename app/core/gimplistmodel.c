/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplist.c
 * Copyright (C) 2001-2016 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimplistmodel.h"

/**
 * GimpListModel:
 *
 * An object that represents a list of GimpObjects, and that is itself a
 * GimpObject.
 */


struct _GimpListModel
{
  GimpObject parent_instance;
};

enum
{
  PROP_0,
  N_PROPS
};



G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GimpListModel, gimp_list_model, GIMP_TYPE_OBJECT,
                                  G_ADD_PRIVATE (GimpListModel)
                                  G_IMPLEMENT_INTERFACE ())

static void
gimp_list_model_class_init (GimpListModelClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass    *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize              = gimp_list_finalize;
  object_class->set_property          = gimp_list_set_property;
  object_class->get_property          = gimp_list_get_property;

  gimp_object_class->get_memsize      = gimp_list_get_memsize;

}

static void
gimp_list_model_init (GimpListModel *self)
{
}

static void
gimp_list_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpListModel *self = GIMP_LIST_MODEL (object);

  switch (prop_id)
    {
    case PAGES_PROP_ITEM_TYPE:
      g_value_set_gtype (value, g_list_model_get_item_type (G_LIST_MODEL (self)));
      break;

    case PAGES_PROP_N_ITEMS:
      g_value_set_uint (value, g_list_model_get_n_items (G_LIST_MODEL (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gint64
gimp_list_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GListModel    *list    = G_LIST_MODEL (self);
  gint64         memsize = 0;

  for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
      GimpObject *item;

      item = g_list_model_get_item (list, i);
      memsize += gimp_object_get_memsize (gui_size);
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}
