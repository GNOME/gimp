/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerinfo.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerinfo.h"

#include "gimp-intl.h"


static void   gimp_controller_info_class_init   (GimpControllerInfoClass *klass);
static void   gimp_controller_info_init         (GimpControllerInfo      *info);

static void   gimp_controller_info_finalize     (GObject      *object);
static void   gimp_controller_info_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_controller_info_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


static GimpObjectClass *parent_class = NULL;


GType
gimp_controller_info_get_type (void)
{
  static GType controller_type = 0;

  if (! controller_type)
    {
      static const GTypeInfo controller_info =
      {
        sizeof (GimpControllerInfoClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_controller_info_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpControllerInfo),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_controller_info_init,
      };

      controller_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                                "GimpControllerInfo",
                                                &controller_info, 0);
    }

  return controller_type;
}

static void
gimp_controller_info_class_init (GimpControllerInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_controller_info_finalize;
  object_class->set_property = gimp_controller_info_set_property;
  object_class->get_property = gimp_controller_info_get_property;
}

static void
gimp_controller_info_init (GimpControllerInfo *info)
{
  info->controller = NULL;
  info->mapping    = g_hash_table_new_full (g_int_hash,
                                            g_int_equal,
                                            (GDestroyNotify) g_free,
                                            (GDestroyNotify) g_object_unref);
}

static void
gimp_controller_info_finalize (GObject *object)
{
  GimpControllerInfo *info = GIMP_CONTROLLER_INFO (object);

  if (info->controller)
    g_object_unref (info->controller);

  if (info->mapping)
    g_hash_table_destroy (info->mapping);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_controller_info_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpControllerInfo *info = GIMP_CONTROLLER_INFO (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_controller_info_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpControllerInfo *info = GIMP_CONTROLLER_INFO (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
