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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpitem.h"
#include "gimpsavable.h"
#include "gimpsavable-load.h"
#include "gimpunit.h"


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
  GDestroyNotify           free_data;
};

typedef struct _GimpLoadValue GimpLoadValue;
struct _GimpLoadValue
{
  GValue         *value;
  GDestroyNotify  free_data;
};

typedef struct _GimpSimpleHandlerData GimpSimpleHandlerData;
struct _GimpSimpleHandlerData
{
  gchar                  *value_prefix;
  gchar                  *text_value_format;
  GHashTable             *attributes;
  gboolean                all_attributes_needed;
  gboolean                fatal_on_missing;
  GimpExitElementhandler  secondary_exit_handler;

  gpointer                user_data;
  GDestroyNotify          free_data;
};

typedef struct _GimpConfigHandlerData GimpConfigHandlerData;
struct _GimpConfigHandlerData
{
  gchar                   *element_name;
  GType                    parent_type;
  GType                    config_type;

  gchar                  **prop_names;
  GValue                  *prop_values;
  gint                     n_props;

  GimpExitElementhandler   secondary_exit_handler;
};


static void     gimp_savable_load_init_state    (GimpLoadState        *state,
                                                 Gimp                 *gimp,
                                                 GFile                *backup_dir);
static void     gimp_wlbr_load_start_element    (GMarkupParseContext  *context,
                                                 const gchar          *element_name,
                                                 const gchar         **attribute_names,
                                                 const gchar         **attribute_values,
                                                 gpointer              user_data,
                                                 GError              **error);
static void     gimp_wlbr_load_end_element      (GMarkupParseContext  *context,
                                                 const gchar          *element_name,
                                                 gpointer              user_data,
                                                 GError              **error);
static void     gimp_wlbr_load_text             (GMarkupParseContext  *context,
                                                 const gchar          *text,
                                                 gsize                 text_len,
                                                 gpointer              user_data,
                                                 GError              **error);


static GValue * gimp_savable_load_get_gvalue_in (GimpLoadState        *state,
                                                 GimpLoadContext      *context,
                                                 const gchar          *element_name);
static gboolean gimp_savable_load_get_all       (GimpLoadState        *state,
                                                 GimpLoadContext      *context,
                                                 va_list               args);
static void     gimp_savable_load_store_all     (GimpLoadState        *state,
                                                 va_list               args);
static void     gimp_savable_load_store_one     (GimpLoadState        *state,
                                                 const gchar          *key,
                                                 const gchar          *format,
                                                 va_list               args);

static void     gimp_savable_load_push_context  (GimpLoadState        *state);
static void     gimp_savable_load_pop_context   (GimpLoadState        *state);
static void     gimp_savable_free_context_value (GimpLoadValue        *value);
static void     gimp_savable_free_handlers      (GimpLoadHandlers     *handlers);
static void     gimp_savable_free_simple_data   (GimpSimpleHandlerData *data);
static void     gimp_savable_free_config_data   (GimpConfigHandlerData *data);

static void     gimp_savable_load_free_context  (GimpLoadContext      *context);
static gchar  * gimp_savable_validate_base64    (const gchar          *text,
                                                 gsize                 len,
                                                 GError              **error);

static gboolean gimp_savable_load_enter_simple  (GimpLoadState        *state,
                                                 const gchar         **attribute_names,
                                                 const gchar         **attribute_values,
                                                 gpointer              user_data,
                                                 GError               **error);
static gboolean gimp_savable_load_exit_simple   (GimpLoadState        *state,
                                                 const gchar          *text,
                                                 gsize                 len,
                                                 gpointer              user_data,
                                                 GError              **error);
static gboolean gimp_savable_load_enter_config  (GimpLoadState        *state,
                                                 const gchar         **attribute_names,
                                                 const gchar         **attribute_values,
                                                 gpointer              user_data,
                                                 GError              **error);
static gboolean gimp_savable_load_exit_config   (GimpLoadState        *state,
                                                 const gchar          *text,
                                                 gsize                 len,
                                                 gpointer              user_data,
                                                 GError              **error);
static gboolean gimp_savable_load_exit_parasite (GimpLoadState        *state,
                                                 const gchar          *text,
                                                 gsize                 len,
                                                 gpointer              user_data,
                                                 GError              **error);
static gboolean gimp_savable_exit_icc           (GimpLoadState        *state,
                                                 const gchar          *text,
                                                 gsize                 len,
                                                 gpointer              user_data,
                                                 GError              **error);
static gboolean gimp_savable_exit_pixel         (GimpLoadState        *state,
                                                 const gchar          *text,
                                                 gsize                 len,
                                                 gpointer              user_data,
                                                 GError              **error);


void
gimp_savable_load (GType          savable_type,
                   GimpLoadState *state)
{
  GObjectClass         *klass;
  GimpSavableInterface *iface;

  klass = g_type_class_ref (savable_type);
  iface = g_type_interface_peek (klass, GIMP_TYPE_SAVABLE);

  if (iface->tag)
    gimp_savable_load_add_handlers (state, iface->tag,
                                    iface->load_enter,
                                    iface->load_exit,
                                    NULL, NULL);

  g_type_class_unref (klass);
}

