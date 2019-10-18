/* tiff loading for GIMP
 *  -Peter Mattis
 *
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 * Added support for layer offsets - pablo.dangelo@web.de -- 7 Jan 2004
 * Honor EXTRASAMPLES tag while loading images with alphachannel
 * pablo.dangelo@web.de -- 16 Jan 2004
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <tiffio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "file-tiff-io.h"


typedef struct
{
  GFile         *file;
  GObject       *stream;
  GInputStream  *input;
  GOutputStream *output;
  gboolean       can_seek;

  gchar         *buffer;
  gsize          allocated;
  gsize          used;
  gsize          position;
} TiffIO;


static void      tiff_io_warning       (const gchar *module,
                                        const gchar *fmt,
                                        va_list      ap) G_GNUC_PRINTF (2, 0);
static void      tiff_io_error         (const gchar *module,
                                        const gchar *fmt,
                                        va_list      ap) G_GNUC_PRINTF (2, 0);
static tsize_t   tiff_io_read          (thandle_t    handle,
                                        tdata_t      buffer,
                                        tsize_t      size);
static tsize_t   tiff_io_write         (thandle_t    handle,
                                        tdata_t      buffer,
                                        tsize_t      size);
static toff_t    tiff_io_seek          (thandle_t    handle,
                                        toff_t       offset,
                                        gint         whence);
static gint      tiff_io_close         (thandle_t    handle);
static toff_t    tiff_io_get_file_size (thandle_t    handle);


static TiffIO tiff_io = { 0, };


TIFF *
tiff_open (GFile        *file,
           const gchar  *mode,
           GError      **error)
{
  TIFFSetWarningHandler ((TIFFErrorHandler) tiff_io_warning);
  TIFFSetErrorHandler ((TIFFErrorHandler) tiff_io_error);

  tiff_io.file = file;

  if (! strcmp (mode, "r"))
    {
      tiff_io.input = G_INPUT_STREAM (g_file_read (file, NULL, error));
      if (! tiff_io.input)
        return NULL;

      tiff_io.stream = G_OBJECT (tiff_io.input);
    }
  else if(! strcmp (mode, "w"))
    {
      tiff_io.output = G_OUTPUT_STREAM (g_file_replace (file,
                                                        NULL, FALSE,
                                                        G_FILE_CREATE_NONE,
                                                        NULL, error));
      if (! tiff_io.output)
        return NULL;

      tiff_io.stream = G_OBJECT (tiff_io.output);
    }
  else if(! strcmp (mode, "a"))
    {
      GIOStream *iostream = G_IO_STREAM (g_file_open_readwrite (file, NULL,
                                                                error));
      if (! iostream)
        return NULL;

      tiff_io.input  = g_io_stream_get_input_stream (iostream);
      tiff_io.output = g_io_stream_get_output_stream (iostream);
      tiff_io.stream = G_OBJECT (iostream);
    }
  else
    {
      g_assert_not_reached ();
    }

#if 0
#warning FIXME !can_seek code is broken
  tiff_io.can_seek = g_seekable_can_seek (G_SEEKABLE (tiff_io.stream));
#endif
  tiff_io.can_seek = TRUE;

  return TIFFClientOpen ("file-tiff", mode,
                         (thandle_t) &tiff_io,
                         tiff_io_read,
                         tiff_io_write,
                         tiff_io_seek,
                         tiff_io_close,
                         tiff_io_get_file_size,
                         NULL, NULL);
}

static void
tiff_io_warning (const gchar *module,
                 const gchar *fmt,
                 va_list      ap)
{
  gint tag = 0;

  /* Between libtiff 3.7.0beta2 and 4.0.0alpha. */
  if (! strcmp (fmt, "%s: unknown field with tag %d (0x%x) encountered") ||
      /* Before libtiff 3.7.0beta2. */
      ! strcmp (fmt, "%.1000s: unknown field with tag %d (0x%x) encountered"))
    {
      va_list ap_test;

      G_VA_COPY (ap_test, ap);

      va_arg (ap_test, const char *); /* ignore first arg */

      tag = va_arg (ap_test, int);

      va_end (ap_test);
    }
  /* for older versions of libtiff? */
  else if (! strcmp (fmt, "unknown field with tag %d (0x%x) ignored") ||
           /* Since libtiff 4.0.0alpha. */
           ! strcmp (fmt, "Unknown field with tag %d (0x%x) encountered"))
    {
      va_list ap_test;

      G_VA_COPY (ap_test, ap);

      tag = va_arg (ap_test, int);

      va_end (ap_test);
    }

  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=131975
   * Ignore the warnings about unregistered private tags (>= 32768).
   */
  if (tag >= 32768)
    return;

  /* Other unknown fields are only reported to stderr. */
  if (tag > 0)
    {
      gchar *msg = g_strdup_vprintf (fmt, ap);

      g_printerr ("%s\n", msg);
      g_free (msg);

      return;
    }
  else if (! strcmp (module, "TIFFReadDirectory") &&
           ! strcmp (fmt,
                     "Sum of Photometric type-related color channels and ExtraSamples doesn't match SamplesPerPixel."
                     " Defining non-color channels as ExtraSamples."))
    {
      /* We will process this issue in our code. Just report to stderr. */
      g_printerr ("%s: [%s] %s\n", G_STRFUNC, module, fmt);

      return;
    }

  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static void
tiff_io_error (const gchar *module,
               const gchar *fmt,
               va_list      ap)
{
  /* Workaround for: http://bugzilla.gnome.org/show_bug.cgi?id=132297
   * Ignore the errors related to random access and JPEG compression
   */
  if (! strcmp (fmt, "Compression algorithm does not support random access"))
    return;

  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, fmt, ap);
}

