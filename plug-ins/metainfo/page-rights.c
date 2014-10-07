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
#include "page-rights.h"
#include "datapage.h"


#define STRUCTURES_ON_PAGE 3
#define COMBOBOX_WITH_DATA 1

static StructureElement struct_element [] =
    {
        {0,
            N_("Image creator"),
            "Xmp.plus.ImageCreator",
            STRUCTURE_TYPE_SEQ,
            "imagecreator-combo",
            "imagecreator-liststore",
            "imagecreator-button-plus",
            "imagecreator-button-minus"},

        {1,
            N_("Copyright Owner"),
            "Xmp.plus.CopyrightOwner",
            STRUCTURE_TYPE_SEQ,
            "copyrightowner-combo",
            "copyrightowner-liststore",
            "copyrightowner-button-plus",
            "copyrightowner-button-minus"},

        {2,
            N_("Licensor"),
            "Xmp.plus.Licensor",
            STRUCTURE_TYPE_BAG,
            "licensor-combo",
            "licensor-liststore",
            "licensor-button-plus",
            "licensor-button-minus"}
    };


static ComboBoxData combobox_data[][5] =
{
    {
        {N_(" "),                                         ""},
        {N_("None"),                                      "http://ns.useplus.org/ldf/vocab/PR-NON"},
        {N_("Not applicable"),                            "http://ns.useplus.org/ldf/vocab/PR-NAP"},
        {N_("Unlimited property release"),                "http://ns.useplus.org/ldf/vocab/PR-UPR"},
        {N_("Limited or Incomplete Property Releases"),   "http://ns.useplus.org/ldf/vocab/PR-LPR"}
    },
};

static MetadataEntry rights_entries[] =
    {
        {N_("Image creator name"),
            "imagecreator-name-label",
            "imagecreator-name-entry",
            "Xmp.plus.ImageCreator[x]/plus:ImageCreatorName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Image creator ID"),
            "imagecreator-id-label",
            "imagecreator-id-entry",
            "Xmp.plus.ImageCreator[x]/plus:ImageCreatorID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Copyright owner name"),
            "copyrightowner-name-label",
            "copyrightowner-name-entry",
            "Xmp.plus.CopyrightOwner[x]/plus:CopyrightOwnerName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Copyright owner ID"),
            "copyrightowner-id-label",
            "copyrightowner-id-entry",
            "Xmp.plus.CopyrightOwner[x]/plus:CopyrightOwnerID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Name"),
            "licensor-name-label",
            "licensor-name-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("ID"),
            "licensor-id-label",
            "licensor-id-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Address"),
            "licensor-streetaddress-label",
            "licensor-streetaddress-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorStreetAddress",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Extended Address"),
            "licensor-extendedaddress-label",
            "licensor-extendedaddress-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorExtendedAddress",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("City"),
            "licensor-city-label",
            "licensor-city-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorCity",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Region"),
            "licensor-region-label",
            "licensor-region-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorRegion",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Postal code"),
            "licensor-postalcode-label",
            "licensor-postalcode-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorPostalCode",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Country"),
            "licensor-country-label",
            "licensor-country-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorCountry",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Telephone 1"),
            "licensor-telephone1-label",
            "licensor-telephone1-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorTelephone1",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Telephone 2"),
            "licensor-telephone2-label",
            "licensor-telephone2-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorTelephone2",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("E-Mail"),
            "licensor-email-label",
            "licensor-email-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorEmail",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("URL"),
            "licensor-url-label",
            "licensor-url-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorURL",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Release ID"),
            "release-id-label",
            "release-id-entry",
            "Xmp.plus.PropertyReleaseID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Release status"),
            "release-status-label",
            "release-status-combobox",
            "Xmp.plus.PropertyReleaseStatus",
            WIDGET_TYPE_COMBOBOX,
            0}
    };

static Datapage *datapage;

void
page_rights_start (GtkBuilder     *builder,
                   GimpAttributes *attributes)
{
  datapage = datapage_new (builder);

  datapage_set_structure_element (datapage,
                                  struct_element,
                                  3);

  datapage_set_combobox_data (datapage,
                              combobox_data,
                              1,
                              5);

  datapage_set_metadata_entry (datapage,
                               rights_entries,
                               18);

  datapage_read_from_attributes (datapage, &attributes);

}

void
page_rights_get_attributes (GimpAttributes **attributes)
{
  datapage_save_to_attributes (datapage, attributes);
}