/**
 * gimp_savable_config_load:
 * @parent_type:
 * @element_name:
 * @state:
 * @secondary_exit_handler:
 *
 * This will load a GimpConfig object which has been saved with
 * gimp_savable_config_save(). The stored object must be a subtype of
 * @parent_type.
 *
 * You may add more values which will be used as properties when
 * creating the object. This is necessary for some objects when e.g. the
 * associated GimpImage or Gimp object must be set at creation (and even
 * more if it's a %G_PARAM_CONSTRUCT_ONLY property).
 * Additional values are triplets: the property name, followed by its
 * GType, and finally the data.
 *
 * In the end, the config object will be stored under the name
 * @element_name, and bubbled-up to parent element.
 *
 * If @secondary_exit_handler is set, it will be run after the automatic
 * handler creating the config object and the bubbling-up of the value
 * won't happen. This can be useful when repeatedly parsing similar
 * elements under the same level.
 */
void
gimp_savable_config_load (GType                   parent_type,
                          const gchar            *element_name,
                          GimpLoadState          *state,
                          GimpExitElementhandler  secondary_exit_handler,
                          ...)
{
  GimpConfigHandlerData  *data;
  va_list                 args;
  const gchar            *prop_name;
  gchar                 **names   = NULL;
  GValue                 *values  = NULL;
  GList                  *lnames  = NULL;
  GList                  *lvalues = NULL;
  gint                    n_props = 0;

  va_start (args, secondary_exit_handler);
  prop_name = va_arg (args, gchar *);
  while (prop_name)
    {
      GValue *value;
      GType   gtype = va_arg (args, GType);

      value = g_new0 (GValue, 1);
      g_value_init (value, gtype);
      if (gtype == G_TYPE_STRING)
        {
          gchar *strval = va_arg (args, gchar *);
          g_value_set_string (value, strval);
        }
      else if (gtype == G_TYPE_INT)
        {
          gint ival = va_arg (args, gint);
          g_value_set_int (value, ival);
        }
      else if (gtype == G_TYPE_LONG)
        {
          glong ival = va_arg (args, glong);
          g_value_set_long (value, ival);
        }
      else if (gtype == G_TYPE_UINT)
        {
          guint ival = va_arg (args, guint);
          g_value_set_uint (value, ival);
        }
      else if (gtype == G_TYPE_ULONG)
        {
          gulong ival = va_arg (args, gulong);
          g_value_set_ulong (value, ival);
        }
      else if (gtype == G_TYPE_DOUBLE)
        {
          gdouble fval = va_arg (args, gdouble);
          g_value_set_double (value, fval);
        }
      else if (gtype == G_TYPE_BOOLEAN)
        {
          gboolean bval = va_arg (args, gboolean);
          g_value_set_boolean (value, bval);
        }
      else if (gtype == G_TYPE_GTYPE)
        {
          GType val = va_arg (args, GType);
          g_value_set_gtype (value, val);
        }
      else if (gtype == G_TYPE_POINTER)
        {
          gpointer val = va_arg (args, gpointer);
          g_value_set_pointer (value, val);
        }
      else if (g_type_is_a (gtype, G_TYPE_OBJECT))
        {
          GObject *val = va_arg (args, GObject *);
          g_value_set_object (value, val);
        }
      else if (g_type_is_a (gtype, G_TYPE_ENUM))
        {
          gint val = va_arg (args, gint);
          g_value_set_enum (value, val);
        }
      else
        {
          g_return_if_reached ();
        }

      lnames  = g_list_prepend (lnames, g_strdup (prop_name));
      lvalues = g_list_prepend (lvalues, value);
      n_props++;

      prop_name = va_arg (args, gchar *);
    }
  va_end (args);

  if (n_props > 0)
    {
      GList *iter_name  = lnames;
      GList *iter_value = lvalues;
      gint   i          = 0;

      names  = g_new0 (gchar *, n_props);;
      values = g_new0 (GValue, n_props);

      for (; iter_name; iter_name = iter_name->next)
        {
          GValue *value = iter_value->data;

          names[i] = iter_name->data;

          g_value_init (&values[i], G_VALUE_TYPE (value));
          g_value_copy (value, &values[i]);

          g_value_unset (value);

          iter_value = iter_value->next;
          i++;
        }

      g_list_free (lnames);
      g_list_free_full (lvalues, g_free);
    }

  data = g_new0 (GimpConfigHandlerData, 1);

  data->element_name           = g_strdup (element_name);
  data->parent_type            = parent_type;
  data->config_type            = G_TYPE_NONE;
  data->prop_names             = names;
  data->prop_values            = values;
  data->n_props                = n_props;
  data->secondary_exit_handler = secondary_exit_handler;

  gimp_savable_load_add_handlers (state, element_name,
                                  gimp_savable_load_enter_config,
                                  gimp_savable_load_exit_config,
                                  data,
                                  (GDestroyNotify) gimp_savable_free_config_data);
}

void
gimp_savable_parasite_load (GimpLoadState *state,
                            GObject       *image_or_item)
{
  gimp_savable_load_add_simple_handler (state, "parasite", "%s",
                                        (GimpExitElementhandler) gimp_savable_load_exit_parasite,
                                        image_or_item, NULL,
                                        TRUE, FALSE,
                                        "name",  "%s",
                                        "flags", "%lu",
                                        NULL);
}

void
gimp_savable_load_push_active_object (GimpLoadState *state,
                                      GObject       *object)
{
  g_queue_push_head (state->objects, object);
}

