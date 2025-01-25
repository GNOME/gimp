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
#include <gio/gio.h>
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


/*
 * GIMP_TYPE_PARAM_CHOICE
 */

#define GIMP_PARAM_SPEC_CHOICE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CHOICE, GimpParamSpecChoice))

typedef struct _GimpParamSpecChoice GimpParamSpecChoice;

struct _GimpParamSpecChoice
{
  GParamSpecString  parent_instance;

  GimpChoice       *choice;
};

static void       gimp_param_choice_class_init        (GParamSpecClass *klass);
static void       gimp_param_choice_init              (GParamSpec      *pspec);
static void       gimp_param_choice_finalize          (GParamSpec      *pspec);
static gboolean   gimp_param_choice_validate          (GParamSpec      *pspec,
                                                       GValue          *value);
static gboolean   gimp_param_choice_value_is_valid    (GParamSpec      *pspec,
                                                       const GValue    *value);
static gint       gimp_param_choice_values_cmp        (GParamSpec      *pspec,
                                                      const GValue    *value1,
                                                      const GValue    *value2);

GType
gimp_param_choice_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_choice_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecChoice),
        0,
        (GInstanceInitFunc) gimp_param_choice_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "GimpParamChoice", &info, 0);
    }

  return type;
}

static void
gimp_param_choice_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_STRING;
  klass->finalize       = gimp_param_choice_finalize;
  klass->value_validate = gimp_param_choice_validate;
  klass->value_is_valid = gimp_param_choice_value_is_valid;
  klass->values_cmp     = gimp_param_choice_values_cmp;
}

static void
gimp_param_choice_init (GParamSpec *pspec)
{
  GimpParamSpecChoice *choice = GIMP_PARAM_SPEC_CHOICE (pspec);

  choice->choice = NULL;
}

static void
gimp_param_choice_finalize (GParamSpec *pspec)
{
  GimpParamSpecChoice *spec_choice  = GIMP_PARAM_SPEC_CHOICE (pspec);
  GParamSpecClass     *parent_class = g_type_class_peek (g_type_parent (GIMP_TYPE_PARAM_CHOICE));

  g_object_unref (spec_choice->choice);

  parent_class->finalize (pspec);
}

static gboolean
gimp_param_choice_validate (GParamSpec *pspec,
                            GValue     *value)
{
  GimpParamSpecChoice *spec_choice = GIMP_PARAM_SPEC_CHOICE (pspec);
  GParamSpecString    *spec_string = G_PARAM_SPEC_STRING (pspec);
  GimpChoice          *choice      = spec_choice->choice;
  const gchar         *strval      = g_value_get_string (value);

  if (! gimp_choice_is_valid (choice, strval))
    {
      if (gimp_choice_is_valid (choice, spec_string->default_value))
        {
          g_value_set_string (value, spec_string->default_value);
        }
      else
        {
          /* This might happen if the default value is set insensitive. Then we
           * should just set any valid random nick.
           */
          GList *nicks;

          nicks = gimp_choice_list_nicks (choice);
          for (GList *iter = nicks; iter; iter = iter->next)
            if (gimp_choice_is_valid (choice, (gchar *) iter->data))
              {
                g_value_set_string (value, (gchar *) iter->data);
                break;
              }
        }
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_param_choice_value_is_valid (GParamSpec   *pspec,
                                  const GValue *value)
{
  GimpParamSpecChoice *cspec  = GIMP_PARAM_SPEC_CHOICE (pspec);
  const gchar         *strval = g_value_get_string (value);
  GimpChoice          *choice = cspec->choice;

  return gimp_choice_is_valid (choice, strval);
}

static gint
gimp_param_choice_values_cmp (GParamSpec   *pspec,
                              const GValue *value1,
                              const GValue *value2)
{
  const gchar *choice1 = g_value_get_string (value1);
  const gchar *choice2 = g_value_get_string (value2);

  return g_strcmp0 (choice1, choice2);
}

/**
 * gimp_param_spec_choice:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @choice: (transfer full): the %GimpChoice describing allowed choices.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecChoice specifying a
 * #G_TYPE_STRING property.
 * This %GimpParamSpecChoice takes ownership of the reference on @choice.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecChoice.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_choice (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        GimpChoice  *choice,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  GimpParamSpecChoice *choice_spec;
  GParamSpecString    *string_spec;

  g_return_val_if_fail (GIMP_IS_CHOICE (choice), NULL);
  g_return_val_if_fail (gimp_choice_is_valid (choice, default_value), NULL);

  choice_spec = g_param_spec_internal (GIMP_TYPE_PARAM_CHOICE,
                                       name, nick, blurb, flags);

  g_return_val_if_fail (choice_spec, NULL);

  string_spec = G_PARAM_SPEC_STRING (choice_spec);

  choice_spec->choice        = choice;
  string_spec->default_value = g_strdup (default_value);

  return G_PARAM_SPEC (choice_spec);
}

/**
 * gimp_param_spec_choice_get_choice:
 * @pspec: a #GParamSpec to hold a #GimpParamSpecChoice value.
 *
 * Returns: (transfer none): the choice object defining the valid values.
 *
 * Since: 3.0
 **/
GimpChoice *
gimp_param_spec_choice_get_choice (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  return GIMP_PARAM_SPEC_CHOICE (pspec)->choice;
}

/**
 * gimp_param_spec_choice_get_default:
 * @pspec: a #GParamSpec to hold a #GimpParamSpecChoice value.
 *
 * Returns: the default value.
 *
 * Since: 3.0
 **/
const gchar *
gimp_param_spec_choice_get_default (GParamSpec *pspec)
{
  const GValue *value;

  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  value = g_param_spec_get_default_value (pspec);

  return g_value_get_string (value);
}
