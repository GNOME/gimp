/*
 * gimp-thumbnail-list.c
 */

#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgimpthumb/gimpthumb.h>


#define STATE_NONE  -1
#define STATE_ERROR -2


static gboolean  parse_option_state (const gchar  *option_name,
                                     const gchar  *value,
                                     gpointer      data,
                                     GError      **error);
static gboolean  parse_option_path  (const gchar  *option_name,
                                     const gchar  *value,
                                     gpointer      data,
                                     GError      **error);
static void      process_folder     (const gchar  *folder);
static void      process_thumbnail  (const gchar  *filename);


static GimpThumbState  option_state   = STATE_NONE;
static gboolean        option_verbose = FALSE;
static gchar          *option_path    = NULL;


static const GOptionEntry main_entries[] =
{
  {
    "state", 's', 0,
    G_OPTION_ARG_CALLBACK, parse_option_state,
    "Filter by thumbnail state "
    "(unknown|remote|folder|special|not-found|exists|old|failed|ok|error)",
    "<state>"
  },
  {
    "path", 'p', 0,
    G_OPTION_ARG_CALLBACK, parse_option_path,
    "Filter by original file's path",
    "<path>"
  },
  {
    "verbose", 'v', 0,
    G_OPTION_ARG_NONE, &option_verbose,
    "Print additional info per matched file", NULL
  },
  { NULL }
};


gint
main (gint   argc,
      gchar *argv[])
{
  GOptionContext *context;
  GDir           *dir;
  const gchar    *thumb_folder;
  const gchar    *folder;
  GError         *error = NULL;

  gimp_thumb_init ("gimp-thumbnail-list", NULL);

  thumb_folder = gimp_thumb_get_thumb_base_dir ();

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, main_entries, NULL);

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return -1;
    }

  dir = g_dir_open (thumb_folder, 0, &error);

  if (! dir)
    g_error ("Error opening %s: %s", thumb_folder, error->message);

  while ((folder = g_dir_read_name (dir)))
    {
      gchar *filename;

      filename = g_build_filename (thumb_folder, folder, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_DIR))
        process_folder (filename);

      g_free (filename);
    }

  g_dir_close (dir);

  return 0;
}

static gboolean
parse_option_state (const gchar  *option_name,
                    const gchar  *value,
                    gpointer      data,
                    GError      **error)
{
  if (strcmp (value, "unknown") == 0)
    option_state = GIMP_THUMB_STATE_UNKNOWN;
  else if (strcmp (value, "remote") == 0)
    option_state = GIMP_THUMB_STATE_REMOTE;
  else if (strcmp (value, "folder") == 0)
    option_state = GIMP_THUMB_STATE_FOLDER;
  else if (strcmp (value, "special") == 0)
    option_state = GIMP_THUMB_STATE_SPECIAL;
  else if (strcmp (value, "not-found") == 0)
    option_state = GIMP_THUMB_STATE_NOT_FOUND;
  else if (strcmp (value, "exists") == 0)
    option_state = GIMP_THUMB_STATE_EXISTS;
  else if (strcmp (value, "old") == 0)
    option_state = GIMP_THUMB_STATE_OLD;
  else if (strcmp (value, "failed") == 0)
    option_state = GIMP_THUMB_STATE_FAILED;
  else if (strcmp (value, "ok") == 0)
    option_state = GIMP_THUMB_STATE_OK;
  else if (strcmp (value, "error") == 0)
    option_state = STATE_ERROR;
  else
    return FALSE;

  return TRUE;
}

static gboolean
parse_option_path (const gchar  *option_name,
                   const gchar  *value,
                   gpointer      data,
                   GError      **error)
{
  option_path = g_strdup (value);

  return TRUE;
}

static void
process_folder (const gchar *folder)
{
  GDir        *dir;
  const gchar *name;
  GError      *error = NULL;

#if 0
  g_print ("processing folder: %s\n", folder);
#endif

  dir = g_dir_open (folder, 0, &error);

  if (! dir)
    {
      g_printerr ("Error opening '%s': %s", folder, error->message);
      return;
    }

  while ((name = g_dir_read_name (dir)))
    {
      gchar *filename;

      filename = g_build_filename (folder, name, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_DIR))
        process_folder (filename);
      else
        process_thumbnail (filename);

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
process_thumbnail (const gchar *filename)
{
  GimpThumbnail *thumbnail;
  GError        *error = NULL;

  thumbnail = gimp_thumbnail_new ();

  if (! gimp_thumbnail_set_from_thumb (thumbnail, filename, &error))
    {
      if (option_state == STATE_ERROR)
        {
          if (option_verbose)
            g_print ("%s '%s'\n", filename, error->message);
          else
            g_print ("%s\n", filename);
        }

      g_clear_error (&error);
    }
  else
    {
      GimpThumbState state = gimp_thumbnail_peek_image (thumbnail);

      if ((option_state == STATE_NONE || state == option_state)

          &&

          (option_path == NULL ||
           strstr (thumbnail->image_uri, option_path)))
        {
          if (option_verbose)
            g_print ("%s '%s'\n", filename, thumbnail->image_uri);
          else
            g_print ("%s\n", filename);
        }

#if 0
      switch (foo)
        {
        case GIMP_THUMB_STATE_REMOTE:
          g_print ("%s Remote image '%s'\n", filename, thumbnail->image_uri);
          break;

        case GIMP_THUMB_STATE_FOLDER:
          g_print ("%s Folder '%s'\n", filename, thumbnail->image_uri);
          break;

        case GIMP_THUMB_STATE_SPECIAL:
          g_print ("%s Special file '%s'\n", filename, thumbnail->image_uri);
          break;

        case GIMP_THUMB_STATE_NOT_FOUND:
          g_print ("%s Image not found '%s'\n", filename, thumbnail->image_uri);
          break;

        case GIMP_THUMB_STATE_OLD:
          g_print ("%s Thumbnail old '%s'\n", filename, thumbnail->image_uri);
          break;

        case GIMP_THUMB_STATE_FAILED:
          g_print ("%s EEEEEEEEK '%s'\n", filename, thumbnail->image_uri);
          break;

        default:
          g_print ("%s '%s'\n", filename, thumbnail->image_uri);
          break;
        }
#endif
    }

  g_object_unref (thumbnail);
}
