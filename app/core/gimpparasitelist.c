/* parasitelist.c: Copyright 1998 Jay Cox <jaycox@gimp.org>
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

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpparasitelist.h"


enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};


static void     gimp_parasite_list_finalize          (GObject     *object);
static gint64   gimp_parasite_list_get_memsize       (GimpObject  *object,
                                                      gint64      *gui_size);

static void     gimp_parasite_list_config_iface_init (gpointer     iface,
                                                      gpointer     iface_data);
static gboolean gimp_parasite_list_serialize    (GimpConfig       *list,
                                                 GimpConfigWriter *writer,
                                                 gpointer          data);
static gboolean gimp_parasite_list_deserialize  (GimpConfig       *list,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);

static void     parasite_serialize           (const gchar      *key,
                                              GimpParasite     *parasite,
                                              GimpConfigWriter *writer);
static void     parasite_copy                (const gchar      *key,
                                              GimpParasite     *parasite,
                                              GimpParasiteList *list);
static gboolean parasite_free                (const gchar      *key,
                                              GimpParasite     *parasite,
                                              gpointer          unused);
static void     parasite_count_if_persistent (const gchar      *key,
                                              GimpParasite     *parasite,
                                              gint             *count);


G_DEFINE_TYPE_WITH_CODE (GimpParasiteList, gimp_parasite_list, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_parasite_list_config_iface_init))

#define parent_class gimp_parasite_list_parent_class

static guint        parasite_list_signals[LAST_SIGNAL] = { 0 };
static const gchar  parasite_symbol[]                  = "parasite";


static void
gimp_parasite_list_class_init (GimpParasiteListClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parasite_list_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpParasiteListClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  parasite_list_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpParasiteListClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->finalize         = gimp_parasite_list_finalize;

  gimp_object_class->get_memsize = gimp_parasite_list_get_memsize;

  klass->add                     = NULL;
  klass->remove                  = NULL;
}

static void
gimp_parasite_list_config_iface_init (gpointer  iface,
                                      gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->serialize   = gimp_parasite_list_serialize;
  config_iface->deserialize = gimp_parasite_list_deserialize;
}

static void
gimp_parasite_list_init (GimpParasiteList *list)
{
  list->table = NULL;
}

static void
gimp_parasite_list_finalize (GObject *object)
{
  GimpParasiteList *list = GIMP_PARASITE_LIST (object);

  if (list->table)
    {
      g_hash_table_foreach_remove (list->table, (GHRFunc) parasite_free, NULL);
      g_hash_table_destroy (list->table);
      list->table = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_parasite_list_get_memsize (GimpObject *object,
                                gint64     *gui_size)
{
  GimpParasiteList *list    = GIMP_PARASITE_LIST (object);
  gint64            memsize = 0;

  memsize += gimp_g_hash_table_get_memsize_foreach (list->table,
                                                    (GimpMemsizeFunc)
                                                    gimp_parasite_get_memsize,
                                                    gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_parasite_list_serialize (GimpConfig       *list,
                              GimpConfigWriter *writer,
                              gpointer          data)
{
  if (GIMP_PARASITE_LIST (list)->table)
    g_hash_table_foreach (GIMP_PARASITE_LIST (list)->table,
                          (GHFunc) parasite_serialize,
                          writer);

  return TRUE;
}

static gboolean
gimp_parasite_list_deserialize (GimpConfig *list,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  GTokenType token;

  g_scanner_scope_add_symbol (scanner, 0,
                              parasite_symbol, (gpointer) parasite_symbol);

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == parasite_symbol)
            {
              gchar        *parasite_name      = NULL;
              gint          parasite_flags     = 0;
              guint8       *parasite_data      = NULL;
              gint          parasite_data_size = 0;
              GimpParasite *parasite;

              token = G_TOKEN_STRING;

              if (g_scanner_peek_next_token (scanner) != token)
                break;

              if (! gimp_scanner_parse_string (scanner, &parasite_name))
                break;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token)
                goto cleanup;

              if (! gimp_scanner_parse_int (scanner, &parasite_flags))
                goto cleanup;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token)
                {
                  /*  old format -- plain string  */

                  gchar *str;

                  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
                    goto cleanup;

                  if (! gimp_scanner_parse_string (scanner, &str))
                    goto cleanup;

                  parasite_data_size = strlen (str);
                  parasite_data      = (guint8 *) str;
                }
              else
                {
                  /*  new format -- properly encoded binary data  */

                  if (! gimp_scanner_parse_int (scanner, &parasite_data_size))
                    goto cleanup;

                  token = G_TOKEN_STRING;

                  if (g_scanner_peek_next_token (scanner) != token)
                    goto cleanup;

                  if (! gimp_scanner_parse_data (scanner, parasite_data_size,
                                                 &parasite_data))
                    goto cleanup;
                }

              parasite = gimp_parasite_new (parasite_name,
                                            parasite_flags,
                                            parasite_data_size,
                                            parasite_data);
              gimp_parasite_list_add (GIMP_PARASITE_LIST (list),
                                      parasite);  /* adds a copy */
              gimp_parasite_free (parasite);

              token = G_TOKEN_RIGHT_PAREN;

              g_free (parasite_data);
            cleanup:
              g_free (parasite_name);
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

