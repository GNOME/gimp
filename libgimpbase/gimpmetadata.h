
#ifndef __GIMPMETADATA_H__
#define __GIMPMETADATA_H__

G_BEGIN_DECLS

#include <gexiv2/gexiv2.h>

#define TYPE_GIMP_METADATA (gimp_metadata_get_type ())
#define GIMP_METADATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GIMP_METADATA, GimpMetadata))
#define GIMP_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GIMP_METADATA, GimpMetadataClass))
#define IS_GIMP_METADATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GIMP_METADATA))
#define IS_GIMP_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GIMP_METADATA))
#define GIMP_METADATA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GIMP_METADATA, GimpMetadataClass))
#define GIMP_METADATA_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_GIMP_METADATA, GimpMetadataPrivate))

typedef enum
{
  GIMP_METADATA_LOAD_COMMENT     = 1 << 0,
  GIMP_METADATA_LOAD_RESOLUTION  = 1 << 1,
  GIMP_METADATA_LOAD_ORIENTATION = 1 << 2,
  GIMP_METADATA_LOAD_COLORSPACE  = 1 << 3,

  GIMP_METADATA_LOAD_ALL         = 0xffffffff
} GimpMetadataLoadFlags;

typedef enum
{
  GIMP_METADATA_SAVE_EXIF      = 1 << 0,
  GIMP_METADATA_SAVE_XMP       = 1 << 1,
  GIMP_METADATA_SAVE_IPTC      = 1 << 2,
  GIMP_METADATA_SAVE_THUMBNAIL = 1 << 3,

  GIMP_METADATA_SAVE_ALL       = 0xffffffff
} GimpMetadataSaveFlags;

typedef enum
{
  GIMP_METADATA_COLORSPACE_UNSPECIFIED,
  GIMP_METADATA_COLORSPACE_UNCALIBRATED,
  GIMP_METADATA_COLORSPACE_SRGB,
  GIMP_METADATA_COLORSPACE_ADOBERGB
} GimpMetadataColorspace;

GType                                     gimp_metadata_get_type                                   (void) G_GNUC_CONST;
GimpMetadata *                            gimp_metadata_new                                        (void);
gint                                      gimp_metadata_size                                       (GimpMetadata             *metadata);
GimpMetadata *                            gimp_metadata_duplicate                                  (GimpMetadata             *metadata);
GHashTable *                              gimp_metadata_get_table                                  (GimpMetadata             *metadata);
void                                      gimp_metadata_add_attribute                              (GimpMetadata             *metadata,
                                                                                                    GimpAttribute            *attribute);
GimpAttribute *                           gimp_metadata_get_attribute                              (GimpMetadata             *metadata,
                                                                                                    const gchar              *name);
gboolean                                  gimp_metadata_remove_attribute                           (GimpMetadata             *metadata,
                                                                                                    const gchar              *name);
gboolean                                  gimp_metadata_has_attribute                              (GimpMetadata             *metadata,
                                                                                                    const gchar              *name);
gboolean                                  gimp_metadata_new_attribute                              (GimpMetadata             *metadata,
                                                                                                    const gchar              *name,
                                                                                                    gchar                    *value,
                                                                                                    GimpAttributeValueType    type);
gchar *                                   gimp_metadata_serialize                                  (GimpMetadata             *metadata);
GimpMetadata *                            gimp_metadata_deserialize                                (const gchar              *xml);
void                                      gimp_metadata_print                                      (GimpMetadata             *metadata);
const gchar *                             gimp_metadata_to_xmp_packet                              (GimpMetadata             *metadata,
                                                                                                    const gchar              *mime_type);
gboolean                                  gimp_metadata_has_tag_type                               (GimpMetadata             *metadata,
                                                                                                    GimpAttributeTagType      tag_type);
GList *                                   gimp_metadata_iter_init                                  (GimpMetadata             *metadata,
                                                                                                    GList                   **iter);
gboolean                                  gimp_metadata_iter_next                                  (GimpMetadata             *metadata,
                                                                                                    GimpAttribute           **attribute,
                                                                                                    GList                   **prev);
gboolean                                  gimp_metadata_save_to_file                               (GimpMetadata             *metadata,
                                                                                                    GFile                    *file,
                                                                                                    GError                  **error);
GimpMetadata *                            gimp_metadata_load_from_file                             (GFile                    *file,
                                                                                                    GError                  **error);
gboolean                                  gimp_metadata_set_from_exif                              (GimpMetadata             *metadata,
                                                                                                    const guchar             *exif_data,
                                                                                                    gint                      exif_data_length,
                                                                                                    GError                  **error);
gboolean                                  gimp_metadata_set_from_xmp                               (GimpMetadata             *metadata,
                                                                                                    const guchar             *xmp_data,
                                                                                                    gint                      xmp_data_length,
                                                                                                    GError                  **error);
gboolean                                  gimp_metadata_is_tag_supported                           (const gchar              *tag,
                                                                                                    const gchar              *mime_type);
GimpMetadataColorspace                    gimp_metadata_get_colorspace                             (GimpMetadata             *metadata);
gboolean                                  gimp_metadata_get_resolution                             (GimpMetadata             *metadata,
                                                                                                    gdouble                  *xres,
                                                                                                    gdouble                  *yres,
                                                                                                    GimpUnit                 *unit);
void                                      gimp_metadata_set_resolution                             (GimpMetadata             *metadata,
                                                                                                    gdouble                   xres,
                                                                                                    gdouble                   yres,
                                                                                                    GimpUnit                  unit);
void                                      gimp_metadata_set_bits_per_sample                        (GimpMetadata             *metadata,
                                                                                                    gint                      bits_per_sample);
void                                      gimp_metadata_set_pixel_size                             (GimpMetadata             *metadata,
                                                                                                    gint                      width,
                                                                                                    gint                      height);
void                                      gimp_metadata_set_colorspace                             (GimpMetadata             *metadata,
                                                                                                    GimpMetadataColorspace    colorspace);
//GimpMetadata *                            gimp_metadata_from_gexiv2metadata                        (GimpMetadata             *metadata,
//                                                                                                    GimpMetadata             *gexivdata);
//void                                      gimp_metadata_to_gexiv2metadata                          (GimpMetadata             *metadata,
//                                                                                                    GimpMetadata             *gexivdata,
//                                                                                                    const gchar              *mime_type);


G_END_DECLS

#endif
