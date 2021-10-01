/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-eval.c
 * Copyright (C) 2017 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Less General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* This is a simple interpreter for the GUM language (the GEGL UI Meta
 * language), used in certain property keys of GEGL operations.  What follows
 * is a hand-wavy summary of the syntax and semantics (no BNF for you!)
 *
 * There are currently two types of expressions:
 *
 * Boolean expressions
 * -------------------
 *
 * There are three types of simple boolean expressions:
 *
 *   - Literal:  Either `0` or `1`, evaluating to FALSE and TRUE, respectively.
 *
 *   - Reference:  Has the form `$key` or `$property.key`.  Evaluates to the
 *     value of key `key`, which should itself be a boolean expression.  In
 *     the first form, `key` refers to a key of the same property to which the
 *     currently-evaluated key belongs.  In the second form, `key` refers to a
 *     key of `property`.
 *
 *   - Dependency:  Dependencies begin with the name of a property, on which
 *     the result depends.  Currently supported property types are:
 *
 *       - Boolean:  The expression consists of the property name alone, and
 *         its value is the value of the property.
 *
 *       - Enum:  The property name shall be followed by a brace-enclosed,
 *         comma-separated list of enum values, given as nicks.  The expression
 *         evaluates to TRUE iff the property matches any of the values.
 *
 * Complex boolean expressions can be formed using `!` (negation), `&`
 * (conjunction), `|` (disjunction), and parentheses (grouping), following the
 * usual precedence rules.
 *
 * String expressions
 * ------------------
 *
 * There are three types of simple string expressions:
 *
 *   - Literal:  A string literal, surrounded by single quotes (`'`).  Special
 *     characters (in particular, single quotes) can be escaped using a
 *     backslash (`\`).
 *
 *   - Reference:  Same as a boolean reference, but should refer to a key
 *     containing a string expression.
 *
 *   - Deferred literal:  Names a key, in the same fashion as a reference, but
 *     without the leading `$`.  The value of this key is taken literally as
 *     the value of the expression.  Deferred literals should usually be
 *     favored over inline string literals, because they can be translated
 *     independently of the expression.
 *
 * Currently, the only complex string expression is string selection:  It has
 * the form of a bracket-enclosed, comma-separated list of expressions of the
 * form `<condition> : <value>`, where `<condition>` is a boolean expression,
 * and `<value>` is a string expression.  The result of the expression is the
 * associated value of the first condition that evaluates to TRUE.  If no
 * condition is met, the result is NULL.
 *
 *
 * Whitespace separating subexpressions is insignificant.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "propgui-types.h"

#include "gimppropgui-eval.h"


typedef enum
{
  GIMP_PROP_EVAL_FAILED  /* generic error condition */
} GimpPropEvalErrorCode;


static gboolean   gimp_prop_eval_boolean_impl     (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar    *key,
                                                   gint            default_value,
                                                   GError        **error,
                                                   gint            depth);
static gboolean   gimp_prop_eval_boolean_or       (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);
static gboolean   gimp_prop_eval_boolean_and      (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);
static gboolean   gimp_prop_eval_boolean_not      (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);
static gboolean   gimp_prop_eval_boolean_group    (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);
static gboolean   gimp_prop_eval_boolean_simple   (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);

static gchar    * gimp_prop_eval_string_impl      (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar    *key,
                                                   const gchar    *default_value,
                                                   GError        **error,
                                                   gint            depth);
static gchar    * gimp_prop_eval_string_selection (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);
static gchar    * gimp_prop_eval_string_simple    (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   gint            depth);

static gboolean   gimp_prop_eval_parse_reference  (GObject        *config,
                                                   GParamSpec     *pspec,
                                                   const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error,
                                                   GParamSpec    **ref_pspec,
                                                   gchar         **ref_key);

static gboolean   gimp_prop_eval_depth_test       (gint            depth,
                                                   GError        **error);

static gchar    * gimp_prop_eval_read_token       (const gchar   **expr,
                                                   gchar         **t,
                                                   GError        **error);
static gboolean   gimp_prop_eval_is_name          (const gchar    *token);

#define GIMP_PROP_EVAL_ERROR (gimp_prop_eval_error_quark ())

static GQuark     gimp_prop_eval_error_quark      (void);


/*  public functions  */

gboolean
gimp_prop_eval_boolean (GObject     *config,
                        GParamSpec  *pspec,
                        const gchar *key,
                        gboolean     default_value)
{
  GError   *error = NULL;
  gboolean  result;

  result = gimp_prop_eval_boolean_impl (config, pspec,
                                        key, default_value, &error, 0);

  if (error)
    {
      g_warning ("in object of type '%s': %s",
                 G_OBJECT_TYPE_NAME (config),
                 error->message);

      g_error_free (error);

      return default_value;
    }

  return result;
}

gchar *
gimp_prop_eval_string (GObject     *config,
                       GParamSpec  *pspec,
                       const gchar *key,
                       const gchar *default_value)
{
  GError *error = NULL;
  gchar  *result;

  result = gimp_prop_eval_string_impl (config, pspec,
                                       key, default_value, &error, 0);

  if (error)
    {
      g_warning ("in object of type '%s': %s",
                 G_OBJECT_TYPE_NAME (config),
                 error->message);

      g_error_free (error);

      return g_strdup (default_value);
    }

  return result;
}


/*  private functions  */

static gboolean
gimp_prop_eval_boolean_impl (GObject      *config,
                             GParamSpec   *pspec,
                             const gchar  *key,
                             gint          default_value,
                             GError      **error,
                             gint          depth)
{
  const gchar *expr;
  gchar       *t      = NULL;
  gboolean     result = FALSE;

  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  expr = gegl_param_spec_get_property_key (pspec, key);

  if (! expr)
    {
      /* we use `default_value < 0` to specify that the key must exist */
      if (default_value < 0)
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "key '%s' of property '%s' not found",
                       key,
                       g_param_spec_get_name (pspec));

          return FALSE;
        }

      return default_value;
    }

  gimp_prop_eval_read_token (&expr, &t, error);

  if (! *error)
    {
      result = gimp_prop_eval_boolean_or (config, pspec,
                                          &expr, &t, error, depth);
    }

  /* check for trailing tokens at the end of the expression */
  if (! *error && t)
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid expression");
    }

  g_free (t);

  if (*error)
    {
      g_prefix_error (error,
                      "in key '%s' of property '%s': ",
                      key,
                      g_param_spec_get_name (pspec));

      return FALSE;
    }

  return result;
}

static gboolean
gimp_prop_eval_boolean_or (GObject      *config,
                           GParamSpec   *pspec,
                           const gchar **expr,
                           gchar       **t,
                           GError      **error,
                           gint          depth)
{
  gboolean result;

  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  result = gimp_prop_eval_boolean_and (config, pspec,
                                       expr, t, error, depth);

  while (! *error && ! g_strcmp0 (*t, "|"))
    {
      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return FALSE;

      /* keep evaluating even if `result` is TRUE, because we still need to
       * parse the rest of the subexpression.
       */
      result |= gimp_prop_eval_boolean_and (config, pspec,
                                            expr, t, error, depth);
    }

  return result;
}

static gboolean
gimp_prop_eval_boolean_and (GObject      *config,
                            GParamSpec   *pspec,
                            const gchar **expr,
                            gchar       **t,
                            GError      **error,
                            gint          depth)
{
  gboolean result;

  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  result = gimp_prop_eval_boolean_not (config, pspec,
                                       expr, t, error, depth);

  while (! *error && ! g_strcmp0 (*t, "&"))
    {
      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return FALSE;

      /* keep evaluating even if `result` is FALSE, because we still need to
       * parse the rest of the subexpression.
       */
      result &= gimp_prop_eval_boolean_not (config, pspec,
                                            expr, t, error, depth);
    }

  return result;
}

static gboolean
gimp_prop_eval_boolean_not (GObject      *config,
                            GParamSpec   *pspec,
                            const gchar **expr,
                            gchar       **t,
                            GError      **error,
                            gint          depth)
{
  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  if (! g_strcmp0 (*t, "!"))
    {
      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return FALSE;

      return ! gimp_prop_eval_boolean_not (config, pspec,
                                           expr, t, error, depth + 1);
    }

  return gimp_prop_eval_boolean_group (config, pspec,
                                       expr, t, error, depth);
}

static gboolean
gimp_prop_eval_boolean_group (GObject      *config,
                              GParamSpec   *pspec,
                              const gchar **expr,
                              gchar       **t,
                              GError      **error,
                              gint          depth)
{
  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  if (! g_strcmp0 (*t, "("))
    {
      gboolean result;

      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return FALSE;

      result = gimp_prop_eval_boolean_or (config, pspec,
                                          expr, t, error, depth + 1);

      if (*error)
        return FALSE;

      if (g_strcmp0 (*t, ")"))
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "unterminated group");

          return FALSE;
        }

      gimp_prop_eval_read_token (expr, t, error);

      return result;
    }

  return gimp_prop_eval_boolean_simple (config, pspec,
                                        expr, t, error, depth);
}

static gboolean
gimp_prop_eval_boolean_simple (GObject      *config,
                               GParamSpec   *pspec,
                               const gchar **expr,
                               gchar       **t,
                               GError      **error,
                               gint          depth)
{
  gboolean result;

  if (! gimp_prop_eval_depth_test (depth, error))
    return FALSE;

  if (! *t)
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid expression");

      return FALSE;
    }

  /* literal */
  if (! strcmp (*t, "0"))
    {
      result = FALSE;

      gimp_prop_eval_read_token (expr, t, error);
    }
  else if (! strcmp (*t, "1"))
    {
      result = TRUE;

      gimp_prop_eval_read_token (expr, t, error);
    }
  /* reference */
  else if (! strcmp (*t, "$"))
    {
      gchar *key;

      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return FALSE;

      if (! gimp_prop_eval_parse_reference (config, pspec,
                                            expr, t, error, &pspec, &key))
        return FALSE;

      result = gimp_prop_eval_boolean_impl (config, pspec,
                                            key, -1, error, depth + 1);

      g_free (key);
    }
  /* dependency */
  else if (gimp_prop_eval_is_name (*t))
    {
      const gchar *property_name;
      GParamSpec  *pspec;
      GType        type;

      property_name = *t;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                            property_name);

      if (! pspec)
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "property '%s' not found",
                       property_name);

          return TRUE;
        }

      property_name = g_param_spec_get_name (pspec);
      type          = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (g_type_is_a (type, G_TYPE_BOOLEAN))
        {
          g_object_get (config, property_name, &result, NULL);
        }
      else if (g_type_is_a (type, G_TYPE_ENUM))
        {
          GEnumClass *enum_class;
          gint        value;

          gimp_prop_eval_read_token (expr, t, error);

          if (*error)
            return FALSE;

          if (g_strcmp0 (*t , "{"))
            {
              g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                           "missing enum value set "
                           "for property '%s'",
                           property_name);

              return FALSE;
            }

          enum_class = g_type_class_peek (type);

          g_object_get (config, property_name, &value, NULL);

          result = FALSE;

          while (gimp_prop_eval_read_token (expr, t, error) &&
                 gimp_prop_eval_is_name (*t))
            {
              const gchar *nick;
              GEnumValue  *enum_value;

              nick       = *t;
              enum_value = g_enum_get_value_by_nick (enum_class, nick);

              if (! enum_value)
                {
                  g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                               "invalid enum value '%s' "
                               "for property '%s'",
                               nick,
                               property_name);

                  return FALSE;
                }

              if (value == enum_value->value)
                result = TRUE;

              gimp_prop_eval_read_token (expr, t, error);

              if (*error)
                return FALSE;

              if (! g_strcmp0 (*t, ","))
                {
                  continue;
                }
              else if (! g_strcmp0 (*t, "}"))
                {
                  break;
                }
              else
                {
                  g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                               "invalid enum value set "
                               "for property '%s'",
                               property_name);

                  return FALSE;
                }
            }

          if (*error)
            return FALSE;

          if (g_strcmp0 (*t, "}"))
            {
              g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                           "unterminated enum value set "
                           "for property '%s'",
                           property_name);

              return FALSE;
            }
        }
      else
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "invalid type "
                       "for property '%s'",
                       property_name);

          return FALSE;
        }

      gimp_prop_eval_read_token (expr, t, error);
    }
  else
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid expression");

      return FALSE;
    }

  return result;
}

static gchar *
gimp_prop_eval_string_impl (GObject      *config,
                            GParamSpec   *pspec,
                            const gchar  *key,
                            const gchar  *default_value,
                            GError      **error,
                            gint          depth)
{
  const gchar *expr;
  gchar       *t      = NULL;
  gchar       *result = NULL;

  if (! gimp_prop_eval_depth_test (depth, error))
    return NULL;

  expr = gegl_param_spec_get_property_key (pspec, key);

  if (! expr)
    return g_strdup (default_value);

  gimp_prop_eval_read_token (&expr, &t, error);

  if (! *error)
    {
      result = gimp_prop_eval_string_selection (config, pspec,
                                                &expr, &t, error, depth);
    }

  /* check for trailing tokens at the end of the expression */
  if (! *error && t)
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid expression");

      g_clear_pointer (&result, g_free);
    }

  g_free (t);

  if (*error)
    {
      g_prefix_error (error,
                      "in key '%s' of property '%s': ",
                      key,
                      g_param_spec_get_name (pspec));

      return NULL;
    }

  if (result)
    return result;
  else
    return g_strdup (default_value);
}

static gchar *
gimp_prop_eval_string_selection (GObject      *config,
                                 GParamSpec   *pspec,
                                 const gchar **expr,
                                 gchar       **t,
                                 GError      **error,
                                 gint          depth)
{
  if (! gimp_prop_eval_depth_test (depth, error) || ! t)
    return NULL;

  if (! g_strcmp0 (*t, "["))
    {
      gboolean  match  = FALSE;
      gchar    *result = NULL;

      if (! g_strcmp0 (gimp_prop_eval_read_token (expr, t, error), "]"))
        return NULL;

      while (! *error)
        {
          gboolean  cond;
          gchar    *value;

          cond = gimp_prop_eval_boolean_or (config, pspec,
                                            expr, t, error, depth + 1);

          if (*error)
            break;

          if (g_strcmp0 (*t, ":"))
            {
              g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                           "missing string selection value");

              break;
            }

          gimp_prop_eval_read_token (expr, t, error);

          if (*error)
            break;

          value = gimp_prop_eval_string_selection (config, pspec,
                                                   expr, t, error, depth + 1);

          if (*error)
            break;

          if (! match && cond)
            {
              match  = TRUE;
              result = value;
            }
          else
            {
              g_free (value);
            }

          if (! g_strcmp0 (*t, ","))
            {
              gimp_prop_eval_read_token (expr, t, error);

              continue;
            }
          else if (! g_strcmp0 (*t, "]"))
            {
              gimp_prop_eval_read_token (expr, t, error);

              break;
            }
          else
            {
              if (*t)
                {
                  g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                               "invalid string selection");
                }
              else
                {
                  g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                               "unterminated string selection");
                }

              break;
            }
        }

      if (*error)
        {
          g_free (result);

          return NULL;
        }

      return result;
    }

  return gimp_prop_eval_string_simple (config,
                                       pspec, expr, t, error, depth);
}

static gchar *
gimp_prop_eval_string_simple (GObject      *config,
                              GParamSpec   *pspec,
                              const gchar **expr,
                              gchar       **t,
                              GError      **error,
                              gint          depth)
{
  gchar *result = NULL;

  if (! gimp_prop_eval_depth_test (depth, error))
    return NULL;

  /* literal */
  if (*t && **t == '\'')
    {
      gchar *escaped;

      escaped = g_strndup (*t + 1, strlen (*t + 1) - 1);

      result = g_strcompress (escaped);

      g_free (escaped);

      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        {
          g_free (result);

          return NULL;
        }
    }
  /* reference */
  else if (! g_strcmp0 (*t, "$"))
    {
      gchar *key;

      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        return NULL;

      if (! gimp_prop_eval_parse_reference (config, pspec,
                                            expr, t, error, &pspec, &key))
        return NULL;

      result = gimp_prop_eval_string_impl (config, pspec,
                                           key, NULL, error, depth + 1);

      g_free (key);
    }
  /* deferred literal */
  else if (gimp_prop_eval_is_name (*t))
    {
      GParamSpec  *str_pspec;
      gchar       *str_key;
      const gchar *str;

      if (! gimp_prop_eval_parse_reference (config, pspec,
                                            expr, t, error,
                                            &str_pspec, &str_key))
        return NULL;

      str = gegl_param_spec_get_property_key (str_pspec, str_key);

      if (! str)
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "key '%s' of property '%s' not found",
                       str_key,
                       g_param_spec_get_name (str_pspec));

          g_free (str_key);

          return NULL;
        }

      g_free (str_key);

      result = g_strdup (str);
    }
  else
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid expression");

      return NULL;
    }

  return result;
}

static gboolean
gimp_prop_eval_parse_reference (GObject      *config,
                                GParamSpec   *pspec,
                                const gchar **expr,
                                gchar       **t,
                                GError      **error,
                                GParamSpec  **ref_pspec,
                                gchar       **ref_key)
{
  if (! gimp_prop_eval_is_name (*t))
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "invalid reference");

      return FALSE;
    }

  *ref_pspec = pspec;
  *ref_key   = g_strdup (*t);

  gimp_prop_eval_read_token (expr, t, error);

  if (*error)
    {
      g_free (*ref_key);

      return FALSE;
    }

  if (! g_strcmp0 (*t, "."))
    {
      gchar *property_name;

      property_name = *ref_key;

      if (! gimp_prop_eval_read_token (expr, t, error) ||
          ! gimp_prop_eval_is_name (*t))
        {
          if (! *error)
            {
              g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                           "invalid reference");
            }

          g_free (property_name);

          return FALSE;
        }

      *ref_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                 property_name);

      if (! *ref_pspec)
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "property '%s' not found",
                       property_name);

          g_free (property_name);

          return FALSE;
        }

      g_free (property_name);

      *ref_key = g_strdup (*t);

      gimp_prop_eval_read_token (expr, t, error);

      if (*error)
        {
          g_free (*ref_key);

          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
gimp_prop_eval_depth_test (gint     depth,
                           GError **error)
{
  /* make sure we don't recurse too deep.  in particular, guard against
   * circular references.
   */
  if (depth == 100)
    {
      g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                   "maximal nesting level exceeded");

      return FALSE;
    }

  return TRUE;
}

static gchar *
gimp_prop_eval_read_token (const gchar **expr,
                           gchar       **t,
                           GError      **error)
{
  const gchar *token;

  g_free (*t);
  *t = NULL;

  /* skip whitespace */
  while (g_ascii_isspace (**expr))
    ++*expr;

  token = *expr;

  if (*token == '\0')
    return NULL;

  /* name */
  if (gimp_prop_eval_is_name (token))
    {
      do { ++*expr; } while (g_ascii_isalnum (**expr) ||
                             **expr == '_'            ||
                             **expr == '-');
    }
  /* string literal */
  else if (token[0] == '\'')
    {
      for (++*expr; **expr != '\0' && **expr != '\''; ++*expr)
        {
          if (**expr == '\\')
            {
              ++*expr;

              if (**expr == '\0')
                break;
            }
        }

      if (**expr == '\0')
        {
          g_set_error (error, GIMP_PROP_EVAL_ERROR, GIMP_PROP_EVAL_FAILED,
                       "unterminated string literal");

          return NULL;
        }

      ++*expr;
    }
  /* punctuation or boolean literal */
  else
    {
      ++*expr;
    }

  *t = g_strndup (token, *expr - token);

  return *t;
}

static gboolean
gimp_prop_eval_is_name (const gchar *token)
{
  return token && (g_ascii_isalpha (*token) || *token == '_');
}

static GQuark
gimp_prop_eval_error_quark (void)
{
  return g_quark_from_static_string ("gimp-prop-eval-error-quark");
}