GObject *
gimp_savable_load_pop_active_object (GimpLoadState *state)
{
  return g_queue_pop_head (state->objects);
}

GObject *
gimp_savable_load_peek_active_object (GimpLoadState *state)
{
  return g_queue_peek_head (state->objects);
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

  g_hash_table_insert (values, (gpointer) g_strdup (key), value);
}

/* Get stored contextual value */
gboolean
gimp_savable_load_get_values (GimpLoadState *state,
                              ...)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);
  va_list          args;
  gboolean         success;

  va_start (args, state);
  success = gimp_savable_load_get_all (state, context, args);
  va_end (args);

  return success;
}

GValue *
gimp_savable_load_get_gvalue (GimpLoadState *state,
                              const gchar   *value_name)
{
  GimpLoadContext *context = g_queue_peek_head (state->contexts);

  return gimp_savable_load_get_gvalue_in (state, context, value_name);
}

gboolean
gimp_savable_load_get_parent_values (GimpLoadState *state,
                                     ...)
{
  GimpLoadContext *context = g_queue_peek_nth (state->contexts, 1);
  va_list          args;
  gboolean         success;

  va_start (args, state);
  success = gimp_savable_load_get_all (state, context, args);
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
                                   (gpointer *) &key,
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
                                gpointer                 user_data,
                                GDestroyNotify           free_data)
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
  h->free_data     = free_data;
  g_hash_table_insert (context->handlers, (gpointer) element_name, h);
}

/**
 * gimp_savable_load_add_simple_handler:
 * @state:
 * @element_name:
 * @text_value_format:
 * @secondary_exit_handler:
 * @user_data:
 * @free_data:
 * @all_attributes_needed:
 * @fatal_on_missing:
 *
 * Use this function instead of gimp_savable_load_add_handlers() when
 * you want to parse a *simple* XML element which only contains
 * attributes and/or text data, all of which must be decodable by
 * gimp_savable_load_store_from_string() format.
 *
 * If @text_value_format is set, then the function will look for text
 * data, and will decode it according to said format. For instance:
 *
 * ```C
 * gimp_savable_load_add_simple_handler (state, "style", "%[GimpGridStyle]",
 *                                       NULL);
 * ```
 *
 * will decode `<style>solid</style>` into %GIMP_GRID_SOLID and will
 * store it, then bubble it up in value "style".
 *
 * If @text_value_format is %NULL, then the @element_name value would
 * just be set as a %TRUE boolean value.
 *
 * Any couple of additional arguments must be an attribute name,
 * followed by a format value. The value name will be the element name
 * and attribute name, separated by a colon.
 * The list must end with %NULL.
 *
 * For instance:
 * ```C
 * gimp_savable_load_add_simple_handler (state, "spacing", NULL,
 *                                       NULL, NULL, NULL,
 *                                       "x", "%f",
 *                                       "y", "%f",
 *                                       NULL);
 * ```
 *
 * will decode `<spacing x='10.000000' y='10.000000'/>` into 2 double
 * values named respectively "spacing:x" and "spacing:y".
 *
 * If @secondary_exit_handler is set, it will be run with @user_data,
 * after all values have been saved, instead of bubbling them up. This
 * can be useful when repeatedly parsing similar elements under the same
 * level.
 *
 * This function also supports XML elements with both simple attribute
 * names and text data.
 *
 * Note that if @all_attributes_needed is %TRUE, this function will
 * trigger an error message when not all the attributes were set on
 * @element_name. Nevertheless the error message will only be fatal is
 * @fatal_on_missing is also %TRUE.
 */
void
gimp_savable_load_add_simple_handler (GimpLoadState          *state,
                                      const gchar            *element_name,
                                      const gchar            *text_value_format,
                                      GimpExitElementhandler  secondary_exit_handler,
                                      gpointer                user_data,
                                      GDestroyNotify          free_data,
                                      gboolean                all_attributes_needed,
                                      gboolean                fatal_on_missing,
                                      ...)
{
  GimpSimpleHandlerData *data;
  const gchar           *attribute;
  va_list                args;

  g_return_if_fail (element_name != NULL && *element_name != '\0');

  data = g_new0 (GimpSimpleHandlerData, 1);

  data->value_prefix           = g_strdup (element_name);
  data->text_value_format      = g_strdup (text_value_format);
  data->attributes             = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                        g_free, g_free);
  data->all_attributes_needed  = all_attributes_needed;
  data->fatal_on_missing       = fatal_on_missing;
  data->secondary_exit_handler = secondary_exit_handler;
  data->user_data              = user_data;
  data->free_data              = free_data;

  va_start (args, fatal_on_missing);
  attribute = va_arg (args, gchar *);
  while (attribute)
    {
      const gchar *attribute_format;

      attribute_format = va_arg (args, gchar *);
      g_return_if_fail (attribute_format != NULL);

      g_hash_table_insert (data->attributes,
                           g_strdup (attribute),
                           g_strdup (attribute_format));

      attribute = va_arg (args, gchar *);
    }
  va_end (args);

  gimp_savable_load_add_handlers (state, element_name,
                                  gimp_savable_load_enter_simple,
                                  gimp_savable_load_exit_simple,
                                  data,
                                  (GDestroyNotify) gimp_savable_free_simple_data);
}

