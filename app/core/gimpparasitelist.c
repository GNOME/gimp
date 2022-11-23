/* parasitelist.c: Copyright 1998 Jay Cox <jaycox@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmaparasitelist.h"


enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};


static void     ligma_parasite_list_finalize          (GObject     *object);
static gint64   ligma_parasite_list_get_memsize       (LigmaObject  *object,
                                                      gint64      *gui_size);

static void     ligma_parasite_list_config_iface_init (gpointer     iface,
                                                      gpointer     iface_data);
static gboolean ligma_parasite_list_serialize    (LigmaConfig       *list,
                                                 LigmaConfigWriter *writer,
                                                 gpointer          data);
static gboolean ligma_parasite_list_deserialize  (LigmaConfig       *list,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);

static void     parasite_serialize           (const gchar      *key,
                                              LigmaParasite     *parasite,
                                              LigmaConfigWriter *writer);
static void     parasite_copy                (const gchar      *key,
                                              LigmaParasite     *parasite,
                                              LigmaParasiteList *list);
static gboolean parasite_free                (const gchar      *key,
                                              LigmaParasite     *parasite,
                                              gpointer          unused);
static void     parasite_count_if_persistent (const gchar      *key,
                                              LigmaParasite     *parasite,
                                              gint             *count);


G_DEFINE_TYPE_WITH_CODE (LigmaParasiteList, ligma_parasite_list, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_parasite_list_config_iface_init))

#define parent_class ligma_parasite_list_parent_class

static guint        parasite_list_signals[LAST_SIGNAL] = { 0 };
static const gchar  parasite_symbol[]                  = "parasite";


static void
ligma_parasite_list_class_init (LigmaParasiteListClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  parasite_list_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaParasiteListClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  parasite_list_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaParasiteListClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->finalize         = ligma_parasite_list_finalize;

  ligma_object_class->get_memsize = ligma_parasite_list_get_memsize;

  klass->add                     = NULL;
  klass->remove                  = NULL;
}

static void
ligma_parasite_list_config_iface_init (gpointer  iface,
                                      gpointer  iface_data)
{
  LigmaConfigInterface *config_iface = (LigmaConfigInterface *) iface;

  config_iface->serialize   = ligma_parasite_list_serialize;
  config_iface->deserialize = ligma_parasite_list_deserialize;
}

static void
ligma_parasite_list_init (LigmaParasiteList *list)
{
  list->table = NULL;
}

static void
ligma_parasite_list_finalize (GObject *object)
{
  LigmaParasiteList *list = LIGMA_PARASITE_LIST (object);

  if (list->table)
    {
      g_hash_table_foreach_remove (list->table, (GHRFunc) parasite_free, NULL);
      g_hash_table_destroy (list->table);
      list->table = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_parasite_list_get_memsize (LigmaObject *object,
                                gint64     *gui_size)
{
  LigmaParasiteList *list    = LIGMA_PARASITE_LIST (object);
  gint64            memsize = 0;

  memsize += ligma_g_hash_table_get_memsize_foreach (list->table,
                                                    (LigmaMemsizeFunc)
                                                    ligma_parasite_get_memsize,
                                                    gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_parasite_list_serialize (LigmaConfig       *list,
                              LigmaConfigWriter *writer,
                              gpointer          data)
{
  if (LIGMA_PARASITE_LIST (list)->table)
    g_hash_table_foreach (LIGMA_PARASITE_LIST (list)->table,
                          (GHFunc) parasite_serialize,
                          writer);

  return TRUE;
}

static gboolean
ligma_parasite_list_deserialize (LigmaConfig *list,
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
              LigmaParasite *parasite;

              token = G_TOKEN_STRING;

              if (g_scanner_peek_next_token (scanner) != token)
                break;

              if (! ligma_scanner_parse_string (scanner, &parasite_name))
                break;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token)
                goto cleanup;

              if (! ligma_scanner_parse_int (scanner, &parasite_flags))
                goto cleanup;

              token = G_TOKEN_INT;

              if (g_scanner_peek_next_token (scanner) != token)
                {
                  /*  old format -- plain string  */

                  gchar *str;

                  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
                    goto cleanup;

                  if (! ligma_scanner_parse_string (scanner, &str))
                    goto cleanup;

                  parasite_data_size = strlen (str);
                  parasite_data      = (guint8 *) str;
                }
              else
                {
                  /*  new format -- properly encoded binary data  */

                  if (! ligma_scanner_parse_int (scanner, &parasite_data_size))
                    goto cleanup;

                  token = G_TOKEN_STRING;

                  if (g_scanner_peek_next_token (scanner) != token)
                    goto cleanup;

                  if (! ligma_scanner_parse_data (scanner, parasite_data_size,
                                                 &parasite_data))
                    goto cleanup;
                }

              parasite = ligma_parasite_new (parasite_name,
                                            parasite_flags,
                                            parasite_data_size,
                                            parasite_data);
              ligma_parasite_list_add (LIGMA_PARASITE_LIST (list),
                                      parasite);  /* adds a copy */
              ligma_parasite_free (parasite);

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

  return ligma_config_deserialize_return (scanner, token, nest_level);
}

