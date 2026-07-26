/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsavable.c
 * Copyright (C) 2026 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpsavable.h"
#include "gimpsavable-load.h"


typedef struct _GimpLoadContext GimpLoadContext;
struct _GimpLoadContext
{
  GHashTable *handlers;
  GHashTable *values;
  gint        level;
  GString    *text;
};

typedef struct _GimpLoadHandlers GimpLoadHandlers;
struct _GimpLoadHandlers
{
  GimpEnterElementHandler  enter_handler;
  GimpExitElementhandler   exit_handler;
  gpointer                 user_data;
};

typedef struct _GimpLoadValue GimpLoadValue;
struct _GimpLoadValue
{
  GValue         *value;
  GDestroyNotify  free_data;
};


static gboolean gimp_savable_load_get_all       (GimpLoadState *state,
                                                 va_list        args);
static void     gimp_savable_load_store_all     (GimpLoadState *state,
                                                 va_list        args);
static void     gimp_savable_load_store_one     (GimpLoadState *state,
                                                 const gchar   *key,
                                                 const gchar   *format,
                                                 va_list        args);

static void     gimp_savable_load_push_context  (GimpLoadState *state);
static void     gimp_savable_load_pop_context   (GimpLoadState *state);
static void     gimp_savable_free_context_value (GimpLoadValue *value);

static gboolean gimp_savable_exit_icc           (GimpLoadState  *state,
                                                 const gchar    *text,
                                                 gsize           len,
                                                 gpointer        user_data,
                                                 GError        **error);


void
gimp_savable_load (GType          savable_type,
                   GimpLoadState *state)
{
  GObjectClass         *klass;
  GimpSavableInterface *iface;

  klass = g_type_class_ref (savable_type);
  iface = g_type_interface_peek (klass, GIMP_TYPE_SAVABLE);

  if (iface->load)
    iface->load (state);

  g_type_class_unref (klass);
}

/* Load values from their string representations. */
void
gimp_savable_load_store_from_string (GimpLoadState *state,
                                     ...)
{
  va_list args;

  va_start (args, state);
  gimp_savable_load_store_all (state, args);
  va_end (args);
}

/* Load a value as a pointer */
void
gimp_savable_load_store_value (GimpLoadState  *state,
                               const gchar    *key,
                               gpointer        data,
                               GDestroyNotify  free_data)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);
  GHashTable      *values  = context->values;
  GimpLoadValue   *value   = g_new0 (GimpLoadValue, 1);
  GValue          *gvalue  = g_new0 (GValue, 1);

  g_value_init (gvalue, G_TYPE_POINTER);
  g_value_set_pointer (gvalue, data);

  value->value     = gvalue;
  value->free_data = free_data;

  g_hash_table_insert (values, (gpointer) key, value);
}

/* Get stored contextual value */
gboolean
gimp_savable_load_get_value (GimpLoadState *state,
                             ...)
{
  va_list  args;
  gboolean success;

  va_start (args, state);
  success = gimp_savable_load_get_all (state, args);
  va_end (args);

  return success;
}

/* Bubble up a loaded value to the parent context. */
void
gimp_savable_load_bubble_up (GimpLoadState *state,
                             const gchar   *key)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);
  GHashTable      *values  = context->values;
  gpointer         val;

  if (g_hash_table_steal_extended (values,
                                   (gconstpointer) key,
                                   NULL,
                                   &val))
    {
      context = g_queue_peek_nth (state->contexts, 1);
      g_hash_table_insert (context->values, (gpointer) key, val);
    }
}

void
gimp_savable_load_add_handlers (GimpLoadState           *state,
                                const gchar             *element_name,
                                GimpEnterElementHandler  enter_callback,
                                GimpExitElementhandler   exit_callback,
                                gpointer                 user_data)
{
  GimpLoadContext  *context = g_queue_peek_head (state->contexts);
  GimpLoadHandlers *h       = g_new0 (GimpLoadHandlers, 1);

  if (context == NULL)
    {
      /* The root context. */
      gimp_savable_load_push_context (state);
      context = g_queue_peek_head (state->contexts);
    }

  h->enter_handler = enter_callback;
  h->exit_handler  = exit_callback;
  h->user_data     = user_data;
  g_hash_table_insert (context->handlers, (gpointer) element_name, h);
}

