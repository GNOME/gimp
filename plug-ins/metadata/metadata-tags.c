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


/* The meaning of MODE_SINGLE and MODE_MULTI here denotes whether it is used
 * in a single line or a multi line edit field.
 * Depending on it's xmp type multi line can be saved as either:
 * - one tag of type text, possibly including newlines
 * - an array of tags of the same type for seq and bag, where each line in
 *   the multi line edit will be one item in the array
 */
const metadata_tag default_metadata_tags[] =
{
  /* Description */
  { "Xmp.dc.title",                              MODE_SINGLE, 16,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  0
  { "Xmp.dc.creator",                            MODE_MULTI,  13,  TAG_TYPE_XMP, GIMP_XMP_SEQ   }, //  1
  { "Xmp.dc.description",                        MODE_MULTI,  14,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  2
  { "Xmp.dc.subject",                            MODE_MULTI,  15,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, //  3
  { "Xmp.dc.rights",                             MODE_SINGLE, 17,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  4
  { "Xmp.photoshop.AuthorsPosition",             MODE_SINGLE, 19,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  5
  { "Xmp.photoshop.CaptionWriter",               MODE_SINGLE, 21,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  6
  { "Xmp.xmp.Rating",                            MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, //  7
  { "Xmp.xmpRights.Marked",                      MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, //  8
  { "Xmp.xmpRights.WebStatement",                MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, //  9

  /* IPTC */
  { "Xmp.photoshop.DateCreated",                 MODE_SINGLE,  0,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 10
  { "Xmp.photoshop.Headline",                    MODE_MULTI,   3,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 11
  { "Xmp.photoshop.TransmissionReference",       MODE_SINGLE,  1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 12
  { "Xmp.photoshop.Instructions",                MODE_MULTI,   2,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 13
  { "Xmp.iptc.IntellectualGenre",                MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 14
  { "Xmp.iptc.Scene",                            MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 15
  { "Xmp.iptc.Location",                         MODE_SINGLE, 18,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 16
  { "Xmp.iptc.CountryCode",                      MODE_SINGLE, 20,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 17
  { "Xmp.iptc.SubjectCode",                      MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 18
  { "Xmp.xmpRights.UsageTerms",                  MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 19
  { "Xmp.photoshop.City",                        MODE_SINGLE,  5,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 20
  { "Xmp.photoshop.State",                       MODE_SINGLE,  6,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 21
  { "Xmp.photoshop.Country",                     MODE_SINGLE,  7,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 22
  /* Xmp.photoshop.CaptionWriter here is a duplicate of #6 above. We keep it here to not have
   * to renumber the tag references. It seems it is not used on the IPTC tab. */
  { "Xmp.photoshop.CaptionWriter",               MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 23
  { "Xmp.photoshop.Credit",                      MODE_SINGLE,  8,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 24
  { "Xmp.photoshop.Source",                      MODE_SINGLE,  9,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 25
  { "Xmp.photoshop.Urgency",                     MODE_COMBO,  11,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 26

  /* IPTC Extension */
  { "Xmp.iptcExt.PersonInImage",                 MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 27
  { "Xmp.iptcExt.Sublocation",                   MODE_SINGLE, 12,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 28
  { "Xmp.iptcExt.City",                          MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 29
  { "Xmp.iptcExt.ProvinceState",                 MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 30
  { "Xmp.iptcExt.CountryName",                   MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 31
  { "Xmp.iptcExt.CountryCode",                   MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 32
  { "Xmp.iptcExt.WorldRegion",                   MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 33
  { "Xmp.iptcExt.LocationShown",                 MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 34
  { "Xmp.iptcExt.OrganisationInImageName",       MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 35
  { "Xmp.iptcExt.OrganisationInImageCode",       MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 36
  { "Xmp.iptcExt.Event",                         MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 37
  { "Xmp.iptcExt.RegistryId",                    MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 38
  { "Xmp.iptcExt.ArtworkOrObject",               MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 39
  { "Xmp.iptcExt.AddlModelInfo",                 MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 40
  { "Xmp.iptcExt.ModelAge",                      MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 41
  { "Xmp.iptcExt.MaxAvailWidth",                 MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 42
  { "Xmp.iptcExt.MaxAvailHeight",                MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 43
  { "Xmp.iptcExt.DigitalSourceType",             MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 44
  { "Xmp.plus.MinorModelAgeDisclosure",          MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 45
  { "Xmp.plus.ModelReleaseStatus",               MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 46
  { "Xmp.plus.ModelReleaseID",                   MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 47
  { "Xmp.plus.ImageSupplierName",                MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 48
  { "Xmp.plus.ImageSupplierID",                  MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 49
  { "Xmp.plus.ImageSupplierImageID",             MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 50
  { "Xmp.plus.ImageCreator",                     MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 51
  { "Xmp.plus.CopyrightOwner",                   MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 52
  { "Xmp.plus.Licensor",                         MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 53
  { "Xmp.plus.PropertyReleaseStatus",            MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 54
  { "Xmp.plus.PropertyReleaseID",                MODE_LIST,   -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 55

  /* Categories */
  { "Xmp.photoshop.Category",                    MODE_SINGLE,  4,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 56
  { "Xmp.photoshop.SupplementalCategories",      MODE_MULTI,  10,  TAG_TYPE_XMP, GIMP_XMP_BAG   }, // 57

  /* GPS */
  { "Exif.GPSInfo.GPSLongitude",                 MODE_SINGLE, -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 58
  { "Exif.GPSInfo.GPSLongitudeRef",              MODE_COMBO,  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 59
  { "Exif.GPSInfo.GPSLatitude",                  MODE_SINGLE, -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 60
  { "Exif.GPSInfo.GPSLatitudeRef",               MODE_COMBO,  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 61
  { "Exif.GPSInfo.GPSAltitude",                  MODE_SINGLE, -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 62
  { "Exif.GPSInfo.GPSAltitudeRef",               MODE_COMBO,  -1,  TAG_TYPE_EXIF, GIMP_XMP_NONE }, // 63

  /* DICOM */
  { "Xmp.DICOM.PatientName",                     MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 64
  { "Xmp.DICOM.PatientID",                       MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 65
  { "Xmp.DICOM.PatientDOB",                      MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 66
  { "Xmp.DICOM.PatientSex",                      MODE_COMBO,  -1,  TAG_TYPE_XMP, GIMP_XMP_NONE  }, // 67
  { "Xmp.DICOM.StudyID",                         MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 68
  { "Xmp.DICOM.StudyPhysician",                  MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 69
  { "Xmp.DICOM.StudyDateTime",                   MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 70
  { "Xmp.DICOM.StudyDescription",                MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 71
  { "Xmp.DICOM.SeriesNumber",                    MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 72
  { "Xmp.DICOM.SeriesModality",                  MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 73
  { "Xmp.DICOM.SeriesDateTime",                  MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 74
  { "Xmp.DICOM.SeriesDescription",               MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 75
  { "Xmp.DICOM.EquipmentInstitution",            MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 76
  { "Xmp.DICOM.EquipmentManufacturer",           MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 77

  /* IPTC */
  { "Xmp.iptc.CiAdrExtadr",                      MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 78
  { "Xmp.iptc.CiAdrCity",                        MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 79
  { "Xmp.iptc.CiAdrRegion",                      MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 80
  { "Xmp.iptc.CiAdrPcode",                       MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 81
  { "Xmp.iptc.CiAdrCtry",                        MODE_SINGLE, -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 82
  { "Xmp.iptc.CiTelWork",                        MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 83
  { "Xmp.iptc.CiEmailWork",                      MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }, // 84
  { "Xmp.iptc.CiUrlWork",                        MODE_MULTI,  -1,  TAG_TYPE_XMP, GIMP_XMP_TEXT  }  // 85

};
const gint n_default_metadata_tags = G_N_ELEMENTS (default_metadata_tags);

/* Then meaning of MODE_SINGLE and MODE_MULTI below is a little different than above.
 * MODE_SINGLE - for iptc tags that can appear only once,
 * MODE_MULTI  - for iptc tags that are repeatable, i.e. can appear multiple times.
 */
const iptc_tag_info equivalent_metadata_tags[] =
{
  { "Iptc.Application2.DateCreated",             MODE_SINGLE, 10, -1 }, //  0
  { "Iptc.Application2.TransmissionReference",   MODE_SINGLE, 12, -1 }, //  1
  { "Iptc.Application2.SpecialInstructions",     MODE_SINGLE, 13, -1 }, //  2
  { "Iptc.Application2.Headline",                MODE_SINGLE, 11, -1 }, //  3
  { "Iptc.Application2.Category",                MODE_SINGLE, 56, -1 }, //  4
  { "Iptc.Application2.City",                    MODE_SINGLE, 20, -1 }, //  5
  { "Iptc.Application2.ProvinceState",           MODE_SINGLE, 21, -1 }, //  6
  { "Iptc.Application2.CountryName",             MODE_SINGLE, 22, -1 }, //  7
  { "Iptc.Application2.Credit",                  MODE_SINGLE, 24, -1 }, //  8
  { "Iptc.Application2.Source",                  MODE_SINGLE, 25, -1 }, //  9
  { "Iptc.Application2.SuppCategory",            MODE_MULTI,  57, -1 }, // 10
  { "Iptc.Application2.Urgency",                 MODE_COMBO,  26, -1 }, // 11
  { "Iptc.Application2.SubLocation",             MODE_SINGLE, 28, -1 }, // 12
  { "Iptc.Application2.Byline",                  MODE_SINGLE,  1,  0 }, // 13
  { "Iptc.Application2.Caption",                 MODE_SINGLE,  2,  1 }, // 14
  { "Iptc.Application2.Keywords",                MODE_MULTI,   3, -1 }, // 15
  { "Iptc.Application2.ObjectName",              MODE_SINGLE,  0, -1 }, // 16
  { "Iptc.Application2.Copyright",               MODE_SINGLE,  4,  2 }, // 17
  { "Iptc.Application2.LocationName",            MODE_MULTI,  16, -1 }, // 18
  { "Iptc.Application2.BylineTitle",             MODE_MULTI,   5, -1 }, // 19
  { "Iptc.Application2.CountryCode",             MODE_SINGLE, 17, -1 }, // 20
  { "Iptc.Application2.Writer",                  MODE_MULTI,   6, -1 }, // 21
};
const gint n_equivalent_metadata_tags = G_N_ELEMENTS (equivalent_metadata_tags);

const exif_tag_info exif_equivalent_tags[] =
{
  { 1, "Exif.Image.Artist",           MODE_SINGLE}, //  0
  { 2, "Exif.Image.ImageDescription", MODE_SINGLE}, //  1
  { 4, "Exif.Image.Copyright",        MODE_SINGLE}, //  2
};

/* Digital Source Type Combobox Items
 * http://cv.iptc.org/newscodes/digitalsourcetype/
 */
const combobox_str_tag digitalsourcetype[] =
{
  { "",                                                                       N_("Select a value")                                   },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/digitalCapture",          N_("Original digital capture of a real life scene")    },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/negativeFilm",            N_("Digitized from a negative on film")                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/positiveFilm",            N_("Digitized from a positive on film")                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/print",                   N_("Digitized from a print on non-transparent medium") },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/minorHumanEdits",         N_("Original media with minor human edits")            },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/compositeCapture",        N_("Composite of captured elements")                   },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/algorithmicallyEnhanced", N_("Algorithmically-enhanced media")                   },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/dataDrivenMedia",         N_("Data-driven media")                                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/digitalArt",              N_("Digital art")                                      },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/virtualRecording",        N_("Virtual recording")                                },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/compositeSynthetic",      N_("Composite including synthetic elements")           },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/trainedAlgorithmicMedia", N_("Trained algorithmic media")                        },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/algorithmicMedia",        N_("Pure algorithmic media")                           },
  { "http://cv.iptc.org/newscodes/digitalsourcetype/softwareImage",           N_("Created by software")                              }
};
const gint n_digitalsourcetype = G_N_ELEMENTS (digitalsourcetype);

/* Model Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#ModelReleaseStatus
 */
const combobox_str_tag modelreleasestatus[] =
{
  { "",                                       N_("Select a value")                       },
  { "http://ns.useplus.org/ldf/vocab/MR-NON", N_("None")                                 },
  { "http://ns.useplus.org/ldf/vocab/MR-NAP", N_("Not Applicable")                       },
  { "http://ns.useplus.org/ldf/vocab/MR-UMR", N_("Unlimited Model Releases")             },
  { "http://ns.useplus.org/ldf/vocab/MR-LMR", N_("Limited or Incomplete Model Releases") }
};
const gint n_modelreleasestatus = G_N_ELEMENTS (modelreleasestatus);

/* Property Release Status Combobox Items
 * http://ns.useplus.org/LDF/ldf-XMPSpecification#PropertyReleaseStatus
 */
const combobox_str_tag propertyreleasestatus[] =
{
  { "http://ns.useplus.org/ldf/vocab/PR-NON", N_("None")                                    },
  { "http://ns.useplus.org/ldf/vocab/PR-NAP", N_("Not Applicable")                          },
  { "http://ns.useplus.org/ldf/vocab/PR-UPR", N_("Unlimited Property Releases")             },
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
  N_("Unknown"), N_("Above sea level"), N_("Below sea level")
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
  "m", "ft"
};
const gint n_gpsaltsys = G_N_ELEMENTS (gpsaltsys);

const TranslateTag creatorContactInfoTags[] =
{
  { "Xmp.iptc.CiAdrExtadr", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrExtadr", MODE_MULTI,  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrCity",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrCity",   MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrRegion", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrRegion", MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrPcode",  "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrPcode",  MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiAdrCtry",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiAdrCtry",   MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiTelWork",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiTelWork",   MODE_MULTI,  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiEmailWork", "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiEmailWork", MODE_MULTI,  -1,  TAG_TYPE_XMP },
  { "Xmp.iptc.CiUrlWork",   "Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:CiUrlWork",   MODE_MULTI,  -1,  TAG_TYPE_XMP }
};
const gint n_creatorContactInfoTags = G_N_ELEMENTS (creatorContactInfoTags);

const TranslateTag locationCreationInfoTags[] =
{
  { "Xmp.iptcExt.Sublocation",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:Sublocation",   MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.City",          "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:City",          MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.ProvinceState", "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:ProvinceState", MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.CountryName",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:CountryName",   MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.CountryCode",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:CountryCode",   MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.iptcExt.WorldRegion",   "Xmp.iptcExt.LocationCreated[1]/Iptc4xmpExt:WorldRegion",   MODE_SINGLE, -1,  TAG_TYPE_XMP }
};
const gint n_locationCreationInfoTags = G_N_ELEMENTS (locationCreationInfoTags);

const TranslateTag imageSupplierInfoTags[] =
{
  { "Xmp.plus.ImageSupplierName",   "Xmp.plus.ImageSupplier[1]/plus:ImageSupplierName", MODE_SINGLE, -1,  TAG_TYPE_XMP },
  { "Xmp.plus.ImageSupplierID",     "Xmp.plus.ImageSupplier[1]/plus:ImageSupplierID",   MODE_SINGLE, -1,  TAG_TYPE_XMP }
};
const gint n_imageSupplierInfoTags = G_N_ELEMENTS (imageSupplierInfoTags);

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

const gint licensor_special_handling[] =
{
  METADATA_NONE,
  METADATA_NONE,
  METADATA_NONE,
  METADATA_PHONETYPE,
  METADATA_NONE,
  METADATA_PHONETYPE,
  METADATA_NONE,
  METADATA_NONE
};

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

const gchar *registryid_alternative[] =
{
  "/iptcExt:RegOrgId",
  "/iptcExt:RegItemId"
};

const gchar *artworkorobject[] =
{
  "/Iptc4xmpExt:AOTitle",
  "/Iptc4xmpExt:AODateCreated",
  "/Iptc4xmpExt:AOCreator",
  "/Iptc4xmpExt:AOSource",
  "/Iptc4xmpExt:AOSourceInvNo",
  "/Iptc4xmpExt:AOCopyrightNotice",
};
const gint n_artworkorobject = G_N_ELEMENTS (artworkorobject);

const gchar *artworkorobject_alternative[] =
{
  "/iptcExt:AOTitle",
  "/iptcExt:AODateCreated",
  "/iptcExt:AOCreator",
  "/iptcExt:AOSource",
  "/iptcExt:AOSourceInvNo",
  "/iptcExt:AOCopyrightNotice",
};

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

const gchar *locationshown_alternative[] =
{
  "/iptcExt:Sublocation",
  "/iptcExt:City",
  "/iptcExt:ProvinceState",
  "/iptcExt:CountryName",
  "/iptcExt:CountryCode",
  "/iptcExt:WorldRegion"
};


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


gchar *
metadata_format_gps_longitude_latitude (const gdouble value)
{
  gint     deg, min;
  gdouble  sec;
  gdouble  gps_value = value;

  if (gps_value < 0.f)
    gps_value *= -1.f;

  deg = (gint) gps_value;
  min = (gint) ((gps_value - (gdouble) deg) * 60.f);
  sec = ((gps_value - (gdouble) deg - (gdouble) (min / 60.f)) * 60.f * 60.f);

  return g_strdup_printf ("%ddeg %d' %.3f\"", deg, min, sec);
}

/*
 * use_meter: True return meters, False return feet
 * measurement_symbol: Should be "m", "ft", or empty string (not NULL)
 */
gchar *
metadata_format_gps_altitude (const gdouble  value,
                              gboolean       use_meter,
                              gchar         *measurement_symbol)
{
  gdouble  gps_value = value;

  if (gps_value < 0.f)
    gps_value *= -1.f;

  if (! use_meter)
    {
      gps_value *= 3.28;
    }

  return g_strdup_printf ("%.2f%s", gps_value, measurement_symbol);
}
