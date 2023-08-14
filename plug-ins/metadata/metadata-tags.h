/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2016, 2017 Ben Touchette
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

#ifndef __METADATA_TAGS_H__
#define __METADATA_TAGS_H__

#include "metadata-misc.h"

#define TAG_TYPE_XMP            1
#define TAG_TYPE_EXIF           2
#define TAG_TYPE_IPTC           3

enum
{
  GIMP_XMP_NONE = 0,
  GIMP_XMP_TEXT,
  GIMP_XMP_BAG,
  GIMP_XMP_SEQ,
  GIMP_XMP_LANG,
  GIMP_XMP_ALT
};

enum
{
  COL_LICENSOR_NAME = 0,
  COL_LICENSOR_ID,
  COL_LICENSOR_PHONE1,
  COL_LICENSOR_PHONE_TYPE1,
  COL_LICENSOR_PHONE2,
  COL_LICENSOR_PHONE_TYPE2,
  COL_LICENSOR_EMAIL,
  COL_LICENSOR_WEB,
  COL_LICENSOR_NUM_COLS
};

enum
{
  COL_CR_OWNER_NAME = 0,
  COL_CR_OWNER_ID,
  COL_CR_OWNER_NUM_COLS
};

enum
{
  COL_IMG_CR8_NAME = 0,
  COL_IMG_CR8_ID,
  COL_IMG_CR8_NUM_COLS
};

enum
{
  COL_AOO_TITLE = 0,
  COL_AOO_DATE_CREAT,
  COL_AOO_CREATOR,
  COL_AOO_SOURCE,
  COL_AOO_SRC_INV_ID,
  COL_AOO_CR_NOT,
  COL_AOO_NUM_COLS
};

enum
{
  COL_REGISTRY_ORG_ID = 0,
  COL_REGISTRY_ITEM_ID,
  COL_REGISTRY_NUM_COLS
};

enum
{
  COL_LOC_SHO_SUB_LOC = 0,
  COL_LOC_SHO_CITY,
  COL_LOC_SHO_STATE_PROV,
  COL_LOC_SHO_CNTRY,
  COL_LOC_SHO_CNTRY_ISO,
  COL_LOC_SHO_CNTRY_WRLD_REG,
  COL_LOC_SHO_NUM_COLS
};

enum
{
  COL_ORG_IMG_CODE = 0,
  ORG_IMG_CODE_REL_NUM_COLS
};

enum
{
  COL_ORG_IMG_NAME = 0,
  ORG_IMG_NAME_REL_NUM_COLS
};

enum
{
  COL_MOD_REL_ID = 0,
  MOD_REL_NUM_COLS
};

enum
{
  COL_PROP_REL_ID = 0,
  PROP_REL_NUM_COLS
};

enum METADATA_SPECIAL_PROCESSING
{
  METADATA_NONE = 0,
  METADATA_PHONETYPE,
  METADATA_PREPROCESS_TEXT
};

extern const metadata_tag  default_metadata_tags[];
extern const gint          n_default_metadata_tags;

extern const iptc_tag_info equivalent_metadata_tags[];
extern const gint          n_equivalent_metadata_tags;

extern const exif_tag_info exif_equivalent_tags[];

/* Tag indexes in equivalent_metadata_tags that need special processing. */
#define SPECIAL_PROCESSING_DATE_CREATED 0

/* Digital Source Type Combobox Items
 * http://cv.iptc.org/newscodes/digitalsourcetype/
 */
extern const combobox_str_tag digitalsourcetype[];
extern const gint             n_digitalsourcetype;

/* Model Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#ModelReleaseStatus
 */
extern const combobox_str_tag modelreleasestatus[];
extern const gint             n_modelreleasestatus;

/* Property Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#PropertyReleaseStatus
 */
extern const combobox_str_tag propertyreleasestatus[];
extern const gint             n_propertyreleasestatus;

/* Minor Model Age Disclosure Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#MinorModelAgeDisclosure
 */
extern const combobox_str_tag minormodelagedisclosure[];
extern const gint             n_minormodelagedisclosure;

/* Urgency */
extern const gchar *urgency[];
extern const gint   n_urgency;

/* Marked */
extern const combobox_int_tag marked[];
extern const gint             n_marked;

/* Phone Types */
extern const combobox_str_tag phone_types[];
extern const gint             n_phone_types;

/* DICOM Patient Sex
 * http://dicomlookup.com/lookup.asp?sw=Ttable&q=C.7-1
 * http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/XMP.html#DICOM
 * https://dicomiseasy.blogspot.ca/2011/11/introduction-to-dicom-chapter-iii-dicom.html
 * http://dicom.nema.org/standard.html
 */
extern const combobox_str_tag dicom[];
extern const gint             n_dicom;

/* GPS Altitude Ref */
extern const gchar *gpsaltref[];
extern const gint   n_gpsaltref;

/* GPS Latitude Ref */
extern const gchar *gpslatref[];
extern const gint   n_gpslatref;

/* GPS Longitude Ref */
extern const gchar *gpslngref[];
extern const gint   n_gpslngref;

/* GPS Measurement System */
extern const gchar *gpsaltsys[];
extern const gint   n_gpsaltsys;

extern const TranslateTag creatorContactInfoTags[];
extern const gint n_creatorContactInfoTags;

extern const TranslateTag locationCreationInfoTags[];
extern const gint n_locationCreationInfoTags;

extern const TranslateTag imageSupplierInfoTags[];
extern const gint n_imageSupplierInfoTags;

/* Plus and IPTC extension tags */

#define LICENSOR_HEADER "Xmp.plus.Licensor"
extern const gchar *licensor[];
extern const gint   n_licensor;
extern const gint   licensor_special_handling[];

#ifdef USE_TAGS
#define IMAGESUPPLIER_HEADER "Xmp.plus.ImageSupplier"
extern const gchar *imagesupplier[];
extern const gint   n_imagesupplier;
#endif

#define IMAGECREATOR_HEADER "Xmp.plus.ImageCreator"
extern const gchar *imagecreator[];
extern const gint   n_imagecreator;

#define COPYRIGHTOWNER_HEADER "Xmp.plus.CopyrightOwner"
extern const gchar *copyrightowner[];
extern const gint   n_copyrightowner;

#define REGISTRYID_HEADER "Xmp.iptcExt.RegistryId"
extern const gchar *registryid[];
extern const gchar *registryid_alternative[];
extern const gint   n_registryid;

#define ARTWORKOROBJECT_HEADER "Xmp.iptcExt.ArtworkOrObject"
extern const gchar *artworkorobject[];
extern const gchar *artworkorobject_alternative[];
extern const gint   n_artworkorobject;

#define LOCATIONSHOWN_HEADER "Xmp.iptcExt.LocationShown"
extern const gchar *locationshown[];
extern const gchar *locationshown_alternative[];
extern const gint   n_locationshown;

#ifdef USE_TAGS
#define LOCATIONCREATED_HEADER "Xmp.iptcExt.LocationCreated"
extern const gchar *locationcreated[];
extern const gint   n_locationcreated;
#endif


gchar * metadata_format_gps_longitude_latitude (const gdouble  value);
gchar * metadata_format_gps_altitude           (const gdouble  value,
                                                gboolean       use_meter,
                                                gchar         *measurement_symbol);

#endif /* __METADATA_TAGS_H__ */
