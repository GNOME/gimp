/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gexiv2/gexiv2.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include "libgimp/stdplugins-intl.h"

#include "metadata-tags.h"


const metadata_tag default_metadata_tags[] =
{
  /* Description */
  { "Xmp.dc.title",                              "single", 16,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  0
  { "Xmp.dc.creator",                            "single", 13,  TAG_TYPE_XMP, GIMP_XMP_SEQ   }, //  1
  { "Xmp.dc.description",                        "multi",  14,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  2
  { "Xmp.dc.subject",                            "multi",  15,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, //  3
  { "Xmp.dc.rights",                             "single", 17,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  4
  { "Xmp.photoshop.AuthorsPosition",             "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  5
  { "Xmp.photoshop.CaptionWriter",               "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  6
  { "Xmp.xmp.Rating",                            "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, //  7
  { "Xmp.xmpRights.Marked",                      "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, //  8
  { "Xmp.xmpRights.WebStatement",                "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  9

  /* IPTC */
  { "Xmp.photoshop.DateCreated",                 "single",  0,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 10
  { "Xmp.photoshop.Headline",                    "multi",   3,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 11
  { "Xmp.photoshop.TransmissionReference",       "single",  1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 12
  { "Xmp.photoshop.Instructions",                "multi",   2,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 13
  { "Xmp.iptc.IntellectualGenre",                "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 14
  { "Xmp.iptc.Scene",                            "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 15
  { "Xmp.iptc.Location",                         "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 16
  { "Xmp.iptc.CountryCode",                      "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 17
  { "Xmp.iptc.SubjectCode",                      "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 18
  { "Xmp.xmpRights.UsageTerms",                  "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 19
  { "Xmp.photoshop.City",                        "single",  5,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 20
  { "Xmp.photoshop.State",                       "single",  6,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 21
  { "Xmp.photoshop.Country",                     "single",  7,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 22
  { "Xmp.photoshop.CaptionWriter",               "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 23
  { "Xmp.photoshop.Credit",                      "single",  8,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 24
  { "Xmp.photoshop.Source",                      "single",  9,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 25
  { "Xmp.photoshop.Urgency",                     "combo",  11,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 26

  /* IPTC Extension */
  { "Xmp.iptcExt.PersonInImage",                 "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 27
  { "Xmp.iptcExt.Sublocation",                   "single", 12,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 28
  { "Xmp.iptcExt.City",                          "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 29
  { "Xmp.iptcExt.ProvinceState",                 "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 30
  { "Xmp.iptcExt.CountryName",                   "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 31
  { "Xmp.iptcExt.CountryCode",                   "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 32
  { "Xmp.iptcExt.WorldRegion",                   "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 33
  { "Xmp.iptcExt.LocationShown",                 "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 34
  { "Xmp.iptcExt.OrganisationInImageName",       "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 35
  { "Xmp.iptcExt.OrganisationInImageCode",       "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 36
  { "Xmp.iptcExt.Event",                         "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 37
  { "Xmp.iptcExt.RegistryId",                    "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 38
  { "Xmp.iptcExt.ArtworkOrObject",               "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 39
  { "Xmp.iptcExt.AddlModelInfo",                 "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 40
  { "Xmp.iptcExt.ModelAge",                      "single", -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 41
  { "Xmp.iptcExt.MaxAvailWidth",                 "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 42
  { "Xmp.iptcExt.MaxAvailHeight",                "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 43
  { "Xmp.iptcExt.DigitalSourceType",             "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 44
  { "Xmp.plus.MinorModelAgeDisclosure",          "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 45
  { "Xmp.plus.ModelReleaseStatus",               "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 46
  { "Xmp.plus.ModelReleaseID",                   "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 47
  { "Xmp.plus.ImageSupplierName",                "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 48
  { "Xmp.plus.ImageSupplierID",                  "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 49
  { "Xmp.plus.ImageSupplierImageID",             "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 50
  { "Xmp.plus.ImageCreator",                     "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 51
  { "Xmp.plus.CopyrightOwner",                   "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 52
  { "Xmp.plus.Licensor",                         "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 53
  { "Xmp.plus.PropertyReleaseStatus",            "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 54
  { "Xmp.plus.PropertyReleaseID",                "list",   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 55

  /* Categories */
  { "Xmp.photoshop.Category",                    "single",  4,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 56
  { "Xmp.photoshop.SupplementalCategories",      "multi",  10,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 57

  /* GPS */
  { "Exif.GPSInfo.GPSLongitude",                 "single", -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 58
  { "Exif.GPSInfo.GPSLongitudeRef",              "combo",  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 59
  { "Exif.GPSInfo.GPSLatitude",                  "single", -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 60
  { "Exif.GPSInfo.GPSLatitudeRef",               "combo",  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 61
  { "Exif.GPSInfo.GPSAltitude",                  "single", -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 62
  { "Exif.GPSInfo.GPSAltitudeRef",               "combo",  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 63

  /* DICOM */
  { "Xmp.DICOM.PatientName",                     "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 64
  { "Xmp.DICOM.PatientID",                       "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 65
  { "Xmp.DICOM.PatientDOB",                      "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 66
  { "Xmp.DICOM.PatientSex",                      "combo",  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 67
  { "Xmp.DICOM.StudyID",                         "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 68
  { "Xmp.DICOM.StudyPhysician",                  "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 69
  { "Xmp.DICOM.StudyDateTime",                   "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 70
  { "Xmp.DICOM.StudyDescription",                "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 71
  { "Xmp.DICOM.SeriesNumber",                    "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 72
  { "Xmp.DICOM.SeriesModality",                  "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 73
  { "Xmp.DICOM.SeriesDateTime",                  "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 74
  { "Xmp.DICOM.SeriesDescription",               "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 75
  { "Xmp.DICOM.EquipmentInstitution",            "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 76
  { "Xmp.DICOM.EquipmentManufacturer",           "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 77

  /* IPTC */
  { "Xmp.iptc.CiAdrExtadr",                      "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 78
  { "Xmp.iptc.CiAdrCity",                        "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 79
  { "Xmp.iptc.CiAdrRegion",                      "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 80
  { "Xmp.iptc.CiAdrPcode",                       "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 81
  { "Xmp.iptc.CiAdrCtry",                        "single", -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 82
  { "Xmp.iptc.CiTelWork",                        "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 83
  { "Xmp.iptc.CiEmailWork",                      "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 84
  { "Xmp.iptc.CiUrlWork",                        "multi",  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }  // 85

};
const gint n_default_metadata_tags = G_N_ELEMENTS (default_metadata_tags);

const metadata_tag equivalent_metadata_tags[] =
{
  { "Iptc.Application2.DateCreated",             "single", 10,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  0
  { "Iptc.Application2.TransmissionReference",   "single", 12,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  1
  { "Iptc.Application2.SpecialInstructions",     "single", 13,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  2
  { "Iptc.Application2.Headline",                "multi",  11,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  3
  { "Iptc.Application2.Category",                "single", 56,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  4
  { "Iptc.Application2.City",                    "single", 20,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  5
  { "Iptc.Application2.ProvinceState",           "single", 21,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  6
  { "Iptc.Application2.CountryName",             "single", 22,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  7
  { "Iptc.Application2.Credit",                  "single", 24,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  8
  { "Iptc.Application2.Source",                  "single", 25,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, //  9
  { "Iptc.Application2.SuppCategory",            "single", 57,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 10
  { "Iptc.Application2.Urgency",                 "combo",  26,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 11
  { "Iptc.Application2.SubLocation",             "single", 28,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 12
  { "Iptc.Application2.Byline",                  "single",  1,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 13
  { "Iptc.Application2.Caption",                 "multi",   2,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 14
  { "Iptc.Application2.Keywords",                "single",  3,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 15
  { "Iptc.Application2.ObjectName",              "multi",   0,  TAG_TYPE_IPTC, GIMP_XMP_NONE }, // 16
  { "Iptc.Application2.Copyright",               "single",  4,  TAG_TYPE_IPTC, GIMP_XMP_NONE }  // 17
};
const gint n_equivalent_metadata_tags = G_N_ELEMENTS (equivalent_metadata_tags);

/* Digital Source Type Combobox Items
 * http://cv.iptc.org/newscodes/digitalsourcetype/
 */
const combobox_str_tag digitalsourcetype[] =
{
  { "http://cv.iptc.org/newscodes/digitalsourcetype/digitalCapture", N_("Original digital capture of a real life scene")    },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/negativeFilm",   N_("Digitized from a negative on film")                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/positiveFilm",   N_("Digitized from a positive on film")                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/print",          N_("Digitized from a print on non-transparent medium") },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/softwareImage",  N_("Created by software")                              }
};
const gint n_digitalsourcetype = G_N_ELEMENTS (digitalsourcetype);

/* Model Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#ModelReleaseStatus
 */
const combobox_str_tag modelreleasestatus[] =
{
  { "http://ns.useplus.org/ldf/vocab/MR-NON", N_("None")                                 },
  { "http://ns.useplus.org/ldf/vocab/MR-NAP", N_("Not Applicable")                       },
  { "http://ns.useplus.org/ldf/vocab/MR-NAP", N_("Unlimited Model Releases")             },
  { "http://ns.useplus.org/ldf/vocab/MR-LPR", N_("Limited or Incomplete Model Releases") }
};
const gint n_modelreleasestatus = G_N_ELEMENTS (modelreleasestatus);

/* Property Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#PropertyReleaseStatus
 */
const combobox_str_tag propertyreleasestatus[] =
{
  { "http://ns.useplus.org/ldf/vocab/PR-NON", N_("None")                                    },
  { "http://ns.useplus.org/ldf/vocab/PR-NAP", N_("Not Applicable")                          },
  { "http://ns.useplus.org/ldf/vocab/PR-NAP", N_("Unlimited Property Releases")             },
  { "http://ns.useplus.org/ldf/vocab/PR-LPR", N_("Limited or Incomplete Property Releases") }
};
const gint n_propertyreleasestatus = G_N_ELEMENTS (propertyreleasestatus);

/* Minor Model Age Disclosure Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#MinorModelAgeDisclosure
 */
const combobox_str_tag minormodelagedisclosure[] =
{
  { "http://ns.useplus.org/ldf/vocab/AG-UNK", N_("Age Unknown")     },
  { "http://ns.useplus.org/ldf/vocab/AG-A25", N_("Age 25 or Over")  },
  { "http://ns.useplus.org/ldf/vocab/AG-A24", N_("Age 24")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A23", N_("Age 23")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A22", N_("Age 22")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A21", N_("Age 21")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A20", N_("Age 20")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A19", N_("Age 19")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A18", N_("Age 18")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A17", N_("Age 17")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A16", N_("Age 16")          },
  { "http://ns.useplus.org/ldf/vocab/AG-A15", N_("Age 15")          },
  { "http://ns.useplus.org/ldf/vocab/AG-U14", N_("Age 14 or Under") }
};
const gint n_minormodelagedisclosure = G_N_ELEMENTS (minormodelagedisclosure);

/* Urgency */
const gchar *urgency[] =
{
  N_("None"), N_("High"), N_("2"), N_("3"), N_("4"), N_("Normal"), N_("6"), N_("7"), N_("Low")
};
const gint n_urgency = G_N_ELEMENTS (urgency);

/* Marked */
const combobox_int_tag marked[] =
{
  { -1,     N_("Unknown")       }, // DO NOT SAVE
  {  TRUE,  N_("Copyrighted")   }, // TRUE
  {  FALSE, N_("Public Domain") }, // FALSE
};
const gint n_marked = G_N_ELEMENTS (marked);

/* Phone Types */
const combobox_str_tag phone_types[] =
{
  { "",                                      N_("Select a value") },
  { "http://ns.useplus.org/ldf/vocab/work",  N_("Work")  },
  { "http://ns.useplus.org/ldf/vocab/cell",  N_("Cell")  },
  { "http://ns.useplus.org/ldf/vocab/fax",   N_("Fax")   },
  { "http://ns.useplus.org/ldf/vocab/home",  N_("Home")  },
  { "http://ns.useplus.org/ldf/vocab/pager", N_("Pager") }
};
const gint n_phone_types = G_N_ELEMENTS (phone_types);

/* DICOM Patient Sex
 * http://dicomlookup.com/lookup.asp?sw=Ttable&q=C.7-1
 * http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/XMP.html#DICOM
 * https://dicomiseasy.blogspot.ca/2011/11/introduction-to-dicom-chapter-iii-dicom.html
 * http://dicom.nema.org/standard.html
 */
const combobox_str_tag dicom[] =
{
  { "",        N_("Select a value") },
  { "male",    N_("Male")    },
  { "female",  N_("Female")  },
  { "other",   N_("Other")   },
};
const gint n_dicom = G_N_ELEMENTS (dicom);

/* GPS Altitude Ref */
const gchar *gpsaltref[] =
{
  N_("Unknown"), N_("Above Sea Level"), N_("Below Sea Level")
};
const gint n_gpsaltref = G_N_ELEMENTS (gpsaltref);

/* GPS Latitude Ref */
const gchar *gpslatref[] =
{
  N_("Unknown"), N_("North"), N_("South")
};
const gint n_gpslatref = G_N_ELEMENTS (gpslatref);

/* GPS Longitude Ref */
const gchar *gpslngref[] =
{
  N_("Unknown"), N_("East"), N_("West")
};
const gint n_gpslngref = G_N_ELEMENTS (gpslngref);

/* GPS Measurement System */
const gchar *gpsaltsys[] =
{
  "M", "FT"
};
const gint n_gpsaltsys = G_N_ELEMENTS (gpsaltsys);

const TranslateHeaderTag creatorContactInfoHeader =
{
  "Xmp.iptc.CreatorContactInfo", "type=\"Struct\"", 8
};

const TranslateTag creatorContactInfoTags[] =
{
  { "Xmp.iptc.CiAdrExtadr", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrExtadr", "multi",  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrCity",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrCity",   "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrRegion", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrRegion", "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrPcode",  "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrPcode",  "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrCtry",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrCtry",   "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiTelWork",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiTelWork",   "multi",  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiEmailWork", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiEmailWork", "multi",  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiUrlWork",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiUrlWork",   "multi",  -1,  TAG_TYPE_XMP }
};

const TranslateHeaderTag locationCreationInfoHeader =
{
  "Xmp.iptcExt.LocationCreated", "type=\"Bag\"", 6
};

const TranslateTag locationCreationInfoTags[] =
{
  { "Xmp.iptcExt.Sublocation",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:Sublocation",   "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.City",          "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:City",          "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.ProvinceState", "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:ProvinceState", "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.CountryName",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:CountryName",   "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.CountryCode",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:CountryCode",   "single", -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.WorldRegion",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:WorldRegion",   "single", -1,  TAG_TYPE_XMP }
};

const TranslateHeaderTag imageSupplierInfoHeader =
{
  "Xmp.plus.ImageSupplier", "type=\"Seq\"", 2
};

const TranslateTag imageSupplierInfoTags[] =
{
  { "Xmp.plus.ImageSupplierName",   "Xmp.plus.ImageSupplier[1]/plus:ImageSupplierName", "multi",  -1,  TAG_TYPE_XMP },
  { "Xmp.plus.ImageSupplierID",     "Xmp.plus.ImageSupplier[1]/plus:ImageSupplierID",   "single", -1,  TAG_TYPE_XMP }
};

/* Plus and IPTC extension tags */

const gchar *licensor[] =
{
  "/plus:LicensorName",
  "/plus:LicensorID",
  "/plus:LicensorTelephone1",
  "/plus:LicensorTelephoneType1",
  "/plus:LicensorTelephone2",
  "/plus:LicensorTelephoneType2",
  "/plus:LicensorEmail",
  "/plus:LicensorURL"
};
const gint n_licensor = G_N_ELEMENTS (licensor);

#ifdef USE_TAGS
const gchar *imagesupplier[] =
{
  "/plus:ImageSupplierName",
  "/plus:ImageSupplierID"
};
const gint n_imagesupplier = G_N_ELEMENTS (imagesupplier);
#endif

const gchar *imagecreator[] =
{
  "/plus:ImageCreatorName",
  "/plus:ImageCreatorID"
};
const gint n_imagecreator = G_N_ELEMENTS (imagecreator);

const gchar *copyrightowner[] =
{
  "/plus:CopyrightOwnerName",
  "/plus:CopyrightOwnerID"
};
const gint n_copyrightowner = G_N_ELEMENTS (copyrightowner);

const gchar *registryid[] =
{
  "/Iptc4xmpExt:RegOrgId",
  "/Iptc4xmpExt:RegItemId"
};
const gint n_registryid = G_N_ELEMENTS (registryid);

const gchar *artworkorobject[] =
{
  "/Iptc4xmpExt:AODateCreated",
  "/Iptc4xmpExt:AOSource",
  "/Iptc4xmpExt:AOSourceInvNo",
  "/Iptc4xmpExt:AOTitle",
  "/Iptc4xmpExt:AOCopyrightNotice",
  "/Iptc4xmpExt:AOCreator"
};
const gint n_artworkorobject = G_N_ELEMENTS (artworkorobject);

const gchar *locationshown[] =
{
  "/Iptc4xmpExt:Sublocation",
  "/Iptc4xmpExt:City",
  "/Iptc4xmpExt:ProvinceState",
  "/Iptc4xmpExt:CountryName",
  "/Iptc4xmpExt:CountryCode",
  "/Iptc4xmpExt:WorldRegion"
};
const gint n_locationshown = G_N_ELEMENTS (locationshown);

#ifdef USE_TAGS
const gchar *locationcreated[] =
{
  "/Iptc4xmpExt:Sublocation",
  "/Iptc4xmpExt:City",
  "/Iptc4xmpExt:ProvinceState",
  "/Iptc4xmpExt:CountryName",
  "/Iptc4xmpExt:CountryCode",
  "/Iptc4xmpExt:WorldRegion"
};
const gint n_locationcreated = G_N_ELEMENTS (locationcreated);
#endif
