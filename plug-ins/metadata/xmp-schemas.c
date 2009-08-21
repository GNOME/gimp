/* xmp-schemas.h - standard schemas defined in the XMP specifications
 *
 * Copyright (C) 2004-2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "xmp-schemas.h"
#include "xmp-model.h"

static XMPProperty dc_properties[] =
{
  { "contributor",        XMP_TYPE_TEXT,               TRUE  },
  { "coverage",           XMP_TYPE_TEXT,               TRUE  },
  { "creator",            XMP_TYPE_TEXT_SEQ,           TRUE  },
  { "date",               XMP_TYPE_DATE,               TRUE  },
  { "description",        XMP_TYPE_LANG_ALT,           TRUE  },
  { "format",             XMP_TYPE_MIME_TYPE,          XMP_AUTO_UPDATE },
  { "identifier",         XMP_TYPE_TEXT,               TRUE  }, /*xmp:Identifier*/
  { "language",           XMP_TYPE_LOCALE_BAG,         FALSE },
  { "publisher",          XMP_TYPE_TEXT_BAG,           TRUE  },
  { "relation",           XMP_TYPE_TEXT_BAG,           TRUE  },
  { "rights",             XMP_TYPE_LANG_ALT,           TRUE  },
  { "source",             XMP_TYPE_TEXT,               TRUE  },
  { "subject",            XMP_TYPE_TEXT_BAG,           TRUE  },
  { "title",              XMP_TYPE_LANG_ALT,           TRUE  },
  { "type",               XMP_TYPE_TEXT_BAG,           TRUE  },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmp_properties[] =
{
  { "Advisory",           XMP_TYPE_XPATH_BAG,          TRUE  },
  { "BaseURL",            XMP_TYPE_URI,                FALSE },
  { "CreateDate",         XMP_TYPE_DATE,               TRUE  },
  { "CreatorTool",        XMP_TYPE_TEXT,               FALSE },
  { "Identifier",         XMP_TYPE_TEXT_BAG,           TRUE  },
  { "MetadataDate",       XMP_TYPE_DATE,               XMP_AUTO_UPDATE },
  { "ModifyDate",         XMP_TYPE_DATE,               XMP_AUTO_UPDATE },
  { "NickName",           XMP_TYPE_TEXT,               TRUE  },
  { "Thumbnails",         XMP_TYPE_THUMBNAIL_ALT,      TRUE  },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmprights_properties[] =
{
  { "Certificate",        XMP_TYPE_URI,                TRUE  },
  { "Marked",             XMP_TYPE_BOOLEAN,            TRUE  },
  { "Owner",              XMP_TYPE_TEXT_BAG,           TRUE  },
  { "UsageTerms",         XMP_TYPE_LANG_ALT,           TRUE  },
  { "WebStatement",       XMP_TYPE_URI,                TRUE  },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmpmm_properties[] =
{
  { "DerivedFrom",        XMP_TYPE_RESOURCE_REF,       FALSE },
  { "DocumentID",         XMP_TYPE_URI,                FALSE },
  { "History",            XMP_TYPE_RESOURCE_EVENT_SEQ, FALSE },
  { "ManagedFrom",        XMP_TYPE_RESOURCE_REF,       FALSE },
  { "Manager",            XMP_TYPE_TEXT,               FALSE },
  { "ManageTo",           XMP_TYPE_URI,                FALSE },
  { "ManageUI",           XMP_TYPE_URI,                FALSE },
  { "ManagerVariant",     XMP_TYPE_TEXT,               FALSE },
  { "RenditionClass",     XMP_TYPE_TEXT,               FALSE },
  { "RenditionParams",    XMP_TYPE_TEXT,               FALSE },
  { "VersionID",          XMP_TYPE_TEXT,               FALSE },
  { "Versions",           XMP_TYPE_TEXT_SEQ,           FALSE },
  { "LastURL",            XMP_TYPE_URI,                FALSE }, /*deprecated*/
  { "RenditionOf",        XMP_TYPE_RESOURCE_REF,       FALSE }, /*deprecated*/
  { "SaveID",             XMP_TYPE_INTEGER,            FALSE }, /*deprecated*/
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmpbj_properties[] =
{
  { "JobRef",             XMP_TYPE_JOB_BAG,            TRUE  },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmptpg_properties[] =
{
  { "MaxPageSize",        XMP_TYPE_DIMENSIONS,         FALSE },
  { "NPages",             XMP_TYPE_INTEGER,            FALSE },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty pdf_properties[] =
{
  { "Keywords",           XMP_TYPE_TEXT,               TRUE  },
  { "PDFVersion",         XMP_TYPE_TEXT,               FALSE },
  { "Producer",           XMP_TYPE_TEXT,               FALSE },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty photoshop_properties[] =
{
  { "AuthorsPosition",    XMP_TYPE_TEXT,               TRUE  },
  { "CaptionWriter",      XMP_TYPE_TEXT,               TRUE  },
  { "Category",           XMP_TYPE_TEXT,               TRUE  },/* 3 ascii chars */
  { "City",               XMP_TYPE_TEXT,               TRUE  },
  { "Country",            XMP_TYPE_TEXT,               TRUE  },
  { "Credit",             XMP_TYPE_TEXT,               TRUE  },
  { "DateCreated",        XMP_TYPE_DATE,               TRUE  },
  { "Headline",           XMP_TYPE_TEXT,               TRUE  },
  { "Instructions",       XMP_TYPE_TEXT,               TRUE  },
  { "Source",             XMP_TYPE_TEXT,               TRUE  },
  { "State",              XMP_TYPE_TEXT,               TRUE  },
  { "SupplementalCategories",   XMP_TYPE_TEXT,         TRUE  },
  { "TransmissionReference",   XMP_TYPE_TEXT,          TRUE  },
  { "Urgency",            XMP_TYPE_INTEGER,            TRUE  },/* range: 1-8 */
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty tiff_properties[] =
{
  { "ImageWidth",         XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "ImageLength",        XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "BitsPerSample",      XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "Compression",        XMP_TYPE_INTEGER,            FALSE },/* 1 or 6 */
  { "PhotometricInterpretation",   XMP_TYPE_INTEGER,   FALSE },/* 2 or 6 */
  { "Orientation",        XMP_TYPE_INTEGER,            FALSE },/* 1-8 */
  { "SamplesPerPixel",    XMP_TYPE_INTEGER,            FALSE },
  { "PlanarConfiguration",   XMP_TYPE_INTEGER,         FALSE },/* 1 or 2 */
  { "YCbCrSubSampling",   XMP_TYPE_INTEGER_SEQ,        FALSE },/* 2,1 or 2,2 */
  { "YCbCrPositioning",   XMP_TYPE_INTEGER,            FALSE },/* 1 or 2 */
  { "XResolution",        XMP_TYPE_RATIONAL,           XMP_AUTO_UPDATE },
  { "YResolution",        XMP_TYPE_RATIONAL,           XMP_AUTO_UPDATE },
  { "ResolutionUnit",     XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },/*2or3*/
  { "TransferFunction",   XMP_TYPE_INTEGER_SEQ,        FALSE },/*3 * 256 ints*/
  { "WhitePoint",         XMP_TYPE_RATIONAL_SEQ,       FALSE },
  { "PrimaryChromaticities",   XMP_TYPE_RATIONAL_SEQ,  FALSE },
  { "YCbCrCoefficients",   XMP_TYPE_RATIONAL_SEQ,      FALSE },
  { "ReferenceBlackWhite",   XMP_TYPE_RATIONAL_SEQ,    FALSE },
  { "DateTime",           XMP_TYPE_DATE,               FALSE },/*xmp:ModifyDate*/
  { "ImageDescription",   XMP_TYPE_LANG_ALT,           TRUE  },/*dc:description*/
  { "Make",               XMP_TYPE_TEXT,               FALSE },
  { "Model",              XMP_TYPE_TEXT,               FALSE },
  { "Software",           XMP_TYPE_TEXT,               FALSE },/*xmp:CreatorTool*/
  { "Artist",             XMP_TYPE_TEXT,               TRUE  },/*dc:creator*/
  { "Copyright",          XMP_TYPE_TEXT,               TRUE  },/*dc:rights*/
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty exif_properties[] =
{
  { "ExifVersion",        XMP_TYPE_TEXT,               XMP_AUTO_UPDATE },/*"0210*/
  { "FlashpixVersion",    XMP_TYPE_TEXT,               FALSE },/*"0100"*/
  { "ColorSpace",         XMP_TYPE_INTEGER,            FALSE },/*1 or -32768*/
  { "ComponentsConfiguration",   XMP_TYPE_INTEGER_SEQ, FALSE },/*4 ints*/
  { "CompressedBitsPerPixel",   XMP_TYPE_RATIONAL,     FALSE },
  { "PixelXDimension",    XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "PixelYDimension",    XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "MakerNote",          XMP_TYPE_TEXT,               FALSE },/*base64 enc.?*/
  { "UserComment",        XMP_TYPE_TEXT,               TRUE  },
  { "RelatedSoundFile",   XMP_TYPE_TEXT,               FALSE },/*DOS 8.3*/
  { "DateTimeOriginal",   XMP_TYPE_DATE,               FALSE },
  { "DateTimeDigitized",   XMP_TYPE_DATE,              FALSE },
  { "ExposureTime",       XMP_TYPE_RATIONAL,           FALSE },
  { "FNumber",            XMP_TYPE_RATIONAL,           FALSE },
  { "ExposureProgram",    XMP_TYPE_INTEGER,            FALSE },/* 0-8 */
  { "SpectralSensitivity",   XMP_TYPE_TEXT,            FALSE },/* ? */
  { "ISOSpeedRatings",    XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "OECF",               XMP_TYPE_OECF_SFR,           FALSE },
  { "ShutterSpeedValue",   XMP_TYPE_RATIONAL,          FALSE },
  { "ApertureValue",      XMP_TYPE_RATIONAL,           FALSE },
  { "BrightnessValue",    XMP_TYPE_RATIONAL,           FALSE },
  { "ExposureBiasValue",   XMP_TYPE_RATIONAL,          FALSE },
  { "MaxApertureValue",   XMP_TYPE_RATIONAL,           FALSE },
  { "SubjectDistance",    XMP_TYPE_RATIONAL,           FALSE },/*in meters*/
  { "MeteringMode",       XMP_TYPE_INTEGER,            FALSE },/*0-6 or 255*/
  { "LightSource",        XMP_TYPE_INTEGER,            FALSE },/*0-3,17-22,255*/
  { "Flash",              XMP_TYPE_FLASH,              FALSE },
  { "FocalLength",        XMP_TYPE_RATIONAL,           FALSE },
  { "SubjectArea",        XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "FlashEnergy",        XMP_TYPE_RATIONAL,           FALSE },
  { "SpatialFrequencyResponse",   XMP_TYPE_OECF_SFR,   FALSE },
  { "FocalPlaneXResolution",   XMP_TYPE_RATIONAL,      FALSE },
  { "FocalPlaneYResolution",   XMP_TYPE_RATIONAL,      FALSE },
  { "FocalPlaneResolutionUnit",   XMP_TYPE_INTEGER,    FALSE },/*unit: 2 or 3*/
  { "SubjectLocation",    XMP_TYPE_INTEGER_SEQ,        FALSE },/*2 ints: X, Y*/
  { "ExposureIndex",      XMP_TYPE_RATIONAL,           FALSE },
  { "SensingMethod",      XMP_TYPE_INTEGER,            FALSE },/* 1-8 */
  { "FileSource",         XMP_TYPE_INTEGER,            FALSE },/* 3 */
  { "SceneType",          XMP_TYPE_INTEGER,            FALSE },/* 1 */
  { "CFAPattern",         XMP_TYPE_CFA_PATTERN,        FALSE },
  { "CustomRendered",     XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "ExposureMode",       XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "WhiteBalance",       XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "DigitalZoomRatio",   XMP_TYPE_RATIONAL,           FALSE },
  { "FocalLengthIn35mmFilm",   XMP_TYPE_INTEGER,       FALSE },/* in mm */
  { "SceneCaptureType",   XMP_TYPE_INTEGER,            FALSE },/* 0-3 */
  { "GainControl",        XMP_TYPE_INTEGER,            FALSE },/* 0-4 */
  { "Contrast",           XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "Saturation",         XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "Sharpness",          XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "DeviceSettingDescription",   XMP_TYPE_DEVICE_SETTINGS, FALSE },
  { "SubjectDistanceRange",   XMP_TYPE_INTEGER,        FALSE },/* 0-3 */
  { "ImageUniqueID",      XMP_TYPE_TEXT,               FALSE },/* 32 chars */
  { "GPSVersionID",       XMP_TYPE_TEXT,               FALSE },/* "2.0.0.0" */
  { "GPSLatitude",        XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSLongitude",       XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSAltitudeRef",     XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "GPSAltitude",        XMP_TYPE_RATIONAL,           FALSE },/* in meters */
  { "GPSTimeStamp",       XMP_TYPE_DATE,               FALSE },
  { "GPSSatellites",      XMP_TYPE_TEXT,               FALSE },/* ? */
  { "GPSStatus",          XMP_TYPE_TEXT,               FALSE },/* "A" or "V" */
  { "GPSMeasureMode",     XMP_TYPE_INTEGER,            FALSE },/* 2-3 */
  { "GPSDOP",             XMP_TYPE_RATIONAL,           FALSE },
  { "GPSSpeedRef",        XMP_TYPE_TEXT,               FALSE },/*"K","M","N"*/
  { "GPSSpeed",           XMP_TYPE_RATIONAL,           FALSE },
  { "GPSTrackRef",        XMP_TYPE_TEXT,               FALSE },/* "T" or "M" */
  { "GPSTrack",           XMP_TYPE_RATIONAL,           FALSE },
  { "GPSImgDirectionRef",   XMP_TYPE_TEXT,             FALSE },/* "T" or "M" */
  { "GPSImgDirection",    XMP_TYPE_RATIONAL,           FALSE },
  { "GPSMapDatum",        XMP_TYPE_TEXT,               FALSE },
  { "GPSDestLatitude",    XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSDestLongitude",   XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSDestBearingRef",   XMP_TYPE_TEXT,              FALSE },/* "T" or "M" */
  { "GPSDestBearing",     XMP_TYPE_RATIONAL,           FALSE },
  { "GPSDestDistanceRef",   XMP_TYPE_TEXT,             FALSE },/* "K","M","N"*/
  { "GPSDestDistance",    XMP_TYPE_RATIONAL,           FALSE },
  { "GPSProcessingMethod",   XMP_TYPE_TEXT,            FALSE },
  { "GPSAreaInformation",   XMP_TYPE_TEXT,             FALSE },
  { "GPSDifferential",    XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmpplus_properties[] =
{
  { "CreditLineReq",      XMP_TYPE_BOOLEAN,            TRUE  },
  { "ReuseAllowed",       XMP_TYPE_BOOLEAN,            TRUE  },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty iptccore_properties[] =
{
  { "CountryCode",        XMP_TYPE_TEXT,               TRUE }, /* ISO 3166 */
  { "CreatorContactInfo", XMP_TYPE_CONTACT_INFO,       TRUE }, /* RFC 2426 */
  { "IntellectualGenre",  XMP_TYPE_TEXT,               TRUE },
  { "Location",           XMP_TYPE_TEXT,               TRUE },
  { "Scene",              XMP_TYPE_TEXT_BAG,           TRUE }, /*newscodes*/
  { "SubjectCode",        XMP_TYPE_TEXT_BAG,           TRUE }, /*newscodes*/
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty cc_properties[] =
{
  { "license",            XMP_TYPE_URI,                TRUE },
  { NULL,                 XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPSchema xmp_schemas_array[] =
{
  /* XMP schemas defined as of January 2004 */
  { XMP_SCHEMA_DUBLIN_CORE,                             XMP_PREFIX_DUBLIN_CORE,
    "Dublin Core",                                      dc_properties },
  { XMP_SCHEMA_XMP_BASIC,                               XMP_PREFIX_XMP_BASIC,
    "XMP Basic",                                        xmp_properties },
  { XMP_SCHEMA_XMP_RIGHTS,                              XMP_PREFIX_XMP_RIGHTS,
    "XMP Rights Management",                            xmprights_properties },
  { XMP_SCHEMA_XMP_MM,                                  XMP_PREFIX_XMP_MM,
    "XMP Media Management",                             xmpmm_properties },
  { XMP_SCHEMA_XMP_BJ,                                  XMP_PREFIX_XMP_BJ,
    "XMP Basic Job Ticket",                             xmpbj_properties },
  { XMP_SCHEMA_XMP_TPG,                                 XMP_PREFIX_XMP_TPG,
    "XMP Paged-Text",                                   xmptpg_properties },
  { XMP_SCHEMA_PDF,                                     XMP_PREFIX_PDF,
    "Adobe PDF",                                        pdf_properties },
  { XMP_SCHEMA_PHOTOSHOP,                               XMP_PREFIX_PHOTOSHOP,
    "Photoshop",                                        photoshop_properties },
  { XMP_SCHEMA_TIFF,                                    XMP_PREFIX_TIFF,
    "EXIF (TIFF Properties)",                           tiff_properties },
  { XMP_SCHEMA_EXIF,                                    XMP_PREFIX_EXIF,
    "EXIF (EXIF-specific Properties)",                  exif_properties },
  /* Additional schemas published in March 2005 */
  { XMP_SCHEMA_XMP_PLUS,                                XMP_PREFIX_XMP_PLUS,
    "XMP Photographic Licensing Universal System",      xmpplus_properties },
  { XMP_SCHEMA_IPTC_CORE,                               XMP_PREFIX_IPTC_CORE,
    "IPTC Core",                                        iptccore_properties },
  /* other useful namespaces */
  { "http://web.resource.org/cc/",                      "cc",
    "Creative Commons",                                 cc_properties },
  { "http://ns.adobe.com/iX/1.0/",                      "iX",
    NULL,                                               NULL },
  /* XMP sub-types (not top-level schemas) */
  { "http://ns.adobe.com/xmp/Identifier/qual/1.0/",     "xmpidq",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/g/img/",               "xapGImg",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Dimensions#",    "stDim",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/sType/ResourceEvent#", "stEvt",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/sType/ResourceRef#",   "stRef",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Version#",       "stVer",
    NULL,                                               NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Job#",           "stJob",
    NULL,                                               NULL },
  { NULL, NULL, NULL }
};

XMPSchema * const xmp_schemas = xmp_schemas_array;