gboolean
gimp_savable_enter_unit (GimpLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  GimpUnit *unit = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "built-in") == 0)
        {
          gint id = -1;

          /* Making sure the GType exists. */
          (void) GIMP_TYPE_UNIT_ID;
          /* Round-trip to validate the built-in ID. */
          gimp_savable_load_store_from_string (state,
                                               "built-in", "%[GimpUnitID]", *attribute_values,
                                               NULL);
          gimp_savable_load_get_values (state, "built-in", &id, NULL);
          if (id == -1)
            {
              g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                           "%s: invalid built-in unit: '%s'",
                           G_STRFUNC, *attribute_values);
            }
          else
            {
              unit = gimp_unit_get_by_id (id);
              gimp_savable_load_store_value (state, "unit", unit, NULL);
            }
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

  if (unit == NULL)
    {
      gimp_savable_load_add_simple_handler (state, "factor", "%f",
                                            NULL, NULL, NULL,
                                            FALSE, FALSE,
                                            NULL);
      gimp_savable_load_add_simple_handler (state, "digits", "%d",
                                            NULL, NULL, NULL,
                                            FALSE, FALSE,
                                            NULL);
      gimp_savable_load_add_simple_handler (state, "name", "%s",
                                            NULL, NULL, NULL,
                                            FALSE, FALSE,
                                            NULL);
      gimp_savable_load_add_simple_handler (state, "symbol", "%s",
                                            NULL, NULL, NULL,
                                            FALSE, FALSE,
                                            NULL);
      gimp_savable_load_add_simple_handler (state, "abbreviation", "%s",
                                            NULL, NULL, NULL,
                                            FALSE, FALSE,
                                            NULL);
    }

  return TRUE;
}

gboolean
gimp_savable_exit_unit (GimpLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  const GimpUnit *unit = NULL;

  if (! gimp_savable_load_get_values (state,
                                      "unit", &unit,
                                      NULL))
    {
      const gchar *name;
      const gchar *symbol;
      const gchar *abbreviation;
      gdouble      factor;
      guint32      digits;

      if (! gimp_savable_load_get_values (state,
                                          "factor",       &factor,
                                          "digits",       &digits,
                                          "name",         &name,
                                          "symbol",       &symbol,
                                          "abbreviation", &abbreviation,
                                          NULL))
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                       "%s: missing one or more attribute for a custom unit.",
                       G_STRFUNC);
        }
      else
        {
          unit = _gimp_unit_get (state->gimp, name, factor, digits,
                                 symbol, abbreviation, FALSE, NULL);
          gimp_savable_load_store_value (state, "unit", (gpointer) unit, NULL);
        }
    }

  gimp_savable_load_bubble_up (state, "unit");

  return TRUE;
}

gboolean
gimp_savable_enter_color (GimpLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError        **error)
{
  gimp_savable_load_add_handlers (state, "format",
                                  gimp_savable_enter_format,
                                  gimp_savable_exit_format,
                                  user_data, NULL);
  gimp_savable_load_add_handlers (state, "pixel",
                                  NULL,
                                  gimp_savable_exit_pixel,
                                  user_data, NULL);

  return TRUE;
}

gboolean
gimp_savable_exit_color (GimpLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  const Babl  *format = NULL;
  const gchar *b64    = NULL;

  gimp_savable_load_get_values (state,
                                "format", &format,
                                "pixel",  &b64,
                                NULL);

  if (format && b64)
    {
      guchar *decoded;
      gsize   decoded_len;

      decoded = g_base64_decode (b64, &decoded_len);

      if (decoded == NULL)
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                       "%s: invalid Base64 data: %s",
                       G_STRFUNC, b64);
        }
      else if (decoded_len != babl_format_get_bytes_per_pixel (format))
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                       "%s: decoded Base64 data has size %" G_GSIZE_FORMAT
                       ". Expected size is %d for format '%s'.",
                       G_STRFUNC, decoded_len,
                       babl_format_get_bytes_per_pixel (format),
                       babl_get_name (format));
        }
      else
        {
          GeglColor *color = gegl_color_new (NULL);

          gegl_color_set_pixel (color, format, decoded);
          gimp_savable_load_store_value (state, "color", (gpointer) color, g_object_unref);
          gimp_savable_load_bubble_up (state, "color");
        }

      g_free (decoded);
    }
  else
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_DATA,
                   "%s: missing format and/or pixel data.",
                   G_STRFUNC);
    }

  return TRUE;
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
                                      user_data, NULL);
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

  gimp_savable_load_get_values (state, "space", &space, NULL);
  gimp_savable_load_get_values (state, "encoding", &encoding, NULL);

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
 * These handlers will create or retrieve a Babl space.
 *
 * If @user_data is set, it must be a %GHashTable. Then we expect the
 * attribute "id" to be set, and it will be used as the key with which
 * the space will be added to the table.
 *
 * Otherwise the attribute "idref" can be set, then it will look up the
 * hash table (now in @state) to retrieve the corresponding space.
 *
 * Either way, the created or retrieved space object will bubble up to
 * the parent context under the key "space", once exiting the element.
 */
