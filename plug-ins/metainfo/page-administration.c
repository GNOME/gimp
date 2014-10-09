/* page-administration.c
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
#include "page-administration.h"
#include "datapage.h"


#define STRUCTURES_ON_PAGE 2
#define COMBOBOX_WITH_DATA 1


static StructureElement struct_element [] =
    {
        {0,
            N_("Image supplier"),
            "Xmp.plus.ImageSupplier",
            STRUCTURE_TYPE_SEQ,
            "imagesupplier-label",
            "imagesupplier-combo",
            "imagesupplier-liststore",
            "imagesupplier-button-plus",
            "imagesupplier-button-minus"},

        {1,
            N_("Registry ID"),
            "Xmp.iptcExt.RegistryId",
            STRUCTURE_TYPE_BAG,
            "registryid-label",
            "registryid-combo",
            "registryid-liststore",
            "registryid-button-plus",
            "registryid-button-minus"}
    };


static ComboBoxData combobox_data[][6] =
{
    {
        {N_(" "),                                                ""},
        {N_("Original digital capture of a real life scene"),    "http://cv.iptc.org/newscodes/digitalsourcetype/digitalCapture"},
        {N_("Digitised from a negative on film"),                "http://cv.iptc.org/newscodes/digitalsourcetype/negativeFilm"},
        {N_("Digitised from a positive on film"),                "http://cv.iptc.org/newscodes/digitalsourcetype/positiveFilm"},
        {N_("Digitised from a print on non-transparent medium"), "http://cv.iptc.org/newscodes/digitalsourcetype/print"},
        {N_("Created by software"),                              "http://cv.iptc.org/newscodes/digitalsourcetype/softwareImage"}
    },
};

static MetadataEntry administration_entries[] =
    {
        {"Image supplier name",
            "imagesupplier-name-label",
            "imagesupplier-name-entry",
            "Xmp.plus.ImageSupplier[x]/plus:ImageSupplierName",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Image supplier ID",
            "imagesupplier-id-label",
            "imagesupplier-id-entry",
            "Xmp.plus.ImageSupplier[x]/plus:ImageSupplierID",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Image supplier image ID",
            "imagesupplier-imageid-label",
            "imagesupplier-imageid-entry",
            "Xmp.plus.ImageSupplierImageID",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Registry organistaion ID",
            "regorgid-label",
            "regorgid-entry",
            "Xmp.iptcExt.RegistryId[x]/iptcExt:RegOrgId",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Registry item ID",
            "regitemid-label",
            "regitemid-entry",
            "Xmp.iptcExt.RegistryId[x]/iptcExt:RegItemId",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Max available height",
            "maxavailheight-label",
            "maxavailheight-entry",
            "Xmp.iptcExt.MaxAvailHeight",
            WIDGET_TYPE_ENTRY,
            -1},

        {"Max available width",
            "maxavailwidth-label",
            "maxavailwidth-entry",
            "Xmp.iptcExt.MaxAvailWidth",
             WIDGET_TYPE_ENTRY,
             -1},

        {"Digital source type",
            "digitalsourcetype-label",
            "digitalsourcetype-combobox",
            "Xmp.iptcExt.DigitalSourceType",
            WIDGET_TYPE_COMBOBOX,
            0}
    };

static Datapage *datapage;

void
page_administration_start (GtkBuilder     *builder,
                           GimpAttributes *attributes)
{
  datapage = datapage_new (builder);

  datapage_set_structure_element (datapage,
                                  struct_element,
                                  2);

  datapage_set_combobox_data (datapage,
                              combobox_data,
                              1,
                              6);

  datapage_set_metadata_entry (datapage,
                               administration_entries,
                               8);

  datapage_read_from_attributes (datapage, &attributes);

}

void
page_administration_get_attributes (GimpAttributes **attributes)
{
  datapage_save_to_attributes (datapage, attributes);
}
