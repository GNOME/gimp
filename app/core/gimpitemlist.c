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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitemlist.h"
#include "gimplayer.h"
#include "gimpmarshal.h"

#include "vectors/gimppath.h"

#include "gimp-intl.h"


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


typedef struct _GimpItemListPrivate GimpItemListPrivate;

struct _GimpItemListPrivate
{
  GimpImage        *image;

  gboolean          is_pattern;    /* Whether a named fixed set or a pattern-search. */
  GimpSelectMethod  select_method; /* Pattern format if is_pattern is TRUE           */

  GList            *items;         /* Fixed item list if is_pattern is TRUE.         */
  GList            *deleted_items; /* Removed item list kept for undoes.             */
  GType             item_type;
};


/*  local function prototypes  */

static void       gimp_item_list_constructed         (GObject        *object);
static void       gimp_item_list_dispose             (GObject        *object);
static void       gimp_item_list_finalize            (GObject        *object);
static void       gimp_item_list_set_property        (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void       gimp_item_list_get_property        (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);

static void       gimp_item_list_item_add            (GimpContainer  *container,
                                                      GimpObject     *object,
                                                      GimpItemList   *set);
static void       gimp_item_list_item_remove         (GimpContainer  *container,
                                                      GimpObject     *object,
                                                      GimpItemList   *set);

static GList *    gimp_item_list_get_items_by_substr (GimpItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static GList *    gimp_item_list_get_items_by_glob   (GimpItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static GList *    gimp_item_list_get_items_by_regexp (GimpItemList   *set,
                                                      const gchar    *pattern,
                                                      GError        **error);
static void       gimp_item_list_clean_deleted_items (GimpItemList   *set,
                                                      GimpItem       *searched,
                                                      gboolean       *found);
static void       gimp_item_list_free_deleted_item   (GWeakRef       *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpItemList, gimp_item_list, GIMP_TYPE_OBJECT)

#define parent_class gimp_item_list_parent_class

static guint       gimp_item_list_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *gimp_item_list_props[N_PROPS]       = { NULL, };

static void
gimp_item_list_class_init (GimpItemListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * GimpItemList::empty:
   *
   * Sent when the item set changed and would return an empty set of
   * items.
   */
  gimp_item_list_signals[EMPTY] =
    g_signal_new ("empty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemListClass, empty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed       = gimp_item_list_constructed;
  object_class->dispose           = gimp_item_list_dispose;
  object_class->finalize          = gimp_item_list_finalize;
  object_class->set_property      = gimp_item_list_set_property;
  object_class->get_property      = gimp_item_list_get_property;

  gimp_item_list_props[PROP_IMAGE]         = g_param_spec_object ("image", NULL, NULL,
                                                                 GIMP_TYPE_IMAGE,
                                                                 GIMP_PARAM_READWRITE |
                                                                 G_PARAM_CONSTRUCT_ONLY);
  gimp_item_list_props[PROP_IS_PATTERN]    = g_param_spec_boolean ("is-pattern", NULL, NULL,
                                                                   FALSE,
                                                                   GIMP_PARAM_READWRITE |
                                                                   G_PARAM_CONSTRUCT_ONLY);
  gimp_item_list_props[PROP_SELECT_METHOD] = g_param_spec_enum ("select-method", NULL, NULL,
                                                                GIMP_TYPE_SELECT_METHOD,
                                                                GIMP_SELECT_PLAIN_TEXT,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT_ONLY);
  gimp_item_list_props[PROP_ITEMS]         = g_param_spec_pointer ("items",
                                                                   NULL, NULL,
                                                                   GIMP_PARAM_READWRITE |
                                                                   G_PARAM_CONSTRUCT_ONLY);
  gimp_item_list_props[PROP_ITEM_TYPE]     = g_param_spec_gtype ("item-type",
                                                                 NULL, NULL,
                                                                 G_TYPE_NONE,
                                                                 GIMP_PARAM_READWRITE |
                                                                 G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, gimp_item_list_props);
}

static void
gimp_item_list_init (GimpItemList *set)
{
  set->p = gimp_item_list_get_instance_private (set);

  set->p->items         = NULL;
  set->p->select_method = GIMP_SELECT_PLAIN_TEXT;
  set->p->is_pattern    = FALSE;
}

static void
gimp_item_list_constructed (GObject *object)
{
  GimpItemList *set = GIMP_ITEM_LIST (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_IMAGE (set->p->image));
  gimp_assert (set->p->item_type == GIMP_TYPE_LAYER   ||
               set->p->item_type == GIMP_TYPE_PATH ||
               set->p->item_type == GIMP_TYPE_CHANNEL);

  if (! set->p->is_pattern)
    {
      GimpContainer *container;

      if (set->p->item_type == GIMP_TYPE_LAYER)
        container = gimp_image_get_layers (set->p->image);
      else if (set->p->item_type == GIMP_TYPE_PATH)
        container = gimp_image_get_paths (set->p->image);
      else
        container = gimp_image_get_channels (set->p->image);
      g_signal_connect (container, "remove",
                        G_CALLBACK (gimp_item_list_item_remove),
                        set);
      g_signal_connect (container, "add",
                        G_CALLBACK (gimp_item_list_item_add),
                        set);
    }
}

static void
gimp_item_list_dispose (GObject *object)
{
  GimpItemList *set = GIMP_ITEM_LIST (object);

  if (! set->p->is_pattern)
    {
      GimpContainer *container;

      if (set->p->item_type == GIMP_TYPE_LAYER)
        container = gimp_image_get_layers (set->p->image);
      else if (set->p->item_type == GIMP_TYPE_PATH)
        container = gimp_image_get_paths (set->p->image);
      else
        container = gimp_image_get_channels (set->p->image);
      g_signal_handlers_disconnect_by_func (container,
                                            G_CALLBACK (gimp_item_list_item_remove),
                                            set);
      g_signal_handlers_disconnect_by_func (container,
                                            G_CALLBACK (gimp_item_list_item_add),
                                            set);
    }
}

static void
gimp_item_list_finalize (GObject *object)
{
  GimpItemList *set = GIMP_ITEM_LIST (object);

  g_list_free (set->p->items);
  g_list_free_full (set->p->deleted_items,
                    (GDestroyNotify) gimp_item_list_free_deleted_item);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_list_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpItemList *set = GIMP_ITEM_LIST (object);

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
gimp_item_list_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpItemList *set = GIMP_ITEM_LIST (object);

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
 * gimp_item_list_named_new:
 * @image:     The new item_list's #GimpImage.
 * @item_type: The type of #GimpItem in the list.
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
 * Returns: The newly created #GimpItemList of %NULL if it corresponds
 *          to no items.
 */
GimpItemList *
gimp_item_list_named_new (GimpImage   *image,
                          GType        item_type,
                          const gchar *name,
                          GList       *items)
{
  GimpItemList *set;
  GList        *iter;

  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (iter = items; iter; iter = iter->next)
    g_return_val_if_fail (g_type_is_a (G_OBJECT_TYPE (iter->data), item_type), NULL);

  if (! items)
    {
      if (item_type == GIMP_TYPE_LAYER)
        items = gimp_image_get_selected_layers (image);
      else if (item_type == GIMP_TYPE_PATH)
        items = gimp_image_get_selected_paths (image);
      else if (item_type == GIMP_TYPE_CHANNEL)
        items = gimp_image_get_selected_channels (image);

      if (! items)
        return NULL;
    }

  set = g_object_new (GIMP_TYPE_ITEM_LIST,
                      "image",      image,
                      "name",       name,
                      "is-pattern", FALSE,
                      "item-type",  item_type,
                      "items",      items,
                      NULL);

  return set;
}

/**
 * gimp_item_list_pattern_new:
 * @image:     The new item_list's #GimpImage.
 * @item_type: The type of #GimpItem in the list.
 * @pattern_syntax: type of patterns we are handling.
 * @pattern:   The pattern generating the contents of the list.
 *
 * Create a list of items generated from a pattern. It cannot be edited.
 *
 * Returns: The newly created #GimpItemList.
 */
GimpItemList *
gimp_item_list_pattern_new (GimpImage        *image,
                            GType             item_type,
                            GimpSelectMethod  pattern_syntax,
                            const gchar      *pattern)
{
  GimpItemList *set;

  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  /* TODO: check pattern first and fail if invalid. */
  set = g_object_new (GIMP_TYPE_ITEM_LIST,
                      "image",          image,
                      "name",           pattern,
                      "is-pattern",     TRUE,
                      "select-method",  pattern_syntax,
                      "item-type",      item_type,
                      NULL);

  return set;
}

GType
gimp_item_list_get_item_type (GimpItemList *set)
{
  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), FALSE);

  return set->p->item_type;
}

/**
 * gimp_item_list_get_items:
 * @set:
 *
 * Returns: (transfer container): The unordered list of items
 *          represented by @set to be freed with g_list_free().
 */
GList *
gimp_item_list_get_items (GimpItemList  *set,
                          GError       **error)
{
  GList *items = NULL;

  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (set->p->is_pattern)
    {
      switch (set->p->select_method)
        {
        case GIMP_SELECT_PLAIN_TEXT:
          items = gimp_item_list_get_items_by_substr (set,
                                                      gimp_object_get_name (set),
                                                      error);
          break;
        case GIMP_SELECT_GLOB_PATTERN:
          items = gimp_item_list_get_items_by_glob (set,
                                                    gimp_object_get_name (set),
                                                    error);
          break;
        case GIMP_SELECT_REGEX_PATTERN:
          items = gimp_item_list_get_items_by_regexp (set,
                                                      gimp_object_get_name (set),
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
 * gimp_item_list_is_pattern:
 * @set:            The #GimpItemList.
 * @pattern_syntax: The type of patterns @set handles.
 *
 * Indicate if @set is a pattern list. If the returned value is %TRUE,
 * then @pattern_syntax will be set to the syntax we are dealing with.
 *
 * Returns: %TRUE if @set is a pattern list, %FALSE if it is a named
 *          list.
 */
gboolean
gimp_item_list_is_pattern (GimpItemList      *set,
                           GimpSelectMethod  *pattern_syntax)
{
  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), FALSE);

  if (set->p->is_pattern && pattern_syntax)
    *pattern_syntax = set->p->select_method;

  return (set->p->is_pattern);
}

/**
 * gimp_item_list_is_pattern:
 * @set:  The #GimpItemList.
 * @item: #GimpItem to add to @set.
 *
 * Add @item to the named list @set whose item type must also agree.
 */
void
gimp_item_list_add (GimpItemList *set,
                    GimpItem     *item)
{
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));
  g_return_if_fail (! gimp_item_list_is_pattern (set, NULL));
  g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (item), set->p->item_type));

  set->p->items = g_list_prepend (set->p->items, item);
}


/*  Private functions  */


static void
gimp_item_list_item_add (GimpContainer  *container,
                         GimpObject     *object,
                         GimpItemList   *set)
{
  gboolean found = FALSE;

  gimp_item_list_clean_deleted_items (set, GIMP_ITEM (object), &found);

  if (found)
    {
      /* Such an item can only have been added back as part of an redo
       * step.
       */
      set->p->items = g_list_prepend (set->p->items, object);
    }
}

static void
gimp_item_list_item_remove (GimpContainer  *container,
                            GimpObject     *object,
                            GimpItemList   *set)
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
 * @gimp_item_list_get_items_by_substr:
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
gimp_item_list_get_items_by_substr (GimpItemList  *set,
                                    const gchar   *pattern,
                                    GError       **error)
{
  GList  *items;
  GList  *match = NULL;
  GList  *iter;

  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (pattern == NULL)
    return NULL;

  if (set->p->item_type == GIMP_TYPE_LAYER)
    {
      items = gimp_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of GimpLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      if (g_str_match_string (pattern,
                              gimp_object_get_name (iter->data),
                              TRUE))
        match = g_list_prepend (match, iter->data);
    }

  return match;
}

/*
 * @gimp_item_list_get_items_by_glob:
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
gimp_item_list_get_items_by_glob (GimpItemList  *set,
                                  const gchar   *pattern,
                                  GError       **error)
{
  GList        *items;
  GList        *match = NULL;
  GList        *iter;
  GPatternSpec *spec;

  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (pattern == NULL)
    return NULL;

  if (set->p->item_type == GIMP_TYPE_LAYER)
    {
      items = gimp_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of GimpLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  spec = g_pattern_spec_new (pattern);
  for (iter = items; iter; iter = iter->next)
    {
      if (g_pattern_spec_match_string (spec, gimp_object_get_name (iter->data)))
        match = g_list_prepend (match, iter->data);
    }
  g_pattern_spec_free (spec);

  return match;
}

/*
 * @gimp_item_list_get_items_by_regexp:
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
gimp_item_list_get_items_by_regexp (GimpItemList  *set,
                                    const gchar   *pattern,
                                    GError       **error)
{
  GList  *items;
  GList  *match = NULL;
  GList  *iter;
  GRegex *regex;

  g_return_val_if_fail (GIMP_IS_ITEM_LIST (set), FALSE);
  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  regex = g_regex_new (pattern, 0, 0, error);

  if (regex == NULL)
    return NULL;

  if (set->p->item_type == GIMP_TYPE_LAYER)
    {
      items = gimp_image_get_layer_list (set->p->image);
    }
  else
    {
      g_critical ("%s: only list of GimpLayer supported for now.",
                  G_STRFUNC);
      return NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      if (g_regex_match (regex,
                         gimp_object_get_name (iter->data),
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
gimp_item_list_clean_deleted_items (GimpItemList *set,
                                    GimpItem     *searched,
                                    gboolean     *found)
{
  GList *iter;

  g_return_if_fail (GIMP_IS_ITEM_LIST (set));
  g_return_if_fail (! searched || (found && *found == FALSE));

  for (iter = set->p->deleted_items; iter; iter = iter->next)
    {
      GimpItem *item = g_weak_ref_get (iter->data);

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
    gimp_item_list_clean_deleted_items (set,
                                        (found && *found) ? NULL : searched,
                                        found);
}

static void
gimp_item_list_free_deleted_item (GWeakRef *item)
{
  g_weak_ref_clear (item);

  g_slice_free (GWeakRef, item);
}