gboolean
gimp_savable_enter_space (GimpLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError        **error)
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
      else if (state->spaces && g_strcmp0 (*attribute_names, "idref") == 0)
        {
          /* If spaces hashtable is as user_data, it means we are
           * constructing it. Otherwise it means we are within the main
           * file and we can retrieve spaces from it.
           */
          gpointer data;

          if (g_hash_table_lookup_extended (state->spaces, *attribute_values, NULL, &data))
            space = (const Babl *) data;
          else
            g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                         "%s: failed to retrieve a space by idref='%s'",
                         G_STRFUNC, *attribute_names);
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
                                    NULL, NULL);

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
      gimp_savable_load_get_values (state,
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

  gimp_savable_load_bubble_up (state, "space");

  return TRUE;
}


/* Friend Functions for gimpimage-savable */

gboolean
gimp_savable_load_parse (GimpLoadState  *state,
                         Gimp           *gimp,
                         GFile          *backup_dir,
                         GError        **error)
{
  gimp_savable_load_init_state (state, gimp, backup_dir);
  gimp_savable_load (GIMP_TYPE_IMAGE, state);

  return gimp_xml_parser_parse_gfile (state->xml_parser, state->xml_file, error);
}

void
gimp_savable_load_free_state (GimpLoadState *state)
{
  g_clear_object (&state->xml_file);
  g_clear_pointer (&state->xml_parser, gimp_xml_parser_free);
  g_queue_free_full (state->contexts,
                     (GDestroyNotify) gimp_savable_load_free_context);
  g_queue_free (state->objects);
  g_clear_pointer (&state->spaces, g_hash_table_unref);
}

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
  gboolean          success  = TRUE;

  if (state->unexpected > -1)
    {
      /* We are in the sublevel of an already unexpected element. Don't
       * stack new context, and just ignore everything.
       */
    }
  else
    {
      h = g_hash_table_lookup (handlers, element_name);
      if (h == NULL)
        {
          g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                       "%s: unexpected element: %s", G_STRFUNC, element_name);
          /* We enter an unexpected element. Set an error, but don't
           * fail loading so that we will be able to salvage as much
           * data as possible once we get out of this element.
           * Until then we ignore everything from now on.
           */
          state->unexpected = state->level;
        }
      else
        {
          gimp_savable_load_push_context (state);
          if (h->enter_handler)
            success = h->enter_handler (state, attribute_names, attribute_values,
                                        h->user_data, error);
        }
    }

  return success;
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

  if (state->unexpected > -1 && state->level > state->unexpected)
    {
      /* No-op: ignore everything inside an unknown tag. */
    }
  else if (state->unexpected == state->level)
    {
      /* Ending the unexpected branching. */
      state->unexpected = -1;
    }
  else if (h == NULL)
    {
      /* This should never happen since GMarkup API already handles
       * cases when the closing tag doesn't match the opening tag.
       */
      g_return_val_if_reached (FALSE);
    }
  else
    {
      if (h->exit_handler)
        success = h->exit_handler (state, text, text_len, h->user_data, error);
      gimp_savable_load_pop_context (state);
    }

  return success;
}


/* Private Functions */

static void
gimp_savable_load_init_state (GimpLoadState *state,
                              Gimp          *gimp,
                              GFile         *backup_dir)
{
  state->markup_parser.start_element = gimp_wlbr_load_start_element;
  state->markup_parser.end_element   = gimp_wlbr_load_end_element;
  state->markup_parser.text          = gimp_wlbr_load_text;
  state->markup_parser.passthrough   = NULL;
  state->markup_parser.error         = NULL;

  state->gimp       = gimp;
  state->image      = NULL;
  state->objects    = g_queue_new ();
  state->xml_file   = g_file_get_child (backup_dir, "wlbr-project.xml");
  state->subdir     = backup_dir;
  state->xml_parser = gimp_xml_parser_new (&state->markup_parser, state);
  state->contexts   = g_queue_new ();
  state->level      = 0;
  state->unexpected = -1;
  state->spaces     = NULL;
}

static void
gimp_wlbr_load_start_element (GMarkupParseContext *context,
                              const gchar         *element_name,
                              const gchar        **attribute_names,
                              const gchar        **attribute_values,
                              gpointer             user_data,
                              GError             **error)
{
  GimpLoadState *state = user_data;

  state->level++;

  /* GMarkup API will stop at any error but we have a more subtle
   * approach (allowing non-fatal error messages, while trying to load
   * as much as possible).
   */
  if (gimp_savable_enter_element (state,
                                  element_name,
                                  attribute_names,
                                  attribute_values,
                                  error))
    {
      if (*error)
        /* Success with an error is to be used for non-fatal errors.
         * For such case, we print the error for informational
         * purpose, then clear it to prevent the XML parser to stop.
         *
         * TODO: rather than printing to stderr, we should probably
         * send the message to an error dialog.
         */
        g_printerr ("WLBR WARNING: %s\n", (*error)->message);

      g_clear_error (error);
    }
  else
    {
      if (*error == NULL)
        /* This should only happen when asserting with g_return*(). */
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_INTERNAL,
                     "%s: an internal error happened.", G_STRFUNC);
    }
}

static void
gimp_wlbr_load_end_element (GMarkupParseContext *context,
                            const gchar         *element_name,
                            gpointer             user_data,
                            GError             **error)
{
  GimpLoadState *state = user_data;
  const gchar   *text;
  gsize          text_len;

  text = gimp_savable_load_get_text (state, &text_len);
  if (gimp_savable_exit_element (state, element_name, text, text_len, error))
    {
      if (*error)
        g_printerr ("WLBR WARNING: %s\n", (*error)->message);

      g_clear_error (error);
    }
  else
    {
      if (*error == NULL)
        g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_INTERNAL,
                     "%s: an internal error happened.", G_STRFUNC);
    }

  state->level--;
}

