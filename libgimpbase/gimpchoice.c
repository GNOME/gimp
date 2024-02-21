/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpchoice.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpchoice.h"
#include "gimpparamspecs.h"


typedef struct _GimpChoiceDesc
{
  gchar    *label;
  gchar    *help;
  gint      id;
  gboolean  sensitive;
} GimpChoiceDesc;

enum
{
  SENSITIVITY_CHANGED,
  LAST_SIGNAL
};

struct _GimpChoice
{
  GObject     parent_instance;

  GHashTable *choices;
  GList      *keys;
};


static void gimp_choice_finalize     (GObject        *object);

static void gimp_choice_desc_free    (GimpChoiceDesc *desc);


G_DEFINE_TYPE (GimpChoice, gimp_choice, G_TYPE_OBJECT)

#define parent_class gimp_choice_parent_class

static guint gimp_choice_signals[LAST_SIGNAL] = { 0 };

static void
gimp_choice_class_init (GimpChoiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_choice_signals[SENSITIVITY_CHANGED] =
    g_signal_new ("sensitivity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, /*G_STRUCT_OFFSET (GimpChoiceClass, sensitivity_changed),*/
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  object_class->finalize = gimp_choice_finalize;
}

static void
gimp_choice_init (GimpChoice *choice)
{
  choice->choices = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                           (GDestroyNotify) gimp_choice_desc_free);
}

static void
gimp_choice_finalize (GObject *object)
{
  GimpChoice *choice = GIMP_CHOICE (object);

  g_hash_table_unref (choice->choices);
  g_list_free_full (choice->keys, (GDestroyNotify) g_free);
}

/* Public API */

/**
 * gimp_choice_new:
 *
 * Returns: (transfer full): a #GimpChoice.
 *
 * Since: 3.0
 **/
GimpChoice *
gimp_choice_new (void)
{
  GimpChoice *choice;

  choice = g_object_new (GIMP_TYPE_CHOICE, NULL);

  return choice;
}

/**
 * gimp_choice_new_with_values:
 * @nick:  the first value.
 * @id:    integer ID for @nick.
 * @label: the label of @nick.
 * @help:  longer help text for @nick.
 * ...:    more triplets of string to pre-fill the created %GimpChoice.
 *
 * Returns: (transfer full): a #GimpChoice.
 *
 * Since: 3.0
 **/
GimpChoice *
gimp_choice_new_with_values (const gchar *nick,
                             gint         id,
                             const gchar *label,
                             const gchar *help,
                             ...)
{
  GimpChoice *choice;
  va_list     va_args;

  g_return_val_if_fail (nick != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);

  choice = gimp_choice_new ();

  va_start (va_args, help);

  do
    {
      gimp_choice_add (choice, nick, id, label, help);
      nick  = va_arg (va_args, const gchar *);
      if (nick == NULL)
        break;
      id    = va_arg (va_args, gint);
      label = va_arg (va_args, const gchar *);
      if (label == NULL)
        {
          g_critical ("%s: nick '%s' cannot have a NULL label.", G_STRFUNC, nick);
          break;
        }
      help  = va_arg (va_args, const gchar *);
    }
  while (TRUE);

  va_end (va_args);

  return choice;
}

/**
 * gimp_choice_add:
 * @choice: the %GimpChoice.
 * @nick:   the nick of @choice.
 * @id:     optional integer ID for @nick.
 * @label:  the label of @choice.
 * @help:   optional longer help text for @nick.
 *
 * This procedure adds a new possible value to @choice list of values.
 * The @id is an optional integer identifier. This can be useful for instance
 * when you want to work with different enum values mapped to each @nick.
 *
 * Since: 3.0
 **/
void
gimp_choice_add (GimpChoice  *choice,
                 const gchar *nick,
                 gint         id,
                 const gchar *label,
                 const gchar *help)
{
  GimpChoiceDesc *desc;
  GList          *duplicate;

  g_return_if_fail (label != NULL);

  desc            = g_new0 (GimpChoiceDesc, 1);
  desc->id        = id;
  desc->label     = g_strdup (label);
  desc->help      = help != NULL ? g_strdup (help) : NULL;
  desc->sensitive = TRUE;
  g_hash_table_insert (choice->choices, g_strdup (nick), desc);

  duplicate = g_list_find_custom (choice->keys, nick, (GCompareFunc) g_strcmp0);
  if (duplicate != NULL)
    {
      choice->keys = g_list_remove_link (choice->keys, duplicate);
      gimp_choice_desc_free (duplicate->data);
      g_list_free (duplicate);
    }
  choice->keys = g_list_append (choice->keys, g_strdup (nick));
}

