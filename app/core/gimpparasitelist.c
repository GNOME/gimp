/* parasitelist.c: Copyright 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfigwriter.h"
#include "config/gimpscanner.h"

#include "gimp-utils.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"


enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};


static void     gimp_parasite_list_class_init  (GimpParasiteListClass *klass);
static void     gimp_parasite_list_init        (GimpParasiteList      *list);
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


static guint parasite_list_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;

static const gchar     *parasite_symbol = "parasite";


GType
gimp_parasite_list_get_type (void)
{
  static GType list_type = 0;

  if (! list_type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpParasiteListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_parasite_list_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpParasiteList),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_parasite_list_init,
      };
      static const GInterfaceInfo list_iface_info =
      {
        gimp_parasite_list_config_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      list_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                          "GimpParasiteList",
                                          &list_info, 0);
      g_type_add_interface_static (list_type, GIMP_TYPE_CONFIG,
                                   &list_iface_info);
    }

  return list_type;
}

static void
gimp_parasite_list_class_init (GimpParasiteListClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  parasite_list_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpParasiteListClass, add),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  parasite_list_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpParasiteListClass, remove),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
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

static void
gimp_parasite_list_get_memsize_foreach (gpointer key,
                                        gpointer p,
                                        gpointer m)
{
  GimpParasite *parasite = p;
  gint64       *memsize  = m;

  *memsize += (sizeof (GimpParasite) +
               strlen (parasite->name) + 1 +
               parasite->size);
}

static gint64
gimp_parasite_list_get_memsize (GimpObject *object,
                                gint64     *gui_size)
{
  GimpParasiteList *list    = GIMP_PARASITE_LIST (object);
  gint64            memsize = 0;

  if (list->table)
    {
      memsize += gimp_g_hash_table_get_memsize (list->table);

      g_hash_table_foreach (list->table,
                            gimp_parasite_list_get_memsize_foreach,
                            &memsize);
    }

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
                  parasite_data      = str;
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
gimp_parasite_list_copy (const GimpParasiteList *list)
{
  GimpParasiteList *newlist;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  newlist = gimp_parasite_list_new ();

  if (list->table)
    g_hash_table_foreach (list->table, (GHFunc) parasite_copy, newlist);

  return newlist;
}

void
gimp_parasite_list_add (GimpParasiteList *list,
                        GimpParasite     *parasite)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));
  g_return_if_fail (parasite != NULL);
  g_return_if_fail (parasite->name != NULL);

  if (list->table == NULL)
    list->table = g_hash_table_new (g_str_hash, g_str_equal);

  gimp_parasite_list_remove (list, parasite->name);
  parasite = gimp_parasite_copy (parasite);
  g_hash_table_insert (list->table, parasite->name, parasite);

  g_signal_emit (list, parasite_list_signals[ADD], 0, parasite);
}

void
gimp_parasite_list_remove (GimpParasiteList *list,
                           const gchar      *name)
{
  GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  if (list->table)
    {
      parasite = gimp_parasite_list_find (list, name);

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

GimpParasite *
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
  if (! gimp_parasite_is_persistent (parasite))
    return;

  gimp_config_writer_open (writer, parasite_symbol);

  gimp_config_writer_printf (writer, "\"%s\" %lu %lu",
                             gimp_parasite_name (parasite),
                             gimp_parasite_flags (parasite),
                             gimp_parasite_data_size (parasite));

  gimp_config_writer_data (writer,
                           gimp_parasite_data_size (parasite),
                           gimp_parasite_data (parasite));

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