static void
gimp_wlbr_load_text (GMarkupParseContext *context,
                     const gchar         *text,
                     gsize                text_len,
                     gpointer             user_data,
                     GError             **error)
{
  GimpLoadState *state = user_data;

  gimp_savable_load_append_text (state, text, text_len);
}

static GValue *
gimp_savable_load_get_gvalue_in (GimpLoadState   *state,
                                 GimpLoadContext *context,
                                 const gchar     *element_name)
{
  GHashTable    *values = context->values;
  GValue        *gvalue = NULL;
  GimpLoadValue *value;

  if (g_hash_table_lookup_extended (values,
                                    (gconstpointer) element_name,
                                    NULL,
                                    (gpointer *) &value))
    gvalue = value->value;

  return gvalue;
}

static gboolean
gimp_savable_load_get_all (GimpLoadState   *state,
                           GimpLoadContext *context,
                           va_list          args)
{
  gboolean     success = TRUE;
  const gchar *key;

  key = va_arg (args, gchar *);
  while (key)
    {
      GValue *gvalue;

      gvalue = gimp_savable_load_get_gvalue_in (state, context, key);
      if (! gvalue)
        {
          /* Any failed value lookup triggers a failure, even if we may
           * have successfully set other values.
           */
          success = FALSE;
          (void) va_arg (args, void *);
        }
      else
        {
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
          else if (g_type_is_a (G_VALUE_TYPE (gvalue), G_TYPE_ENUM))
            {
              gint *eval = va_arg (args, gint *);
              *eval = g_value_get_enum (gvalue);
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
      gboolean bval = (g_strcmp0 (strval, "true") == 0) ? TRUE : FALSE;
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
      gsize flen = strlen (format);

      if (flen > 3 && format[0] == '%' && format[1] == '[' &&
          format[flen - 1] == ']')
        {
          /* Custom format: enum types. */
          GEnumClass *klass;
          GEnumValue *enum_value;
          gchar      *type_name;
          GType       enum_type;

          type_name = g_strdup (format + 2);
          type_name[strlen (type_name) - 1] = '\0';
          enum_type = g_type_from_name (type_name);
          if (enum_type == 0)
            {
              g_printerr ("Unknown enum type: %s\n", type_name);
              g_free (type_name);
              return;
            }
          g_return_if_fail (G_TYPE_IS_ENUM (enum_type));
          g_value_init (gvalue, enum_type);

          klass = g_type_class_ref (enum_type);
          enum_value = g_enum_get_value_by_nick (klass, strval);
          if (enum_value == NULL)
            {
              g_printerr ("Unknown enum value of type %s: %s\n", type_name, strval);
              g_free (type_name);
              g_type_class_unref (klass);
              return;
            }
          g_value_set_enum (gvalue, enum_value->value);

          g_free (type_name);
          g_type_class_unref (klass);
        }
      else
        {
          g_return_if_reached ();
        }
    }

  value->value     = gvalue;
  value->free_data = NULL;
  g_hash_table_insert (values, (gpointer) g_strdup (key), value);
}

static void
gimp_savable_load_push_context (GimpLoadState *state)
{
  GimpLoadContext *context;
  GHashTable      *handlers;
  GHashTable      *values;

  context  = g_new0 (GimpLoadContext, 1);
  handlers = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                    (GDestroyNotify) gimp_savable_free_handlers);
  values   = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
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
  gimp_savable_load_free_context (context);
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

static void
gimp_savable_free_handlers (GimpLoadHandlers *handlers)
{
  if (handlers->free_data)
    handlers->free_data (handlers->user_data);

  g_free (handlers);
}

static void
gimp_savable_free_simple_data (GimpSimpleHandlerData *data)
{
  g_free (data->value_prefix);
  g_free (data->text_value_format);
  g_hash_table_unref (data->attributes);
  if (data->free_data)
    data->free_data (data->user_data);
  g_free (data);
}

static void
gimp_savable_free_config_data (GimpConfigHandlerData *data)
{
  g_free (data->element_name);
  for (gint i = 0; i < data->n_props; i++)
    {
      g_free (data->prop_names[i]);
      g_value_unset (&data->prop_values[i]);
    }
  g_free (data->prop_names);
  g_free (data->prop_values);
  g_free (data);
}

static void
gimp_savable_load_free_context (GimpLoadContext *context)
{
  g_hash_table_destroy (context->handlers);
  g_hash_table_destroy (context->values);
  g_string_free (context->text, TRUE);
  g_free (context);
}

/* g_base64_decode() does not properly validate the text is valid
 * Base64. Since this is external data, let's make basic validation.
 * Also ensure the text is NUL-terminated by returning a new version (to
 * be freed).
 */
static gchar *
gimp_savable_validate_base64 (const gchar  *text,
                              gsize         len,
                              GError      **error)
{
  gchar *b64;

  if (len == 0)
    {
      g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                   "%s: invalid empty Base64 encoding.",
                   G_STRFUNC);
      return NULL;
    }

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
        g_free (b64);
        return NULL;
      }

  return b64;
}


/* Handlers */

