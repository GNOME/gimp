/* xmp-model.c - treeview model for XMP metadata
 *
 * Copyright (C) 2004-2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#ifdef DEBUG_XMP_PARSER
#  include <stdio.h>
#endif

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "xmp-parse.h"
#include "xmp-model.h"

/* The main part of the XMPModel structure is the GtkTreeStore in
 * which all references to XMP properties are stored.  In the tree,
 * the elements at the root level are the schemas (namespaces) and the
 * children of the schemas are the XMP properties.
 *
 * If the XMP file contains a schema that is not part of the XMP
 * specification, it will be included in the custom_schemas list and
 * the corresponding element in the tree will get a reference to that
 * list element instead of a reference to one of the static schema
 * definitions included below.  Same for custom properties inside a
 * known or custom schema.
 */
struct _XMPModel
{
  GtkTreeStore *treestore;
  GSList       *custom_schemas;
  GSList       *custom_properties;

  XMPSchema    *current_schema;
  GtkTreeIter   current_schema_iter;
};

static XMPProperty dc_properties[] =
{
  { "contributor",     XMP_TYPE_TEXT,               TRUE  },
  { "coverage",        XMP_TYPE_TEXT,               TRUE  },
  { "creator",         XMP_TYPE_TEXT_SEQ,           TRUE  },
  { "date",            XMP_TYPE_DATE,               TRUE  },
  { "description",     XMP_TYPE_LANG_ALT,           TRUE  },
  { "format",          XMP_TYPE_MIME_TYPE,          XMP_AUTO_UPDATE },
  { "identifier",      XMP_TYPE_TEXT,               TRUE  }, /*xmp:Identifier*/
  { "language",        XMP_TYPE_LOCALE_BAG,         FALSE },
  { "publisher",       XMP_TYPE_TEXT_BAG,           TRUE  },
  { "relation",        XMP_TYPE_TEXT_BAG,           TRUE  },
  { "rights",          XMP_TYPE_LANG_ALT,           TRUE  },
  { "source",          XMP_TYPE_TEXT,               TRUE  },
  { "subject",         XMP_TYPE_TEXT_BAG,           TRUE  },
  { "title",           XMP_TYPE_LANG_ALT,           TRUE  },
  { "type",            XMP_TYPE_TEXT_BAG,           TRUE  },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmp_properties[] =
{
  { "Advisory",        XMP_TYPE_XPATH_BAG,          TRUE  },
  { "BaseURL",         XMP_TYPE_URI,                FALSE },
  { "CreateDate",      XMP_TYPE_DATE,               TRUE  },
  { "CreatorTool",     XMP_TYPE_TEXT,               FALSE },
  { "Identifier",      XMP_TYPE_TEXT_BAG,           TRUE  },
  { "MetadataDate",    XMP_TYPE_DATE,               XMP_AUTO_UPDATE },
  { "ModifyDate",      XMP_TYPE_DATE,               XMP_AUTO_UPDATE },
  { "NickName",        XMP_TYPE_TEXT,               TRUE  },
  { "Thumbnails",      XMP_TYPE_THUMBNAIL_ALT,      TRUE  },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmprights_properties[] =
{
  { "Certificate",     XMP_TYPE_URI,                TRUE  },
  { "Marked",          XMP_TYPE_BOOLEAN,            TRUE  },
  { "Owner",           XMP_TYPE_TEXT_BAG,           TRUE  },
  { "UsageTerms",      XMP_TYPE_LANG_ALT,           TRUE  },
  { "WebStatement",    XMP_TYPE_URI,                TRUE  },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmpmm_properties[] =
{
  { "DerivedFrom",     XMP_TYPE_RESOURCE_REF,       FALSE },
  { "DocumentID",      XMP_TYPE_URI,                FALSE },
  { "History",         XMP_TYPE_RESOURCE_EVENT_SEQ, FALSE },
  { "ManagedFrom",     XMP_TYPE_RESOURCE_REF,       FALSE },
  { "Manager",         XMP_TYPE_TEXT,               FALSE },
  { "ManageTo",        XMP_TYPE_URI,                FALSE },
  { "ManageUI",        XMP_TYPE_URI,                FALSE },
  { "ManagerVariant",  XMP_TYPE_TEXT,               FALSE },
  { "RenditionClass",  XMP_TYPE_TEXT,               FALSE },
  { "RenditionParams", XMP_TYPE_TEXT,               FALSE },
  { "VersionID",       XMP_TYPE_TEXT,               FALSE },
  { "Versions",        XMP_TYPE_TEXT_SEQ,           FALSE },
  { "LastURL",         XMP_TYPE_URI,                FALSE }, /*deprecated*/
  { "RenditionOf",     XMP_TYPE_RESOURCE_REF,       FALSE }, /*deprecated*/
  { "SaveID",          XMP_TYPE_INTEGER,            FALSE }, /*deprecated*/
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmpbj_properties[] =
{
  { "JobRef",          XMP_TYPE_JOB_BAG,            TRUE  },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty xmptpg_properties[] =
{
  { "MaxPageSize",     XMP_TYPE_DIMENSIONS,         FALSE },
  { "NPages",          XMP_TYPE_INTEGER,            FALSE },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty pdf_properties[] =
{
  { "Keywords",        XMP_TYPE_TEXT,               TRUE  },
  { "PDFVersion",      XMP_TYPE_TEXT,               FALSE },
  { "Producer",        XMP_TYPE_TEXT,               FALSE },
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty photoshop_properties[] =
{
  { "AuthorsPosition", XMP_TYPE_TEXT,               TRUE  },
  { "CaptionWriter",   XMP_TYPE_TEXT,               TRUE  },
  { "Category",        XMP_TYPE_TEXT,               TRUE  },/* 3 ascii chars */
  { "City",            XMP_TYPE_TEXT,               TRUE  },
  { "Country",         XMP_TYPE_TEXT,               TRUE  },
  { "Credit",          XMP_TYPE_TEXT,               TRUE  },
  { "DateCreated",     XMP_TYPE_DATE,               TRUE  },
  { "Headline",        XMP_TYPE_TEXT,               TRUE  },
  { "Instructions",    XMP_TYPE_TEXT,               TRUE  },
  { "Source",          XMP_TYPE_TEXT,               TRUE  },
  { "State",           XMP_TYPE_TEXT,               TRUE  },
  { "SupplementalCategories",XMP_TYPE_TEXT,         TRUE  },
  { "TransmissionReference",XMP_TYPE_TEXT,          TRUE  },
  { "Urgency",         XMP_TYPE_INTEGER,            TRUE  },/* range: 1-8 */
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty tiff_properties[] =
{
  { "ImageWidth",      XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "ImageLength",     XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "BitsPerSample",   XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "Compression",     XMP_TYPE_INTEGER,            FALSE },/* 1 or 6 */
  { "PhotometricInterpretation",XMP_TYPE_INTEGER,   FALSE },/* 2 or 6 */
  { "Orientation",     XMP_TYPE_INTEGER,            FALSE },/* 1-8 */
  { "SamplesPerPixel", XMP_TYPE_INTEGER,            FALSE },
  { "PlanarConfiguration",XMP_TYPE_INTEGER,         FALSE },/* 1 or 2 */
  { "YCbCrSubSampling",XMP_TYPE_INTEGER_SEQ,        FALSE },/* 2,1 or 2,2 */
  { "YCbCrPositioning",XMP_TYPE_INTEGER,            FALSE },/* 1 or 2 */
  { "XResolution",     XMP_TYPE_RATIONAL,           XMP_AUTO_UPDATE },
  { "YResolution",     XMP_TYPE_RATIONAL,           XMP_AUTO_UPDATE },
  { "ResolutionUnit",  XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },/*2or3*/
  { "TransferFunction",XMP_TYPE_INTEGER_SEQ,        FALSE },/* 3 * 256 ints */
  { "WhitePoint",      XMP_TYPE_RATIONAL_SEQ,       FALSE },
  { "PrimaryChromaticities",XMP_TYPE_RATIONAL_SEQ,  FALSE },
  { "YCbCrCoefficients",XMP_TYPE_RATIONAL_SEQ,      FALSE },
  { "ReferenceBlackWhite",XMP_TYPE_RATIONAL_SEQ,    FALSE },
  { "DateTime",        XMP_TYPE_DATE,               FALSE },/*xmp:ModifyDate*/
  { "ImageDescription",XMP_TYPE_LANG_ALT,           TRUE  },/*dc:description*/
  { "Make",            XMP_TYPE_TEXT,               FALSE },
  { "Model",           XMP_TYPE_TEXT,               FALSE },
  { "Software",        XMP_TYPE_TEXT,               FALSE },/*xmp:CreatorTool*/
  { "Artist",          XMP_TYPE_TEXT,               TRUE  },/*dc:creator*/
  { "Copyright",       XMP_TYPE_TEXT,               TRUE  },/*dc:rights*/
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPProperty exif_properties[] =
{
  { "ExifVersion",     XMP_TYPE_TEXT,               XMP_AUTO_UPDATE },/*"0210*/
  { "FlashpixVersion", XMP_TYPE_TEXT,               FALSE },/* "0100" */
  { "ColorSpace",      XMP_TYPE_INTEGER,            FALSE },/* 1 or -32768 */
  { "ComponentsConfiguration",XMP_TYPE_INTEGER_SEQ, FALSE },/* 4 ints */
  { "CompressedBitsPerPixel",XMP_TYPE_RATIONAL,     FALSE },
  { "PixelXDimension", XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "PixelYDimension", XMP_TYPE_INTEGER,            XMP_AUTO_UPDATE },
  { "MakerNote",       XMP_TYPE_TEXT,               FALSE },/* base64 enc.? */
  { "UserComment",     XMP_TYPE_TEXT,               TRUE  },
  { "RelatedSoundFile",XMP_TYPE_TEXT,               FALSE },/* DOS 8.3 fname */
  { "DateTimeOriginal",XMP_TYPE_DATE,               FALSE },
  { "DateTimeDigitized",XMP_TYPE_DATE,              FALSE },
  { "ExposureTime",    XMP_TYPE_RATIONAL,           FALSE },
  { "FNumber",         XMP_TYPE_RATIONAL,           FALSE },
  { "ExposureProgram", XMP_TYPE_INTEGER,            FALSE },/* 0-8 */
  { "SpectralSensitivity",XMP_TYPE_TEXT,            FALSE },/* ? */
  { "ISOSpeedRatings", XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "OECF",            XMP_TYPE_OECF_SFR,           FALSE },
  { "ShutterSpeedValue",XMP_TYPE_RATIONAL,          FALSE },
  { "ApertureValue",   XMP_TYPE_RATIONAL,           FALSE },
  { "BrightnessValue", XMP_TYPE_RATIONAL,           FALSE },
  { "ExposureBiasValue",XMP_TYPE_RATIONAL,          FALSE },
  { "MaxApertureValue",XMP_TYPE_RATIONAL,           FALSE },
  { "SubjectDistance", XMP_TYPE_RATIONAL,           FALSE },/* in meters */
  { "MeteringMode",    XMP_TYPE_INTEGER,            FALSE },/* 0-6 or 255 */
  { "LightSource",     XMP_TYPE_INTEGER,            FALSE },/* 0-3,17-22,255*/
  { "Flash",           XMP_TYPE_FLASH,              FALSE },
  { "FocalLength",     XMP_TYPE_RATIONAL,           FALSE },
  { "SubjectArea",     XMP_TYPE_INTEGER_SEQ,        FALSE },
  { "FlashEnergy",     XMP_TYPE_RATIONAL,           FALSE },
  { "SpatialFrequencyResponse",XMP_TYPE_OECF_SFR,   FALSE },
  { "FocalPlaneXResolution",XMP_TYPE_RATIONAL,      FALSE },
  { "FocalPlaneYResolution",XMP_TYPE_RATIONAL,      FALSE },
  { "FocalPlaneResolutionUnit",XMP_TYPE_INTEGER,    FALSE },/* unit: 2 or 3 */
  { "SubjectLocation", XMP_TYPE_INTEGER_SEQ,        FALSE },/* 2 ints: X, Y */
  { "ExposureIndex",   XMP_TYPE_RATIONAL,           FALSE },
  { "SensingMethod",   XMP_TYPE_INTEGER,            FALSE },/* 1-8 */
  { "FileSource",      XMP_TYPE_INTEGER,            FALSE },/* 3 */
  { "SceneType",       XMP_TYPE_INTEGER,            FALSE },/* 1 */
  { "CFAPattern",      XMP_TYPE_CFA_PATTERN,        FALSE },
  { "CustomRendered",  XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "ExposureMode",    XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "WhiteBalance",    XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "DigitalZoomRatio",XMP_TYPE_RATIONAL,           FALSE },
  { "FocalLengthIn35mmFilm",XMP_TYPE_INTEGER,       FALSE },/* in mm */
  { "SceneCaptureType",XMP_TYPE_INTEGER,            FALSE },/* 0-3 */
  { "GainControl",     XMP_TYPE_INTEGER,            FALSE },/* 0-4 */
  { "Contrast",        XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "Saturation",      XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "Sharpness",       XMP_TYPE_INTEGER,            FALSE },/* 0-2 */
  { "DeviceSettingDescription",XMP_TYPE_DEVICE_SETTINGS, FALSE },
  { "SubjectDistanceRange",XMP_TYPE_INTEGER,        FALSE },/* 0-3 */
  { "ImageUniqueID",   XMP_TYPE_TEXT,               FALSE },/* 32 chars */
  { "GPSVersionID",    XMP_TYPE_TEXT,               FALSE },/* "2.0.0.0" */
  { "GPSLatitude",     XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSLongitude",    XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSAltitudeRef",  XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { "GPSAltitude",     XMP_TYPE_RATIONAL,           FALSE },/* in meters */
  { "GPSTimeStamp",    XMP_TYPE_DATE,               FALSE },
  { "GPSSatellites",   XMP_TYPE_TEXT,               FALSE },/* ? */
  { "GPSStatus",       XMP_TYPE_TEXT,               FALSE },/* "A" or "V" */
  { "GPSMeasureMode",  XMP_TYPE_INTEGER,            FALSE },/* 2-3 */
  { "GPSDOP",          XMP_TYPE_RATIONAL,           FALSE },
  { "GPSSpeedRef",     XMP_TYPE_TEXT,               FALSE },/* "K","M","N" */
  { "GPSSpeed",        XMP_TYPE_RATIONAL,           FALSE },
  { "GPSTrackRef",     XMP_TYPE_TEXT,               FALSE },/* "T" or "M"" */
  { "GPSTrack",        XMP_TYPE_RATIONAL,           FALSE },
  { "GPSImgDirectionRef",XMP_TYPE_TEXT,             FALSE },/* "T" or "M"" */
  { "GPSImgDirection", XMP_TYPE_RATIONAL,           FALSE },
  { "GPSMapDatum",     XMP_TYPE_TEXT,               FALSE },
  { "GPSDestLatitude", XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSDestLongitude",XMP_TYPE_GPS_COORDINATE,     FALSE },
  { "GPSDestBearingRef",XMP_TYPE_TEXT,              FALSE },/* "T" or "M"" */
  { "GPSDestBearing",  XMP_TYPE_RATIONAL,           FALSE },
  { "GPSDestDistanceRef",XMP_TYPE_TEXT,             FALSE },/* "K","M","N" */
  { "GPSDestDistance", XMP_TYPE_RATIONAL,           FALSE },
  { "GPSProcessingMethod",XMP_TYPE_TEXT,            FALSE },
  { "GPSAreaInformation",XMP_TYPE_TEXT,             FALSE },
  { "GPSDifferential", XMP_TYPE_INTEGER,            FALSE },/* 0-1 */
  { NULL,              XMP_TYPE_UNKNOWN,            FALSE }
};

static XMPSchema xmp_schemas[] =
{
  /* XMP schemas defined as of January 2004 */
  { "http://purl.org/dc/elements/1.1/",     "dc",
    "Dublin Core",                          dc_properties },
  { "http://ns.adobe.com/xap/1.0/",         "xmp",
    "XMP Basic",                            xmp_properties },
  { "http://ns.adobe.com/xap/1.0/rights/",  "xmpRights",
    "XMP Rights Management",                xmprights_properties },
  { "http://ns.adobe.com/xap/1.0/mm/",      "xmpMM",
    "XMP Media Management",                 xmpmm_properties },
  { "http://ns.adobe.com/xap/1.0/bj/",      "xmpBJ",
    "XMP Basic Job Ticket",                 xmpbj_properties },
  { "http://ns.adobe.com/xap/1.0/t/pg/",    "xmpTPg",
    "XMP Paged-Text",                       xmptpg_properties },
  { "http://ns.adobe.com/pdf/1.3/",         "pdf",
    "Adobe PDF",                            pdf_properties },
  { "http://ns.adobe.com/photoshop/1.0/",   "photoshop",
    "Photoshop",                            photoshop_properties },
  { "http://ns.adobe.com/tiff/1.0/",        "tiff",
    "EXIF (TIFF Properties)",               tiff_properties },
  { "http://ns.adobe.com/exif/1.0/",        "exif",
    "EXIF (EXIF-specific Properties)",      exif_properties },
  /* XMP sub-types */
  { "http://ns.adobe.com/xmp/Identifier/qual/1.0/",     "xmpidq",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/g/img/",               "xapGImg",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Dimensions#",    "stDim",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/sType/ResourceEvent#", "stEvt",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/sType/ResourceRef#",   "stRef",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Version#",       "stVer",
    NULL,                                   NULL },
  { "http://ns.adobe.com/xap/1.0/sType/Job#",           "stJob",
    NULL,                                   NULL },
  /* other useful namespaces */
  { "http://web.resource.org/cc/",          "cc",
    "Creative Commons",                     NULL },
  { "http://ns.adobe.com/iX/1.0/",          "iX",
    NULL,                                   NULL },
  { NULL, NULL, NULL }
};

/**
 * xmp_model_new:
 *
 * Return value: a new #XMPModel.
 **/
XMPModel *
xmp_model_new (void)
{
  XMPModel *xmp_model;

  xmp_model = g_new (XMPModel, 1);
  /* columns defined by the XMPModelColumns enum */
  xmp_model->treestore =
    gtk_tree_store_new (XMP_MODEL_NUM_COLUMNS,
                        G_TYPE_STRING,   /* name */
                        G_TYPE_STRING,   /* value as string (for viewing) */
                        G_TYPE_POINTER,  /* value as array (from parser) */
                        G_TYPE_POINTER,  /* XMPProperty or XMPSchema */
                        G_TYPE_POINTER,  /* GtkWidget cross-reference */
                        G_TYPE_INT,      /* editable? */
                        GDK_TYPE_PIXBUF, /* edit icon */
                        G_TYPE_BOOLEAN,  /* visible? */
                        G_TYPE_INT,      /* font weight */
                        G_TYPE_BOOLEAN   /* font weight set? */
                        );
  xmp_model->custom_schemas = NULL;
  xmp_model->custom_properties = NULL;
  xmp_model->current_schema = NULL;
  return xmp_model;
}

/**
 * xmp_model_free:
 * @xmp_model: an #XMPModel
 *
 * Frees an #XMPModel.
 **/
void
xmp_model_free (XMPModel *xmp_model)
{
  GtkTreeModel  *model;
  GtkTreeIter    iter;
  GtkTreeIter    child;
  gchar        **value_array;
  gint           i;

  g_return_if_fail (xmp_model != NULL);
  /* we used XMP_FLAG_DEFER_VALUE_FREE for the parser, so now we must free
     all value arrays */
  model = xmp_model_get_tree_model (xmp_model);
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
    {
      do
        {
          if (gtk_tree_model_iter_children (model, &child, &iter))
            {
              do
                {
                  gtk_tree_model_get (model, &child,
                                      COL_XMP_VALUE_RAW, &value_array,
                                      -1);
                  /* FIXME: this does not free everything */
                  for (i = 0; value_array[i] != NULL; i++)
                    g_free (value_array[i]);
                  g_free (value_array);
                }
              while (gtk_tree_model_iter_next (model, &child));
            }
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  g_object_unref (xmp_model->treestore);
  /* FIXME: free custom schemas */
  g_free (xmp_model);
}

/**
 * xmp_model_is_empty:
 * @xmp_model: an #XMPModel
 *
 * Return value: %TRUE if @xmp_model is empty (no shemas, no properties)
 **/
gboolean
xmp_model_is_empty (XMPModel *xmp_model)
{
  GtkTreeIter iter;

  g_return_val_if_fail (xmp_model != NULL, TRUE);
  if ((xmp_model->custom_schemas != NULL)
      || (xmp_model->custom_properties != NULL))
    return FALSE;
  return !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (xmp_model->treestore),
                                         &iter);
}

/* check if the given schema_uri matches a known schema; else return NULL */
static XMPSchema *
find_xmp_schema (XMPModel    *xmp_model,
                 const gchar *schema_uri)
{
  int     i;
  GSList *list;

  /* check if we know about this schema (exact match for URI) */
  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    {
      if (! strcmp (xmp_schemas[i].uri, schema_uri))
        {
#ifdef DEBUG_XMP_PARSER
          if (xmp_schemas[i].name != NULL)
            printf ("%s \t[%s]\n", xmp_schemas[i].name, xmp_schemas[i].uri);
          else
            printf ("*** \t[%s]\n", xmp_schemas[i].uri);
#endif
          return &(xmp_schemas[i]);
        }
    }
  /* try again but accept "http:" without "//", or missing "http://" */
  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    {
      if (g_str_has_prefix (xmp_schemas[i].uri, "http://")
          && ((! strcmp (xmp_schemas[i].uri + 7, schema_uri))
              || (g_str_has_prefix (schema_uri, "http:")
                  && ! strcmp (xmp_schemas[i].uri + 7, schema_uri + 5))
              ))
        {
#ifdef DEBUG_XMP_PARSER
          printf ("%s \t~~~[%s]\n", xmp_schemas[i].name, xmp_schemas[i].uri);
#endif
          return &(xmp_schemas[i]);
        }
    }
  /* this is not a standard shema; now check the custom schemas */
  for (list = xmp_model->custom_schemas; list != NULL; list = list->next)
    {
      if (! strcmp (((XMPSchema *)(list->data))->uri, schema_uri))
        {
#ifdef DEBUG_XMP_PARSER
          printf ("CUSTOM %s \t[%s]\n",
                  ((XMPSchema *)(list->data))->name,
                  ((XMPSchema *)(list->data))->uri);
#endif
          return (XMPSchema *)(list->data);
        }
    }
#ifdef DEBUG_XMP_PARSER
  printf ("Unknown schema URI %s\n", schema_uri);
#endif
  return NULL;
}

/* check if the given prefix matches a known schema; else return NULL */
static XMPSchema *
find_xmp_schema_prefix (XMPModel    *xmp_model,
                        const gchar *prefix)
{
  int     i;
  GSList *list;

  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    if (! strcmp (xmp_schemas[i].prefix, prefix))
      return &(xmp_schemas[i]);
  for (list = xmp_model->custom_schemas; list != NULL; list = list->next)
    if (! strcmp (((XMPSchema *)(list->data))->prefix, prefix))
      return (XMPSchema *)(list->data);
  return NULL;
}

/* make the next lookup a bit faster if the tree is not modified */
static void
save_iter_for_schema (XMPModel    *xmp_model,
                      XMPSchema   *schema,
                      GtkTreeIter *iter)
{
  xmp_model->current_schema = schema;
  if (iter != NULL)
    memcpy (&(xmp_model->current_schema_iter), iter, sizeof (GtkTreeIter));
}

/* find the GtkTreeIter for the given schema and return TRUE if the schema was
   found in the tree; else return FALSE */
static gboolean
find_iter_for_schema (XMPModel    *xmp_model,
                      XMPSchema   *schema,
                      GtkTreeIter *iter)
{
  XMPSchema *schema_xref;

  if (schema == xmp_model->current_schema)
    {
      memcpy (iter, &(xmp_model->current_schema_iter), sizeof (GtkTreeIter));
      return TRUE;
    }
  /* check where this schema has been stored in the tree */
  if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (xmp_model->treestore),
                                       iter))
    return FALSE;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), iter,
                          COL_XMP_TYPE_XREF, &schema_xref,
                          -1);
      if (schema_xref == schema)
        {
          save_iter_for_schema (xmp_model, schema, iter);
          return TRUE;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (xmp_model->treestore),
                                   iter));
  return FALSE;
}

/* remove a property from the list of children of schema_iter */
static void
find_and_remove_property (XMPModel    *xmp_model,
                          XMPProperty *property,
                          GtkTreeIter *schema_iter)
{
  GtkTreeIter  child_iter;
  XMPProperty *property_xref;

  if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (xmp_model->treestore),
                                      &child_iter, schema_iter))
    return;
  for (;;)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), &child_iter,
                          COL_XMP_TYPE_XREF, &property_xref,
                          -1);
      if (property_xref == property)
        {
          if (! gtk_tree_store_remove (GTK_TREE_STORE (xmp_model->treestore),
                                       &child_iter))
            break;
        }
      else
        {
          if (! gtk_tree_model_iter_next (GTK_TREE_MODEL(xmp_model->treestore),
                                          &child_iter))
            break;
        }
    }
}

/* add a schema to the tree */
static void
add_known_schema (XMPModel    *xmp_model,
                  XMPSchema   *schema,
                  GtkTreeIter *iter)
{
  gtk_tree_store_append (xmp_model->treestore, iter, NULL);
  gtk_tree_store_set (xmp_model->treestore, iter,
                      COL_XMP_NAME, schema->name,
                      COL_XMP_VALUE, schema->uri,
                      COL_XMP_VALUE_RAW, NULL,
                      COL_XMP_TYPE_XREF, schema,
                      COL_XMP_WIDGET_XREF, NULL,
                      COL_XMP_EDITABLE, FALSE,
                      COL_XMP_EDIT_ICON, NULL,
                      COL_XMP_VISIBLE, FALSE,
                      COL_XMP_WEIGHT, PANGO_WEIGHT_BOLD,
                      COL_XMP_WEIGHT_SET, TRUE,
                      -1);
  save_iter_for_schema (xmp_model, schema, iter);
}

/* called by the XMP parser - new schema */
static gpointer
parse_start_schema (XMPParseContext     *context,
                    const gchar         *ns_uri,
                    const gchar         *ns_prefix,
                    gpointer             user_data,
                    GError             **error)
{
  XMPModel    *xmp_model = user_data;
  GtkTreeIter  iter;
  XMPSchema   *schema;

  g_return_val_if_fail (xmp_model != NULL, NULL);
  schema = find_xmp_schema (xmp_model, ns_uri);
  if (schema == NULL)
    {
      /* add schema to custom_schemas */
      schema = g_new (XMPSchema, 1);
      schema->uri = g_strdup (ns_uri);
      schema->prefix = g_strdup (ns_prefix);
      schema->name = schema->uri;
      schema->properties = NULL;
      xmp_model->custom_schemas = g_slist_prepend (xmp_model->custom_schemas,
                                                   schema);
    }
  else if (find_iter_for_schema (xmp_model, schema, &iter))
    {
      /* already in the tree, so no need to add it again */
      return schema;
    }
  /* schemas with NULL names are special and should not go in the tree */
  if (schema->name == NULL)
    {
      save_iter_for_schema (xmp_model, NULL, NULL);
      return schema;
    }
  /* if the schema is not in the tree yet, add it now */
  add_known_schema (xmp_model, schema, &iter);
  return schema;
}

/* called by the XMP parser - end of schema */
static void
parse_end_schema (XMPParseContext     *context,
                  gpointer             ns_user_data,
                  gpointer             user_data,
                  GError             **error)
{
  XMPModel  *xmp_model = user_data;
  XMPSchema *schema = ns_user_data;

  g_return_if_fail (xmp_model != NULL);
  g_return_if_fail (schema != NULL);
  xmp_model->current_schema = NULL;
  /* printf ("End of %s\n", schema->name); */
}

/* called by the XMP parser - new property */
static void
parse_set_property (XMPParseContext     *context,
                    const gchar         *name,
                    XMPParseType         type,
                    const gchar        **value,
                    gpointer             ns_user_data,
                    gpointer             user_data,
                    GError             **error)
{
  XMPModel    *xmp_model = user_data;
  XMPSchema   *schema = ns_user_data;
  int          i;
  const gchar *ns_prefix;
  XMPProperty *property;
  GtkTreeIter  iter;
  GtkTreeIter  child_iter;
  gchar       *tmp_name;
  gchar       *tmp_value;

  g_return_if_fail (xmp_model != NULL);
  g_return_if_fail (schema != NULL);
  if (! find_iter_for_schema (xmp_model, schema, &iter))
    {
      g_warning ("Unable to set XMP property '%s' because its schema is bad",
                 name);
      return;
    }
  ns_prefix = schema->prefix;
  property = NULL;
  if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, name))
        {
          property = &(schema->properties[i]);
          break;
        }
  /* if the same property was already present, remove it (replace it) */
  if (property != NULL)
    find_and_remove_property (xmp_model, property, &iter);

  switch (type)
    {
    case XMP_PTYPE_TEXT:
#ifdef DEBUG_XMP_PARSER
      printf ("\t%s:%s = \"%s\"\n", ns_prefix, name, value[0]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_TEXT;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, name,
                          COL_XMP_VALUE, value[0],
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      break;

    case XMP_PTYPE_RESOURCE:
#ifdef DEBUG_XMP_PARSER
      printf ("\t%s:%s @ = \"%s\"\n", ns_prefix, name,
              value[0]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_URI;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      tmp_name = g_strconcat (name, " @", NULL);
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, tmp_name,
                          COL_XMP_VALUE, value[0],
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      g_free (tmp_name);
      break;

    case XMP_PTYPE_ORDERED_LIST:
    case XMP_PTYPE_UNORDERED_LIST:
#ifdef DEBUG_XMP_PARSER
      printf ("\t%s:%s [] =", ns_prefix, name);
      for (i = 0; value[i] != NULL; i++)
        if (i == 0)
          printf (" \"%s\"", value[i]);
        else
          printf (", \"%s\"", value[i]);
      printf ("\n");
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = ((type == XMP_PTYPE_ORDERED_LIST)
                            ? XMP_TYPE_TEXT_BAG
                            : XMP_TYPE_TEXT_SEQ);
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }

      tmp_name = g_strconcat (name, " []", NULL);
      tmp_value = g_strjoinv ("; ", (gchar **) value);
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, tmp_name,
                          COL_XMP_VALUE, tmp_value,
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      g_free (tmp_value);
      g_free (tmp_name);
      break;

    case XMP_PTYPE_ALT_LANG:
#ifdef DEBUG_XMP_PARSER
      for (i = 0; value[i] != NULL; i += 2)
        printf ("\t%s:%s [lang:%s] = \"%s\"\n", ns_prefix, name,
                value[i], value[i + 1]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_LANG_ALT;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      for (i = 0; value[i] != NULL; i += 2)
        {
          tmp_name = g_strconcat (name, " [", value[i], "]", NULL);
          gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
          gtk_tree_store_set (xmp_model->treestore, &child_iter,
                              COL_XMP_NAME, tmp_name,
                              COL_XMP_VALUE, value[i + 1],
                              COL_XMP_VALUE_RAW, value,
                              COL_XMP_TYPE_XREF, property,
                              COL_XMP_WIDGET_XREF, NULL,
                              COL_XMP_EDITABLE, property->editable,
                              COL_XMP_EDIT_ICON, NULL,
                              COL_XMP_VISIBLE, TRUE,
                              COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                              COL_XMP_WEIGHT_SET, FALSE,
                              -1);
          g_free (tmp_name);
        }
      break;

    case XMP_PTYPE_STRUCTURE:
#ifdef DEBUG_XMP_PARSER
      for (i = 2; value[i] != NULL; i += 2)
        printf ("\t%s:%s [%s] = \"%s\"\n", ns_prefix, name,
                value[i], value[i + 1]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_UNKNOWN;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      for (i = 2; value[i] != NULL; i += 2)
        {
          tmp_name = g_strconcat (name, " [", value[i], "]", NULL);
          gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
          gtk_tree_store_set (xmp_model->treestore, &child_iter,
                              COL_XMP_NAME, tmp_name,
                              COL_XMP_VALUE, value[i + 1],
                              COL_XMP_VALUE_RAW, value,
                              COL_XMP_TYPE_XREF, property,
                              COL_XMP_WIDGET_XREF, NULL,
                              COL_XMP_EDITABLE, property->editable,
                              COL_XMP_EDIT_ICON, NULL,
                              COL_XMP_VISIBLE, TRUE,
                              COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                              COL_XMP_WEIGHT_SET, FALSE,
                              -1);
          g_free (tmp_name);
        }
      break;

    default:
#ifdef DEBUG_XMP_PARSER
      printf ("\t%s:%s = ?\n", ns_prefix, name);
#endif
      break;
    }
}

/* called by the XMP parser - parse error */
static void
parse_error (XMPParseContext *context,
             GError          *error,
             gpointer         user_data)
{
  g_warning ("While parsing XMP metadata:\n%s\n", error->message);
}

static XMPParser xmp_parser = {
  parse_start_schema,
  parse_end_schema,
  parse_set_property,
  parse_error
};

/**
 * xmp_model_parse_buffer:
 * @xmp_model: pointer to the #XMPModel in which the results will be stored
 * @buffer: buffer to be parsed
 * @buffer_length: length of the @buffer
 * @skip_other_data: if %TRUE, allow arbitrary data before XMP packet marker
 * @error: return location for a #GError
 *
 * Parse a buffer containing XMP metadata and merge the parsed contents into
 * the supplied @xmp_model.  If @skip_other_data is %TRUE, then the parser
 * will try to find the <?xpacket...?> marker in the buffer, skipping any
 * unknown data found before it.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 *
 * (Note: this calls the functions from xmp_parse.c, which will call the
 *  functions in this file through the xmp_parser structure defined above.)
 **/
gboolean
xmp_model_parse_buffer (XMPModel     *xmp_model,
                        const gchar  *buffer,
                        gssize        buffer_length,
                        gboolean      skip_other_data,
                        GError      **error)
{
  XMPParseFlags    flags;
  XMPParseContext *context;

  flags = XMP_FLAG_DEFER_VALUE_FREE; /* we will free the array ourselves */
  if (skip_other_data)
    flags |= XMP_FLAG_FIND_XPACKET;

  context = xmp_parse_context_new (&xmp_parser, flags, xmp_model, NULL);

  if (! xmp_parse_context_parse (context, buffer, buffer_length, error))
    {
      xmp_parse_context_free (context);
      return FALSE;
    }

  if (! xmp_parse_context_end_parse (context, error))
    {
      xmp_parse_context_free (context);
      return FALSE;
    }

  xmp_parse_context_free (context);
  return TRUE;
}

/**
 * xmp_model_parse_file:
 * @xmp_model: pointer to the #XMPModel in which the results will be stored
 * @filename: name of the file containing XMP metadata to parse
 * @error: return location for a #GError
  *
 * Try to find XMP metadata in a file and merge its contents into the supplied
 * @xmp_model.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 **/
gboolean
xmp_model_parse_file (XMPModel     *xmp_model,
                      const gchar  *filename,
                      GError      **error)
{
  gchar    *buffer;
  gssize    buffer_length;

  g_return_val_if_fail (filename != NULL, FALSE);
  if (! g_file_get_contents (filename, &buffer, &buffer_length, error))
    return FALSE;
  if (! xmp_model_parse_buffer (xmp_model, buffer, buffer_length,
                                TRUE, error))
    return FALSE;
  g_free (buffer);
  return TRUE;
}

/**
 * xmp_model_get_tree_model:
 * @xmp_model: pointer to an #XMPModel
 *
 * Return a pointer to the #GtkTreeModel contained in the #XMPModel.
 **/
GtkTreeModel *
xmp_model_get_tree_model (XMPModel *xmp_model)
{
  g_return_val_if_fail (xmp_model != NULL, NULL);
  return GTK_TREE_MODEL (xmp_model->treestore);
}

/**
 * xmp_model_get_scalar_property:
 * @xmp_model: pointer to an #XMPModel
 * @schema_name: full URI or usual prefix of the schema
 * @property_name: name of the property to store
 *
 * Store a new value for the specified XMP property.
 *
 * Return value: string representation of the value of that property, or %NULL if the property does not exist
 **/
const gchar *
xmp_model_get_scalar_property (XMPModel    *xmp_model,
                               const gchar *schema_name,
                               const gchar *property_name)
{
  XMPSchema    *schema;
  GtkTreeIter   iter;
  XMPProperty  *property = NULL;
  GtkTreeIter   child_iter;
  int           i;
  XMPProperty  *property_xref;
  const gchar  *value;

  g_return_val_if_fail (xmp_model != NULL, NULL);
  g_return_val_if_fail (schema_name != NULL, NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  schema = find_xmp_schema (xmp_model, schema_name);
  if (! schema)
    schema = find_xmp_schema_prefix (xmp_model, schema_name);
  if (! schema)
    return NULL;
  if (! find_iter_for_schema (xmp_model, schema, &iter))
    return NULL;
 if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, property_name))
        {
          property = &(schema->properties[i]);
          break;
        }
  if (property == NULL)
    return NULL;
  if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (xmp_model->treestore),
                                      &child_iter, &iter))
    return NULL;
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), &child_iter,
                          COL_XMP_TYPE_XREF, &property_xref,
                          COL_XMP_VALUE, &value,
                          -1);
      if (property_xref == property)
        return value;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL(xmp_model->treestore),
                                   &child_iter));
  return NULL;
}

