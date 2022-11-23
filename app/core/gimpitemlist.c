/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmaimage.h"
#include "ligmaitem.h"
#include "ligmaitemlist.h"
#include "ligmalayer.h"
#include "ligmamarshal.h"

#include "vectors/ligmavectors.h"

#include "ligma-intl.h"


enum
{
  EMPTY,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_IS_PATTERN,
  PROP_SELECT_METHOD,
  PROP_ITEMS,
  PROP_ITEM_TYPE,
  N_PROPS
};


typedef struct _LigmaItemListPrivate LigmaItemListPrivate;

struct _LigmaItemListPrivate
{
  LigmaImage        *image;

  gboolean          is_pattern;    /* Whether a named fixed set or a pattern-search. */
  LigmaSelectMethod  select_method; /* Pattern format if is_pattern is TRUE           */

  GList            *items;         /* Fixed item list if is_pattern is TRUE.         */
  GList            *deleted_items; /* Removed item list kept for undoes.             */
  GType             item_type;
};


/*  local function prototypes  */

static void       ligma_item_list_constructed         (GObject        *object);
static void       ligma_item_list_dispose             (GObject        *object);
static void       ligma_item_list_finalize            (GObject        *object);
static void       ligma_item_list_set_property        (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void       ligma_item_list_get_property        (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);

static void       ligma_item_list_item_add            (LigmaContainer  *container,
                                                      LigmaObject     *object,
                                                      LigmaItemList   *set);
static void       ligma_item_list_item_remove         (LigmaContainer  *container,
                                                      LigmaObject     *object,
                                                      LigmaItemList   *set);

static GList *    ligma_item_list_get_items_by_substr (LigmaItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static GList *    ligma_item_list_get_items_by_glob   (LigmaItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static GList *    ligma_item_list_get_items_by_regexp (LigmaItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static void       ligma_item_list_clean_deleted_items (LigmaItemList   *set,
                                                      LigmaItem       *searched,
                                                      gboolean       *found);
static void       ligma_item_list_free_deleted_item   (GWeakRef       *item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaItemList, ligma_item_list, LIGMA_TYPE_OBJECT)

#define parent_class ligma_item_list_parent_class

static guint       ligma_item_list_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *ligma_item_list_props[N_PROPS]       = { NULL, };

static void
ligma_item_list_class_init (LigmaItemListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * LigmaItemList::empty:
   *
   * Sent when the item set changed and would return an empty set of
   * items.
   */
  ligma_item_list_signals[EMPTY] =
    g_signal_new ("empty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemListClass, empty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed       = ligma_item_list_constructed;
  object_class->dispose           = ligma_item_list_dispose;
  object_class->finalize          = ligma_item_list_finalize;
  object_class->set_property      = ligma_item_list_set_property;
  object_class->get_property      = ligma_item_list_get_property;

  ligma_item_list_props[PROP_IMAGE]         = g_param_spec_object ("image", NULL, NULL,
                                                                 LIGMA_TYPE_IMAGE,
                                                                 LIGMA_PARAM_READWRITE |
                                                                 G_PARAM_CONSTRUCT_ONLY);
  ligma_item_list_props[PROP_IS_PATTERN]    = g_param_spec_boolean ("is-pattern", NULL, NULL,
                                                                   FALSE,
                                                                   LIGMA_PARAM_READWRITE |
                                                                   G_PARAM_CONSTRUCT_ONLY);
  ligma_item_list_props[PROP_SELECT_METHOD] = g_param_spec_enum ("select-method", NULL, NULL,
                                                                LIGMA_TYPE_SELECT_METHOD,
                                                                LIGMA_SELECT_PLAIN_TEXT,
                                                                LIGMA_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT_ONLY);
  ligma_item_list_props[PROP_ITEMS]         = g_param_spec_pointer ("items",
                                                                   NULL, NULL,
                                                                   LIGMA_PARAM_READWRITE |
                                                                   G_PARAM_CONSTRUCT_ONLY);
  ligma_item_list_props[PROP_ITEM_TYPE]     = g_param_spec_gtype ("item-type",
                                                                 NULL, NULL,
                                                                 G_TYPE_NONE,
                                                                 LIGMA_PARAM_READWRITE |
                                                                 G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, ligma_item_list_props);
}

static void
ligma_item_list_init (LigmaItemList *set)
{
  set->p = ligma_item_list_get_instance_private (set);

  set->p->items         = NULL;
  set->p->select_method = LIGMA_SELECT_PLAIN_TEXT;
  set->p->is_pattern    = FALSE;
}

static void
ligma_item_list_constructed (GObject *object)
{
  LigmaItemList *set = LIGMA_ITEM_LIST (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_IMAGE (set->p->image));
  ligma_assert (set->p->item_type == LIGMA_TYPE_LAYER   ||
               set->p->item_type == LIGMA_TYPE_VECTORS ||
               set->p->item_type == LIGMA_TYPE_CHANNEL);

  if (! set->p->is_pattern)
    {
      LigmaContainer *container;

      if (set->p->item_type == LIGMA_TYPE_LAYER)
        container = ligma_image_get_layers (set->p->image);
      else if (set->p->item_type == LIGMA_TYPE_VECTORS)
        container = ligma_image_get_vectors (set->p->image);
      else
        container = ligma_image_get_channels (set->p->image);
      g_signal_connect (container, "remove",
                        G_CALLBACK (ligma_item_list_item_remove),
                        set);
      g_signal_connect (container, "add",
                        G_CALLBACK (ligma_item_list_item_add),
                        set);
    }
}

static void
ligma_item_list_dispose (GObject *object)
{
  LigmaItemList *set = LIGMA_ITEM_LIST (object);

  if (! set->p->is_pattern)
    {
      LigmaContainer *container;

      if (set->p->item_type == LIGMA_TYPE_LAYER)
        container = ligma_image_get_layers (set->p->image);
      else if (set->p->item_type == LIGMA_TYPE_VECTORS)
        container = ligma_image_get_vectors (set->p->image);
      else
        container = ligma_image_get_channels (set->p->image);
      g_signal_handlers_disconnect_by_func (container,
                                            G_CALLBACK (ligma_item_list_item_remove),
                                            set);
      g_signal_handlers_disconnect_by_func (container,
                                            G_CALLBACK (ligma_item_list_item_add),
                                            set);
    }
}

static void
ligma_item_list_finalize (GObject *object)
{
  LigmaItemList *set = LIGMA_ITEM_LIST (object);

  g_list_free (set->p->items);
  g_list_free_full (set->p->deleted_items,
                    (GDestroyNotify) ligma_item_list_free_deleted_item);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_item_list_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaItemList *set = LIGMA_ITEM_LIST (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      set->p->image = g_value_get_object (value);
      break;
    case PROP_IS_PATTERN:
      set->p->is_pattern = g_value_get_boolean (value);
      break;
    case PROP_SELECT_METHOD:
      set->p->select_method = g_value_get_enum (value);
      break;
    case PROP_ITEMS:
      set->p->items = g_list_copy (g_value_get_pointer (value));
      break;
    case PROP_ITEM_TYPE:
      set->p->item_type = g_value_get_gtype (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_item_list_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaItemList *set = LIGMA_ITEM_LIST (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, set->p->image);
      break;
    case PROP_IS_PATTERN:
      g_value_set_boolean (value, set->p->is_pattern);
      break;
    case PROP_SELECT_METHOD:
      g_value_set_enum (value, set->p->select_method);
      break;
    case PROP_ITEMS:
      g_value_set_pointer (value, set->p->items);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, set->p->item_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  Public functions  */


/**
 * ligma_item_list_named_new:
 * @image:     The new item_list's #LigmaImage.
 * @item_type: The type of #LigmaItem in the list.
 * @name:      The name to assign the item list.
 * @items:     The items in the list.
 *
 * Create a fixed list of items made of items. It cannot be edited and
 * will only auto-update when items get deleted from @image, until the
 * list reaches 0 (in which case, the list will self-destroy).
 *
 * If @items is %NULL, the current item selection of type @item_type in
 * @image is used. If this selection is empty, then %NULL is returned.
 *
 * Returns: The newly created #LigmaItemList of %NULL if it corresponds
 *          to no items.
 */
LigmaItemList *
ligma_item_list_named_new (LigmaImage   *image,
                          GType        item_type,
                          const gchar *name,
                          GList       *items)
{
  LigmaItemList *set;
  GList        *iter;

  g_return_val_if_fail (g_type_is_a (item_type, LIGMA_TYPE_ITEM), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  for (iter = items; iter; iter = iter->next)
    g_return_val_if_fail (g_type_is_a (G_OBJECT_TYPE (iter->data), item_type), NULL);

  if (! items)
    {
      if (item_type == LIGMA_TYPE_LAYER)
        items = ligma_image_get_selected_layers (image);
      else if (item_type == LIGMA_TYPE_VECTORS)
        items = ligma_image_get_selected_vectors (image);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        items = ligma_image_get_selected_channels (image);

      if (! items)
        return NULL;
    }

  set = g_object_new (LIGMA_TYPE_ITEM_LIST,
                      "image",      image,
                      "name",       name,
                      "is-pattern", FALSE,
                      "item-type",  item_type,
                      "items",      items,
                      NULL);

  return set;
}

/**
 * ligma_item_list_pattern_new:
 * @image:     The new item_list's #LigmaImage.
 * @item_type: The type of #LigmaItem in the list.
 * @pattern_syntax: type of patterns we are handling.
 * @pattern:   The pattern generating the contents of the list.
 *
 * Create a list of items generated from a pattern. It cannot be edited.
 *
 * Returns: The newly created #LigmaItemList.
 */
LigmaItemList *
ligma_item_list_pattern_new (LigmaImage        *image,
                            GType             item_type,
                            LigmaSelectMethod  pattern_syntax,
                            const gchar      *pattern)
{
  LigmaItemList *set;

  g_return_val_if_fail (g_type_is_a (item_type, LIGMA_TYPE_ITEM), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  /* TODO: check pattern first and fail if invalid. */
  set = g_object_new (LIGMA_TYPE_ITEM_LIST,
                      "image",          image,
                      "name",           pattern,
                      "is-pattern",     TRUE,
                      "select-method",  pattern_syntax,
                      "item-type",      item_type,
                      NULL);

  return set;
}

GType
ligma_item_list_get_item_type (LigmaItemList *set)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), FALSE);

  return set->p->item_type;
}

/**
 * ligma_item_list_get_items:
 * @set:
 *
 * Returns: (transfer container): The unordered list of items
 *          represented by @set to be freed with g_list_free().
 */
GList *
ligma_item_list_get_items (LigmaItemList  *set,
                          GError       **error)
{
  GList *items = NULL;

  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (set->p->is_pattern)
    {
      switch (set->p->select_method)
        {
        case LIGMA_SELECT_PLAIN_TEXT:
          items = ligma_item_list_get_items_by_substr (set,
                                                      ligma_object_get_name (set),
                                                      error);
          break;
        case LIGMA_SELECT_GLOB_PATTERN:
          items = ligma_item_list_get_items_by_glob (set,
                                                    ligma_object_get_name (set),
                                                    error);
          break;
        case LIGMA_SELECT_REGEX_PATTERN:
          items = ligma_item_list_get_items_by_regexp (set,
                                                      ligma_object_get_name (set),
                                                      error);
          break;
        }
    }
  else
    {
      items = g_list_copy (set->p->items);
    }

  return items;
}

/**
 * ligma_item_list_is_pattern:
 * @set:            The #LigmaItemList.
 * @pattern_syntax: The type of patterns @set handles.
 *
 * Indicate if @set is a pattern list. If the returned value is %TRUE,
 * then @pattern_syntax will be set to the syntax we are dealing with.
 *
 * Returns: %TRUE if @set is a pattern list, %FALSE if it is a named
 *          list.
 */
gboolean
ligma_item_list_is_pattern (LigmaItemList      *set,
                           LigmaSelectMethod  *pattern_syntax)
{
  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), FALSE);

  if (set->p->is_pattern && pattern_syntax)
    *pattern_syntax = set->p->select_method;

  return (set->p->is_pattern);
}

/**
 * ligma_item_list_is_pattern:
 * @set:  The #LigmaItemList.
 * @item: #LigmaItem to add to @set.
 *
 * Add @item to the named list @set whose item type must also agree.
 */
void
ligma_item_list_add (LigmaItemList *set,
                    LigmaItem     *item)
{
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));
  g_return_if_fail (! ligma_item_list_is_pattern (set, NULL));
  g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (item), set->p->item_type));

  set->p->items = g_list_prepend (set->p->items, item);
}


/*  Private functions  */


static void
ligma_item_list_item_add (LigmaContainer  *container,
                         LigmaObject     *object,
                         LigmaItemList   *set)
{
  gboolean found = FALSE;

  ligma_item_list_clean_deleted_items (set, LIGMA_ITEM (object), &found);

  if (found)
    {
      /* Such an item can only have been added back as part of an redo
       * step.
       */
      set->p->items = g_list_prepend (set->p->items, object);
    }
}

static void
ligma_item_list_item_remove (LigmaContainer  *container,
                            LigmaObject     *object,
                            LigmaItemList   *set)
{
  GWeakRef *deleted_item = g_slice_new (GWeakRef);

  /* Keep a weak link on object so that it disappears by itself when no
   * other piece of code has a reference to it. In particular, we expect
   * undo to keep references to deleted items. So if a redo happens we
   * will get a "add" signal with the same object.
   */
  set->p->items = g_list_remove (set->p->items, object);

  g_weak_ref_init (deleted_item, object);
  set->p->deleted_items = g_list_prepend (set->p->deleted_items, deleted_item);
}

/*
 * @ligma_item_list_get_items_by_substr:
 * @image:
 * @pattern:
 * @error: unused #GError.
 *
 * Replace currently selected items in @image with the items whose
 * names match with the @pattern after tokenisation, case-folding and
 * normalization.
 *
 * Returns: %TRUE if some items matched @pattern (even if it turned out
 *          selected items stay the same), %FALSE otherwise.
 */
static GList *
ligma_item_list_get_items_by_substr (LigmaItemList  *set,
                                    const gchar   *pattern,
                                    GError       **error)
{
  GList  *items;
  GList  *match = NULL;
  GList  *iter;

  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (pattern == NULL)
    return NULL;

  if (set->p->item_type == LIGMA_TYPE_LAYER)
    {
      items = ligma_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of LigmaLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      if (g_str_match_string (pattern,
                              ligma_object_get_name (iter->data),
                              TRUE))
        match = g_list_prepend (match, iter->data);
    }

  return match;
}

/*
 * @ligma_item_list_get_items_by_glob:
 * @image:
 * @pattern:
 * @error: unused #GError.
 *
 * Replace currently selected items in @image with the items whose
 * names match with the @pattern glob expression.
 *
 * Returns: %TRUE if some items matched @pattern (even if it turned out
 *          selected items stay the same), %FALSE otherwise.
 */
static GList *
ligma_item_list_get_items_by_glob (LigmaItemList  *set,
                                  const gchar   *pattern,
                                  GError       **error)
{
  GList        *items;
  GList        *match = NULL;
  GList        *iter;
  GPatternSpec *spec;

  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (pattern == NULL)
    return NULL;

  if (set->p->item_type == LIGMA_TYPE_LAYER)
    {
      items = ligma_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of LigmaLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  spec = g_pattern_spec_new (pattern);
  for (iter = items; iter; iter = iter->next)
    {
      if (g_pattern_match_string (spec,
                                  ligma_object_get_name (iter->data)))
        match = g_list_prepend (match, iter->data);
    }
  g_pattern_spec_free (spec);

  return match;
}

/*
 * @ligma_item_list_get_items_by_regexp:
 * @image:
 * @pattern:
 * @error:
 *
 * Replace currently selected items in @image with the items whose
 * names match with the @pattern regular expression.
 *
 * Returns: %TRUE if some items matched @pattern (even if it turned out
 *          selected items stay the same), %FALSE otherwise or if
 *          @pattern is an invalid regular expression (in which case,
 *          @error will be filled with the appropriate error).
 */
static GList *
ligma_item_list_get_items_by_regexp (LigmaItemList  *set,
                                    const gchar   *pattern,
                                    GError       **error)
{
  GList  *items;
  GList  *match = NULL;
  GList  *iter;
  GRegex *regex;

  g_return_val_if_fail (LIGMA_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  regex = g_regex_new (pattern, 0, 0, error);

  if (regex == NULL)
    return NULL;

  if (set->p->item_type == LIGMA_TYPE_LAYER)
    {
      items = ligma_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of LigmaLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      if (g_regex_match (regex,
                         ligma_object_get_name (iter->data),
                         0, NULL))
        match = g_list_prepend (match, iter->data);
    }
  g_regex_unref (regex);

  return match;
}

/*
 * Remove all deleted items which don't have any reference left anywhere
 * (only leaving the shell of the weak reference), hence whose deletion
 * cannot be undone anyway.
 * If @searched is not %NULL, check if it belonged to the deleted item
 * list and return TRUE if so. In this case, you must call the function
 * with @found set to %FALSE initially.
 */
static void
ligma_item_list_clean_deleted_items (LigmaItemList *set,
                                    LigmaItem     *searched,
                                    gboolean     *found)
{
  GList *iter;

  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));
  g_return_if_fail (! searched || (found && *found == FALSE));

  for (iter = set->p->deleted_items; iter; iter = iter->next)
    {
      LigmaItem *item = g_weak_ref_get (iter->data);

      if (item == NULL)
        {
          set->p->deleted_items = g_list_delete_link (set->p->deleted_items,
                                                      iter);
          break;
        }
      else
        {
          if (searched && item == searched)
            {
              set->p->deleted_items = g_list_delete_link (set->p->deleted_items,
                                                          iter);
              *found = TRUE;
              g_object_unref (item);
              break;
            }
          g_object_unref (item);
        }
    }

  if (iter)
    ligma_item_list_clean_deleted_items (set,
                                        (found && *found) ? NULL : searched,
                                        found);
}

static void
ligma_item_list_free_deleted_item (GWeakRef *item)
{
  g_weak_ref_clear (item);

  g_slice_free (GWeakRef, item);
}
