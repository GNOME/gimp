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


struct _GimpParasiteList
{
  GimpObject parent_instance;

  GPtrArray *parasites;
};


static void     gimp_parasite_list_finalize          (GObject     *object);
static gint64   gimp_parasite_list_get_memsize       (GimpObject  *object,
                                                      gint64      *gui_size);

static void     gimp_parasite_list_model_iface_init  (GListModelInterface *iface);
static void     gimp_parasite_list_config_iface_init (gpointer     iface,
                                                      gpointer     iface_data);
static gboolean gimp_parasite_list_serialize    (GimpConfig       *list,
                                                 GimpConfigWriter *writer,
                                                 gpointer          data);
static gboolean gimp_parasite_list_deserialize  (GimpConfig       *list,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);


G_DEFINE_TYPE_WITH_CODE (GimpParasiteList, gimp_parasite_list, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                gimp_parasite_list_model_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_parasite_list_config_iface_init))

#define parent_class gimp_parasite_list_parent_class

static const gchar  parasite_symbol[]                  = "parasite";


static void
gimp_parasite_list_class_init (GimpParasiteListClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_parasite_list_finalize;

  gimp_object_class->get_memsize = gimp_parasite_list_get_memsize;
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
  list->parasites = g_ptr_array_new_full (1, (GDestroyNotify) gimp_parasite_free);
}

static void
gimp_parasite_list_finalize (GObject *object)
{
  GimpParasiteList *list = GIMP_PARASITE_LIST (object);

  g_clear_pointer (&list->parasites, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GType
gimp_parasite_list_get_item_type (GListModel *list)
{
  return GIMP_TYPE_PARASITE;
}

static guint
gimp_parasite_list_get_n_items (GListModel *list)
{
  GimpParasiteList *self = GIMP_PARASITE_LIST (list);
  return self->parasites->len;
}

static gpointer
gimp_parasite_list_get_item (GListModel *list,
                             guint       index)
{
  GimpParasiteList *self = GIMP_PARASITE_LIST (list);

  if (index >= self->parasites->len)
    return NULL;
  return g_object_ref (g_ptr_array_index (self->parasites, index));
}

static void
gimp_parasite_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = gimp_parasite_list_get_item_type;
  iface->get_n_items = gimp_parasite_list_get_n_items;
  iface->get_item = gimp_parasite_list_get_item;
}

static gint64
gimp_parasite_list_get_memsize (GimpObject *object,
                                gint64     *gui_size)
{
  GimpParasiteList *list    = GIMP_PARASITE_LIST (object);
  gint64            memsize = 0;

  for (guint i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);

      memsize += gimp_parasite_get_memsize (parasite, gui_size);
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_parasite_list_serialize (GimpConfig       *config,
                              GimpConfigWriter *writer,
                              gpointer          data)
{
  GimpParasiteList *list = GIMP_PARASITE_LIST (config);
  guint             i;

  for (i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);
      const guint8 *parasite_contents;
      guint32       parasite_size;

      if (! gimp_parasite_is_persistent (parasite))
        continue;

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

  for (guint i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);

      gimp_parasite_list_add (newlist, parasite);
    }

  return newlist;
}

void
gimp_parasite_list_add (GimpParasiteList   *list,
                        const GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));
  g_return_if_fail (parasite != NULL);
  g_return_if_fail (parasite->name != NULL);

  gimp_parasite_list_remove (list, parasite->name);
  g_ptr_array_add (list->parasites, gimp_parasite_copy (parasite));
  g_list_model_items_changed (G_LIST_MODEL (list),
                              list->parasites->len - 1, 0, 1);
}

void
gimp_parasite_list_remove (GimpParasiteList *list,
                           const gchar      *name)
{
  const GimpParasite *parasite;
  guint               idx;

  g_return_if_fail (GIMP_IS_PARASITE_LIST (list));

  parasite = gimp_parasite_list_find_full (list, name, &idx);

  if (parasite)
    {
      g_ptr_array_remove_index (list->parasites, idx);
      g_list_model_items_changed (G_LIST_MODEL (list), idx, 1, 0);
    }
}

guint
gimp_parasite_list_persistent_length (GimpParasiteList *list)
{
  guint count = 0;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), 0);

  for (guint i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);

      if (gimp_parasite_is_persistent (parasite))
        count++;
    }

  return count;
}

const GimpParasite *
gimp_parasite_list_find_full (GimpParasiteList *list,
                              const gchar      *name,
                              guint            *index)
{
  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (guint i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);

      if (g_strcmp0 (parasite->name, name) == 0)
        {
          if (index != NULL)
            *index = i;
          return parasite;
        }
    }

  return NULL;
}

const GimpParasite *
gimp_parasite_list_find (GimpParasiteList *list,
                         const gchar      *name)
{
  return gimp_parasite_list_find_full (list, name, NULL);
}

gchar **
gimp_parasite_list_list_names (GimpParasiteList *list)
{
  gchar **names;

  g_return_val_if_fail (GIMP_IS_PARASITE_LIST (list), NULL);

  names = g_new0 (gchar *, list->parasites->len + 1);

  for (guint i = 0; i < list->parasites->len; i++)
    {
      GimpParasite *parasite = g_ptr_array_index (list->parasites, i);
      names[i] = g_strdup (parasite->name);
    }

  return names;
}