static gboolean
gimp_savable_load_enter_simple (GimpLoadState  *state,
                                const gchar   **attribute_names,
                                const gchar   **attribute_values,
                                gpointer        user_data,
                                GError         **error)
{
  GimpSimpleHandlerData *data    = user_data;
  gboolean               success = TRUE;
  GList                 *found   = NULL;

  while (*attribute_names)
    {
      const gchar *attribute_format;

      if (g_hash_table_lookup_extended (data->attributes,
                                        *attribute_names, NULL,
                                        (gpointer *) &attribute_format))
        {
          gchar *value_name;

          value_name = g_strconcat (data->value_prefix, ":", *attribute_names, NULL);
          found = g_list_prepend (found, (gpointer) *attribute_names);
          gimp_savable_load_store_from_string (state, value_name, attribute_format,
                                               *attribute_values, NULL);
          g_free (value_name);
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

  if (data->all_attributes_needed)
    {
      GList *attributes = g_hash_table_get_keys (data->attributes);

      for (GList *iter = attributes; iter; iter = iter->next)
        {
          const gchar *attr = iter->data;

          if (! g_list_find_custom (found, iter->data, (GCompareFunc) g_strcmp0))
            {
              g_clear_error (error);
              g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                           "%s: missing attribute '%s' on element '%s'.",
                           G_STRFUNC, attr, data->value_prefix);
              success = (! data->fatal_on_missing);
              break;
            }
        }

      g_list_free (attributes);
    }

  g_list_free (found);

  return success;
}

static gboolean
gimp_savable_load_exit_simple (GimpLoadState  *state,
                               const gchar    *text,
                               gsize           len,
                               gpointer        user_data,
                               GError        **error)
{
  GimpSimpleHandlerData *data = user_data;

  if (data->text_value_format)
    {
      gchar *value;

      value = g_strndup (text, len);
      gimp_savable_load_store_from_string (state,
                                           data->value_prefix,
                                           data->text_value_format,
                                           value, NULL);
      g_free (value);
    }
  else
    {
      gimp_savable_load_store_from_string (state, data->value_prefix,
                                           "%b", "true", NULL);
    }

  if (data->secondary_exit_handler)
    {
      data->secondary_exit_handler (state, text, len, data->user_data, error);
    }
  else
    {
      GList *attributes = g_hash_table_get_keys (data->attributes);

      gimp_savable_load_bubble_up (state, data->value_prefix);

      for (GList *iter = attributes; iter; iter = iter->next)
        {
          const gchar *attr = iter->data;
          gchar       *value_name;

          value_name = g_strconcat (data->value_prefix, ":", attr, NULL);
          gimp_savable_load_bubble_up (state, value_name);
          g_free (value_name);
        }
      g_list_free (attributes);
    }

  return TRUE;
}

static gboolean
gimp_savable_load_enter_config (GimpLoadState  *state,
                                const gchar   **attribute_names,
                                const gchar   **attribute_values,
                                gpointer        user_data,
                                GError        **error)
{
  GimpConfigHandlerData *data        = user_data;
  GType                  config_type = G_TYPE_NONE;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "type") == 0)
        {
          /* Round-trip to validate the GType. */
          gimp_savable_load_store_from_string (state,
                                               "config-type", "%t", *attribute_values,
                                               NULL);
          gimp_savable_load_get_values (state, "config-type", &config_type, NULL);
          if (config_type == G_TYPE_NONE ||
              ! g_type_is_a (config_type, GIMP_TYPE_CONFIG))
            {
              g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                           "%s: invalid config type %s.",
                           G_STRFUNC, *attribute_values);
              config_type = G_TYPE_NONE;
            }
          else if (data->parent_type != G_TYPE_NONE &&
                   ! g_type_is_a (config_type, data->parent_type))
            {
              g_set_error (error, GIMP_WLBR_ERROR, GIMP_WLBR_ERROR_FORMAT,
                           "%s: invalid config type %s (a child class of %s is expected)",
                           G_STRFUNC, *attribute_values,
                           g_type_name (data->parent_type));
              config_type = G_TYPE_NONE;
            }
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

  if (config_type != G_TYPE_NONE)
    {
      GObjectClass  *klass;
      GParamSpec   **pspecs;
      guint          n_pspecs = 0;

      klass  = g_type_class_ref (config_type);
      pspecs = g_object_class_list_properties (klass, &n_pspecs);

      for (gint i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec  = pspecs[i];
          gchar      *format = NULL;

          if (! (pspec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
            continue;

          if (pspec->value_type == G_TYPE_STRING)
            {
              format = g_strdup ("%s");
            }
          else if (pspec->value_type == G_TYPE_INT)
            {
              format = g_strdup ("%d");
            }
          else if (pspec->value_type == G_TYPE_LONG)
            {
              format = g_strdup ("%ld");
            }
          else if (pspec->value_type == G_TYPE_UINT)
            {
              format = g_strdup ("%u");
            }
          else if (pspec->value_type == G_TYPE_ULONG)
            {
              format = g_strdup ("%lu");
            }
          else if (pspec->value_type == G_TYPE_DOUBLE)
            {
              format = g_strdup ("%f");
            }
          else if (pspec->value_type == G_TYPE_BOOLEAN)
            {
              format = g_strdup ("%b");
            }
          else if (pspec->value_type == G_TYPE_GTYPE)
            {
              format = g_strdup ("%t");
            }
          else if (g_type_is_a (pspec->value_type, G_TYPE_ENUM))
            {
              format = g_strdup_printf ("%%[%s]", g_type_name (pspec->value_type));
            }
          else
            {
              g_warning ("%s: unsupported GValue type: %s", G_STRFUNC,
                         g_type_name (pspec->value_type));
            }

          if (format)
            /* XXX No type validation with the "type" attribute so far. */
            gimp_savable_load_add_simple_handler (state, pspec->name, format,
                                                  NULL, NULL, NULL,
                                                  FALSE, FALSE,
                                                  "type", "%s",
                                                  NULL);

          g_free (format);
        }

      g_type_class_unref (klass);
      g_free (pspecs);

      data->config_type = config_type;
    }

  return TRUE;
}

static gboolean
gimp_savable_load_exit_config (GimpLoadState  *state,
                               const gchar    *text,
                               gsize           len,
                               gpointer        user_data,
                               GError        **error)
{
  GimpConfigHandlerData *data = user_data;

  if (data->config_type != G_TYPE_NONE)
    {
      GObject          *config;
      GObjectClass     *klass;
      GParamSpec      **pspecs;
      guint             n_pspecs = 0;
      GimpLoadContext  *context  = g_queue_peek_head (state->contexts);

      klass  = g_type_class_ref (data->config_type);
      pspecs = g_object_class_list_properties (klass, &n_pspecs);

      if (n_pspecs > 0)
        {
          const char **names;
          GValue      *values;
          gint         index = 0;

          names  = g_new0 (const char *, n_pspecs);
          values = g_new0 (GValue, n_pspecs);

          for (gint i = 0; i < n_pspecs; i++)
            {
              GParamSpec *pspec = pspecs[i];
              GValue     *gvalue;

              if (! (pspec->flags & GIMP_CONFIG_PARAM_SERIALIZE))
                continue;

              gvalue = gimp_savable_load_get_gvalue_in (state, context, pspec->name);
              if (gvalue)
                {
                  names[index] = pspec->name;
                  g_value_init (&values[index], G_VALUE_TYPE (gvalue));
                  g_value_copy (gvalue, &values[index++]);
                }
            }

          /* Adding the hard-coded properties! */
          for (gint i = 0; i < data->n_props; i++)
            {
              GValue gvalue = data->prop_values[i];

              names[index] = data->prop_names[i];
              g_value_init (&values[index], G_VALUE_TYPE (&gvalue));
              g_value_copy (&gvalue, &values[index++]);
            }

          config = g_object_new_with_properties (data->config_type,
                                                 index,
                                                 (const char **) names,
                                                 (const GValue *) values);

          for (gint i = 0; i < index; i++)
            g_value_unset (&values[i]);
          g_free (names);
          g_free (values);
        }
      else
        {
          config = g_object_new (data->config_type, NULL);
        }

      gimp_savable_load_store_value (state, data->element_name, config, g_object_unref);

      if (data->secondary_exit_handler)
        {
          data->secondary_exit_handler (state, text, len, user_data, error);
        }
      else
        {
          gimp_savable_load_bubble_up (state, data->element_name);
        }

      g_type_class_unref (klass);
      g_free (pspecs);
    }

  return TRUE;
}

static gboolean
gimp_savable_load_exit_parasite (GimpLoadState  *state,
                                 const gchar    *text,
                                 gsize           len,
                                 gpointer        user_data,
                                 GError        **error)
{
  const gchar *b64  = NULL;
  const gchar *name = NULL;
  gulong       flags;

  if (gimp_savable_load_get_values (state,
                                    "parasite",       &b64,
                                    "parasite:name",  &name,
                                    "parasite:flags", &flags,
                                    NULL))
    {
      if (g_strcmp0 (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0 ||
          g_strcmp0 (name, GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
        {
          /* These 2 parasites are outdated ways to set color profiles
           * to the image, so setting these would trigger unwanted
           * processing. We need to avoid this.
           */
          g_printerr ("%s: ignore outdated parasite '%s'.\n",
                      G_STRFUNC, name);
        }
      else
        {
          GimpParasite *parasite;
          guchar       *decoded;
          gsize         decoded_len;

          decoded = g_base64_decode (b64, &decoded_len);

          parasite = gimp_parasite_new (name, (guint32) flags,
                                        (guint32) decoded_len,
                                        (gconstpointer) decoded);

          if (GIMP_IS_IMAGE (user_data))
            gimp_image_parasite_attach (GIMP_IMAGE (user_data), parasite, FALSE);
          else if (GIMP_IS_ITEM (user_data))
            gimp_item_parasite_attach (GIMP_ITEM (user_data), parasite, FALSE);

          gimp_parasite_free (parasite);
          g_free (decoded);
        }
    }

  return TRUE;
}

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

  b64 = gimp_savable_validate_base64 (text, len, error);
  if (b64 == NULL)
    return FALSE;

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
  g_free (b64);

  return TRUE;
}

static gboolean
gimp_savable_exit_pixel (GimpLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  gchar *b64;

  b64 = gimp_savable_validate_base64 (text, len, error);
  if (b64 == NULL)
    return FALSE;

  gimp_savable_load_store_value (state, "pixel", (gpointer) b64, g_free);
  gimp_savable_load_bubble_up (state, "pixel");

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