/**
 * gimp_savable_enter_format:
 * @state:
 * @attribute_names:
 * @attribute_values:
 * @user_data:
 * @error:
 *
 * Use this %GimpEnterElementHandler with %NULL %GimpExitElementhandler
 * to parse a <dimensions/> element.
 *
 * It will create two gint values, bubbling up as "width" and "height".
 */
gboolean
gimp_savable_enter_dimensions (GimpLoadState  *state,
                               const gchar   **attribute_names,
                               const gchar   **attribute_values,
                               gpointer        user_data,
                               GError         **error)
{
  const gchar *width  = NULL;
  const gchar *height = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "width") == 0)
        width = *attribute_values;
      else if (g_strcmp0 (*attribute_names, "height") == 0)
        height = *attribute_values;
      else
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  if (width && height)
    {
      gimp_savable_load_store_from_string (state,
                                           "width",  "%d", width,
                                           "height", "%d", height,
                                           NULL);
      gimp_savable_load_bubble_up (state, "width");
      gimp_savable_load_bubble_up (state, "height");
    }
  else
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: width and/or height attributes are missing",
                   G_STRFUNC);
    }

  return (width && height);
}

/**
 * gimp_savable_enter_format:
 * @state:
 * @attribute_names:
 * @attribute_values:
 * @user_data:
 * @error:
 *
 * Use this %GimpEnterElementHandler with gimp_savable_exit_format() to
 * parse a <format/> element.
 *
 * It will create a Babl format, which it will bubble up to the parent
 * context under the key "format".
 */
gboolean
gimp_savable_enter_format (GimpLoadState  *state,
                           const gchar   **attribute_names,
                           const gchar   **attribute_values,
                           gpointer        user_data,
                           GError         **error)
{
  const gchar *encoding = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "encoding") == 0)
        encoding = *attribute_values;
      else
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  if (encoding)
    {
      gimp_savable_load_store_from_string (state,
                                           "encoding", "%s", encoding,
                                           NULL);
      gimp_savable_load_add_handlers (state, "space",
                                      gimp_savable_enter_space,
                                      gimp_savable_exit_space,
                                      user_data);
    }
  else
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: 'encoding' attribute is missing",
                   G_STRFUNC);
    }

  return (encoding != NULL);
}

gboolean
gimp_savable_exit_format (GimpLoadState  *state,
                          const gchar    *text,
                          gsize           len,
                          gpointer        user_data,
                          GError        **error)
{
  const gchar *encoding = NULL;
  const Babl  *format   = NULL;
  const Babl  *space    = NULL;

  gimp_savable_load_get_value (state, "space", &space, NULL);
  gimp_savable_load_get_value (state, "encoding", &encoding, NULL);

  /* The enter_format() handler should have already taken care of an
   * absent encoding.
   */
  g_return_val_if_fail (encoding, FALSE);
  format = babl_format_with_space (encoding, space);

  if (! format)
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                   "%s: failed to create format with encoding '%s' and space '%s'.",
                   G_STRFUNC, encoding,
                   space ? babl_get_name (space) : "sRGB");
      return FALSE;
    }

  gimp_savable_load_store_value (state, "format", (gpointer) format, NULL);
  gimp_savable_load_bubble_up (state, "format");

  return TRUE;
}

/**
 * gimp_savable_enter_space:
 * @state:
 * @attribute_names:
 * @attribute_values:
 * @user_data:
 * @error:
 *
 * Use this %GimpEnterElementHandler with gimp_savable_exit_space() to
 * parse a <space/> element.
 *
 * If no @user_data is set, it will create a space, which it will bubble
 * up to the parent context under the key "space".
 *
 * If @user_data is set, it must be a %GHashTable. Then we expect the
 * attribute "id" to be set, and it will be used as the key with which
 * the space will be added to the table.
 */
