/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Recent File Storage,
 * see http://freedesktop.org/Standards/recent-file-spec/
 *
 * This code is taken from libegg and has been adapted to the GIMP needs.
 * The original author is James Willcox <jwillcox@cs.indiana.edu>,
 * responsible for bugs in this version is Sven Neumann <sven@gimp.org>.
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

#define _GNU_SOURCE  /* need lockf() and F_TLOCK/F_ULOCK */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#ifndef G_OS_WIN32  /* This code doesn't compile on win32 and the use of
                     * the freedesktop standard doesn't make much sense
                     * there anyway. If someone wants to contribute a win32
                     * specific implementation, that would be appreciated.
                     */

#include "config/config-types.h"

#include "config/gimpxmlparser.h"

#include "gimprecentitem.h"
#include "gimprecentlist.h"


#define GIMP_RECENT_LIST_FILE_NAME      ".recently-used"
#define GIMP_RECENT_LIST_MAX_ITEMS      500
#define GIMP_RECENT_LIST_GROUP_GIMP     "gimp"


#define GIMP_RECENT_ITEM_LIST_UNREF(list) \
  g_list_foreach (list, (GFunc) gimp_recent_item_unref, NULL);  \
  g_list_free (list);


typedef struct
{
  GSList         *states;
  GList          *items;
  GimpRecentItem *current_item;
} ParseInfo;

typedef enum
{
  STATE_START,
  STATE_RECENT_FILES,
  STATE_RECENT_ITEM,
  STATE_URI,
  STATE_MIME_TYPE,
  STATE_TIMESTAMP,
  STATE_PRIVATE,
  STATE_GROUPS,
  STATE_GROUP
} ParseState;


#define TAG_RECENT_FILES "RecentFiles"
#define TAG_RECENT_ITEM  "RecentItem"
#define TAG_URI          "URI"
#define TAG_MIME_TYPE    "Mime-Type"
#define TAG_TIMESTAMP    "Timestamp"
#define TAG_PRIVATE      "Private"
#define TAG_GROUPS       "Groups"
#define TAG_GROUP        "Group"


static void  start_element_handler (GMarkupParseContext  *context,
                                    const gchar          *element_name,
                                    const gchar         **attribute_names,
                                    const gchar         **attribute_values,
                                    gpointer              user_data,
                                    GError              **error);
static void  end_element_handler   (GMarkupParseContext  *context,
                                    const gchar          *element_name,
                                    gpointer              user_data,
                                    GError              **error);
static void  text_handler          (GMarkupParseContext  *context,
                                    const gchar          *text,
                                    gsize                 text_len,
                                    gpointer              user_data,
                                    GError              **error);

static const GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

static void
gimp_recent_list_add_new_groups (GimpRecentItem *item,
                                 GimpRecentItem *upd_item)
{
  const GList *tmp;

  for (tmp = gimp_recent_item_get_groups (upd_item); tmp; tmp = tmp->next)
    {
      const gchar *group = tmp->data;

      if (! gimp_recent_item_in_group (item, group))
        gimp_recent_item_add_group (item, group);
    }
}

static gboolean
gimp_recent_list_update_item (GList          *items,
                              GimpRecentItem *upd_item)
{
  const char *uri = gimp_recent_item_get_uri (upd_item);
  GList      *tmp;

  for (tmp = items; tmp; tmp = tmp->next)
    {
      GimpRecentItem *item = tmp->data;

      /*  gnome_vfs_uris_match (gimp_recent_item_get_uri (item), uri)  */
      if (strcmp (gimp_recent_item_get_uri (item), uri) == 0)
        {
          gimp_recent_item_set_timestamp (item, (time_t) -1);
          gimp_recent_list_add_new_groups (item, upd_item);

          return TRUE;
        }
    }

  return FALSE;
}

static void
parse_info_init (ParseInfo *info)
{
  info->states = g_slist_prepend (NULL, GINT_TO_POINTER (STATE_START));
  info->items = NULL;
}

static void
parse_info_free (ParseInfo *info)
{
  g_slist_free (info->states);
}

static void
push_state (ParseInfo  *info,
            ParseState  state)
{
  info->states = g_slist_prepend (info->states, GINT_TO_POINTER (state));
}

static void
pop_state (ParseInfo *info)
{
  g_return_if_fail (info->states != NULL);

  info->states = g_slist_remove (info->states, info->states->data);
}

static ParseState
peek_state (ParseInfo *info)
{
  g_return_val_if_fail (info->states != NULL, STATE_START);

  return GPOINTER_TO_INT (info->states->data);
}

