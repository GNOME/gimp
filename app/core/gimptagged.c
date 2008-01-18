/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagged.c
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "gimpmarshal.h"
#include "gimptagged.h"


enum
{
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};

static void  gimp_tagged_base_init (gpointer klass);

static guint gimp_tagged_signals[LAST_SIGNAL] = { 0, };


GType
gimp_tagged_interface_get_type (void)
{
  static GType tagged_iface_type = 0;

  if (! tagged_iface_type)
    {
      const GTypeInfo tagged_iface_info =
      {
        sizeof (GimpTaggedInterface),
        gimp_tagged_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      tagged_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                  "GimpTaggedInterface",
                                                  &tagged_iface_info,
                                                  0);
   }

  return tagged_iface_type;
}

static void
gimp_tagged_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      gimp_tagged_signals[TAG_ADDED] =
        g_signal_new ("tag-added",
                      GIMP_TYPE_TAGGED,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GimpTaggedInterface, tag_added),
                      NULL, NULL,
                      gimp_marshal_VOID__INT,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);
      gimp_tagged_signals[TAG_REMOVED] =
        g_signal_new ("tag-removed",
                      GIMP_TYPE_TAGGED,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GimpTaggedInterface, tag_removed),
                      NULL, NULL,
                      gimp_marshal_VOID__INT,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);

      initialized = TRUE;
    }
}

/**
 * gimp_tagged_add_tag:
 * @tagged: an object that implements the %GimpTagged interface
 * @tag:    a %GimpTag
 *
 * Adds @tag to the @tagged object. The GimpTagged::tag-added signal
 * is emitted if and only if the @tag was not already assigned to this
 * object.
 **/
void
gimp_tagged_add_tag (GimpTagged    *tagged,
                     const GimpTag  tag)
{
  g_return_if_fail (GIMP_IS_TAGGED (tagged));

  if (GIMP_TAGGED_GET_INTERFACE (tagged)->add_tag (tagged, tag))
    {
      g_signal_emit (tagged, gimp_tagged_signals[TAG_ADDED], 0, tag);
    }
}

/**
 * gimp_tagged_remove_tag:
 * @tagged: an object that implements the %GimpTagged interface
 * @tag:    a %GimpTag
 *
 * Removes @tag from the @tagged object. The GimpTagged::tag-removed
 * signal is emitted if and only if the @tag was actually assigned to
 * this object.
 **/
void
gimp_tagged_remove_tag (GimpTagged *tagged,
                        GimpTag     tag)
{
  g_return_if_fail (GIMP_IS_TAGGED (tagged));

  if (GIMP_TAGGED_GET_INTERFACE (tagged)->remove_tag (tagged, tag))
    {
      g_signal_emit (tagged, gimp_tagged_signals[TAG_REMOVED], 0, tag);
    }
}

/**
 * gimp_tagged_get_tags:
 * @tagged: an object that implements the %GimpTagged interface
 *
 * Returns the list of tags assigned to this object. The returned %GList
 * is owned by the @tagged object and must not be modified or destroyed.
 *
 * Return value: a list of tags
 **/
GList *
gimp_tagged_get_get_tags (GimpTagged *tagged)
{
  g_return_val_if_fail (GIMP_IS_TAGGED (tagged), NULL);

  return GIMP_TAGGED_GET_INTERFACE (tagged)->get_tags (tagged);
}
