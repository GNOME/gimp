#ifndef OPENEXR_WRAPPER_H
#define OPENEXR_WRAPPER_H

/* Use C linkage so that the plug-in code written in C can use the
 * wrapper.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <lcms2.h>

/* This is fully opaque on purpose, as the calling C code must not be
 * exposed to more than this.
 */
typedef struct _EXRLoader EXRLoader;

typedef enum {
  PREC_UINT,
  PREC_HALF,
  PREC_FLOAT
} EXRPrecision;

typedef enum {
  IMAGE_TYPE_RGB,
  IMAGE_TYPE_GRAY
} EXRImageType;

EXRLoader *
exr_loader_new (const char *filename);

EXRLoader *
exr_loader_ref (EXRLoader *loader);

void
exr_loader_unref (EXRLoader *loader);

int
exr_loader_get_width (EXRLoader *loader);

int
exr_loader_get_height (EXRLoader *loader);

EXRPrecision
exr_loader_get_precision (EXRLoader *loader);

EXRImageType
exr_loader_get_image_type (EXRLoader *loader);

int
exr_loader_has_alpha (EXRLoader *loader);

GimpColorProfile *
exr_loader_get_profile (EXRLoader *loader);

gchar *
exr_loader_get_comment (EXRLoader *loader);

guchar *
exr_loader_get_exif (EXRLoader *loader,
                     guint *size);

guchar *
exr_loader_get_xmp (EXRLoader *loader,
                    guint *size);

int
exr_loader_read_pixel_row (EXRLoader *loader,
                           char *pixels,
                           int bpp,
                           int row);

#ifdef __cplusplus
}
#endif

#endif /* OPENEXR_WRAPPER_H */