GimpParasiteList *
gimp_parasite_list_new (void)
{
  GimpParasiteList *list;

  list = g_object_new (GIMP_TYPE_PARASITE_LIST, NULL);

  return list;
}

GimpParasiteList *
gimp_parasite_list_copy (GimpParasiteList *list)
{
  GimpParasiteList *newlist;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  newlist = gimp_parasite_list_new ();

  if (list->table)
    g_hash_table_foreach (list->table, (GHFunc) parasite_copy, newlist);

  return newlist;
}

void
gimp_parasite_list_add (GimpParasiteList   *list,
                        const GimpParasite *parasite)
{
  GimpParasite *copy;

  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));
  g_return_if_fail (parasite != NULL);
  g_return_if_fail (parasite->name != NULL);

  if (list->table == NULL)
    list->table = g_hash_table_new (g_str_hash, g_str_equal);

  gimp_parasite_list_remove (list, parasite->name);
  copy = gimp_parasite_copy (parasite);
  g_hash_table_insert (list->table, copy->name, copy);

  g_signal_emit (list, parasite_list_signals[ADD], 0, copy);
}

void
gimp_parasite_list_remove (GimpParasiteList *list,
                           const gchar      *name)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  if (list->table)
    {
      GimpParasite *parasite;

      parasite = (GimpParasite *) gimp_parasite_list_find (list, name);

      if (parasite)
        {
          g_hash_table_remove (list->table, name);

          g_signal_emit (list, parasite_list_signals[REMOVE], 0, parasite);

          gimp_parasite_free (parasite);
        }
    }
}

gint
gimp_parasite_list_length (GimpParasiteList *list)
{
  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), 0);

  if (! list->table)
    return 0;

  return g_hash_table_size (list->table);
}

gint
gimp_parasite_list_persistent_length (GimpParasiteList *list)
{
  gint len = 0;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), 0);

  if (! list->table)
    return 0;

  gimp_parasite_list_foreach (list,
                              (GHFunc) parasite_count_if_persistent, &len);

  return len;
}

void
gimp_parasite_list_foreach (GimpParasiteList *list,
                            GHFunc            function,
                            gpointer          user_data)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  if (! list->table)
    return;

  g_hash_table_foreach (list->table, function, user_data);
}

const GimpParasite *
gimp_parasite_list_find (GimpParasiteList *list,
                         const gchar      *name)
{
  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  if (list->table)
    return (GimpParasite *) g_hash_table_lookup (list->table, name);

  return NULL;
}


static void
parasite_serialize (const gchar      *key,
                    GimpParasite     *parasite,
                    GimpConfigWriter *writer)
{
  const guint8 *parasite_contents;
  guint32       parasite_size;

  if (! gimp_parasite_is_persistent (parasite))
    return;

  gimp_config_writer_open (writer, parasite_symbol);

  parasite_contents = gimp_parasite_get_data (parasite, &parasite_size);
  gimp_config_writer_printf (writer, "\"%s\" %lu %lu",
                             gimp_parasite_get_name (parasite),
                             gimp_parasite_get_flags (parasite),
                             (long unsigned int) parasite_size);

  gimp_config_writer_data (writer, parasite_size, parasite_contents);

  gimp_config_writer_close (writer);
  gimp_config_writer_linefeed (writer);
}

static void
parasite_copy (const gchar      *key,
               GimpParasite     *parasite,
               GimpParasiteList *list)
{
  gimp_parasite_list_add (list, parasite);
}

static gboolean
parasite_free (const gchar  *key,
               GimpParasite *parasite,
               gpointer     unused)
{
  gimp_parasite_free (parasite);

  return TRUE;
}

static void
parasite_count_if_persistent (const gchar  *key,
                              GimpParasite *parasite,
                              gint         *count)
{
  if (gimp_parasite_is_persistent (parasite))
    *count = *count + 1;
}
