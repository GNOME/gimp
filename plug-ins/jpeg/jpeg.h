#include "config.h"   /* configure cares about HAVE_PROGRESSIVE_JPEG */

#include <glib.h>     /* We want glib.h first because of some
                       * pretty obscure Win32 compilation issues.
                       */
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <jpeglib.h>
#include <jerror.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif /* HAVE_EXIF */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

typedef struct my_error_mgr
{
  struct jpeg_error_mgr pub;            /* "public" fields */

#ifdef __ia64__
  /* Ugh, the jmp_buf field needs to be 16-byte aligned on ia64 and some
   * glibc/icc combinations don't guarantee this. So we pad. See bug #138357
   * for details.
   */
  long double           dummy;
#endif

  jmp_buf               setjmp_buffer;  /* for return to caller */
} *my_error_ptr;


gint32 volatile  image_ID_global;
gint32           layer_ID_global;
GimpDrawable    *drawable_global;
gboolean         undo_touched;
gint32           display_ID;
gchar           *image_comment;

#ifdef HAVE_EXIF
ExifData  *exif_data;
#endif /* HAVE_EXIF */




gint32    load_image          (const gchar      *filename,
                               GimpRunMode       runmode,
                               gboolean          preview);

void      destroy_preview     (void);

#ifdef HAVE_EXIF

gint32    load_thumbnail_image(const gchar      *filename,
                               gint             *width,
                               gint             *height);

#endif /* HAVE_EXIF */

void   my_error_exit     (j_common_ptr cinfo);
void   my_emit_message   (j_common_ptr cinfo,
                          int          msg_level);
void   my_output_message (j_common_ptr cinfo);