#define ELEMENT_IS(name) (strcmp (element_name, (name)) == 0)

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  ParseInfo *info = user_data;

  if (ELEMENT_IS (TAG_RECENT_FILES))
    {
      push_state (info, STATE_RECENT_FILES);
    }
  else if (ELEMENT_IS (TAG_RECENT_ITEM))
    {
      info->current_item = gimp_recent_item_new ();
      push_state (info, STATE_RECENT_ITEM);
    }
  else if (ELEMENT_IS (TAG_URI))
    {
      push_state (info, STATE_URI);
    }
  else if (ELEMENT_IS (TAG_MIME_TYPE))
    {
      push_state (info, STATE_MIME_TYPE);
    }
  else if (ELEMENT_IS (TAG_TIMESTAMP))
    {
      push_state (info, STATE_TIMESTAMP);
    }
  else if (ELEMENT_IS (TAG_PRIVATE))
    {
      push_state (info, STATE_PRIVATE);
      gimp_recent_item_set_private (info->current_item, TRUE);
    }
  else if (ELEMENT_IS (TAG_GROUPS))
    {
      push_state (info, STATE_GROUPS);
    }
  else if (ELEMENT_IS (TAG_GROUP))
    {
      push_state (info, STATE_GROUP);
    }
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_RECENT_ITEM:
      info->items = g_list_prepend (info->items, info->current_item);
      break;

    default:
      break;
    }

  pop_state (info);
}

static void
text_handler (GMarkupParseContext  *context,
              const gchar          *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  ParseInfo *info = user_data;

  switch (peek_state (info))
    {
    case STATE_START:
    case STATE_RECENT_FILES:
    case STATE_RECENT_ITEM:
    case STATE_PRIVATE:
    case STATE_GROUPS:
      break;

    case STATE_URI:
      gimp_recent_item_set_uri (info->current_item, text);
      break;

    case STATE_MIME_TYPE:
      gimp_recent_item_set_mime_type (info->current_item, text);
      break;

    case STATE_TIMESTAMP:
      gimp_recent_item_set_timestamp (info->current_item, (time_t) atoi (text));
      break;

    case STATE_GROUP:
      gimp_recent_item_add_group (info->current_item, text);
      break;
    }
}

static void
gimp_recent_list_enforce_limit (GList *list,
                                gint   limit)
{
  gint   len;
  GList *end;

  /* limit < 0 means unlimited */
  if (limit <= 0)
    return;

  len = g_list_length (list);

  if (len > limit)
    {
      GList *next;

      end = g_list_nth (list, limit-1);
      next = end->next;

      end->next = NULL;

      GIMP_RECENT_ITEM_LIST_UNREF (next);
    }
}

static GList *
gimp_recent_list_read (gint fd)
{
  GimpXmlParser  *parser;
  GList          *list;
  ParseInfo       info;
  GError         *error = NULL;

  lseek (fd, 0, SEEK_SET);

  parse_info_init (&info);

  parser = gimp_xml_parser_new (&markup_parser, &info);

  if (! gimp_xml_parser_parse_fd (parser, fd, &error))
    {
      g_printerr ("%s", error->message);
      g_error_free (error);
    }

  gimp_xml_parser_free (parser);

  list = info.items;

  parse_info_free (&info);

  return g_list_reverse (list);
}

static gboolean
gimp_recent_list_write_raw (gint         fd,
                            const gchar *content,
                            gssize       len)
{
  struct stat  sbuf;
  gssize       remaining = len;

  lseek (fd, 0, SEEK_SET);

  if (fstat (fd, &sbuf) < 0)
    {
      g_warning ("Couldn't stat XML document.");
    }
  else
    {
      if ((off_t) len < sbuf.st_size)
        ftruncate (fd, len);
    }

  while (remaining > 0)
    {
      gssize  written = write (fd, content, remaining);

      if (written < 0 && errno != EINTR)
        return FALSE;

      remaining -= written;
    }

  fsync (fd);

  return TRUE;
}

static gboolean
gimp_recent_list_write (gint   fd,
                        GList *list)
{
  GString  *string;
  gboolean  success;

  string = g_string_new ("<?xml version=\"1.0\"?>\n");
  string = g_string_append (string, "<" TAG_RECENT_FILES ">\n");

  while (list)
    {
      GimpRecentItem *item = list->data;
      const GList    *groups;
      gchar          *uri;
      const gchar    *mime_type;
      gchar          *escaped_uri;
      time_t          timestamp;

      uri = gimp_recent_item_get_uri_utf8 (item);
      escaped_uri = g_markup_escape_text (uri, strlen (uri));
      g_free (uri);

      mime_type = gimp_recent_item_get_mime_type (item);
      timestamp = gimp_recent_item_get_timestamp (item);

      string = g_string_append (string, "  <" TAG_RECENT_ITEM ">\n");

      g_string_append_printf (string,
                              "    <" TAG_URI ">%s</" TAG_URI ">\n", escaped_uri);

      if (mime_type)
        g_string_append_printf (string,
                                "    <" TAG_MIME_TYPE ">%s</" TAG_MIME_TYPE ">\n", mime_type);
      else
        g_string_append_printf (string,
                                "    <" TAG_MIME_TYPE "></" TAG_MIME_TYPE ">\n");

      g_string_append_printf (string,
                              "    <" TAG_TIMESTAMP ">%d</" TAG_TIMESTAMP ">\n", (gint) timestamp);

      if (gimp_recent_item_get_private (item))
        string = g_string_append (string,
                                  "    <" TAG_PRIVATE "/>\n");

      groups = gimp_recent_item_get_groups (item);

      if (groups)
        {
          /* write the groups */
          string = g_string_append (string,
                                    "    <" TAG_GROUPS ">\n");

          if (groups == NULL && gimp_recent_item_get_private (item))
            g_warning ("Item with URI \"%s\" marked as private, but"
                       " does not belong to any groups.\n", uri);

          while (groups)
            {
              const gchar *group = groups->data;
              gchar       *escaped_group;

              escaped_group = g_markup_escape_text (group, strlen(group));

              g_string_append_printf (string,
                                      "      <" TAG_GROUP ">%s</" TAG_GROUP ">\n",
                                      escaped_group);

              g_free (escaped_group);

              groups = groups->next;
            }

          string = g_string_append (string, "    </" TAG_GROUPS ">\n");
        }

      string = g_string_append (string,
                                "  </" TAG_RECENT_ITEM ">\n");

      g_free (escaped_uri);

      list = list->next;
    }

  string = g_string_append (string, "</" TAG_RECENT_FILES ">");

  success = gimp_recent_list_write_raw (fd, string->str, string->len);

  g_string_free (string, TRUE);

  return success;
}

