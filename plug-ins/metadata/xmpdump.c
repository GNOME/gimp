/* simple program to test the XMP parser on files given on the command line */

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "xmp-parse.h"
#include "xmp-encode.h"


static gpointer
start_schema (XMPParseContext     *context,
              const gchar         *ns_uri,
              const gchar         *ns_prefix,
              gpointer             user_data,
              GError             **error)
{
  g_print ("Schema %s = \"%s\"\n", ns_prefix, ns_uri);
  return (gpointer) ns_prefix;
}

static void
end_schema (XMPParseContext     *context,
            gpointer             user_ns_data,
            gpointer             user_data,
            GError             **error)
{
  /* g_print ("End of %s\n", user_ns_prefix); */
}

static void
set_property (XMPParseContext     *context,
              const gchar         *name,
              XMPParseType         type,
              const gchar        **value,
              gpointer             user_ns_data,
              gpointer             user_data,
              GError             **error)
{
  const gchar *ns_prefix = user_ns_data;
  int          i;

  switch (type)
    {
    case XMP_PTYPE_TEXT:
      g_print ("\t%s:%s = \"%s\"\n", ns_prefix, name,
               value[0]);
      break;

    case XMP_PTYPE_RESOURCE:
      g_print ("\t%s:%s @ = \"%s\"\n", ns_prefix, name,
               value[0]);
      break;

    case XMP_PTYPE_ORDERED_LIST:
    case XMP_PTYPE_UNORDERED_LIST:
      g_print ("\t%s:%s [] =", ns_prefix, name);
      for (i = 0; value[i] != NULL; i++)
        if (i == 0)
          g_print (" \"%s\"", value[i]);
      else
          g_print (", \"%s\"", value[i]);
      g_print ("\n");
      break;

    case XMP_PTYPE_ALT_THUMBS:
      for (i = 0; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [size = %d] = \"...\"\n", ns_prefix, name,
                 *(int*)(value[i])); /* FIXME: show part of image */
      break;

    case XMP_PTYPE_ALT_LANG:
      for (i = 0; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [lang:%s] = \"%s\"\n", ns_prefix, name,
                 value[i], value[i + 1]);
      break;

    case XMP_PTYPE_STRUCTURE:
      g_print ("\tLocal schema %s = \"%s\"\n", value[0], value[1]);
      for (i = 2; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [%s] = \"%s\"\n", ns_prefix, name,
                 value[i], value[i + 1]);
      break;

    default:
      g_print ("\t%s:%s = ?\n", ns_prefix, name);
      break;
    }
}

static void
print_error (XMPParseContext *context,
             GError          *error,
             gpointer         user_data)
{
  gchar *filename = user_data;

  g_printerr ("While parsing XMP metadata in %s:\n%s\n",
              filename, error->message);
}

static XMPParser xmp_parser = {
  start_schema,
  end_schema,
  set_property,
  print_error
};

static void
property_changed (XMPModel     *tree_model,
                  GtkTreeIter  *iter,
                  gpointer      user_data)
{
  g_print ("Wuff Wuff!\n");
}

static int
scan_file (const gchar *filename)
{
  gchar             *contents;
  gsize              length;
  GError            *error;
  XMPParseContext   *context;
  XMPModel          *xmp_model = xmp_model_new ();

  g_print ("\nFile: %s\n", filename);
  error = NULL;
  if (!g_file_get_contents (filename,
                            &contents,
                            &length,
                            &error))
    {
      print_error (NULL, error, (gpointer) filename);
      g_error_free (error);
      return 1;
    }

  context = xmp_parse_context_new (&xmp_parser,
                                   XMP_FLAG_FIND_XPACKET,
                                   (gpointer) filename,
                                   NULL);

  /*
   * used for testing the XMPModel
   */
  g_signal_connect (xmp_model, "property-changed::xmpMM:DocumentID",
                    G_CALLBACK (property_changed), NULL);

  if (! xmp_model_parse_file (xmp_model, filename, &error))
    {
      return 1;
    }

  if (! xmp_parse_context_parse (context, contents, length, NULL))
    {
      xmp_parse_context_free (context);
      return 1;
    }

  if (! xmp_parse_context_end_parse (context, NULL))
    {
      xmp_parse_context_free (context);
      return 1;
    }

  xmp_parse_context_free (context);
  return 0;
}

int
main (int   argc,
      char *argv[])
{
  g_set_prgname ("xmpdump");
  g_type_init();
  if (argc > 1)
    {
      for (argv++, argc--; argc; argv++, argc--)
        if (scan_file (*argv) != 0)

          return 1;
      return 0;
    }
  else
    {
      g_print ("Usage:\n"
               "\txmpdump file [file [...]]\n\n"
               "The file(s) given on the command line will be scanned "
               "for XMP metadata\n");
      return 1;
    }
}
