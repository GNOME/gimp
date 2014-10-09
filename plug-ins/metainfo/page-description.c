/* page-description.c
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

#include "page-description.h"
#include "datapage.h"

#define STRUCTURES_ON_PAGE 2

static StructureElement struct_element [] =
    {
        {0,
            N_("Location created"),
            "Xmp.iptcExt.LocationCreated",
            STRUCTURE_TYPE_BAG,
            "location-created-label",
            "location-created-combo",
            "location-created-liststore",
            "location-created-button-plus",
            "location-created-button-minus"},

        {1,
            N_("Location shown"),
            "Xmp.iptcExt.LocationShown",
            STRUCTURE_TYPE_SEQ,
            "location-shown-label",
            "location-shown-combo",
            "location-shown-liststore",
            "location-shown-button-plus",
            "location-shown-button-minus"}
    };

static MetadataEntry description_entries[] =
    {
        {N_("Person in image"),
            "personinimage-label",
            "personinimage-entry",
            "Xmp.iptcExt.PersonInImage",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Event"),
            "event-label",
            "event-entry",
            "Xmp.iptcExt.Event",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Sublocation"),
            "location-created-sublocation-label",
            "location-created-sublocation-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:Sublocation",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("City"),
            "location-created-city-label",
            "location-created-city-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:City",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Province/State"),
            "location-created-provincestate-label",
            "location-created-provincestate-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:ProvinceState",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Countryname"),
            "location-created-countryname-label",
            "location-created-countryname-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:CountryName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Countrycode"),
            "location-created-countrycode-label",
            "location-created-countrycode-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:CountryCode",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Worldregion"),
            "location-created-worldregion-label",
            "location-created-worldregion-entry",
            "Xmp.iptcExt.LocationCreated[x]/iptcExt:WorldRegion",
            WIDGET_TYPE_ENTRY,
            -1},


        {N_("Sublocation"),
            "location-shown-sublocation-label",
            "location-shown-sublocation-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:Sublocation",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("City"),
            "location-shown-city-label",
            "location-shown-city-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:City",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Province/State"),
            "location-shown-provincestate-label",
            "location-shown-provincestate-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:ProvinceState",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Countryname"),
            "location-shown-countryname-label",
            "location-shown-countryname-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:CountryName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Countrycode"),
            "location-shown-countrycode-label",
            "location-shown-countrycode-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:CountryCode",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Worldregion"),
            "location-shown-worldregion-label",
            "location-shown-worldregion-entry",
            "Xmp.iptcExt.LocationShown[x]/iptcExt:WorldRegion",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Name"),
            "orginimage-name-label",
            "orginimage-name-entry",
            "Xmp.iptcExt.OrganisationInImageName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Code"),
            "orginimage-code-label",
            "orginimage-code-entry",
            "Xmp.iptcExt.OrganisationInImageCode",
            WIDGET_TYPE_ENTRY,
            -1}
    };

static Datapage *datapage;

void
page_description_start (GtkBuilder     *builder,
                        GimpAttributes *attributes)
{
  datapage = datapage_new (builder);

  datapage_set_structure_element (datapage,
                                  struct_element,
                                  2);

  datapage_set_combobox_data (datapage,
                              NULL,
                              0,
                              0);

  datapage_set_metadata_entry (datapage,
                               description_entries,
                               16);

  datapage_read_from_attributes (datapage, &attributes);

}

void
page_description_get_attributes (GimpAttributes **attributes)
{
  datapage_save_to_attributes (datapage, attributes);
}
