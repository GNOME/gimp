/* page-artwork.c
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metainfo-helper.h"
#include "page-artwork.h"
#include "datapage.h"


#define STRUCTURES_ON_PAGE 1
#define COMBOBOX_WITH_DATA 2


static StructureElement struct_element [] =
    {
        {0,
            "Artwork or Object",
            "Xmp.iptcExt.ArtworkOrObject",
            STRUCTURE_TYPE_BAG,
            "artworkorobject-label",
            "artworkorobject-combo",
            "artworkorobject-liststore",
            "artworkorobject-button-plus",
            "artworkorobject-button-minus"}
    };


static ComboBoxData combobox_data[][14] =
{
    {
        {N_(" "),                                    ""},
        {N_("unknown"),                              "http://ns.useplus.org/ldf/vocab/AG-UNK"},
        {N_("Age 25 or over"),                       "http://ns.useplus.org/ldf/vocab/AG-A25"},
        {N_("Age 24"),                               "http://ns.useplus.org/ldf/vocab/AG-A24"},
        {N_("Age 23"),                               "http://ns.useplus.org/ldf/vocab/AG-A23"},
        {N_("Age 22"),                               "http://ns.useplus.org/ldf/vocab/AG-A22"},
        {N_("Age 21"),                               "http://ns.useplus.org/ldf/vocab/AG-A21"},
        {N_("Age 20"),                               "http://ns.useplus.org/ldf/vocab/AG-A20"},
        {N_("Age 19"),                               "http://ns.useplus.org/ldf/vocab/AG-A19"},
        {N_("Age 18"),                               "http://ns.useplus.org/ldf/vocab/AG-A18"},
        {N_("Age 17"),                               "http://ns.useplus.org/ldf/vocab/AG-A17"},
        {N_("Age 16"),                               "http://ns.useplus.org/ldf/vocab/AG-A16"},
        {N_("Age 15"),                               "http://ns.useplus.org/ldf/vocab/AG-A15"},
        {N_("Age 14 or under"),                      "http://ns.useplus.org/ldf/vocab/AG-U14"}
    },
    {
        {N_(" "),                                    ""},
        {N_("None"),                                 "http://ns.useplus.org/ldf/vocab/MR-NON"},
        {N_("Not Applicable"),                       "http://ns.useplus.org/ldf/vocab/MR-NAP"},
        {N_("Unlimited Model Releases"),             "http://ns.useplus.org/ldf/vocab/MR-UMR"},
        {N_("Limited or Incomplete Model Releases"), "http://ns.useplus.org/ldf/vocab/MR-LMR"}
    },
};

static MetadataEntry artwork_entries[] =
    {
        {"Title",
            "artworkorobject-title-label",
            "artworkorobject-title-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AOTitle",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Date created",
            "artworkorobject-datecreated-label",
            "artworkorobject-datecreated-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AODateCreated",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Creator",
            "artworkorobject-creator-label",
            "artworkorobject-creator-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AOCreator",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Source",
            "artworkorobject-source-label",
            "artworkorobject-source-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AOSource",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Source inventory number",
            "artworkorobject-sourceinvno-label",
            "artworkorobject-sourceinvno-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AOSourceInvNo",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Copyright notice",
            "artworkorobject-copyrightnotice-label",
            "artworkorobject-copyrightnotice-entry",
            "Xmp.iptcExt.ArtworkOrObject[x]/iptcExt:AOCopyrightNotice",
            WIDGET_TYPE_ENTRY,
            -1},


        {"Additional model information",
            "addlmodelinfo-label",
            "addlmodelinfo-entry",
            "Xmp.iptcExt.AddlModelInfo",
             WIDGET_TYPE_ENTRY,
             -1},

        {"Model age",
            "modelage-label",
            "modelage-entry",
            "Xmp.iptcExt.ModelAge",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Model release ID",
            "modelreleaseid-label",
            "modelreleaseid-entry",
            "Xmp.plus.ModelReleaseID",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Minor model age",
            "minormodelagedisclosure-label",
            "minormodelagedisclosure-combobox",
            "Xmp.plus.MinorModelAgeDisclosure",
            WIDGET_TYPE_COMBOBOX,
            0},

        {"Model release status",
            "modelreleasestatus-label",
            "modelreleasestatus-combobox",
            "Xmp.plus.ModelReleaseStatus",
            WIDGET_TYPE_COMBOBOX,
            1}
    };

static Datapage *datapage;

void
page_artwork_start (GtkBuilder     *builder,
                    GimpAttributes *attributes)
{
  datapage = datapage_new (builder);

  datapage_set_structure_element (datapage,
                                  struct_element,
                                  1);

  datapage_set_combobox_data (datapage,
                              (ComboBoxData *) combobox_data,
                              2,
                              14);

  datapage_set_metadata_entry (datapage,
                               artwork_entries,
                               11);

  datapage_read_from_attributes (datapage, &attributes);

}

void
page_artwork_get_attributes (GimpAttributes **attributes)
{
  datapage_save_to_attributes (datapage, attributes);
}