static tsize_t
tiff_io_read (thandle_t handle,
              tdata_t   buffer,
              tsize_t   size)
{
  TiffIO *io    = (TiffIO *) handle;
  GError *error = NULL;
  gssize  read  = -1;

  if (io->can_seek)
    {
      gsize bytes_read = 0;

      if (! g_input_stream_read_all (io->input,
                                     (void *) buffer, (gsize) size,
                                     &bytes_read,
                                     NULL, &error))
        {
          g_printerr ("%s", error->message);
          g_clear_error (&error);
        }

      read = bytes_read;
    }
  else
    {
      if (io->position + size > io->used)
        {
          gsize missing;
          gsize bytes_read;

          missing = io->position + size - io->used;

          if (io->used + missing > io->allocated)
            {
              gchar *new_buffer;
              gsize  new_size = 1;
              gsize  needed;

              needed = io->used + missing - io->allocated;
              while (new_size < io->allocated + needed)
                new_size *= 2;

              new_buffer = g_try_realloc (io->buffer, new_size);
              if (! new_buffer)
                return -1;

              io->buffer    = new_buffer;
              io->allocated = new_size;
            }

          if (! g_input_stream_read_all (io->input,
                                         (void *) (io->buffer + io->used),
                                         missing,
                                         &bytes_read, NULL, &error))
            {
              g_printerr ("%s", error->message);
              g_clear_error (&error);
            }

          io->used += bytes_read;
        }

      g_assert (io->position + size <= io->used);

      memcpy (buffer, io->buffer + io->position, size);
      io->position += size;

      read = size;
    }

  return (tsize_t) read;
}

static tsize_t
tiff_io_write (thandle_t handle,
               tdata_t   buffer,
               tsize_t   size)
{
  TiffIO *io      = (TiffIO *) handle;
  GError *error   = NULL;
  gssize  written = -1;

  if (io->can_seek)
    {
      gsize bytes_written = 0;

      if (! g_output_stream_write_all (io->output,
                                       (void *) buffer, (gsize) size,
                                       &bytes_written,
                                       NULL, &error))
        {
          g_printerr ("%s", error->message);
          g_clear_error (&error);
        }

      written = bytes_written;
    }
  else
    {
      if (io->position + size > io->allocated)
        {
          gchar *new_buffer;
          gsize  new_size;

          new_size = io->position + size;

          new_buffer = g_try_realloc (io->buffer, new_size);
          if (! new_buffer)
            return -1;

          io->buffer   = new_buffer;
          io->allocated = new_size;
        }

      g_assert (io->position + size <= io->allocated);

      memcpy (io->buffer + io->position, buffer, size);
      io->position += size;

      io->used = MAX (io->used, io->position);

      written = size;
    }

  return (tsize_t) written;
}

static GSeekType
lseek_to_seek_type (gint whence)
{
  switch (whence)
    {
    default:
    case SEEK_SET:
      return G_SEEK_SET;

    case SEEK_CUR:
      return G_SEEK_CUR;

    case SEEK_END:
      return G_SEEK_END;
    }
}

static toff_t
tiff_io_seek (thandle_t handle,
              toff_t    offset,
              gint      whence)
{
  TiffIO   *io       = (TiffIO *) handle;
  GError   *error    = NULL;
  gboolean  sought   = FALSE;
  goffset   position = -1;

  if (io->can_seek)
    {
      sought = g_seekable_seek (G_SEEKABLE (io->stream),
                                (goffset) offset, lseek_to_seek_type (whence),
                                NULL, &error);
      if (sought)
        {
          position = g_seekable_tell (G_SEEKABLE (io->stream));
        }
      else
        {
          g_printerr ("%s", error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      switch (whence)
        {
        default:
        case SEEK_SET:
          if (offset <= io->used)
            position = io->position = offset;
          break;

        case SEEK_CUR:
          if (io->position + offset <= io->used)
            position = io->position += offset;
          break;

        case G_SEEK_END:
          if (io->used + offset <= io->used)
            position = io->position = io->used + offset;
          break;
        }
    }

  return (toff_t) position;
}

static gint
tiff_io_close (thandle_t handle)
{
  TiffIO   *io     = (TiffIO *) handle;
  GError   *error  = NULL;
  gboolean  closed = FALSE;

  if (io->input && ! io->output)
    {
      closed = g_input_stream_close (io->input, NULL, &error);
    }
  else
    {
      if (! io->can_seek && io->buffer && io->allocated)
        {
          if (! g_output_stream_write_all (io->output,
                                           (void *) io->buffer,
                                           io->allocated,
                                           NULL, NULL, &error))
            {
              g_printerr ("%s", error->message);
              g_clear_error (&error);
            }
        }

      if (io->input)
        {
          closed = g_io_stream_close (G_IO_STREAM (io->stream), NULL, &error);
        }
      else
        {
          closed = g_output_stream_close (io->output, NULL, &error);
        }
    }

  if (! closed)
    {
      g_printerr ("%s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (io->stream);
  io->stream = NULL;
  io->input  = NULL;
  io->output = NULL;

  g_free (io->buffer);
  io->buffer = NULL;

  io->allocated = 0;
  io->used      = 0;
  io->position  = 0;

  return closed ? 0 : -1;
}

static toff_t
tiff_io_get_file_size (thandle_t handle)
{
  TiffIO    *io    = (TiffIO *) handle;
  GError    *error = NULL;
  GFileInfo *info;
  goffset    size = 0;

  info = g_file_query_info (io->file,
                            G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  if (! info)
    {
      g_printerr ("%s", error->message);
      g_clear_error (&error);
    }
  else
    {
      size = g_file_info_get_size (info);
      g_object_unref (info);
    }

  return (toff_t) size;
}
