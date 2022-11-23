/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatagged.c
 * Copyright (C) 2008  Sven Neumann <sven@ligma.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "ligmatag.h"
#include "ligmatagged.h"


enum
{
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (LigmaTagged, ligma_tagged, G_TYPE_OBJECT)


static guint ligma_tagged_signals[LAST_SIGNAL] = { 0, };


/*  private functions  */


static void
ligma_tagged_default_init (LigmaTaggedInterface *iface)
{
  ligma_tagged_signals[TAG_ADDED] =
    g_signal_new ("tag-added",
                  LIGMA_TYPE_TAGGED,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaTaggedInterface, tag_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_TAG);

  ligma_tagged_signals[TAG_REMOVED] =
    g_signal_new ("tag-removed",
                  LIGMA_TYPE_TAGGED,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaTaggedInterface, tag_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_TAG);
}


/*  public functions  */


/**
 * ligma_tagged_add_tag:
 * @tagged: an object that implements the %LigmaTagged interface
 * @tag:    a %LigmaTag
 *
 * Adds @tag to the @tagged object. The LigmaTagged::tag-added signal
 * is emitted if and only if the @tag was not already assigned to this
 * object.
 **/
void
ligma_tagged_add_tag (LigmaTagged *tagged,
                     LigmaTag    *tag)
{
  g_return_if_fail (LIGMA_IS_TAGGED (tagged));
  g_return_if_fail (LIGMA_IS_TAG (tag));

  if (LIGMA_TAGGED_GET_IFACE (tagged)->add_tag (tagged, tag))
    {
      g_signal_emit (tagged, ligma_tagged_signals[TAG_ADDED], 0, tag);
    }
}

/**
 * ligma_tagged_remove_tag:
 * @tagged: an object that implements the %LigmaTagged interface
 * @tag:    a %LigmaTag
 *
 * Removes @tag from the @tagged object. The LigmaTagged::tag-removed
 * signal is emitted if and only if the @tag was actually assigned to
 * this object.
 **/
void
ligma_tagged_remove_tag (LigmaTagged *tagged,
                        LigmaTag    *tag)
{
  GList *tag_iter;

  g_return_if_fail (LIGMA_IS_TAGGED (tagged));
  g_return_if_fail (LIGMA_IS_TAG (tag));

  for (tag_iter = ligma_tagged_get_tags (tagged);
       tag_iter;
       tag_iter = g_list_next (tag_iter))
    {
      LigmaTag *tag_ref = tag_iter->data;

      if (ligma_tag_equals (tag_ref, tag))
        {
          g_object_ref (tag_ref);

          if (LIGMA_TAGGED_GET_IFACE (tagged)->remove_tag (tagged, tag_ref))
            {
              g_signal_emit (tagged, ligma_tagged_signals[TAG_REMOVED], 0,
                             tag_ref);
            }

          g_object_unref (tag_ref);

          return;
        }
    }
}

/**
 * ligma_tagged_set_tags:
 * @tagged: an object that implements the %LigmaTagged interface
 * @tags:   a list of tags
 *
 * Sets the list of tags assigned to this object. The passed list of
 * tags is copied and should be freed by the caller.
 **/
void
ligma_tagged_set_tags (LigmaTagged *tagged,
                      GList      *tags)
{
  GList *old_tags;
  GList *list;

  g_return_if_fail (LIGMA_IS_TAGGED (tagged));

  old_tags = g_list_copy (ligma_tagged_get_tags (tagged));

  for (list = old_tags; list; list = g_list_next (list))
    {
      ligma_tagged_remove_tag (tagged, list->data);
    }

  g_list_free (old_tags);

  for (list = tags; list; list = g_list_next (list))
    {
      g_return_if_fail (LIGMA_IS_TAG (list->data));

      ligma_tagged_add_tag (tagged, list->data);
    }
}

/**
 * ligma_tagged_get_tags:
 * @tagged: an object that implements the %LigmaTagged interface
 *
 * Returns the list of tags assigned to this object. The returned %GList
 * is owned by the @tagged object and must not be modified or destroyed.
 *
 * Returns: a list of tags
 **/
GList *
ligma_tagged_get_tags (LigmaTagged *tagged)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED (tagged), NULL);

  return LIGMA_TAGGED_GET_IFACE (tagged)->get_tags (tagged);
}

/**
 * ligma_tagged_get_identifier:
 * @tagged: an object that implements the %LigmaTagged interface
 *
 * Returns an identifier string which uniquely identifies the tagged
 * object. Two different objects must have unique identifiers but may
 * have the same checksum (which will be the case if one object is a
 * copy of the other). The identifier must be the same across
 * sessions, so for example an instance pointer cannot be used as an
 * identifier.
 *
 * Returns: a newly allocated string containing unique identifier
 * of the object. It must be freed using #g_free.
 **/
gchar *
ligma_tagged_get_identifier (LigmaTagged *tagged)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED (tagged), NULL);

  return LIGMA_TAGGED_GET_IFACE (tagged)->get_identifier (tagged);
}

/**
 * ligma_tagged_get_checksum:
 * @tagged: an object that implements the %LigmaTagged interface
 *
 * Returns the checksum of the @tagged object. It is used to remap the
 * tags for an object for which the identifier has changed, for
 * example if the user has renamed a data file since the last session.
 *
 * If the object does not want to support such remapping (objects not
 * stored in file for example) it can return %NULL.
 *
 * Returns: (nullable): checksum string if object needs identifier remapping,
 * %NULL otherwise. Returned string must be freed with #g_free().
 **/
gchar *
ligma_tagged_get_checksum (LigmaTagged *tagged)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED (tagged), FALSE);

  return LIGMA_TAGGED_GET_IFACE (tagged)->get_checksum (tagged);
}

/**
 * ligma_tagged_has_tag:
 * @tagged: an object that implements the %LigmaTagged interface
 * @tag:    a %LigmaTag
 *
 * Returns: %TRUE if the object has @tag, %FALSE otherwise.
 **/
gboolean
ligma_tagged_has_tag (LigmaTagged *tagged,
                     LigmaTag    *tag)
{
  GList *tag_iter;

  g_return_val_if_fail (LIGMA_IS_TAGGED (tagged), FALSE);
  g_return_val_if_fail (LIGMA_IS_TAG (tag), FALSE);

  for (tag_iter = ligma_tagged_get_tags (tagged);
       tag_iter;
       tag_iter = g_list_next (tag_iter))
    {
      if (ligma_tag_equals (tag_iter->data, tag))
        return TRUE;
    }

  return FALSE;
}