/**
 * xmp_model_set_scalar_property:
 * @xmp_model: pointer to an #XMPModel
 * @schema_name: full URI or usual prefix of the schema
 * @property_name: name of the property to store
 * @property_value: value to store
 *
 * Store a new value for the specified XMP property.
 *
 * Return value: %TRUE if the property was set, %FALSE if an error occured (for example, the @schema_name is invalid)
 **/
gboolean
xmp_model_set_scalar_property (XMPModel    *xmp_model,
                               const gchar *schema_name,
                               const gchar *property_name,
                               const gchar *property_value)
{
  XMPSchema    *schema;
  GtkTreeIter   iter;
  XMPProperty  *property = NULL;
  GtkTreeIter   child_iter;
  int           i;
  gchar       **value;

  g_return_val_if_fail (xmp_model != NULL, FALSE);
  g_return_val_if_fail (schema_name != NULL, FALSE);
  g_return_val_if_fail (property_name != NULL, FALSE);
  g_return_val_if_fail (property_value != NULL, FALSE);
  schema = find_xmp_schema (xmp_model, schema_name);
  if (! schema)
    schema = find_xmp_schema_prefix (xmp_model, schema_name);
  if (! schema)
    return FALSE;

  if (! find_iter_for_schema (xmp_model, schema, &iter))
    add_known_schema (xmp_model, schema, &iter);

 if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, property_name))
        {
          property = &(schema->properties[i]);
          break;
        }
  if (property != NULL)
    find_and_remove_property (xmp_model, property, &iter);
  else
    {
      property = g_new (XMPProperty, 1);
      property->name = g_strdup (property_name);
      property->type = XMP_TYPE_TEXT;
      property->editable = TRUE;
      xmp_model->custom_properties =
        g_slist_prepend (xmp_model->custom_properties, property);
    }

  value = g_new (gchar *, 2);
  value[0] = g_strdup (property_value);
  value[1] = NULL;
  gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
  gtk_tree_store_set (xmp_model->treestore, &child_iter,
                      COL_XMP_NAME, g_strdup (property_name),
                      COL_XMP_VALUE, value[0],
                      COL_XMP_VALUE_RAW, value,
                      COL_XMP_TYPE_XREF, property,
                      COL_XMP_WIDGET_XREF, NULL,
                      COL_XMP_EDITABLE, property->editable,
                      COL_XMP_EDIT_ICON, NULL,
                      COL_XMP_VISIBLE, TRUE,
                      COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                      COL_XMP_WEIGHT_SET, FALSE,
                      -1);
  return TRUE;
}