gboolean
gimp_savable_enter_space (GimpLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError         **error)
{
  const Babl  *space  = NULL;
  const gchar *id     = NULL;
  GHashTable  *spaces = (GHashTable *) user_data;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        {
          if (g_strcmp0 (*attribute_values, "sRGB") == 0)
            space = babl_space ("sRGB");
          else
            g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                         "%s: unknown space name: '%s'",
                         G_STRFUNC, *attribute_names);
        }
      else if (g_strcmp0 (*attribute_names, "id") == 0)
        {
          id = *attribute_values;
        }
      else
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  if (space)
    gimp_savable_load_store_value (state, "space", (gpointer) space, NULL);
  else
    gimp_savable_load_add_handlers (state, "icc",
                                    NULL,
                                    gimp_savable_exit_icc,
                                    NULL);

  if (id)
    {
      if (spaces)
        gimp_savable_load_store_from_string (state,
                                             "space-id", "%s", id,
                                             NULL);
      else
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute 'id'.",
                     G_STRFUNC);
    }

  return TRUE;
}

gboolean
gimp_savable_exit_space (GimpLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  GHashTable  *spaces   = (GHashTable *) user_data;
  const Babl  *space    = NULL;
  const gchar *space_id = NULL;

  if (spaces)
    {
      gimp_savable_load_get_value (state,
                                   "space",    &space,
                                   "space-id", &space_id,
                                   NULL);
      if (! space || ! space_id)
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                     "%s: parsing of stored space failed.",
                     G_STRFUNC);
      else
        g_hash_table_insert (spaces, g_strdup (space_id), (gpointer) space);
    }
  else
    {
      gimp_savable_load_bubble_up (state, "space");
    }

  return TRUE;
}


/* Friend Functions for gimpimage-savable */

void
gimp_savable_load_append_text (GimpLoadState *state,
                               const gchar   *text,
                               gsize          text_len)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);

  /* Ignoring any text before we get a context. */
  if (context)
    g_string_append_len (context->text, text, text_len);
}

const gchar *
gimp_savable_load_get_text (GimpLoadState *state,
                            gsize         *text_len)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);

  *text_len = context->text->len;

  return context->text->str;
}

gboolean
gimp_savable_enter_element (GimpLoadState  *state,
                            const gchar    *element_name,
                            const gchar   **attribute_names,
                            const gchar   **attribute_values,
                            GError        **error)
{
  GimpLoadContext  *context  = g_queue_peek_head (state->contexts);
  GHashTable       *handlers = context->handlers;
  GimpLoadHandlers *h;

  h = g_hash_table_lookup (handlers, element_name);

  gimp_savable_load_push_context (state);
  if (h == NULL)
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: unexpected element: %s", G_STRFUNC, element_name);
      return FALSE;
    }
  else if (h->enter_handler)
    {
      return h->enter_handler (state, attribute_names, attribute_values,
                               h->user_data, error);
    }

  return TRUE;
}

gboolean
gimp_savable_exit_element (GimpLoadState  *state,
                           const gchar    *element_name,
                           const gchar    *text,
                           gsize           text_len,
                           GError        **error)
{
  /* Exit handler is in the parent context. */
  GimpLoadContext  *context  = g_queue_peek_nth (state->contexts, 1);
  GHashTable       *handlers = context->handlers;
  GimpLoadHandlers *h;
  gboolean          success  = TRUE;

  h = g_hash_table_lookup (handlers, element_name);

  if (h == NULL)
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: unexpected element: %s", G_STRFUNC, element_name);
      success = FALSE;
    }
  else if (h->exit_handler)
    {
      success = h->exit_handler (state, text, text_len, h->user_data, error);
    }

  gimp_savable_load_pop_context (state);

  return success;
}


/* Private Functions */