/**
 * gimp_choice_is_valid:
 * @choice: a %GimpChoice.
 * @nick:   the nick to check.
 *
 * This procedure checks if the given @nick is valid and refers to
 * an existing choice.
 *
 * Returns: Whether the choice is valid.
 *
 * Since: 3.0
 **/
gboolean
gimp_choice_is_valid (GimpChoice  *choice,
                      const gchar *nick)
{
  GimpChoiceDesc *desc;

  g_return_val_if_fail (GIMP_IS_CHOICE (choice), FALSE);
  g_return_val_if_fail (nick != NULL, FALSE);

  desc = g_hash_table_lookup (choice->choices, nick);
  return (desc != NULL && desc->sensitive);
}

/**
 * gimp_choice_list_nicks:
 * @choice: a %GimpChoice.
 *
 * This procedure returns the list of nicks allowed for @choice.
 *
 * Returns: (element-type gchar*) (transfer none): The list of @choice's nicks.
 *
 * Since: 3.0
 **/
GList *
gimp_choice_list_nicks (GimpChoice *choice)
{
  /* I don't use g_hash_table_get_keys() on purpose, because I want to retain
   * the adding-time order.
   */
  return choice->keys;
}

/**
 * gimp_choice_get_id:
 * @choice: a %GimpChoice.
 * @nick:   the nick to lookup.
 *
 * Returns: the ID of @nick.
 *
 * Since: 3.0
 **/
gint
gimp_choice_get_id (GimpChoice  *choice,
                    const gchar *nick)
{
  GimpChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    return desc->id;
  else
    return 0;
}

/**
 * gimp_choice_get_label:
 * @choice: a %GimpChoice.
 * @nick:   the nick to lookup.
 *
 * Returns: (transfer none): the label of @nick.
 *
 * Since: 3.0
 **/
const gchar *
gimp_choice_get_label (GimpChoice  *choice,
                       const gchar *nick)
{
  GimpChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    return desc->label;
  else
    return NULL;
}

/**
 * gimp_choice_get_help:
 * @choice: a %GimpChoice.
 * @nick:   the nick to lookup.
 *
 * Returns the longer documentation for @nick.
 *
 * Returns: (transfer none): the help text of @nick.
 *
 * Since: 3.0
 **/
const gchar *
gimp_choice_get_help (GimpChoice  *choice,
                      const gchar *nick)
{
  GimpChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    return desc->help;
  else
    return NULL;
}

/**
 * gimp_choice_get_documentation:
 * @choice: the %GimpChoice.
 * @nick:   the possible value's nick you need documentation for.
 * @label: (transfer none): the label of @nick.
 * @help:  (transfer none): the help text of @nick.
 *
 * Returns the documentation strings for @nick.
 *
 * Returns: %TRUE if @nick is found, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
gimp_choice_get_documentation (GimpChoice   *choice,
                               const gchar  *nick,
                               const gchar **label,
                               const gchar **help)
{
  GimpChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    {
      *label = desc->label;
      *help  = desc->help;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_choice_set_sensitive:
 * @choice: the %GimpChoice.
 * @nick:   the nick to lookup.
 *
 * Change the sensitivity of a possible @nick. Technically a non-sensitive @nick
 * means it cannot be chosen anymore (so [method@Gimp.Choice.is_valid] will
 * return %FALSE; nevertheless [method@Gimp.Choice.list_nicks] and other
 * functions to get information about a choice will still function).
 *
 * Since: 3.0
 **/
void
gimp_choice_set_sensitive (GimpChoice  *choice,
                           const gchar *nick,
                           gboolean     sensitive)
{
  GimpChoiceDesc *desc;

  g_return_if_fail (GIMP_IS_CHOICE (choice));
  g_return_if_fail (nick != NULL);

  desc = g_hash_table_lookup (choice->choices, nick);
  g_return_if_fail (desc != NULL);
  if (desc->sensitive != sensitive)
    {
      desc->sensitive = sensitive;
      g_signal_emit (choice, gimp_choice_signals[SENSITIVITY_CHANGED], 0, nick);
    }
}


/* Private functions */

static void
gimp_choice_desc_free (GimpChoiceDesc *desc)
{
  g_free (desc->label);
  g_free (desc->help);
  g_free (desc);
}