static gboolean
gimp_recent_list_lock_file (gint fd)
{
  gint        i;

  /* Attempt to lock the file 5 times,
   * waiting a random interval (< 1 second)
   * in between attempts.
   * We should really be doing asynchronous
   * locking, but requires substantially larger
   * changes.
   */

  lseek (fd, 0, SEEK_SET);

  for (i = 0; i < 5; i++)
    {
      gint rand_interval;

      if (lockf (fd, F_TLOCK, 0) == 0)
        return TRUE;

      rand_interval = 1 + (gint) (10.0 * rand () / (RAND_MAX + 1.0));

      g_usleep (100000 * rand_interval);
    }

  return FALSE;
}

static gboolean
gimp_recent_list_unlock_file (gint fd)
{
  lseek (fd, 0, SEEK_SET);

  return (lockf (fd, F_ULOCK, 0) == 0) ? TRUE : FALSE;
}

static gboolean
gimp_recent_list_add_item (GimpRecentItem *item)
{
  const gchar *home;
  gchar       *filename;
  gint         fd;
  gboolean     success = FALSE;
  gboolean     created = FALSE;
  gboolean     updated = FALSE;

  home = g_get_home_dir ();
  if (! home)
    return FALSE;

  filename = g_build_filename (home, GIMP_RECENT_LIST_FILE_NAME, NULL);

  fd = g_open (filename, O_RDWR, 0);

  if (fd < 0)
    {
      fd = g_creat (filename, S_IRUSR | S_IWUSR);
      created = TRUE;
    }

  g_free (filename);

  if (fd < 0)
    return FALSE;

  if (gimp_recent_list_lock_file (fd))
    {
      GList *list = NULL;

      if (! created)
        list = gimp_recent_list_read (fd);

      /* if it's already there, we just update it */
      updated = gimp_recent_list_update_item (list, item);

      if (!updated)
        {
          list = g_list_prepend (list, item);
          gimp_recent_list_enforce_limit (list, GIMP_RECENT_LIST_MAX_ITEMS);
        }

      /* write new stuff */
      if (!gimp_recent_list_write (fd, list))
        g_warning ("Write failed: %s", g_strerror (errno));

      if (!updated)
        list = g_list_remove (list, item);

      GIMP_RECENT_ITEM_LIST_UNREF (list);
      success = TRUE;
    }
  else
    {
      g_warning ("Failed to lock:  %s", g_strerror (errno));
      close (fd);
      return FALSE;
    }

  if (! gimp_recent_list_unlock_file (fd))
    g_warning ("Failed to unlock: %s", strerror (errno));

  close (fd);

  return success;
}

/**
 * gimp_recent_list_add_uri:
 * @uri:       an URI
 * @mime_type: a MIME type
 *
 * This function adds an item to the list of recently used URIs.
 * See http://freedesktop.org/Standards/recent-file-spec/.
 *
 * On the Win32 platform, this call is unimplemented and will always
 * fail.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
gimp_recent_list_add_uri (const gchar *uri,
                          const gchar *mime_type)
{
  GimpRecentItem *item;
  gboolean        success;

  g_return_val_if_fail (uri != NULL, FALSE);

  if (! mime_type          ||
      ! strlen (mime_type) ||
      ! g_utf8_validate (mime_type, -1, NULL))
    return FALSE;

  item = gimp_recent_item_new_from_uri (uri);
  if (! item)
    return FALSE;

  gimp_recent_item_set_mime_type (item, mime_type);
  gimp_recent_item_set_timestamp (item, -1);
  gimp_recent_item_add_group (item, GIMP_RECENT_LIST_GROUP_GIMP);

  success = gimp_recent_list_add_item (item);

  gimp_recent_item_unref (item);

  return success;
}

#else  /* G_OS_WIN32  */

gboolean
gimp_recent_list_add_uri (const gchar *uri,
                          const gchar *mime_type)
{
  return FALSE;
}

#endif