static gboolean
gimp_savable_load_get_all (GimpLoadState *state,
                           va_list        args)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);
  GHashTable      *values  = context->values;
  const gchar     *key;
  gboolean         success = TRUE;

  key = va_arg (args, gchar *);
  while (key)
    {
      gpointer val;

      if (! g_hash_table_lookup_extended (values,
                                          (gconstpointer) key,
                                          NULL,
                                          &val))
        {
          /* Any failed value lookup triggers a failure, even if we may
           * have successfully set other values.
           */
          success = FALSE;
          (void) va_arg (args, void *);
        }
      else
        {
          GimpLoadValue *value  = (GimpLoadValue *) val;
          GValue        *gvalue = value->value;
          if (G_VALUE_TYPE (gvalue) == G_TYPE_STRING)
            {
              gchar **strval = va_arg (args, gchar **);
              *strval = (gchar *) g_value_get_string (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_INT)
            {
              gint *intval = va_arg (args, gint *);
              *intval = g_value_get_int (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_LONG)
            {
              glong *intval = va_arg (args, glong *);
              *intval = g_value_get_long (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_UINT)
            {
              guint *intval = va_arg (args, guint *);
              *intval = g_value_get_uint (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_ULONG)
            {
              gulong *intval = va_arg (args, gulong *);
              *intval = g_value_get_ulong (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_DOUBLE)
            {
              gdouble *fval = va_arg (args, gdouble *);
              *fval = g_value_get_double (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_BOOLEAN)
            {
              gboolean *bval = va_arg (args, gboolean *);
              *bval = g_value_get_boolean (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_GTYPE)
            {
              GType *gtype = va_arg (args, GType *);
              *gtype = g_value_get_gtype (gvalue);
            }
          else if (G_VALUE_TYPE (gvalue) == G_TYPE_POINTER)
            {
              gpointer *pointer = va_arg (args, gpointer *);
              *pointer = g_value_get_pointer (gvalue);
            }
          else
            {
              g_warning ("%s: unsupported GValue type: %s", G_STRFUNC,
                         g_type_name (G_VALUE_TYPE (gvalue)));
              (void) va_arg (args, void *);
            }
        }

      key = va_arg (args, gchar *);
    }

  return success;
}

static void
gimp_savable_load_store_all (GimpLoadState *state,
                             va_list        args)
{
  const gchar *key;
  const gchar *format;

  key = va_arg (args, char *);
  while (key)
    {
      format = va_arg (args, char *);
      g_return_if_fail (format != NULL);

      gimp_savable_load_store_one (state, key, format, args);

      key = va_arg (args, char *);
    }
}

static void
gimp_savable_load_store_one (GimpLoadState *state,
                             const gchar   *key,
                             const gchar   *format,
                             va_list        args)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);
  GHashTable      *values  = context->values;
  const gchar     *strval  = va_arg (args, gchar *);
  GValue          *gvalue;
  GimpLoadValue   *value;

  g_return_if_fail (format != NULL);
  g_return_if_fail (context != NULL && values != NULL);

  value  = g_new0 (GimpLoadValue, 1);
  gvalue = g_new0 (GValue, 1);

  if (strstr (format, "%s"))
    {
      g_value_init (gvalue, G_TYPE_STRING);
      g_value_set_string (gvalue, strval);
    }
  else if (strstr (format, "%d") || strstr (format, "%ld"))
    {
      gint64 intval = g_ascii_strtoll (strval, NULL, 10);

      if ((intval == G_MAXINT64 || intval == G_MININT64) &&
          errno == ERANGE)
        g_printerr ("Value overflows: %s\n", strval);

      if (strstr (format, "%d"))
        {
          gint val = (gint) intval;

          if ((gint64) val != intval)
            g_printerr ("Value overflows as int: %s\n", strval);

          g_value_init (gvalue, G_TYPE_INT);
          g_value_set_int (gvalue, val);
        }
      else
        {
          glong val = (glong) intval;

          if ((gint64) val != intval)
            g_printerr ("Value overflows as long: %s\n", strval);

          g_value_init (gvalue, G_TYPE_LONG);
          g_value_set_long (gvalue, val);
        }
    }
  else if (strstr (format, "%u") || strstr (format, "%lu"))
    {
      guint64 intval = g_ascii_strtoull (strval, NULL, 10);

      if (intval == G_MAXUINT64 && errno == ERANGE)
        g_printerr ("Unsigned value overflows: %s\n", strval);

      if (strstr (format, "%u"))
        {
          guint val = (guint) intval;

          if ((guint64) val != intval)
            g_printerr ("Value overflows as unsigned int: %s\n", strval);

          g_value_init (gvalue, G_TYPE_UINT);
          g_value_set_uint (gvalue, val);
        }
      else
        {
          gulong val = (gulong) intval;

          if ((guint64) val != intval)
            g_printerr ("Value overflows as unsigned long: %s\n", strval);

          g_value_init (gvalue, G_TYPE_ULONG);
          g_value_set_ulong (gvalue, val);
        }
    }
  else if (strstr (format, "%f"))
    {
      gdouble fval = g_ascii_strtod (strval, NULL);
      g_value_init (gvalue, G_TYPE_DOUBLE);
      g_value_set_double (gvalue, fval);
    }
  else if (g_strcmp0 ("%b", format) == 0)
    {
      /* Custom format: boolean type. */
      gboolean bval = g_strcmp0 (strval, "true") ? TRUE : FALSE;
      g_value_init (gvalue, G_TYPE_BOOLEAN);
      g_value_set_boolean (gvalue, bval);
    }
  else if (g_strcmp0 ("%t", format) == 0)
    {
      /* Custom format: type name of passed object. */
      GType gtype = g_type_from_name (strval);

      if (gtype == 0)
        {
          g_printerr ("Unknown GType: %s\n", strval);
          return;
        }
      g_value_init (gvalue, G_TYPE_GTYPE);
      g_value_set_gtype (gvalue, gtype);
    }
  else
    {
      g_return_if_reached ();
    }

  value->value     = gvalue;
  value->free_data = NULL;
  g_hash_table_insert (values, (gpointer) key, value);
}

static void
gimp_savable_load_push_context (GimpLoadState *state)
{
  GimpLoadContext *context;
  GHashTable      *handlers;
  GHashTable      *values;

  context  = g_new0 (GimpLoadContext, 1);
  handlers = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  values   = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                   (GDestroyNotify) gimp_savable_free_context_value);
  context->handlers = handlers;
  context->values   = values;
  context->level    = state->level;
  context->text     = g_string_new (NULL);
  g_queue_push_head (state->contexts, context);
}

static void
gimp_savable_load_pop_context (GimpLoadState *state)
{
  GimpLoadContext *context;

  context = g_queue_pop_head (state->contexts);
  g_hash_table_destroy (context->handlers);
  g_hash_table_destroy (context->values);
  g_string_free (context->text, TRUE);
  g_free (context);
}

static void
gimp_savable_free_context_value (GimpLoadValue *value)
{
  if (value->free_data)
    {
      g_return_if_fail (G_VALUE_TYPE (value->value) == G_TYPE_POINTER);
      value->free_data (g_value_get_pointer (value->value));
    }
  g_value_unset (value->value);
  g_free (value->value);
  g_free (value);
}


/* Handlers */

/**
 * gimp_savable_exit_icc:
 * @state:
 * @attribute_names:
 * @attribute_values:
 * @user_data:
 * @error:
 *
 * Use this %GimpExitElementhandler with a %NULL
 * %GimpEnterElementHandler to parse an <icc/> element.
 *
 * It will create a space, which it will bubble up to the parent context
 * under the key "space".
 */
static gboolean
gimp_savable_exit_icc (GimpLoadState  *state,
                       const gchar    *text,
                       gsize           len,
                       gpointer        user_data,
                       GError        **error)
{
  const Babl *space;
  gchar      *b64;
  guchar     *icc;
  gsize       icc_len;
  const char *babl_error = NULL;

  /* This is external data. Let's make sure this is valid base64 and
   * that it is NUL-terminated.
   */
  b64 = g_strndup (text, len);
  for (gint i = 0; i < len; i++)
    if (b64[i] != '/' && b64[i] != '+' && b64[i] != '=' &&
        (b64[i] < '0' || b64[i] > '9') &&
        (b64[i] < 'A' || b64[i] > 'Z') &&
        (b64[i] < 'a' || b64[i] > 'z'))
      {
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                     "%s: invalid Base64 encoding: '%s'",
                     G_STRFUNC, b64);
        return FALSE;
      }
  icc   = g_base64_decode (b64, &icc_len);
  space = babl_space_from_icc ((const char *) icc, icc_len,
                               BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                               &babl_error);
  if (space)
    {
      gimp_savable_load_store_value (state, "space", (gpointer) space, NULL);
      gimp_savable_load_bubble_up (state, "space");
    }
  else
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                   "%s: invalid ICC data: %s",
                   G_STRFUNC, babl_error);
    }

  g_free (icc);
  return TRUE;
}


/* Error Domain */

/**
 * gimp_wlbr_error_quark:
 *
 * This function is used to implement the GIMP_WLBR_ERROR macro. It
 * shouldn't be called directly.
 *
 * Returns: the #GQuark to identify error in the GimpData error domain.
 **/
GQuark
gimp_wlbr_error_quark (void)
{
  return g_quark_from_static_string ("gimp-wlbr-error-quark");
}