LigmaParasiteList *
ligma_parasite_list_new (void)
{
  LigmaParasiteList *list;

  list = g_object_new (LIGMA_TYPE_PARASITE_LIST, NULL);

  return list;
}

LigmaParasiteList *
ligma_parasite_list_copy (LigmaParasiteList *list)
{
  LigmaParasiteList *newlist;

  g_return_val_if_fail (LIGMA_IS_PARASITE_LIST (list), NULL);

  newlist = ligma_parasite_list_new ();

  if (list->table)
    g_hash_table_foreach (list->table, (GHFunc) parasite_copy, newlist);

  return newlist;
}

void
ligma_parasite_list_add (LigmaParasiteList   *list,
                        const LigmaParasite *parasite)
{
  LigmaParasite *copy;

  g_return_if_fail (LIGMA_IS_PARASITE_LIST (list));
  g_return_if_fail (parasite != NULL);
  g_return_if_fail (parasite->name != NULL);

  if (list->table == NULL)
    list->table = g_hash_table_new (g_str_hash, g_str_equal);

  ligma_parasite_list_remove (list, parasite->name);
  copy = ligma_parasite_copy (parasite);
  g_hash_table_insert (list->table, copy->name, copy);

  g_signal_emit (list, parasite_list_signals[ADD], 0, copy);
}

void
ligma_parasite_list_remove (LigmaParasiteList *list,
                           const gchar      *name)
{
  g_return_if_fail (LIGMA_IS_PARASITE_LIST (list));

  if (list->table)
    {
      LigmaParasite *parasite;

      parasite = (LigmaParasite *) ligma_parasite_list_find (list, name);

      if (parasite)
        {
          g_hash_table_remove (list->table, name);

          g_signal_emit (list, parasite_list_signals[REMOVE], 0, parasite);

          ligma_parasite_free (parasite);
        }
    }
}

gint
ligma_parasite_list_length (LigmaParasiteList *list)
{
  g_return_val_if_fail (LIGMA_IS_PARASITE_LIST (list), 0);

  if (! list->table)
    return 0;

  return g_hash_table_size (list->table);
}

gint
ligma_parasite_list_persistent_length (LigmaParasiteList *list)
{
  gint len = 0;

  g_return_val_if_fail (LIGMA_IS_PARASITE_LIST (list), 0);

  if (! list->table)
    return 0;

  ligma_parasite_list_foreach (list,
                              (GHFunc) parasite_count_if_persistent, &len);

  return len;
}

void
ligma_parasite_list_foreach (LigmaParasiteList *list,
                            GHFunc            function,
                            gpointer          user_data)
{
  g_return_if_fail (LIGMA_IS_PARASITE_LIST (list));

  if (! list->table)
    return;

  g_hash_table_foreach (list->table, function, user_data);
}

const LigmaParasite *
ligma_parasite_list_find (LigmaParasiteList *list,
                         const gchar      *name)
{
  g_return_val_if_fail (LIGMA_IS_PARASITE_LIST (list), NULL);

  if (list->table)
    return (LigmaParasite *) g_hash_table_lookup (list->table, name);

  return NULL;
}


static void
parasite_serialize (const gchar      *key,
                    LigmaParasite     *parasite,
                    LigmaConfigWriter *writer)
{
  const guint8 *parasite_contents;
  guint32       parasite_size;

  if (! ligma_parasite_is_persistent (parasite))
    return;

  ligma_config_writer_open (writer, parasite_symbol);

  parasite_contents = ligma_parasite_get_data (parasite, &parasite_size);
  ligma_config_writer_printf (writer, "\"%s\" %lu %lu",
                             ligma_parasite_get_name (parasite),
                             ligma_parasite_get_flags (parasite),
                             (long unsigned int) parasite_size);

  ligma_config_writer_data (writer, parasite_size, parasite_contents);

  ligma_config_writer_close (writer);
  ligma_config_writer_linefeed (writer);
}

static void
parasite_copy (const gchar      *key,
               LigmaParasite     *parasite,
               LigmaParasiteList *list)
{
  ligma_parasite_list_add (list, parasite);
}

static gboolean
parasite_free (const gchar  *key,
               LigmaParasite *parasite,
               gpointer     unused)
{
  ligma_parasite_free (parasite);

  return TRUE;
}

static void
parasite_count_if_persistent (const gchar  *key,
                              LigmaParasite *parasite,
                              gint         *count)
{
  if (ligma_parasite_is_persistent (parasite))
    *count = *count + 1;
}
