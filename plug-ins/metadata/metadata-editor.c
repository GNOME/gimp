/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata-editor.c
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

#include "config.h"

#include <stdlib.h>
#include <ctype.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metadata-tags.h"
#include "metadata-xml.h"
#include "metadata-impexp.h"
#include "metadata-misc.h"
#include "metadata-editor.h"

#define PLUG_IN_PROC            "plug-in-metadata-editor"
#define PLUG_IN_BINARY          "metadata-editor"
#define PLUG_IN_ROLE            "gimp-metadata"

#define DEFAULT_TEMPLATE_FILE   "gimp_metadata_template.xml"


typedef struct _Metadata      Metadata;
typedef struct _MetadataClass MetadataClass;

struct _Metadata
{
  GimpPlugIn parent_instance;
};

struct _MetadataClass
{
  GimpPlugInClass parent_class;
};


#define METADATA_TYPE  (metadata_get_type ())
#define METADATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), METADATA_TYPE, Metadata))

GType                   metadata_get_type         (void) G_GNUC_CONST;

static GList          * metadata_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * metadata_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * metadata_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   gint                  n_drawables,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);


enum
{
  ME_WIDGET_ENTRY,
  ME_WIDGET_TEXT,
  ME_WIDGET_COMBO,
  ME_WIDGET_DATE_BOX,   /* Entry + date select button in a box */
  ME_WIDGET_EC_BOX,     /* Entry + combo in a box */
  ME_WIDGET_TREE_GRID,  /* Treeview + add/remove row buttons in a box */
  ME_WIDGET_SEPARATOR,
  ME_WIDGET_OTHER,
};

enum
{
  ME_RENDER_TEXT,
  ME_RENDER_COMBO,
  ME_RENDER_OTHER,
};

typedef struct
{
  guint     index;
  gchar    *label;
  gint      widget_type;
  gchar    *id;
  gchar    *extra_id1;   /* date_box: button id; tree view: add button id */
  gchar    *extra_id2;   /* date_box: icon name for button; tree view: remove button id */
} me_widget_info;

typedef struct
{
  gchar    *label;
  gint      renderer_type;
} me_column_info;

static GtkWidget * metadata_editor_create_page_grid (GtkWidget         *notebook,
                                                     const gchar       *tab_name);

static void     metadata_editor_create_widgets   (const me_widget_info *widget_info,
                                                  gint                  n_items,
                                                  GtkWidget            *grid,
                                                  metadata_editor      *meta_info);

static void     metadata_editor_create_tree_grid (const me_column_info *tree_info,
                                                  gint                  n_items,
                                                  gint                  grid_row,
                                                  const me_widget_info *widget_info,
                                                  GtkWidget            *grid,
                                                  GtkListStore         *store,
                                                  metadata_editor      *meta_info);

static gboolean metadata_editor_dialog           (GimpImage           *image,
                                                  GimpMetadata        *metadata,
                                                  GimpProcedureConfig *config,
                                                  GError             **error);

static void metadata_dialog_editor_set_metadata  (GExiv2Metadata      *metadata,
                                                  metadata_editor     *meta_info);

static void impex_combo_callback                (GtkComboBoxText      *combo,
                                                 gpointer              data);

static void gpsaltsys_combo_callback            (GtkComboBoxText      *combo,
                                                 gpointer              data);

static void remove_substring                    (const gchar          *string,
                                                 const gchar          *substring);

static gchar * clean_xmp_string                 (const gchar          *value);
static gchar ** split_metadata_string           (gchar                *value);
static void     add_to_store                    (gchar                *value,
                                                 GtkListStore         *liststore,
                                                 gint                  store_column);

static void     set_tag_string                  (GimpMetadata         *metadata,
                                                 const gchar          *name,
                                                 const gchar          *value,
                                                 gboolean              clear_tag);

static gchar *  get_phonetype                   (gchar                *cur_value);

static void     write_metadata_tag              (metadata_editor      *meta_info,
                                                 GimpMetadata         *metadata,
                                                 gchar                *tag,
                                                 gint                  data_column);

static void     write_metadata_tag_multiple     (metadata_editor      *meta_info,
                                                 GimpMetadata         *metadata,
                                                 GExiv2StructureType   type,
                                                 const gchar          *header_tag,
                                                 gint                  n_columns,
                                                 const gchar         **column_tags,
                                                 const gint            special_handling[]);

gboolean hasCreatorTagData                      (metadata_editor      *meta_info);
gboolean hasLocationCreationTagData             (metadata_editor      *meta_info);
gboolean hasImageSupplierTagData                (metadata_editor      *meta_info);

void on_date_button_clicked                     (GtkButton            *widget,
                                                 GtkWidget            *entry_widget,
                                                 gchar                *tag);

void on_create_date_button_clicked              (GtkButton            *widget,
                                                 gpointer              data);

void on_patient_dob_date_button_clicked         (GtkButton            *widget,
                                                 gpointer              data);

void on_study_date_button_clicked               (GtkButton            *widget,
                                                 gpointer              data);

void on_series_date_button_clicked              (GtkButton            *widget,
                                                 gpointer              data);


static void
property_release_id_remove_callback             (GtkWidget            *widget,
                                                 gpointer              data);
static void
property_release_id_add_callback                (GtkWidget            *widget,
                                                 gpointer              data);
static void
model_release_id_remove_callback                (GtkWidget            *widget,
                                                 gpointer              data);
static void
model_release_id_add_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
shown_location_remove_callback                  (GtkWidget            *widget,
                                                 gpointer              data);
static void
shown_location_add_callback                     (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_name_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_name_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_code_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_code_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
artwork_object_add_callback                     (GtkWidget            *widget,
                                                 gpointer              data);
static void
artwork_object_remove_callback                  (GtkWidget            *widget,
                                                 gpointer              data);
static void
reg_entry_add_callback                          (GtkWidget            *widget,
                                                 gpointer              data);
static void
reg_entry_remove_callback                       (GtkWidget            *widget,
                                                 gpointer              data);
static void
image_creator_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
image_creator_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);

static void
copyright_own_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
copyright_own_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
licensor_add_callback                           (GtkWidget            *widget,
                                                 gpointer              data);
static void
licensor_remove_callback                        (GtkWidget            *widget,
                                                 gpointer              data);

static void
list_row_remove_callback                        (GtkWidget            *widget,
                                                 gpointer              data,
                                                 gchar                *tag);

static void
list_row_add_callback                           (GtkWidget            *widget,
                                                 gpointer              data,
                                                 gchar                *tag);

static gint
count_tags                                      (GExiv2Metadata       *metadata,
                                                 const gchar          *header,
                                                 const gchar         **tags,
                                                 int                   items);

static gchar **
get_tags                                        (GExiv2Metadata       *metadata,
                                                 const gchar          *header,
                                                 const gchar         **tags,
                                                 const int             items,
                                                 const int             count);

static void
free_tagdata                                    (gchar               **tagdata,
                                                 gint                  rows,
                                                 gint                  cols);

gboolean
hasModelReleaseTagData                          (metadata_editor      *meta_info);

gboolean
hasPropertyReleaseTagData                       (metadata_editor      *meta_info);

static void
organisation_image_code_cell_edited_callback    (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
organisation_image_name_cell_edited_callback    (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
prop_rel_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_sub_loc_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_city_cell_edited_callback               (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_state_prov_cell_edited_callback         (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_cntry_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_cntry_iso_cell_edited_callback          (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
reg_org_id_cell_edited_callback                 (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
reg_item_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_title_cell_edited_callback                  (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_copyright_notice_cell_edited_callback       (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_source_inv_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_source_cell_edited_callback                 (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_creator_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_date_creat_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
cr_owner_name_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
cr_owner_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_name_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone1_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone_type1_cell_edited_callback       (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone2_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone_type2_cell_edited_callback       (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_email_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_web_cell_edited_callback               (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

void
cell_edited_callback                            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data,
                                                 int                   index);

void
cell_edited_callback_combo                      (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data,
                                                 int                   index);


G_DEFINE_TYPE (Metadata, metadata, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (METADATA_TYPE)
DEFINE_STD_SET_I18N


static int last_gpsaltsys_sel;

gboolean gimpmetadata;
gboolean force_write;

static const gchar *lang_default = "lang=\"x-default\"";
static const gchar *seq_default = "type=\"Seq\"";
static const gchar *bag_default = "type=\"Bag\"";

metadata_editor meta_args;

#define ME_LOG_DOMAIN "metadata-editor"


/* Widget creation data */

static const me_widget_info description_tab_data[] =
{
  { 0, N_("Document Title"),          ME_WIDGET_ENTRY,      "Xmp.dc.title" },
  { 1, N_("Author"),                  ME_WIDGET_TEXT,       "Xmp.dc.creator" },
  { 2, N_("Author Title"),            ME_WIDGET_ENTRY,      "Xmp.photoshop.AuthorsPosition" },
  { 3, N_("Description"),             ME_WIDGET_TEXT,       "Xmp.dc.description" },
  { 4, N_("Description Writer"),      ME_WIDGET_ENTRY,      "Xmp.photoshop.CaptionWriter" },
  { 5, N_("Rating"),                  ME_WIDGET_COMBO,      "Xmp.xmp.Rating" },
  { 6, N_("Keywords"),                ME_WIDGET_TEXT,       "Xmp.dc.subject" },
  { 7, "",                            ME_WIDGET_SEPARATOR,  "" },
  { 8, N_("Copyright Status"),        ME_WIDGET_COMBO,      "Xmp.xmpRights.Marked" },
  { 9, N_("Copyright Notice"),        ME_WIDGET_ENTRY,      "Xmp.dc.rights" },
  { 10, N_("Copyright URL"),          ME_WIDGET_ENTRY,      "Xmp.xmpRights.WebStatement" },
};
static const gint n_description_tab_data = G_N_ELEMENTS (description_tab_data);

static const me_widget_info iptc_tab_data[] =
{
  { 0, N_("Address"),                 ME_WIDGET_TEXT,       "Xmp.iptc.CiAdrExtadr" },
  { 1, N_("City"),                    ME_WIDGET_ENTRY,      "Xmp.iptc.CiAdrCity" },
  { 2, N_("State / Province"),        ME_WIDGET_ENTRY,      "Xmp.iptc.CiAdrRegion" },
  { 3, N_("Postal Code"),             ME_WIDGET_ENTRY,      "Xmp.iptc.CiAdrPcode" },
  { 4, N_("Country"),                 ME_WIDGET_ENTRY,      "Xmp.iptc.CiAdrCtry" },
  { 5, "",                            ME_WIDGET_SEPARATOR,  "" },
  { 6, N_("Phone(s)"),                ME_WIDGET_TEXT,       "Xmp.iptc.CiTelWork" },
  { 7, N_("E-mail(s)"),               ME_WIDGET_TEXT,       "Xmp.iptc.CiEmailWork" },
  { 8, N_("Website(s)"),              ME_WIDGET_TEXT,       "Xmp.iptc.CiUrlWork" },
  { 9, "",                            ME_WIDGET_SEPARATOR,  "" },
  { 10, N_("Creation Date"),          ME_WIDGET_DATE_BOX,   "Xmp.photoshop.DateCreated",
        "create_date_button",         "gimp-grid" },
  { 11, N_("Intellectual Genre"),     ME_WIDGET_ENTRY,      "Xmp.iptc.IntellectualGenre" },
  { 12, N_("IPTC Scene Code"),        ME_WIDGET_TEXT,       "Xmp.iptc.Scene" },
  { 13, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 14, N_("Sublocation"),            ME_WIDGET_ENTRY,      "Xmp.iptc.Location" },
  { 15, N_("City"),                   ME_WIDGET_ENTRY,      "Xmp.photoshop.City" },
  { 16, N_("State / Province"),       ME_WIDGET_ENTRY,      "Xmp.photoshop.State" },
  { 17, N_("Country"),                ME_WIDGET_ENTRY,      "Xmp.photoshop.Country" },
  { 18, N_("Country ISO-Code"),       ME_WIDGET_ENTRY,      "Xmp.iptc.CountryCode" },
  { 19, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 20, N_("Urgency"),                ME_WIDGET_COMBO,      "Xmp.photoshop.Urgency" },
  { 21, N_("Headline"),               ME_WIDGET_TEXT,       "Xmp.photoshop.Headline" },
  { 22, N_("IPTC Subject Code"),      ME_WIDGET_TEXT,       "Xmp.iptc.SubjectCode" },
  { 23, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 24, N_("Job Identifier"),         ME_WIDGET_ENTRY,      "Xmp.photoshop.TransmissionReference" },
  { 25, N_("Instructions"),           ME_WIDGET_TEXT,       "Xmp.photoshop.Instructions" },
  { 26, N_("Credit Line"),            ME_WIDGET_ENTRY,      "Xmp.photoshop.Credit" },
  { 27, N_("Source"),                 ME_WIDGET_ENTRY,      "Xmp.photoshop.Source" },
  { 28, N_("Usage Terms"),            ME_WIDGET_TEXT,       "Xmp.xmpRights.UsageTerms" },
};
static const gint n_iptc_tab_data = G_N_ELEMENTS (iptc_tab_data);

static const me_widget_info iptc_extension_tab_data[] =
{
  { 0, N_("Person Shown"),            ME_WIDGET_TEXT,       "Xmp.iptcExt.PersonInImage" },
  { 1, N_("Sublocation"),             ME_WIDGET_ENTRY,      "Xmp.iptcExt.Sublocation" },
  { 2, N_("City"),                    ME_WIDGET_ENTRY,      "Xmp.iptcExt.City" },
  { 3, N_("State / Province"),        ME_WIDGET_ENTRY,      "Xmp.iptcExt.ProvinceState" },
  { 4, N_("Country"),                 ME_WIDGET_ENTRY,      "Xmp.iptcExt.CountryName" },
  { 5, N_("Country ISO-Code"),        ME_WIDGET_ENTRY,      "Xmp.iptcExt.CountryCode" },
  { 6, N_("World Region"),            ME_WIDGET_ENTRY,      "Xmp.iptcExt.WorldRegion" },
  { 7, N_("Location Shown"),          ME_WIDGET_TREE_GRID,  "Xmp.iptcExt.LocationShown",
       "add_shown_location_button",   "rem_shown_location_button" },
  { 8, N_("Featured Organization"),   ME_WIDGET_TREE_GRID,  "Xmp.iptcExt.OrganisationInImageName",
       "add_feat_org_name_button",    "rem_feat_org_name_button" },
  { 9, N_("Organization Code"),       ME_WIDGET_TREE_GRID,  "Xmp.iptcExt.OrganisationInImageCode",
       "add_feat_org_code_button",    "rem_feat_org_code_button" },
  { 10, N_("Event"),                  ME_WIDGET_ENTRY,      "Xmp.iptcExt.Event" },
  { 11, N_("Artwork or Object"),      ME_WIDGET_TREE_GRID,  "Xmp.iptcExt.ArtworkOrObject",
        "add_artwork_object_button",  "rem_artwork_object_button" },
  { 12, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 13, N_("Additional Model Info"),  ME_WIDGET_TEXT,       "Xmp.iptcExt.AddlModelInfo" },
  { 14, N_("Model Age"),              ME_WIDGET_TEXT,       "Xmp.iptcExt.ModelAge" },
  { 15, N_("Minor Model Age Disclosure"), ME_WIDGET_COMBO,     "Xmp.plus.MinorModelAgeDisclosure" },
  { 16, N_("Model Release Status"),       ME_WIDGET_COMBO,     "Xmp.plus.ModelReleaseStatus" },
  { 17, N_("Model Release Identifier"),   ME_WIDGET_TREE_GRID, "Xmp.plus.ModelReleaseID",
        "add_model_rel_id_button",    "rem_model_rel_id_button" },
  { 18, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 19, N_("Image Supplier Name"),    ME_WIDGET_ENTRY,      "Xmp.plus.ImageSupplierName" },
  { 20, N_("Image Supplier ID"),      ME_WIDGET_ENTRY,      "Xmp.plus.ImageSupplierID" },
  { 21, N_("Supplier's Image ID"),    ME_WIDGET_ENTRY,      "Xmp.plus.ImageSupplierImageID" },
  { 22, N_("Registry Entry"),         ME_WIDGET_TREE_GRID,  "Xmp.iptcExt.RegistryId",
        "add_reg_entry_button",       "rem_reg_entry_button" },
  { 23, N_("Max. Available Width"),   ME_WIDGET_ENTRY,      "Xmp.iptcExt.MaxAvailWidth" },
  { 24, N_("Max. Available Height"),  ME_WIDGET_ENTRY,      "Xmp.iptcExt.MaxAvailHeight" },
  { 25, N_("Digital Source Type"),    ME_WIDGET_COMBO,      "Xmp.iptcExt.DigitalSourceType" },
  { 26, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 27, N_("Image Creator"),          ME_WIDGET_TREE_GRID,  "Xmp.plus.ImageCreator",
        "add_image_creator_button",   "rem_image_creator_button" },
  { 28, N_("Copyright Owner"),        ME_WIDGET_TREE_GRID,  "Xmp.plus.CopyrightOwner",
        "add_copyright_own_button",   "rem_copyright_own_button" },
  { 29, N_("Licensor"),               ME_WIDGET_TREE_GRID,  "Xmp.plus.Licensor",
        "add_licensor_button",        "rem_licensor_button" },
  { 30, N_("Property Release Status"), ME_WIDGET_COMBO,     "Xmp.plus.PropertyReleaseStatus" },
  { 31, N_("Property Release Identifier"), ME_WIDGET_TREE_GRID, "Xmp.plus.PropertyReleaseID",
        "add_prop_rel_id_button",     "rem_prop_rel_id_button" },
};
static const gint n_iptc_extension_tab_data = G_N_ELEMENTS (iptc_extension_tab_data);

/* ME_WIDGET_TREE_GRID indexes */
#define C_LOCATION_SHOWN       7
#define C_FEATURED_ORG         8
#define C_FEATURED_ORG_CODE    9
#define C_ART_OBJECT          11
#define C_MODEL_RELEASE_ID    17
#define C_REGISTRY_ENTRY      22
#define C_IMAGE_CREATOR       27
#define C_COPYRIGHT_OWNER     28
#define C_LICENSOR            29
#define C_PROPERTY_RELEASE_ID 31

static const me_column_info location_shown_info[] =
{
  { N_("Sublocation"),                ME_RENDER_TEXT },
  { N_("City"),                       ME_RENDER_TEXT },
  { N_("Province / State"),           ME_RENDER_TEXT },
  { N_("Country"),                    ME_RENDER_TEXT },
  { N_("Country ISO Code"),           ME_RENDER_TEXT },
  { N_("World Region"),               ME_RENDER_TEXT },
};
static const gint n_location_shown_info = G_N_ELEMENTS (location_shown_info);

static const me_column_info featured_organization_info[] =
{
  { N_("Name"),                       ME_RENDER_TEXT },
};
static const gint n_featured_organization_info = G_N_ELEMENTS (featured_organization_info);

static const me_column_info featured_organization_code_info[] =
{
  { N_("Code"),                       ME_RENDER_TEXT },
};
static const gint n_featured_organization_code_info = G_N_ELEMENTS (featured_organization_code_info);

static const me_column_info artwork_object_info[] =
{
  { N_("Title"),                      ME_RENDER_TEXT },
  { N_("Date Created"),               ME_RENDER_TEXT },
  { N_("Creator"),                    ME_RENDER_TEXT },
  { N_("Source"),                     ME_RENDER_TEXT },
  { N_("Source Inventory ID"),        ME_RENDER_TEXT },
  { N_("Copyright Notice"),           ME_RENDER_TEXT },
};
static const gint n_artwork_object_info = G_N_ELEMENTS (artwork_object_info);

static const me_column_info model_release_id_info[] =
{
  { N_("Model Release Identifier"),   ME_RENDER_TEXT },
};
static const gint n_model_release_id_info = G_N_ELEMENTS (model_release_id_info);

static const me_column_info registry_entry_info[] =
{
  { N_("Organization Identifier"),    ME_RENDER_TEXT },
  { N_("Item Identifier"),            ME_RENDER_TEXT },
};
static const gint n_registry_entry_info = G_N_ELEMENTS (registry_entry_info);

static const me_column_info image_creator_info[] =
{
  { N_("Name"),                       ME_RENDER_TEXT },
  { N_("Identifier"),                 ME_RENDER_TEXT },
};
static const gint n_image_creator_info = G_N_ELEMENTS (image_creator_info);

static const me_column_info copyright_owner_info[] =
{
  { N_("Name"),                       ME_RENDER_TEXT },
  { N_("Identifier"),                 ME_RENDER_TEXT },
};
static const gint n_copyright_owner_info = G_N_ELEMENTS (copyright_owner_info);

static const me_column_info licensor_info[] =
{
  { N_("Name"),                       ME_RENDER_TEXT },
  { N_("Identifier"),                 ME_RENDER_TEXT },
  { N_("Phone Number 1"),             ME_RENDER_TEXT },
  { N_("Phone Type 1"),               ME_RENDER_COMBO },
  { N_("Phone Number 2"),             ME_RENDER_TEXT },
  { N_("Phone Type 2"),               ME_RENDER_COMBO },
  { N_("Email Address"),              ME_RENDER_TEXT },
  { N_("Web Address"),                ME_RENDER_TEXT },
};
static const gint n_licensor_info = G_N_ELEMENTS (licensor_info);

static const me_column_info property_release_id_info[] =
{
  { N_("Identifier"),                 ME_RENDER_TEXT },
};
static const gint n_property_release_id_info = G_N_ELEMENTS (property_release_id_info);


static const me_widget_info categories_labels[] =
{
  { 0, N_("Category"),                ME_WIDGET_ENTRY,      "Xmp.photoshop.Category" },
  { 1, N_("Supplemental Category"),   ME_WIDGET_TEXT,       "Xmp.photoshop.SupplementalCategories" },
};
static const gint n_categories_labels = G_N_ELEMENTS (categories_labels);

static const me_widget_info gps_labels[] =
{
  { 0, N_("Longitude"),               ME_WIDGET_ENTRY,      "Exif.GPSInfo.GPSLongitude" },
  { 1, N_("Longitude Reference"),     ME_WIDGET_COMBO,      "Exif.GPSInfo.GPSLongitudeRef" },
  { 2, N_("Latitude"),                ME_WIDGET_ENTRY,      "Exif.GPSInfo.GPSLatitude" },
  { 3, N_("Latitude Reference"),      ME_WIDGET_COMBO,      "Exif.GPSInfo.GPSLatitudeRef" },
  { 4, N_("Altitude"),                ME_WIDGET_EC_BOX,     "Exif.GPSInfo.GPSAltitude",
       "GPSAltitudeSystem" },
  { 5, N_("Altitude Reference"),      ME_WIDGET_COMBO,      "Exif.GPSInfo.GPSAltitudeRef" },
};
static const gint n_gps_labels = G_N_ELEMENTS (gps_labels);

static const me_widget_info dicom_info_labels[] =
{
  { 0, N_("Patient"),                 ME_WIDGET_ENTRY,      "Xmp.DICOM.PatientName" },
  { 1, N_("Patient ID"),              ME_WIDGET_ENTRY,      "Xmp.DICOM.PatientID" },
  { 2, N_("Date of Birth"),           ME_WIDGET_DATE_BOX,   "Xmp.DICOM.PatientDOB",
      "dob_date_button",              "gimp-grid" },
  { 3, N_("Patient Sex"),             ME_WIDGET_COMBO,      "Xmp.DICOM.PatientSex" },
  { 4, "",                            ME_WIDGET_SEPARATOR,  "" },
  { 5, N_("Study ID"),                ME_WIDGET_ENTRY,      "Xmp.DICOM.StudyID" },
  { 6, N_("Referring Physician"),     ME_WIDGET_ENTRY,      "Xmp.DICOM.StudyPhysician" },
  { 7, N_("Study Date"),              ME_WIDGET_DATE_BOX,   "Xmp.DICOM.StudyDateTime",
      "study_date_button",            "gimp-grid" },
  { 8, N_("Study Description"),       ME_WIDGET_TEXT ,      "Xmp.DICOM.StudyDescription" },
  { 9, N_("Series Number"),           ME_WIDGET_ENTRY,      "Xmp.DICOM.SeriesNumber" },
  { 10, N_("Modality"),               ME_WIDGET_ENTRY,      "Xmp.DICOM.SeriesModality" },
  { 11, N_("Series Date"),            ME_WIDGET_DATE_BOX,   "Xmp.DICOM.SeriesDateTime",
      "series_date_button",           "gimp-grid" },
  { 12, N_("Series Description"),     ME_WIDGET_TEXT ,      "Xmp.DICOM.SeriesDescription" },
  { 13, "",                           ME_WIDGET_SEPARATOR,  "" },
  { 14, N_("Equipment Institution"),  ME_WIDGET_ENTRY,      "Xmp.DICOM.EquipmentInstitution" },
  { 15, N_("Equipment Manufacturer"), ME_WIDGET_ENTRY,      "Xmp.DICOM.EquipmentManufacturer" },
};
static const gint n_dicom_info_labels = G_N_ELEMENTS (dicom_info_labels);


static void
metadata_class_init (MetadataClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = metadata_query_procedures;
  plug_in_class->create_procedure = metadata_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
metadata_init (Metadata *metadata)
{
}

static GList *
metadata_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
metadata_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            metadata_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("_Edit Metadata"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/Metadata");

      gimp_procedure_set_documentation (procedure,
                                        _("Edit metadata (IPTC, EXIF, XMP)"),
                                        "Edit metadata information attached "
                                        "to the current image. Some or all "
                                        "of this metadata will be saved in "
                                        "the file, depending on the output "
                                        "file format.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Ben Touchette",
                                      "Ben Touchette",
                                      "2017");

      gimp_procedure_add_bytes_argument (procedure, "parent-handle",
                                         _("Parent's window handle"),
                                         _("The opaque handle of the window to set this plug-in's dialog transient to"),
                                         G_PARAM_READWRITE | GIMP_PARAM_DONT_SERIALIZE);
    }

  return procedure;
}

static GimpValueArray *
metadata_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              gint                  n_drawables,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpMetadata *metadata;
  GError       *error  = NULL;

  gimp_ui_init (PLUG_IN_BINARY);

  metadata = gimp_image_get_metadata (image);

  /* Always show metadata dialog so we can add appropriate iptc data
   * as needed. Sometimes license data needs to be added after the
   * fact and the image may not contain metadata but should have it
   * added as needed.
   */
  if (! metadata)
    {
      metadata = gimp_metadata_new ();
      gimp_image_set_metadata (image, metadata);
    }

  if (metadata_editor_dialog (image, metadata, config, &error))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
  else
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, error);
}


/* ============================================================================
 * ==[ EDITOR DIALOG UI ]======================================================
 * ============================================================================
 */

GtkWidget *
metadata_editor_get_widget (metadata_editor *meta_info,
                            const gchar     *name)
{
  return GTK_WIDGET (g_hash_table_lookup (meta_info->widgets, name));
}

static GtkWidget *
metadata_editor_create_page_grid (GtkWidget   *notebook,
                                  const gchar *tab_name)
{
  GtkWidget *scrolled_win;
  GtkWidget *viewport;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *grid;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 6);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  label = gtk_label_new (tab_name);
  gtk_widget_set_margin_start (label, 2);
  gtk_widget_set_margin_top (label, 2);
  gtk_widget_set_margin_end (label, 2);
  gtk_widget_set_margin_bottom (label, 2);
  gtk_widget_set_can_focus (label, FALSE);
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), viewport);
  gtk_widget_show (viewport);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (viewport), box);
  gtk_widget_show (box);

  grid = gtk_grid_new ();
  gtk_widget_set_margin_bottom (grid, 5);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 3);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 30);
  gtk_box_pack_start (GTK_BOX (box), grid, FALSE, TRUE, 0);
  gtk_widget_show (grid);

  return grid;
}

static void
metadata_editor_create_widgets (const me_widget_info *widget_info,
                                gint                  n_items,
                                GtkWidget            *grid,
                                metadata_editor      *meta_info)
{
  GtkWidget *label;

  /* Labels on the left, data entry widgets on the right */
  for (int i = 0; i < n_items; i++)
    {
      if (widget_info[i].widget_type != ME_WIDGET_SEPARATOR)
        {
          label = gtk_label_new (_(widget_info[i].label));
          gtk_widget_set_margin_start (label, 3);
          gtk_widget_set_margin_top (label, 3);
          gtk_widget_set_margin_end (label, 3);
          gtk_widget_set_margin_bottom (label, 3);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          if (widget_info[i].widget_type == ME_WIDGET_TEXT ||
              widget_info[i].widget_type == ME_WIDGET_TREE_GRID)
            gtk_label_set_yalign (GTK_LABEL (label), 0.0);
          gtk_grid_attach (GTK_GRID (grid), label,
                           0, widget_info[i].index,
                           1, 1);
          gtk_widget_show (label);
        }

      switch (widget_info[i].widget_type)
        {
        case ME_WIDGET_ENTRY:
          {
            GtkWidget *entry;

            entry = gtk_entry_new ();
            gtk_widget_set_hexpand(GTK_WIDGET (entry), TRUE);
            gtk_widget_set_margin_end (entry, 5);
            gtk_grid_attach (GTK_GRID (grid), entry,
                             1, widget_info[i].index,
                             1, 1);
            gtk_widget_show (entry);
            g_hash_table_insert (meta_info->widgets, widget_info[i].id,
                                 (gpointer) entry);
          }
          break;

        case ME_WIDGET_TEXT:
          {
            GtkWidget *textview;
            GtkWidget *scrolled_window;

            scrolled_window = gtk_scrolled_window_new (NULL, NULL);
            gtk_widget_set_hexpand(GTK_WIDGET (scrolled_window), TRUE);
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                                 GTK_SHADOW_IN);
            gtk_widget_set_margin_end (scrolled_window, 5);
            gtk_widget_show (scrolled_window);

            textview = gtk_text_view_new();
            gtk_container_add (GTK_CONTAINER (scrolled_window), textview);
            gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD);
            gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), 6);
            gtk_text_view_set_right_margin (GTK_TEXT_VIEW (textview), 6);
            gtk_widget_show (textview);
            gtk_grid_attach (GTK_GRID (grid), scrolled_window,
                             1, widget_info[i].index,
                             1, 1);
            g_hash_table_insert (meta_info->widgets, widget_info[i].id,
                                 (gpointer) textview);
          }
          break;

        case ME_WIDGET_COMBO:
          {
            GtkWidget *combo;

            combo = gtk_combo_box_text_new ();
            gtk_widget_set_hexpand(GTK_WIDGET (combo), TRUE);
            gtk_widget_set_margin_end (combo, 5);
            gtk_widget_set_can_focus (combo, FALSE);
            gtk_grid_attach (GTK_GRID (grid), combo,
                             1, widget_info[i].index,
                             1, 1);
            gtk_widget_show (combo);
            g_hash_table_insert (meta_info->widgets, widget_info[i].id,
                                 (gpointer) combo);
          }
          break;

        case ME_WIDGET_DATE_BOX:
          {
            GtkWidget *date_box, *date_entry, *button;

            /* A date_entry and a button in one grid cell using a box as parent */

            date_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_widget_set_hexpand(GTK_WIDGET (date_box), TRUE);
            gtk_grid_attach (GTK_GRID (grid), date_box,
                             1, widget_info[i].index,
                             1, 1);
            gtk_widget_show (date_box);

            date_entry = gtk_entry_new ();
            gtk_widget_set_hexpand(GTK_WIDGET (date_entry), TRUE);
            gtk_box_pack_start (GTK_BOX (date_box), date_entry, TRUE, TRUE, 0);
            gtk_widget_show (date_entry);
            g_hash_table_insert (meta_info->widgets, widget_info[i].id,
                                 (gpointer) date_entry);

            button = gtk_button_new_from_icon_name (widget_info[i].extra_id2,
                                                    GTK_ICON_SIZE_BUTTON);
            gtk_widget_set_size_request (button, 24, 24);
            gtk_widget_set_margin_start (button, 5);
            gtk_widget_set_margin_end (button, 5);
            gtk_widget_set_margin_bottom (button, 1);
            gtk_container_add (GTK_CONTAINER (date_box), button);
            gtk_widget_show (button);
            g_hash_table_insert (meta_args.widgets, widget_info[i].extra_id1,
                                 (gpointer) button);
          }
          break;

        case ME_WIDGET_EC_BOX:
          {
            GtkWidget *ec_box, *ec_entry, *ec_combo;

            /* A box containing an Entry widget and a Combo widget,
             * where the combo widget is limited in size (100).
             * If needed, you can change size manually after creation. */

            ec_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_widget_set_hexpand(GTK_WIDGET (ec_box), TRUE);
            gtk_widget_set_margin_end (ec_box, 5);
            gtk_grid_attach (GTK_GRID (grid), ec_box,
                             1, widget_info[i].index,
                             1, 1);
            gtk_widget_show (ec_box);

            ec_entry = gtk_entry_new ();
            gtk_widget_set_hexpand(GTK_WIDGET (ec_entry), TRUE);
            gtk_box_pack_start (GTK_BOX (ec_box), ec_entry, TRUE, TRUE, 0);
            gtk_widget_show (ec_entry);
            g_hash_table_insert (meta_info->widgets, widget_info[i].id,
                                 (gpointer) ec_entry);

            ec_combo = gtk_combo_box_text_new ();
            gtk_widget_set_margin_start (ec_combo, 5);
            gtk_widget_set_can_focus (ec_combo, FALSE);
            g_object_set (G_OBJECT (ec_combo), "width_request", 100, NULL);
            gtk_box_pack_start (GTK_BOX (ec_box), ec_combo, FALSE, FALSE, 0);
            gtk_widget_show (ec_combo);
            g_hash_table_insert (meta_info->widgets, widget_info[i].extra_id1,
                                 (gpointer) ec_combo);
          }
          break;

        case ME_WIDGET_TREE_GRID:
          /* Needs to be handled separately. */
          break;

        case ME_WIDGET_SEPARATOR:
          {
            GtkWidget *separator;

            separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
            g_object_set (G_OBJECT (separator), "margin", 8, NULL);
            gtk_widget_show (separator);
            gtk_grid_attach (GTK_GRID (grid), separator,
                             0, widget_info[i].index,
                             2, 1);
          }
          break;

        default:
          g_assert_not_reached ();

          break;
        }
    }
}

static void
metadata_editor_create_tree_grid (const me_column_info *tree_info,
                                  gint                  n_items,
                                  gint                  grid_row,
                                  const me_widget_info *widget_info,
                                  GtkWidget            *grid,
                                  GtkListStore         *store,
                                  metadata_editor      *meta_info)
{
  /* Creates a GtkBox containing a GtkTreeView and a GtkBox which holds the
     add/remove buttons. */

  GtkWidget *tree_box, *tree;
  GtkWidget *button_box, *button;

  tree_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(GTK_WIDGET (tree_box), TRUE);
  gtk_widget_set_margin_end (tree_box, 5);
  gtk_widget_show (tree_box);
  gtk_grid_attach (GTK_GRID (grid), tree_box,
                   1, grid_row,
                   1, 1);

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (tree), GTK_TREE_VIEW_GRID_LINES_BOTH);
  gtk_box_pack_start (GTK_BOX (tree_box), tree, TRUE, TRUE, 0);
  gtk_widget_show (tree);

  g_hash_table_insert (meta_info->widgets, widget_info[grid_row].id, (gpointer) tree);

  for (int i = 0; i < n_items; i++)
    {
      GtkTreeViewColumn *column;
      GtkCellRenderer   *render;

      switch (tree_info[i].renderer_type)
        {
        case ME_RENDER_TEXT:
          render = gtk_cell_renderer_text_new ();
          column = gtk_tree_view_column_new_with_attributes (_(tree_info[i].label),
                                                             render,
                                                             "text", i,
                                                             NULL);

          break;

        case ME_RENDER_COMBO:
          render = gtk_cell_renderer_combo_new ();
          column = gtk_tree_view_column_new_with_attributes (_(tree_info[i].label),
                                                             render,
                                                             "text", i,
                                                             NULL);

          break;

        default:
          g_assert_not_reached ();

          break;
        }

      gtk_tree_view_column_set_resizable (column, TRUE);
      gtk_tree_view_column_set_spacing (column, 3);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    }

  button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_margin_top (button_box, 5);
  gtk_widget_set_margin_bottom (button_box, 5);
  gtk_box_pack_start (GTK_BOX (tree_box), button_box, FALSE, FALSE, 0);
  gtk_widget_show (button_box);

  button = gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_size_request (button, 24, 24);
  gtk_widget_set_margin_end (button, 5);
  gtk_container_add (GTK_CONTAINER (button_box), button);
  gtk_widget_show (button);
  g_hash_table_insert (meta_args.widgets, widget_info[grid_row].extra_id1, (gpointer) button);

  button = gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_size_request (button, 24, 24);
  gtk_widget_set_margin_start (button, 5);
  gtk_container_add (GTK_CONTAINER (button_box), button);
  gtk_widget_show (button);
  g_hash_table_insert (meta_args.widgets, widget_info[grid_row].extra_id2, (gpointer) button);
}

static gboolean
metadata_editor_dialog (GimpImage            *image,
                        GimpMetadata         *g_metadata,
                        GimpProcedureConfig  *config,
                        GError              **error)
{
  GExiv2Metadata *metadata;
  GtkWidget      *dialog;
  GtkWidget      *content_area;
  GtkWidget      *metadata_vbox;
  GtkWidget      *impex_combo;
  GtkWidget      *notebook;
  GtkWidget      *box;
  GtkWidget      *grid;
  GtkWidget      *widget;
  GtkListStore   *store;
  GBytes         *parent_handle = NULL;
  gchar          *title;
  gchar          *name;

  g_object_get (config, "parent-handle", &parent_handle, NULL);

  metadata = GEXIV2_METADATA (g_metadata);

  meta_args.image    = image;
  /* Default filename used for import/export */
  meta_args.filename = g_strconcat (g_get_home_dir (), "/", DEFAULT_TEMPLATE_FILE,
                                    NULL);
  meta_args.widgets  = g_hash_table_new (g_str_hash, g_str_equal);
  meta_args.metadata = metadata;

  name = gimp_image_get_name (image);
  title = g_strdup_printf (_("Metadata Editor: %s"), name);
  g_free (name);

  dialog = gimp_dialog_new (title,
                            "gimp-metadata-editor-dialog",
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
                            _("_Cancel"),         GTK_RESPONSE_CANCEL,
                            _("_Write Metadata"), GTK_RESPONSE_OK,
                            NULL);
  g_free (title);

  gtk_widget_set_size_request(dialog, 650, 500);

  meta_args.dialog = dialog;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  if (parent_handle && g_bytes_get_size (parent_handle) != 0)
    gimp_window_set_transient_for (GTK_WINDOW (dialog), parent_handle);
  else
    gimp_window_set_transient (GTK_WINDOW (dialog));

  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  g_bytes_unref (parent_handle);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  metadata_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (content_area), metadata_vbox, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (metadata_vbox), 12);
  gtk_box_set_spacing (GTK_BOX (metadata_vbox), 6);
  gtk_widget_show (metadata_vbox);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (metadata_vbox), notebook, TRUE, TRUE, 0);

  /* Description tab */

  grid = metadata_editor_create_page_grid (notebook, _("Description"));

  metadata_editor_create_widgets (description_tab_data, n_description_tab_data, grid, &meta_args);

  /* IPTC tab */

  grid = metadata_editor_create_page_grid (notebook, _("IPTC"));

  metadata_editor_create_widgets (iptc_tab_data, n_iptc_tab_data, grid, &meta_args);

  /* IPTC Extension tab */

  grid = metadata_editor_create_page_grid (notebook, _("IPTC Extension"));

  metadata_editor_create_widgets (iptc_extension_tab_data, n_iptc_extension_tab_data, grid, &meta_args);

  store = gtk_list_store_new (n_location_shown_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_LOCATION_SHOWN].id, ==, "Xmp.iptcExt.LocationShown");
  metadata_editor_create_tree_grid (location_shown_info, n_location_shown_info,
                                    C_LOCATION_SHOWN, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_featured_organization_info,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_FEATURED_ORG].id, ==, "Xmp.iptcExt.OrganisationInImageName");
  metadata_editor_create_tree_grid (featured_organization_info, n_featured_organization_info,
                                    C_FEATURED_ORG, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_featured_organization_code_info,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_FEATURED_ORG_CODE].id, ==, "Xmp.iptcExt.OrganisationInImageCode");
  metadata_editor_create_tree_grid (featured_organization_code_info, n_featured_organization_code_info,
                                    C_FEATURED_ORG_CODE, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_artwork_object_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_ART_OBJECT].id, ==, "Xmp.iptcExt.ArtworkOrObject");
  metadata_editor_create_tree_grid (artwork_object_info, n_artwork_object_info,
                                    C_ART_OBJECT, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_model_release_id_info,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_MODEL_RELEASE_ID].id, ==, "Xmp.plus.ModelReleaseID");
  metadata_editor_create_tree_grid (model_release_id_info, n_model_release_id_info,
                                    C_MODEL_RELEASE_ID, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_registry_entry_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_REGISTRY_ENTRY].id, ==, "Xmp.iptcExt.RegistryId");
  metadata_editor_create_tree_grid (registry_entry_info, n_registry_entry_info,
                                    C_REGISTRY_ENTRY, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_image_creator_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_IMAGE_CREATOR].id, ==, "Xmp.plus.ImageCreator");
  metadata_editor_create_tree_grid (image_creator_info, n_image_creator_info,
                                    C_IMAGE_CREATOR, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_copyright_owner_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_COPYRIGHT_OWNER].id, ==, "Xmp.plus.CopyrightOwner");
  metadata_editor_create_tree_grid (copyright_owner_info, n_copyright_owner_info,
                                    C_COPYRIGHT_OWNER, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_licensor_info,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_LICENSOR].id, ==, "Xmp.plus.Licensor");
  metadata_editor_create_tree_grid (licensor_info, n_licensor_info,
                                    C_LICENSOR, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  store = gtk_list_store_new (n_property_release_id_info,
                              G_TYPE_STRING);

  g_assert_cmpstr (iptc_extension_tab_data[C_PROPERTY_RELEASE_ID].id, ==, "Xmp.plus.PropertyReleaseID");
  metadata_editor_create_tree_grid (property_release_id_info, n_property_release_id_info,
                                    C_PROPERTY_RELEASE_ID, iptc_extension_tab_data,
                                    grid, store, &meta_args);

  /* Categories tab */

  grid = metadata_editor_create_page_grid (notebook, _("Categories"));

  metadata_editor_create_widgets (categories_labels, n_categories_labels, grid, &meta_args);

  /* GPS tab */

  grid = metadata_editor_create_page_grid (notebook, _("GPS"));

  metadata_editor_create_widgets (gps_labels, n_gps_labels, grid, &meta_args);

  /* Update GPSAltitudeSystem combo width */

  widget = metadata_editor_get_widget (&meta_args, "GPSAltitudeSystem");
  g_object_set (G_OBJECT (widget), "width_request", 60, NULL);

  /* DICOM tab */

  grid = metadata_editor_create_page_grid (notebook, _("DICOM"));

  metadata_editor_create_widgets (dicom_info_labels, n_dicom_info_labels, grid, &meta_args);

  /* Show notebook */

  gtk_widget_show (notebook);

  /* Import / Export options box */

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (metadata_vbox), box, FALSE, TRUE, 0);
  gtk_widget_show (box);

  impex_combo = gtk_combo_box_text_new ();
  g_object_set (G_OBJECT (impex_combo), "width_request", 160, NULL);
  gtk_box_pack_start (GTK_BOX (box), impex_combo, FALSE, FALSE, 6);
  gtk_widget_show (impex_combo);

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Select:"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Import metadata"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Export metadata"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (impex_combo), 0);

  g_signal_connect (G_OBJECT (impex_combo),
                    "changed", G_CALLBACK (impex_combo_callback), &meta_args);

  /* Add signals, combobox choices, and actual metadata */

  metadata_dialog_editor_set_metadata (metadata, &meta_args);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      metadata_editor_write_callback (dialog, &meta_args, image);
    }

  g_free (meta_args.filename);

  return TRUE;
}

/* ============================================================================
 * ==[                   ]=====================================================
 * ==[ PRIVATE FUNCTIONS ]=====================================================
 * ==[                   ]=====================================================
 * ============================================================================
 */
static void
remove_substring (const gchar *string,
                  const gchar *substring)
{
  if (string != NULL && substring != NULL && substring[0] != '\0')
    {
      gchar *p = strstr (string, substring);
      if (p)
        {
          memmove (p, p + strlen (substring), strlen (p + strlen (substring)) + 1);
        }
    }
}

static gchar *
clean_xmp_string (const gchar *value)
{
  gchar *value_clean;
  gchar *value_utf8;

  value_clean = g_strdup (value);

  if (strstr (value_clean, lang_default) != NULL)
    {
      remove_substring (value_clean, lang_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (strstr (value_clean, bag_default) != NULL)
    {
      remove_substring (value_clean, bag_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (strstr (value_clean, seq_default) != NULL)
    {
      remove_substring (value_clean, seq_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (! g_utf8_validate (value_clean, -1, NULL))
    {
      value_utf8 = g_locale_to_utf8 (value_clean, -1, NULL, NULL, NULL);
    }
  else
    {
      value_utf8 = g_strdup (value_clean);
    }

  g_free (value_clean);

  return value_utf8;
}

/* We split a string and accept "," and ";" as delimiters.
 * The result needs to be freed with g_strfreev.
 */
static gchar **
split_metadata_string (gchar *value)
{
  gchar  **split;
  gint     item;

  /* Can't use g_strsplit_set since we work with utf-8 here. */
  split = g_strsplit (g_strdelimit (value, ";", ','), ",", 0);

  for (item = 0; split[item]; item++)
    {
      split[item] = g_strstrip(split[item]);
    }

  return split;
}

static void
add_to_store (gchar *value, GtkListStore *liststore, gint store_column)
{
  gchar       **strings;
  gint          cnt = 0;
  gint          item;
  GtkTreeIter   iter;

  if (value)
    {
      strings = split_metadata_string (value);
      if (strings)
        {
          for (item = 0; strings[item]; item++)
            {
              if (strings[item][0] != '\0')
                {
                  cnt++;

                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      store_column, strings[item],
                                      -1);
                }
            }
          g_strfreev(strings);
        }
    }

  /* If there are less than two rows, add empty ones. */
  for (item = cnt; item < 2; item++)
    {
      gtk_list_store_append (liststore, &iter);
      gtk_list_store_set (liststore, &iter,
                          store_column, NULL,
                          -1);
    }
}

static gint
count_tags (GExiv2Metadata  *metadata,
            const gchar     *header,
            const gchar    **tags,
            gint             items)
{
  int   tagcount;
  gchar tag[256];
  int   row, col;

  tagcount = 0;
  for (row = 1; row < 256; row++)
    {
      for (col = 0; col < items; col++)
        {
          g_snprintf ((gchar *) &tag, 256, "%s[%d]", header, row);
          g_snprintf ((gchar *) &tag, 256, "%s%s",
                      (gchar *) &tag, (gchar *) tags[col]);
          if (gexiv2_metadata_try_has_tag (metadata, (gchar *) &tag, NULL))
            {
              tagcount++;
              break;
            }
        }
    }
  return tagcount;
}

static gchar **
get_tags (GExiv2Metadata  *metadata,
          const gchar     *header,
          const gchar    **tags,
          const gint       items,
          const gint       count)
{
  gchar **tagdata;
  gchar **_datarow;
  gchar   tag[256];
  int     row, col;

  g_return_val_if_fail (header != NULL && tags != NULL, NULL);
  g_return_val_if_fail (items > 0, NULL);

  if (count <= 0)
    return NULL;
  tagdata = g_new0 (gchar *, count);
  if (! tagdata)
    return NULL;

  for (row = 1; row < count + 1; row++)
    {
      tagdata[row-1] = g_malloc0 (sizeof (gchar *) * items);
      _datarow = (gchar **) tagdata[row-1];
      for (col = 0; col < items; col++)
        {
          gchar *value;

          g_snprintf ((gchar *) &tag, 256, "%s[%d]", header, row);
          g_snprintf ((gchar *) &tag, 256, "%s%s",
                      (gchar *) &tag, (gchar *) tags[col]);

          value = gexiv2_metadata_try_get_tag_string (metadata, (gchar *) &tag, NULL);

          g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "get_tags tag: %s, value: %s", (gchar *) &tag, value);

          if (_datarow && value)
            _datarow[col] = strdup (value);
        }
    }
  return tagdata;
}

static void
free_tagdata(gchar **tagdata, gint rows, gint cols)
{
  gint    row, col;
  gchar **tagdatarow;

  for (row = 0; row < rows; row++)
    {
      tagdatarow = (gpointer) tagdata[row];

      for (col = 0; col < cols; col++)
        {
          g_free (tagdatarow[col]);
        }
      g_free (tagdatarow);
    }
  g_free (tagdata);
}

/* ============================================================================
 * ==[ DATE CALLBACKS ]========================================================
 * ============================================================================
 */
void
on_create_date_button_clicked (GtkButton *widget,
                               gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.photoshop.DateCreated");
}

void
on_patient_dob_date_button_clicked (GtkButton *widget,
                                    gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.PatientDOB");
}

void
on_study_date_button_clicked (GtkButton *widget,
                              gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.StudyDateTime");
}

void
on_series_date_button_clicked (GtkButton *widget,
                               gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.SeriesDateTime");
}

void
on_date_button_clicked (GtkButton *widget,
                        GtkWidget *entry_widget,
                        gchar     *tag)
{
  GtkWidget      *calendar_dialog;
  GtkWidget      *calendar_content_area;
  GtkWidget      *calendar_vbox;
  GtkWidget      *calendar;
  const gchar    *date_text;
  GDateTime      *current_datetime;
  guint           year, month, day;

  date_text = gtk_entry_get_text (GTK_ENTRY (entry_widget));
  if (date_text && date_text[0] != '\0')
    {
      sscanf (date_text, "%u-%u-%u;", &year, &month, &day);
      month--;
    }
  else
    {
      current_datetime = g_date_time_new_now_local ();
      year = g_date_time_get_year (current_datetime);
      month = g_date_time_get_month (current_datetime) - 1;
      day = g_date_time_get_day_of_month (current_datetime);
    }

  calendar_dialog =
    gtk_dialog_new_with_buttons (_("Choose Date"),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                 _("Set Date"), GTK_RESPONSE_OK,
                                 NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (calendar_dialog),
                                   GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (calendar_dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (calendar_dialog));

  calendar_content_area = gtk_dialog_get_content_area (GTK_DIALOG (calendar_dialog));

  calendar_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (calendar_vbox), 6);
  gtk_box_pack_start (GTK_BOX (calendar_content_area), calendar_vbox, TRUE, TRUE, 0);

  calendar = gtk_calendar_new ();
  gtk_calendar_select_month (GTK_CALENDAR (calendar), month, year);
  gtk_calendar_select_day (GTK_CALENDAR (calendar), day);
  gtk_calendar_mark_day (GTK_CALENDAR (calendar), day);
  gtk_widget_show (calendar);

  gtk_container_add (GTK_CONTAINER (calendar_vbox), calendar);
  gtk_widget_show (calendar_vbox);

  if (gtk_dialog_run (GTK_DIALOG (calendar_dialog)) == GTK_RESPONSE_OK)
    {
      gchar date[25];
      gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);
      g_sprintf ((gchar*) &date, "%u-%02u-%02uT00:00:00+00:00", year, month+1, day);
      gtk_entry_set_text (GTK_ENTRY (entry_widget), date);
    }

  gtk_widget_destroy (calendar_dialog);
}

/* ============================================================================
 * ==[ SPECIAL TAGS HANDLERS ]=================================================
 * ============================================================================
 */

gboolean
hasImageSupplierTagData (metadata_editor *meta_info)
{
  gint loop;

  for (loop = 0; loop < n_imageSupplierInfoTags; loop++)
    {
      GtkWidget   *object;
      const gchar *text;

      object = metadata_editor_get_widget (meta_info, imageSupplierInfoTags[loop].id);

      if (imageSupplierInfoTags[loop].mode == MODE_SINGLE)
        {
          text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            return TRUE;
        }
      else if (imageSupplierInfoTags[loop].mode == MODE_MULTI)
        {
          text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            return TRUE;
        }
    }

  return FALSE;
}

gboolean
hasLocationCreationTagData (metadata_editor *meta_info)
{
  gint loop;

  for (loop = 0; loop < n_locationCreationInfoTags; loop++)
    {
      GtkWidget   *widget;
      const gchar *text;

      widget = metadata_editor_get_widget (meta_info, locationCreationInfoTags[loop].id);

      if (locationCreationInfoTags[loop].mode == MODE_SINGLE)
        {
          text = gtk_entry_get_text (GTK_ENTRY (widget));

          if (text && *text)
            return TRUE;
        }
    }

  return FALSE;
}

gboolean
hasModelReleaseTagData (metadata_editor *meta_info)
{
  return FALSE;
}

gboolean
hasPropertyReleaseTagData (metadata_editor *meta_info)
{
  return FALSE;
}


gboolean
hasCreatorTagData (metadata_editor *meta_info)
{
  gboolean has_data = FALSE;
  gint     loop;

  for (loop = 0; loop < n_creatorContactInfoTags; loop++)
    {
      GtkWidget *widget;

      widget = metadata_editor_get_widget (meta_info, creatorContactInfoTags[loop].id);

      if (GTK_IS_ENTRY (widget))
        {
          const gchar *text = gtk_entry_get_text (GTK_ENTRY (widget));

          if (text && *text)
            has_data = TRUE;
        }
      else if (GTK_IS_TEXT_VIEW (widget))
        {
          GtkTextView   *text_view = GTK_TEXT_VIEW (widget);
          GtkTextBuffer *buffer    = gtk_text_view_get_buffer (text_view);
          GtkTextIter    start;
          GtkTextIter    end;
          gchar         *text;

          gtk_text_buffer_get_start_iter (buffer, &start);
          gtk_text_buffer_get_end_iter (buffer, &end);

          text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

          if (text && *text)
            has_data = TRUE;

          if (text)
            g_free (text);
        }
    }

  return has_data;
}

/* ============================================================================
 * ==[ SET DIALOG METADATA ]===================================================
 * ============================================================================
 */

/* CELL EDITED */

void
cell_edited_callback (GtkCellRendererText *cell,
                      const gchar         *path_string,
                      const gchar         *new_text,
                      gpointer             data,
                      int                  index)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, index,
                      new_text, -1);
}

void
cell_edited_callback_combo (GtkCellRendererCombo *cell,
                            const gchar          *path_string,
                            const gchar          *new_text,
                            gpointer              data,
                            int                   column)
{
  GtkWidget        *widget;
  GtkTreeModel     *treemodel;
  GtkListStore     *liststore;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  GtkTreeSelection *selection;

  widget = GTK_WIDGET (data);

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
  liststore = GTK_LIST_STORE (treemodel);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection),
                                       NULL, &iter))
    {
      path = gtk_tree_model_get_path (treemodel, &iter);
      gtk_tree_path_free (path);
      gtk_list_store_set (liststore, &iter, column, new_text, -1);
    }
}

static void
licensor_name_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
licensor_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
licensor_phone1_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
licensor_phone_type1_cell_edited_callback (GtkCellRendererCombo *cell,
                                           const gchar          *path_string,
                                           const gchar          *new_text,
                                           gpointer              data)
{
  cell_edited_callback_combo (cell, path_string, new_text, data, 3);
}

static void
licensor_phone2_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
licensor_phone_type2_cell_edited_callback (GtkCellRendererCombo *cell,
                                           const gchar          *path_string,
                                           const gchar          *new_text,
                                           gpointer              data)
{
  cell_edited_callback_combo (cell, path_string, new_text, data, 5);
}

static void
licensor_email_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 6);
}

static void
licensor_web_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 7);
}

static void
cr_owner_name_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
cr_owner_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
img_cr8_name_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
img_cr8_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
aoo_copyright_notice_cell_edited_callback (GtkCellRendererText *cell,
                                           const gchar         *path_string,
                                           const gchar         *new_text,
                                           gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 5);
}

static void
aoo_source_inv_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
aoo_source_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 3);
}

static void
aoo_creator_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
aoo_date_creat_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
aoo_title_cell_edited_callback (GtkCellRendererText *cell,
                                const gchar         *path_string,
                                const gchar         *new_text,
                                gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
reg_org_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
reg_item_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
loc_sho_sub_loc_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
loc_sho_city_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
loc_sho_state_prov_cell_edited_callback (GtkCellRendererText *cell,
    const gchar         *path_string,
    const gchar         *new_text,
    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
loc_sho_cntry_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 3);
}

static void
loc_sho_cntry_iso_cell_edited_callback (GtkCellRendererText *cell,
                                        const gchar         *path_string,
                                        const gchar         *new_text,
                                        gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
loc_sho_wrld_reg_cell_edited_callback (GtkCellRendererText *cell,
                                       const gchar         *path_string,
                                       const gchar         *new_text,
                                       gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 5);
}

static void
prop_rel_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;
  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
mod_rel_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
organisation_image_name_cell_edited_callback (GtkCellRendererText *cell,
                                              const gchar         *path_string,
                                              const gchar         *new_text,
                                              gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
organisation_image_code_cell_edited_callback (GtkCellRendererText *cell,
                                              const gchar         *path_string,
                                              const gchar         *new_text,
                                              gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}


/* CELL / ROW REMOVE */

static void
list_row_remove_callback (GtkWidget *widget,
                          gpointer   data,
                          gchar     *tag)
{
  metadata_editor  *meta_info = data;
  GtkWidget        *list_widget;
  GtkListStore     *liststore;
  GtkTreeIter       iter;
  GtkTreeModel     *treemodel;
  GtkTreeSelection *selection;
  GtkTreePath      *path;

  list_widget = metadata_editor_get_widget (meta_info, tag);

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));
  liststore = GTK_LIST_STORE (treemodel);

  selection = gtk_tree_view_get_selection ((GtkTreeView *)list_widget);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint number_of_rows;

      path = gtk_tree_model_get_path (treemodel, &iter);
      gtk_list_store_remove (liststore, &iter);
      gtk_tree_path_free (path);

      number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);
      /* Make sur that two rows are always showing, else it looks ugly. */
      if (number_of_rows < 2)
        {
          gtk_list_store_append (liststore, &iter);
        }
    }
}

static void
property_release_id_remove_callback (GtkWidget *widget,
                                     gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.PropertyReleaseID");
}

static void
model_release_id_remove_callback (GtkWidget *widget,
                                  gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.ModelReleaseID");
}

static void
shown_location_remove_callback (GtkWidget *widget,
                                gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.LocationShown");
}

static void
feat_org_name_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.OrganisationInImageName");
}

static void
feat_org_code_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.OrganisationInImageCode");
}

static void
artwork_object_remove_callback (GtkWidget *widget,
                                gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.ArtworkOrObject");
}

static void
reg_entry_remove_callback (GtkWidget *widget,
                           gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.RegistryId");
}

static void
image_creator_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.ImageCreator");
}

static void
copyright_own_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.CopyrightOwner");
}

static void
licensor_remove_callback (GtkWidget *widget,
                          gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.Licensor");
}


/* CELL / ROW ADD */

static void
list_row_add_callback (GtkWidget *widget,
                       gpointer   data,
                       gchar     *tag)
{
  metadata_editor  *meta_info = data;
  GtkWidget        *list_widget;
  GtkListStore     *liststore;
  GtkTreeIter       iter;

  list_widget = metadata_editor_get_widget (meta_info, tag);

  liststore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget)));

  gtk_list_store_append (liststore, &iter);
}

static void
property_release_id_add_callback (GtkWidget *widget,
                                  gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.PropertyReleaseID");
}

static void
model_release_id_add_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.ModelReleaseID");
}

static void
shown_location_add_callback (GtkWidget *widget,
                             gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.LocationShown");
}

static void
feat_org_name_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.OrganisationInImageName");
}

static void
feat_org_code_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.OrganisationInImageCode");
}

static void
artwork_object_add_callback (GtkWidget *widget,
                             gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.ArtworkOrObject");
}

static void
reg_entry_add_callback (GtkWidget *widget,
                        gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.RegistryId");
}

static void
image_creator_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.ImageCreator");
}

static void
copyright_own_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.CopyrightOwner");
}

static void
licensor_add_callback (GtkWidget *widget,
                       gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.Licensor");
}

const gchar *gpstooltips[] =
{
    N_ ("Enter or edit GPS value here.\n"
        "Valid values consist of 1, 2 or 3 numbers "
        "(degrees, minutes, seconds), see the following examples:\n"
        "10deg 15' 20\", or 10\u00b0 15' 20\", or 10:15:20.45, or "
        "10 15 20, or 10 15.30, or 10.45\n"
        "Delete all text to remove the current value."),
    N_ ("Enter or edit GPS altitude value here.\n"
        "A valid value consists of one number:\n"
        "e.g. 100, or 12.24\n"
        "Depending on the selected measurement type "
        "the value should be entered in meter (m) "
        "or feet (ft)\n"
        "Delete all text to remove the current value.")
};

enum
{
  GPS_LONG_LAT_TOOLTIP,
  GPS_ALTITUDE_TOOLTIP,
};

/* Set dialog display settings and data */

static void
metadata_dialog_editor_set_metadata (GExiv2Metadata  *metadata,
                                     metadata_editor *meta_info)
{
  GtkWidget *combo_widget;
  GtkWidget *entry_widget;
  GtkWidget *button_widget;
  gint       width, height;
  gchar     *value;
  gint       i;

  gint32 numele = n_default_metadata_tags;

  /* Setup Buttons */
  button_widget = metadata_editor_get_widget (meta_info, "add_licensor_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (licensor_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_licensor_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (licensor_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_copyright_own_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (copyright_own_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_copyright_own_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (copyright_own_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_image_creator_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (image_creator_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_image_creator_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (image_creator_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_reg_entry_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (reg_entry_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_reg_entry_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (reg_entry_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_artwork_object_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (artwork_object_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_artwork_object_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (artwork_object_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_feat_org_code_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_code_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_feat_org_code_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_code_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_feat_org_name_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_name_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_feat_org_name_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_name_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_shown_location_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (shown_location_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_shown_location_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (shown_location_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_model_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (model_release_id_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_model_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (model_release_id_remove_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "add_prop_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (property_release_id_add_callback),
                    meta_info);

  button_widget = metadata_editor_get_widget (meta_info, "rem_prop_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (property_release_id_remove_callback),
                    meta_info);

  /* Setup Comboboxes */
  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.xmp.Rating");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                  _("Unrated"));
  for (i = 1; i < 6; i++)
    {
      gchar *display = g_strdup_printf ("%d", i);

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      display);
      g_free (display);
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.xmpRights.Marked");
  for (i = 0; i < n_marked; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (marked[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.photoshop.Urgency");
  for (i = 0; i < n_urgency; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (urgency[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.plus.MinorModelAgeDisclosure");
  for (i = 0; i < n_minormodelagedisclosure; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (minormodelagedisclosure[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.plus.ModelReleaseStatus");
  for (i = 0; i < n_modelreleasestatus; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (modelreleasestatus[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);
  gtk_widget_get_size_request (combo_widget, &width, &height);
  gtk_widget_set_size_request (combo_widget, 180, height);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.iptcExt.DigitalSourceType");
  for (i = 0; i < n_digitalsourcetype; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (digitalsourcetype[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.plus.PropertyReleaseStatus");
  for (i = 0; i < n_propertyreleasestatus; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (propertyreleasestatus[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);
  gtk_widget_get_size_request (combo_widget, &width, &height);
  gtk_widget_set_size_request (combo_widget, 180, height);

  combo_widget = metadata_editor_get_widget (meta_info, "Xmp.DICOM.PatientSex");
  for (i = 0; i < n_dicom; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (dicom[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Exif.GPSInfo.GPSLatitudeRef");
  for (i = 0; i < n_gpslatref; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpslatref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Exif.GPSInfo.GPSLongitudeRef");
  for (i = 0; i < n_gpslngref; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpslngref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "Exif.GPSInfo.GPSAltitudeRef");
  for (i = 0; i < n_gpsaltref; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpsaltref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = metadata_editor_get_widget (meta_info, "GPSAltitudeSystem");
  for (i = 0; i < n_gpsaltsys; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpsaltsys[i]));
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  g_signal_connect (G_OBJECT (combo_widget), "changed",
                    G_CALLBACK (gpsaltsys_combo_callback),
                    meta_info);

  /* Set up text view heights */

  /* Set up lists */
  for (i = 0; i < n_imageSupplierInfoTags; i++)
    {
      GtkWidget *widget;

      widget = metadata_editor_get_widget (meta_info,
                                           imageSupplierInfoTags[i].id);

      value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                              imageSupplierInfoTags[i].tag,
                                                              NULL);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);
          g_free (value);

          if (imageSupplierInfoTags[i].mode == MODE_SINGLE)
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
          else if (imageSupplierInfoTags[i].mode == MODE_MULTI)
            {
              GtkTextBuffer *buffer;

              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value_utf, -1);
            }
          g_free (value_utf);
        }
    }

  for (i = 0; i < n_locationCreationInfoTags; i++)
    {
      GtkWidget *widget;

      widget = metadata_editor_get_widget (meta_info,
                                           locationCreationInfoTags[i].id);

      value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                              locationCreationInfoTags[i].tag,
                                                              NULL);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);
          g_free (value);

          if (locationCreationInfoTags[i].mode == MODE_SINGLE)
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
          g_free (value_utf);
        }
    }

  /* Set up tag data */

  for (i = 0; i < numele; i++)
    {
      GtkWidget *widget;
      gint       index;

      widget = metadata_editor_get_widget (meta_info, default_metadata_tags[i].tag);

      if (! strcmp ("Exif.GPSInfo.GPSLongitude",
                    default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_try_get_gps_longitude (metadata, &gps_value, NULL))
            {
              str = metadata_format_gps_longitude_latitude (gps_value);
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_LONG_LAT_TOOLTIP]));
          continue;
        }
      else if (! strcmp ("Exif.GPSInfo.GPSLatitude",
                         default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_try_get_gps_latitude (metadata, &gps_value, NULL))
            {
              str = metadata_format_gps_longitude_latitude (gps_value);
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_LONG_LAT_TOOLTIP]));
          continue;
        }
      else if (! strcmp ("Exif.GPSInfo.GPSAltitude",
                        default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_try_get_gps_altitude (metadata, &gps_value, NULL))
            {
              str = metadata_format_gps_altitude (gps_value, TRUE, "");
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_ALTITUDE_TOOLTIP]));
          continue;
        }

      index = default_metadata_tags[i].other_tag_index;

      if (default_metadata_tags[i].xmp_type == GIMP_XMP_BAG ||
          default_metadata_tags[i].xmp_type == GIMP_XMP_SEQ)
        {
          gchar **values;

          value = NULL;
          values = gexiv2_metadata_try_get_tag_multiple (metadata,
                                                         default_metadata_tags[i].tag,
                                                         NULL);

          if (values)
            {
              gint vi;

              for (vi = 0; values[vi] != NULL; vi++)
                {
                  gchar *value_clean;

                  value_clean = clean_xmp_string (values[vi]);

                  if (value_clean != NULL && value_clean[0] != '\0')
                    {
                      if (! value)
                        {
                          value = g_strdup (value_clean);
                        }
                      else
                        {
                          gchar *tmpvalue;

                          tmpvalue = value;
                          value = g_strconcat (value, "\n", value_clean, NULL);
                          g_free (tmpvalue);
                        }
                    }
                  g_free (value_clean);
                }
            }

          if (index > -1)
            {
              gchar **equiv_values;

              if (equivalent_metadata_tags[index].exif_tag_index > -1)
                {
                  gint32       i_exif       = equivalent_metadata_tags[index].exif_tag_index;
                  const gchar *exif_tag_str = exif_equivalent_tags[i_exif].tag;
                  gchar       *exif_value   = NULL;

                  exif_value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                                               exif_tag_str,
                                                                               NULL);

                  if (exif_value)
                    {
                      if (! value)
                        {
                          value = exif_value;
                          exif_value = NULL;
                        }
                      else
                        {
                          if (g_strcmp0 (value, exif_value))
                            {
                              g_printerr ("Value of tag %s: '%s' is not the same as %s: '%s'. "
                                          "Ignoring value of %s.\n",
                                          default_metadata_tags[equivalent_metadata_tags[index].default_tag_index].tag,
                                          value, exif_tag_str, exif_value, exif_tag_str);
                            }
                        }
                    }
                  g_free (exif_value);
                }

              /* These are all IPTC tags some of which can appear multiple times so
                * we will use get_tag_multiple. Also IPTC most commonly uses UTF-8
                * not current locale so get_tag_interpreted was wrong anyway.
                * FIXME For now lets interpret as UTF-8 and in the future read
                * and interpret based on the CharacterSet tag.
                */
              equiv_values = gexiv2_metadata_try_get_tag_multiple (metadata,
                                                                   equivalent_metadata_tags[index].tag,
                                                                   NULL);

              if (equiv_values)
                {
                  gint evi;

                  for (evi = 0; equiv_values[evi] != NULL; evi++)
                    {
                      if (equiv_values[evi][0] != '\0')
                        {
                          if (! value)
                            {
                              value = g_strdup (equiv_values[evi]);
                            }
                          else
                            {
                              if (! g_strv_contains ((const gchar * const *) values, equiv_values[evi]))
                                {
                                  gchar *tmpvalue;

                                  tmpvalue = value;
                                  value = g_strconcat (value, "\n", equiv_values[evi], NULL);
                                  g_free (tmpvalue);
                                }
                            }
                        }
                    }
                  g_strfreev (equiv_values);
                }
            }
          g_strfreev (values);
        }
      else
        {
          value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                                  default_metadata_tags[i].tag,
                                                                  NULL);

          if (value)
            {
              gchar *value_utf8 = clean_xmp_string (value);

              g_free (value);

              if (value_utf8 && value_utf8[0] != '\0')
                {
                  value = g_strdup (value_utf8);
                }
              else
                {
                  value = NULL;
                }

              g_free (value_utf8);
            }

          if (index == SPECIAL_PROCESSING_DATE_CREATED)
            {
              gchar *date_value      = NULL;
              gchar *date_time_value = NULL;

              /* XMP: Check if Xmp.photoshop.DateCreated has the same value as
                 IPTC: Iptc.Application2.DateCreated, and
                       Iptc.Application2.TimeCreated combined.
                 If empty, this can be filled from:
                 Exif: DateTimeOriginal, OffsetTimeOriginal
               */
              date_value = gexiv2_metadata_try_get_tag_string (metadata,
                                                               "Iptc.Application2.DateCreated",
                                                               NULL);
              if (date_value)
                {
                  gchar *time_value = NULL;

                  time_value = gexiv2_metadata_try_get_tag_string (metadata,
                                                                   "Iptc.Application2.TimeCreated",
                                                                   NULL);
                  if (time_value)
                    {
                      date_time_value = g_strconcat (date_value, "T", time_value, NULL);
                      g_free (time_value);

                      if (! value)
                        {
                          /* Copy Xmp value from IPTC values*/
                          value = date_time_value;
                          date_time_value = NULL;
                        }
                      else if (strcmp (value, date_time_value))
                        {
                          g_printerr ("Xmp.Photoshop.DateCreated %s is not the same as "
                                      "the combined IPTC DateCreated and TimeCreated "
                                      "values: %s. The IPTC values will be ignored.\n",
                                      value, date_time_value);
                        }
                    }
                  else
                    {
                      if (! value)
                        {
                          value = g_strconcat (date_value, "T00:00:00+00:00", NULL);
                        }
                      else
                        {
                          g_printerr ("Missing IPTC TimeCreated tag. We won't "
                                      "compare the IPTC DateCreated value"
                                      "with Xmp.Photoshop.DateCreated.\n");
                        }
                    }
                }
              else if (! value)
                {
                  /* Set initial value from Exif */
                  date_value = gexiv2_metadata_try_get_tag_string (metadata,
                                                                   "Exif.Photo.DateTimeOriginal",
                                                                   NULL);
                  if (date_value)
                    {
                      gchar **date_time_split = NULL;

                      /* Exif has space as separator between date and time,
                         and no timezone, which is a separate tag. */
                      date_time_split = g_strsplit (date_value, " ", 2);

                      if (date_time_split[0] != NULL)
                        {
                          gchar  *time_zone = NULL;

                          /* Exif uses ':' as date delimiter instead of '-' */
                          g_strdelimit (date_time_split[0], ":", '-');

                          if (date_time_split[1] != NULL)
                            {
                              date_time_value = g_strconcat (date_time_split[0],
                                                             "T", date_time_split[1],
                                                             NULL);
                            }
                          else
                            {
                              date_time_value = g_strconcat (date_time_split[0],
                                                             "T00:00:00", NULL);
                            }

                          time_zone = gexiv2_metadata_try_get_tag_string (metadata,
                                                                          "Exif.Photo.OffsetTimeOriginal",
                                                                          NULL);
                          if (time_zone)
                            {
                              value = g_strconcat (date_time_value, time_zone, NULL);
                            }
                          else
                            {
                              value = g_strconcat (date_time_value, "+00:00", NULL);
                            }
                          g_free (time_zone);
                        }
                      g_strfreev (date_time_split);
                    }
                }
              g_free (date_value);
              g_free (date_time_value);
            }
          else if (index > -1)
            {
              gchar **values;

              if (equivalent_metadata_tags[index].exif_tag_index > -1)
                {
                  gint32       i_exif       = equivalent_metadata_tags[index].exif_tag_index;
                  const gchar *exif_tag_str = exif_equivalent_tags[i_exif].tag;
                  gchar       *exif_value   = NULL;

                  exif_value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                                               exif_tag_str,
                                                                               NULL);

                  if (exif_value)
                    {

                      if (! value)
                        {
                          value = exif_value;
                          exif_value = NULL;
                        }
                      else
                        {
                          if (g_strcmp0 (value, exif_value))
                            {
                              g_printerr ("Value of tag %s: '%s' is not the same as %s: '%s'. "
                                          "Ignoring value of %s.\n",
                                          default_metadata_tags[equivalent_metadata_tags[index].default_tag_index].tag,
                                          value, exif_tag_str, exif_value, exif_tag_str);
                            }
                        }
                    }
                  g_free (exif_value);
                }

              /* It's not very likely we will have an XMP tag that can only
               * have a single value instead of an array, which corresponds to
               * an IPTC tag that can have multiple values, but since we
               * already have this code it can't hurt to keep testing for it.
               * FIXME For now lets interpret as UTF-8 and in the future read
               * and interpret based on the CharacterSet tag.
               */
              values = gexiv2_metadata_try_get_tag_multiple (metadata,
                                                             equivalent_metadata_tags[index].tag,
                                                             NULL);

              if (values)
                {
                  gint     i;
                  GString *str = NULL;

                  for (i = 0; values[i] != NULL; i++)
                    {
                      if (values[i][0] != '\0')
                        {
                          if (! str)
                            {
                              str = g_string_new (values[i]);
                            }
                          else
                            {
                              if (equivalent_metadata_tags[index].mode == MODE_MULTI)
                                {
                                  g_string_append (str, "\n");
                                }
                              else
                                {
                                  g_string_append (str, ", ");
                                }
                              g_string_append (str, values[i]);
                            }
                        }
                    }

                  if (str)
                    {
                      /* If we got values from both Xmp and Iptc then compare those
                       * values and if they are different concatenate them. Usually they
                       * should be the same in which case we won't duplicate the string.
                       */
                      if (value && strcmp (value, str->str))
                        {
                          if (equivalent_metadata_tags[index].mode == MODE_MULTI)
                            {
                              g_string_prepend (str, "\n");
                            }
                          else
                            {
                              g_string_prepend (str, ", ");
                            }
                          g_string_prepend (str, value);
                          g_free (value);
                        }
                      value = g_string_free (str, FALSE);
                    }
                  g_strfreev (values);
                }
            }
        }

      if (default_metadata_tags[i].mode == MODE_LIST)
        {
          /* Tab: IPTC Extension, Label: Location Shown */
          if (! strcmp ("Xmp.iptcExt.LocationShown",
                        default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* LOCATION SHOWN - SUB LOCATION */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_SUB_LOC);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_sub_loc_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_SUB_LOC));
                }

              /* LOCATION SHOWN - CITY */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CITY);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_city_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CITY));
                }

              /* LOCATION SHOWN - STATE PROVINCE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_STATE_PROV);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_state_prov_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_STATE_PROV));
                }

              /* LOCATION SHOWN - COUNTRY */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_cntry_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY));
                }

              /* LOCATION SHOWN - COUNTRY ISO */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY_ISO);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_cntry_iso_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY_ISO));
                }

              /* LOCATION SHOWN - WORLD REGION */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY_WRLD_REG);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_wrld_reg_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY_WRLD_REG));
                }

              /* Favor the most common form: /Iptc4xmpExt:* */
              counter = count_tags (metadata, LOCATIONSHOWN_HEADER,
                                    locationshown,
                                    n_locationshown);

              tagdata = get_tags (metadata, LOCATIONSHOWN_HEADER,
                                  locationshown,
                                  n_locationshown, counter);

              if (counter == 0 || ! tagdata)
                {
                  /* Alternatively try: /iptcExt:* */
                  counter = count_tags (metadata, LOCATIONSHOWN_HEADER,
                                        locationshown_alternative,
                                        n_locationshown);

                  tagdata = get_tags (metadata, LOCATIONSHOWN_HEADER,
                                      locationshown_alternative,
                                      n_locationshown, counter);
                }

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        tagdatarow[0],
                                          COL_LOC_SHO_CITY,           tagdatarow[1],
                                          COL_LOC_SHO_STATE_PROV,     tagdatarow[2],
                                          COL_LOC_SHO_CNTRY,          tagdatarow[3],
                                          COL_LOC_SHO_CNTRY_ISO,      tagdatarow[4],
                                          COL_LOC_SHO_CNTRY_WRLD_REG, tagdatarow[5],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_locationshown);

                  if (counter == 1)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        NULL,
                                          COL_LOC_SHO_CITY,           NULL,
                                          COL_LOC_SHO_STATE_PROV,     NULL,
                                          COL_LOC_SHO_CNTRY,          NULL,
                                          COL_LOC_SHO_CNTRY_ISO,      NULL,
                                          COL_LOC_SHO_CNTRY_WRLD_REG, NULL,
                                          -1);
                    }
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        NULL,
                                          COL_LOC_SHO_CITY,           NULL,
                                          COL_LOC_SHO_STATE_PROV,     NULL,
                                          COL_LOC_SHO_CNTRY,          NULL,
                                          COL_LOC_SHO_CNTRY_ISO,      NULL,
                                          COL_LOC_SHO_CNTRY_WRLD_REG, NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Featured Organization - Name */
          else if (! strcmp ("Xmp.iptcExt.OrganisationInImageName",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (organisation_image_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_ORG_IMG_NAME));
                }

              add_to_store (value, liststore, COL_ORG_IMG_NAME);
            }
          /* Tab: IPTC Extension, Label: Featured Organization - Code */
          else if (! strcmp ("Xmp.iptcExt.OrganisationInImageCode",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (organisation_image_code_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_ORG_IMG_CODE));
                }

              add_to_store (value, liststore, COL_ORG_IMG_CODE);
            }
          /* Tab: IPTC Extension, Label: Artwork or Object */
          else if (! strcmp ("Xmp.iptcExt.ArtworkOrObject",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* ARTWORK OR OBJECT - TITLE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_TITLE);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_title_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_TITLE));
                }

              /* ARTWORK OR OBJECT - DATE CREATED */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_DATE_CREAT);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_date_creat_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_DATE_CREAT));
                }

              /* ARTWORK OR OBJECT - CREATOR */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_CREATOR);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_creator_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_CREATOR));
                }

              /* ARTWORK OR OBJECT - SOURCE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_SOURCE);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_source_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_SOURCE));
                }

              /* ARTWORK OR OBJECT - SOURCE INVENTORY ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_SRC_INV_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_source_inv_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_SRC_INV_ID));
                }

              /* ARTWORK OR OBJECT - COPYRIGHT NOTICE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_CR_NOT);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_copyright_notice_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_CR_NOT));
                }

              counter = count_tags (metadata, ARTWORKOROBJECT_HEADER,
                                    artworkorobject,
                                    n_artworkorobject);

              tagdata = get_tags (metadata, ARTWORKOROBJECT_HEADER,
                                  artworkorobject,
                                  n_artworkorobject, counter);

              if (counter == 0 || ! tagdata)
                {
                  /* Alternatively try: /iptcExt:* */
                  counter = count_tags (metadata, ARTWORKOROBJECT_HEADER,
                                        artworkorobject_alternative,
                                        n_artworkorobject);

                  tagdata = get_tags (metadata, ARTWORKOROBJECT_HEADER,
                                      artworkorobject_alternative,
                                      n_artworkorobject, counter);
                }


              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      /* remove substring for language id in title field */
                      remove_substring (tagdatarow[COL_AOO_TITLE], lang_default);
                      if (strstr (tagdatarow[COL_AOO_TITLE], " "))
                        {
                          remove_substring (tagdatarow[COL_AOO_TITLE], " ");
                        }

                      remove_substring (tagdatarow[COL_AOO_TITLE], bag_default);
                      if (strstr (tagdatarow[COL_AOO_TITLE], " "))
                        {
                          remove_substring (tagdatarow[COL_AOO_TITLE], " ");
                        }

                      remove_substring (tagdatarow[COL_AOO_TITLE], seq_default);
                      if (strstr (tagdatarow[COL_AOO_TITLE], " "))
                        {
                          remove_substring (tagdatarow[COL_AOO_TITLE], " ");
                        }

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_AOO_TITLE,      tagdatarow[0],
                                          COL_AOO_DATE_CREAT, tagdatarow[1],
                                          COL_AOO_CREATOR,    tagdatarow[2],
                                          COL_AOO_SOURCE,     tagdatarow[3],
                                          COL_AOO_SRC_INV_ID, tagdatarow[4],
                                          COL_AOO_CR_NOT,     tagdatarow[5],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_artworkorobject);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_AOO_TITLE,      NULL,
                                          COL_AOO_DATE_CREAT, NULL,
                                          COL_AOO_CREATOR,    NULL,
                                          COL_AOO_SOURCE,     NULL,
                                          COL_AOO_SRC_INV_ID, NULL,
                                          COL_AOO_CR_NOT,     NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Model Release Identifier */
          else if (! strcmp ("Xmp.plus.ModelReleaseID",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (mod_rel_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_PROP_REL_ID));
                }

              add_to_store (value, liststore, COL_MOD_REL_ID);
            }
          /* Tab: IPTC Extension, Label: Registry Entry */
          else if (! strcmp ("Xmp.iptcExt.RegistryId",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* REGISTRY - ORGANIZATION ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_REGISTRY_ORG_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = (GtkCellRenderer*) r->data;
                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);
                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (reg_org_id_cell_edited_callback),
                                    treemodel);
                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_REGISTRY_ORG_ID));
                }

              /* REGISTRY - ITEM ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_REGISTRY_ITEM_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (reg_item_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_REGISTRY_ITEM_ID));
                }

              counter = count_tags (metadata, REGISTRYID_HEADER,
                                    registryid,
                                    n_registryid);

              tagdata = get_tags (metadata, REGISTRYID_HEADER,
                                  registryid,
                                  n_registryid, counter);

              if (counter == 0 || ! tagdata)
                {
                  /* Alternatively try: /iptcExt:* */
                  counter = count_tags (metadata, REGISTRYID_HEADER,
                                        registryid_alternative,
                                        n_registryid);

                  tagdata = get_tags (metadata, REGISTRYID_HEADER,
                                      registryid_alternative,
                                      n_registryid, counter);
                }

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_REGISTRY_ORG_ID,  tagdatarow[0],
                                          COL_REGISTRY_ITEM_ID, tagdatarow[1],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_registryid);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_REGISTRY_ORG_ID,  NULL,
                                          COL_REGISTRY_ITEM_ID, NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Image Creator */
          else if (! strcmp ("Xmp.plus.ImageCreator",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* IMAGE CREATOR - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_IMG_CR8_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (img_cr8_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_IMG_CR8_NAME));
                }

              /* IMAGE CREATOR - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_IMG_CR8_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (img_cr8_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_IMG_CR8_ID));
                }

                counter = count_tags (metadata, IMAGECREATOR_HEADER,
                                      imagecreator,
                                      n_imagecreator);

                tagdata = get_tags (metadata, IMAGECREATOR_HEADER,
                                    imagecreator,
                                    n_imagecreator, counter);

                if (counter > 0 && tagdata)
                  {
                    gint item;

                    for (item = 0; item < counter; item++)
                      {
                        gchar **tagdatarow = (gchar **) tagdata[item];

                        gtk_list_store_append (liststore, &iter);
                        gtk_list_store_set (liststore, &iter,
                                            COL_IMG_CR8_NAME, tagdatarow[0],
                                            COL_IMG_CR8_ID,   tagdatarow[1],
                                            -1);
                      }
                    free_tagdata(tagdata, counter, n_imagecreator);
                  }
                else
                  {
                    gint item;

                    for (item = 0; item < 2; item++)
                      {
                        gtk_list_store_append (liststore, &iter);
                        gtk_list_store_set (liststore, &iter,
                                            COL_IMG_CR8_NAME, NULL,
                                            COL_IMG_CR8_ID,   NULL,
                                            -1);
                      }
                  }
            }
          /* Tab: IPTC Extension, Label: Copyright Owner */
          else if (! strcmp ("Xmp.plus.CopyrightOwner",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* COPYRIGHT OWNER - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_CR_OWNER_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (cr_owner_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_CR_OWNER_NAME));
                }

              /* COPYRIGHT OWNER - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_CR_OWNER_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (cr_owner_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_CR_OWNER_ID));
                }

              counter = count_tags (metadata, COPYRIGHTOWNER_HEADER,
                                    copyrightowner,
                                    n_copyrightowner);

              tagdata = get_tags (metadata, COPYRIGHTOWNER_HEADER,
                                  copyrightowner,
                                  n_copyrightowner, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_CR_OWNER_NAME, tagdatarow[0],
                                          COL_CR_OWNER_ID,   tagdatarow[1],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_copyrightowner);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_CR_OWNER_NAME, NULL,
                                          COL_CR_OWNER_ID,   NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Licensor */
          else if (! strcmp ("Xmp.plus.Licensor",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkTreeModel      *phonemodel;
              GtkListStore      *liststore;
              GtkListStore      *phonestore;
              GtkTreeIter        iter;
              GtkTreeIter        phoneiter;
              gint               counter;
              gint               j;
              gchar            **tagdata;

              phonestore = gtk_list_store_new (1, G_TYPE_STRING);
              gtk_list_store_append (phonestore, &phoneiter);
              gtk_list_store_set (phonestore, &phoneiter, 0, "Unknown", -1);
              for (j=1; j < n_phone_types; j++)
                {
                  gtk_list_store_append (phonestore, &phoneiter);
                  gtk_list_store_set (phonestore, &phoneiter,
                                      0, gettext (phone_types[j].display),
                                      -1);
                }
              phonemodel = GTK_TREE_MODEL (phonestore);

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* LICENSOR - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_NAME));
                }

              /* LICENSOR - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_ID));
                }

              /* LICENSOR - PHONE NUMBER 1 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE1);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone1_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE1));
                }

              /* LICENSOR - PHONE TYPE 1 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE_TYPE1);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable",    TRUE,
                                "text-column", 0,
                                "has-entry",   FALSE,
                                "model",       phonemodel,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone_type1_cell_edited_callback),
                                    widget);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE_TYPE1));
                }

              /* LICENSOR - PHONE NUMBER 2 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE2);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone2_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE2));
                }

              /* LICENSOR - PHONE TYPE 2 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE_TYPE2);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable",    TRUE,
                                "text-column", 0,
                                "has-entry",   FALSE,
                                "model",       phonemodel,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone_type2_cell_edited_callback),
                                    widget);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE_TYPE2));
                }

              /* LICENSOR - EMAIL */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_EMAIL);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_email_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_EMAIL));
                }

              /* LICENSOR - WEB ADDRESS */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_WEB);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_web_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_WEB));
                }

              counter = count_tags (metadata, LICENSOR_HEADER,
                                    licensor,
                                    n_licensor);

              tagdata = get_tags (metadata, LICENSOR_HEADER,
                                  licensor,
                                  n_licensor, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];
                      gchar   *type1;
                      gchar   *type2;
                      gint    types;

                      type1 = g_strdup (gettext (phone_types[0].display));
                      type2 = g_strdup (gettext (phone_types[0].display));

                      for (types = 0; types < n_phone_types; types++)
                        {
                          /* phone type 1 */
                          if (tagdatarow[3] &&
                              ! strcmp (tagdatarow[3],
                                        phone_types[types].data))
                            {
                              g_free (type1);
                              type1 = g_strdup (gettext (phone_types[types].display));
                            }

                          /* phone type 2 */
                          if (tagdatarow[5] &&
                              ! strcmp (tagdatarow[5],
                                        phone_types[types].data))
                            {
                              g_free (type2);
                              type2 = g_strdup (gettext (phone_types[types].display));
                            }
                        }

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LICENSOR_NAME,        tagdatarow[0],
                                          COL_LICENSOR_ID,          tagdatarow[1],
                                          COL_LICENSOR_PHONE1,      tagdatarow[2],
                                          COL_LICENSOR_PHONE_TYPE1, type1,
                                          COL_LICENSOR_PHONE2,      tagdatarow[4],
                                          COL_LICENSOR_PHONE_TYPE2, type2,
                                          COL_LICENSOR_EMAIL,       tagdatarow[6],
                                          COL_LICENSOR_WEB,         tagdatarow[7],
                                          -1);
                      g_free (type1);
                      g_free (type2);
                    }
                  free_tagdata(tagdata, counter, n_licensor);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LICENSOR_NAME,        NULL,
                                          COL_LICENSOR_ID,          NULL,
                                          COL_LICENSOR_PHONE1,      NULL,
                                          COL_LICENSOR_PHONE_TYPE1, gettext (phone_types[0].display),
                                          COL_LICENSOR_PHONE2,      NULL,
                                          COL_LICENSOR_PHONE_TYPE1, gettext (phone_types[0].display),
                                          COL_LICENSOR_EMAIL,       NULL,
                                          COL_LICENSOR_WEB,         NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Property Release Identifier */
          else if (! strcmp ("Xmp.plus.PropertyReleaseID",
                              default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (prop_rel_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_PROP_REL_ID));
                }

              add_to_store (value, liststore, COL_PROP_REL_ID);
            }
        }

      if (value)
        {
          if (default_metadata_tags[i].mode == MODE_SINGLE)
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value);
            }
          else if (default_metadata_tags[i].mode == MODE_MULTI)
            {
              GtkTextBuffer *buffer;

              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value, -1);
            }
          else if (default_metadata_tags[i].mode == MODE_COMBO)
            {
              gint32 data = 0;

              if (! strcmp ("Exif.GPSInfo.GPSLatitudeRef",
                            default_metadata_tags[i].tag))
                {
                  if (! strncmp ("N", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("S", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Exif.GPSInfo.GPSLongitudeRef",
                                 default_metadata_tags[i].tag))
                {
                  if (! strncmp ("E", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("W", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Exif.GPSInfo.GPSAltitudeRef",
                                 default_metadata_tags[i].tag))
                {
                  if (! strncmp ("A", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("B", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.xmp.Rating", default_metadata_tags[i].tag))
                {
                  if (! strcmp ("1", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("2", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("3", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("4", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("5", value))
                    {
                      data = 5;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.xmpRights.Marked",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("True", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("False", value))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.photoshop.Urgency",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("1", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("2", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("3", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("4", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("5", value))
                    {
                      data = 5;
                    }
                  else if (! strcmp ("6", value))
                    {
                      data = 6;
                    }
                  else if (! strcmp ("7", value))
                    {
                      data = 7;
                    }
                  else if (! strcmp ("8", value))
                    {
                      data = 8;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.plus.MinorModelAgeDisclosure",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("Age Unknown", value))
                    {
                      data = 0;
                    }
                  else if (! strcmp ("Age 25 or Over", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("Age 24", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("Age 23", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("Age 22", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("Age 21", value))
                    {
                      data = 5;
                    }
                  else if (! strcmp ("Age 20", value))
                    {
                      data = 6;
                    }
                  else if (! strcmp ("Age 19", value))
                    {
                      data = 7;
                    }
                  else if (! strcmp ("Age 18", value))
                    {
                      data = 8;
                    }
                  else if (! strcmp ("Age 17", value))
                    {
                      data = 9;
                    }
                  else if (! strcmp ("Age 16", value))
                    {
                      data = 10;
                    }
                  else if (! strcmp ("Age 15", value))
                    {
                      data = 11;
                    }
                  else if (! strcmp ("Age 14 or Under", value))
                    {
                      data = 12;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.plus.ModelReleaseStatus",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < n_modelreleasestatus; loop++)
                    {
                      if (! strcmp (modelreleasestatus[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (modelreleasestatus[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.iptcExt.DigitalSourceType",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < n_digitalsourcetype; loop++)
                    {
                      if (! strcmp (digitalsourcetype[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (digitalsourcetype[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.plus.PropertyReleaseStatus",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < n_propertyreleasestatus; loop++)
                    {
                      if (! strcmp (propertyreleasestatus[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (propertyreleasestatus[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.DICOM.PatientSex",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("male", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("female", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("other", value))
                    {
                      data = 3;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
            }
          g_free (value);
        }
    }

  /* Set Xmp.iptc.CreatorContactInfo/Iptc4xmpCore:* last since the short form
   * Xmp.iptc.Ci* could have been used to set this information too. Because
   * the first (longer) form is the most common let that override the shorter
   * form in the (unlikely) case that both are present and also have
   * different values. Due to a bug in the metadata-editor previously only
   * the short form was saved.
   */
  for (i = 0; i < n_creatorContactInfoTags; i++)
    {
      GtkWidget *widget;

      widget = metadata_editor_get_widget (meta_info, creatorContactInfoTags[i].id);

      value = gexiv2_metadata_try_get_tag_interpreted_string (metadata,
                                                              creatorContactInfoTags[i].tag,
                                                              NULL);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);
          g_free (value);

          if (creatorContactInfoTags[i].mode == MODE_SINGLE)
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
          else if (creatorContactInfoTags[i].mode == MODE_MULTI)
            {
              GtkTextBuffer *buffer;
              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value_utf, -1);
            }
          g_free (value_utf);
        }
    }

  /* Set creation date */
  entry_widget = metadata_editor_get_widget (meta_info, "create_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_create_date_button_clicked),
                    metadata_editor_get_widget (meta_info,
                                                "Xmp.photoshop.DateCreated"));

  /* Set patient dob date */
  entry_widget = metadata_editor_get_widget (meta_info, "dob_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_patient_dob_date_button_clicked),
                    metadata_editor_get_widget (meta_info,
                                                "Xmp.DICOM.PatientDOB"));

  /* Set study date */
  entry_widget = metadata_editor_get_widget (meta_info, "study_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_study_date_button_clicked),
                    metadata_editor_get_widget (meta_info,
                                                "Xmp.DICOM.StudyDateTime"));

  /* Set series date */
  entry_widget = metadata_editor_get_widget (meta_info, "series_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_series_date_button_clicked),
                    metadata_editor_get_widget (meta_info,
                                                "Xmp.DICOM.SeriesDateTime"));
}


/* ============================================================================
 * ==[ WRITE METADATA ]========================================================
 * ============================================================================
 */

static void
set_tag_failed (const gchar *tag, gchar *error_message)
{
  g_log ("", G_LOG_LEVEL_MESSAGE,
         _("Failed to set metadata tag %s: %s"), tag, error_message);
}

static void
set_tag_string (GimpMetadata *metadata,
                const gchar  *name,
                const gchar  *value,
                gboolean      clear_tag)
{
  GError *error = NULL;

  if (! metadata || ! name) return;

  if (clear_tag)
    gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), name, NULL);

  if (! value) return;

  if (! gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                            name, value, &error))
    {
      set_tag_failed (name, error->message);
      g_clear_error (&error);
    }
}

static gchar *
get_phonetype (gchar *cur_value)
{
  gchar *phone_type_value = NULL;
  gint   types;

  if (cur_value != NULL)
    {
      for (types = 0; types < n_phone_types; types++)
        {
          if (! strcmp (cur_value, gettext (phone_types[types].display)))
            {
              phone_type_value = strdup (phone_types[types].data);
              break;
            }
        }
      g_free (cur_value);
    }
  if (! phone_type_value)
    phone_type_value = strdup (phone_types[0].data);
  cur_value = phone_type_value;

  return phone_type_value;
}

static void
write_metadata_tag (metadata_editor *meta_info,
                    GimpMetadata    *metadata,
                    gchar           *tag,
                    gint             data_column)
{
  GtkWidget     *list_widget;
  GtkTreeModel  *treemodel;
  gint           row;
  gint           number_of_rows;
  gchar         *rc_data;
  GString       *data;

  list_widget = metadata_editor_get_widget (meta_info, tag);
  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  if (number_of_rows <= 0)
    return;

  data = g_string_sized_new (256);

  for (row = 0; row < number_of_rows; row++)
    {
      GtkTreeIter iter;

      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gtk_tree_model_get (treemodel, &iter,
                              data_column, &rc_data,
                              -1);
          if (rc_data && rc_data[0] != '\0')
            {
              if (row > 0)
                g_string_append (data, ", ");

              g_string_append (data, rc_data);
            }
          g_free (rc_data);
        }
    }

  g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "write_metadata_tag tag: %s, value: %s",
         tag, data->str);

  set_tag_string (metadata, tag, data->str, TRUE);
  g_string_free (data, TRUE);
}

static void
write_metadata_tag_multiple (metadata_editor *meta_info, GimpMetadata *metadata,
                             GExiv2StructureType type, const gchar * header_tag,
                             gint n_columns, const gchar **column_tags,
                             const gint special_handling[])
{
  GtkWidget     *list_widget;
  GtkTreeModel  *treemodel;
  gint           row;
  gint           number_of_rows;
  gint           counter;
  gchar          temp_tag[1024];

  /* Clear old tag data first */
  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), header_tag, NULL);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_columns; item++)
        {
          g_snprintf (temp_tag, sizeof (temp_tag), "%s[%d]%s",
                      header_tag, row, locationshown[item]);
          gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), temp_tag, NULL);
        }
    }

  list_widget = metadata_editor_get_widget (meta_info, header_tag);
  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  if (number_of_rows <= 0)
    return;

  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (metadata),
                                          header_tag,
                                          GEXIV2_STRUCTURE_XA_BAG,
                                          NULL);

  /* We need a separate counter because an empty row will not be written */
  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      GtkTreeIter iter;

      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gint col;

          for (col = 0; col < n_columns; col++)
            {
              gchar *tag_data;

              gtk_tree_model_get (treemodel, &iter,
                                  col, &tag_data,
                                  -1);

              g_snprintf (temp_tag, sizeof (temp_tag), "%s[%d]%s",
                          header_tag, counter, column_tags[col]);

              if (special_handling)
                switch(special_handling[col])
                  {
                  case METADATA_PHONETYPE:
                    tag_data = get_phonetype (tag_data);
                    break;
                  }

              g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                     "write_metadata_tag_multiple tag: %s, value: %s, col: %d",
                     temp_tag, tag_data, col);

              set_tag_string (metadata, temp_tag, tag_data, TRUE);
              g_free (tag_data);
            }
          counter++;
        }
    }
}

static void
set_gps_longitude_latitude (GimpMetadata *metadata,
                            const gchar  *tag,
                            const gchar  *value)
{
  /* \u00b0 - degree symbol */
  const gchar  delimiters_dms[] = " deg'\":\u00b0";
  gchar        lng_lat[256];
  gchar       *s    = g_strdup (value);
  gchar       *str1 = NULL;
  gchar       *str2 = NULL;
  gchar       *str3 = NULL;
  gdouble      val  = 0.f;
  gint         degrees, minutes;
  gdouble      seconds;
  gboolean     remove_val = FALSE;

  g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "set_gps_longitude_latitude - Tag %s, Input value: %s", tag, value);

  if (s && s[0] != '\0')
    {
      str1 = strtok (s, delimiters_dms);
      str2 = strtok (NULL, delimiters_dms);
      str3 = strtok (NULL, delimiters_dms);

      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "String split into: %s - %s - %s", str1, str2, str3);
    }

  g_free (s);

  if (str1 && str2 && str3)
    {
      /* Assuming degrees, minutes, seconds */
      degrees = g_ascii_strtoll (str1, NULL, 10);
      minutes = g_ascii_strtoll (str2, NULL, 10);
      seconds = g_ascii_strtod (str3, NULL);
    }
  else if (str1 && str2)
    {
      /* Assuming degrees, minutes */
      gdouble min;

      degrees = g_ascii_strtoll (str1, NULL, 10);
      min = g_ascii_strtod (str2, NULL);
      minutes = (gint) min;
      seconds = (min - (gdouble) minutes) * 60.f;
    }
  else if (str1)
    {
      /* Assuming degrees only */
      val = g_ascii_strtod (str1, NULL);
      degrees = (gint) val;
      minutes = (gint) ((val - (gdouble) degrees) * 60.f);
      seconds = ((val - (gdouble) degrees - (gdouble) (minutes / 60.f)) * 60.f * 60.f);
    }
  else
    remove_val = TRUE;

  if (!remove_val)
    {
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Converted values: %d - %d - %f", degrees, minutes, seconds);
      g_snprintf (lng_lat, sizeof (lng_lat),
                  "%d/1 %d/1 %d/1000",
                  abs (degrees), abs (minutes), abs ((gint) (seconds * 1000.f)));
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Tag: %s, output string: %s", tag, lng_lat);

      set_tag_string (metadata, tag, lng_lat, FALSE);
    }
  else
    {
      gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (metadata), tag, NULL);
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Removed tag %s (no value).", tag);
    }
}

void
metadata_editor_write_callback (GtkWidget       *dialog,
                                metadata_editor *meta_info,
                                GimpImage       *image)
{
  GimpMetadata  *g_metadata;
  gint           max_elements;
  gint           i = 0;

  g_metadata = gimp_image_get_metadata (image);

  gimp_metadata_add_xmp_history (g_metadata, "metadata");

  write_metadata_tag (meta_info, g_metadata,
                      "Xmp.iptcExt.OrganisationInImageName",
                      COL_ORG_IMG_NAME);

  write_metadata_tag (meta_info, g_metadata,
                      "Xmp.iptcExt.OrganisationInImageCode",
                      COL_ORG_IMG_CODE);

  write_metadata_tag (meta_info, g_metadata,
                      "Xmp.plus.ModelReleaseID",
                      COL_MOD_REL_ID);

  write_metadata_tag (meta_info, g_metadata,
                      "Xmp.plus.PropertyReleaseID",
                      COL_PROP_REL_ID);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_BAG,
                               "Xmp.iptcExt.LocationShown",
                               n_locationshown, locationshown_alternative, NULL);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_BAG,
                               "Xmp.iptcExt.ArtworkOrObject",
                               n_artworkorobject, artworkorobject_alternative, NULL);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_BAG,
                               "Xmp.iptcExt.RegistryId",
                               n_registryid, registryid_alternative, NULL);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_SEQ,
                               "Xmp.plus.ImageCreator",
                               n_imagecreator, imagecreator, NULL);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_SEQ,
                               "Xmp.plus.CopyrightOwner",
                               n_copyrightowner, copyrightowner, NULL);

  write_metadata_tag_multiple (meta_info, g_metadata, GEXIV2_STRUCTURE_XA_SEQ,
                               "Xmp.plus.Licensor",
                               n_licensor, licensor,
                               licensor_special_handling);

  /* DO CREATOR TAGS */

  if (hasCreatorTagData (meta_info))
    {
      for (i = 0; i < n_creatorContactInfoTags; i++)
        {
          GtkWidget *widget = metadata_editor_get_widget (meta_info,
                                                          creatorContactInfoTags[i].id);

          if (creatorContactInfoTags[i].mode == MODE_SINGLE)
            {
              GtkEntry *entry = GTK_ENTRY (widget);

              set_tag_string (g_metadata, creatorContactInfoTags[i].tag,
                              gtk_entry_get_text (entry), FALSE);
            }
          else if (creatorContactInfoTags[i].mode == MODE_MULTI)
            {
              GtkTextView   *text_view = GTK_TEXT_VIEW (widget);
              GtkTextBuffer *buffer;
              GtkTextIter    start;
              GtkTextIter    end;
              gchar         *text;

              buffer = gtk_text_view_get_buffer (text_view);
              gtk_text_buffer_get_start_iter (buffer, &start);
              gtk_text_buffer_get_end_iter (buffer, &end);

              text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

              set_tag_string (g_metadata, creatorContactInfoTags[i].tag,
                              text, FALSE);

              g_free (text);
            }
        }
    }

  /* DO SINGLE, MULTI AND COMBO TAGS */

  else
    {
      for (i = 0; i < n_creatorContactInfoTags; i++)
        {
          gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                         creatorContactInfoTags[i].tag,
                                         NULL);
        }
    }

  max_elements = n_default_metadata_tags;

  for (i = 0; i < max_elements; i++)
    {
      GtkWidget *widget = metadata_editor_get_widget (meta_info,
                                                      default_metadata_tags[i].tag);

      /* SINGLE TAGS */

      if (default_metadata_tags[i].mode == MODE_SINGLE)
        {
          GtkEntry *entry       = GTK_ENTRY (widget);
          gchar    *value_entry = g_strdup (gtk_entry_get_text (entry));

          if (! strcmp ("Exif.GPSInfo.GPSLongitude",
                        default_metadata_tags[i].tag) ||
              ! strcmp ("Exif.GPSInfo.GPSLatitude",
                        default_metadata_tags[i].tag))
            {
              set_gps_longitude_latitude (g_metadata,
                                          default_metadata_tags[i].tag,
                                          value_entry);
            }
          else if (! strcmp ("Exif.GPSInfo.GPSAltitude",
                             default_metadata_tags[i].tag))
            {
              GtkWidget *combo_widget;
              gchar      alt_str[256];
              gdouble    alt_d;
              gint       msr;

              combo_widget = metadata_editor_get_widget (meta_info,
                                                         "GPSAltitudeSystem");
              msr = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_widget));

              alt_d = atof (gtk_entry_get_text (entry));
              if (msr == 1)
                alt_d = (alt_d * 12 * 2.54);
              else
                alt_d *= 100.f;

              g_snprintf (alt_str, sizeof (alt_str), "%d/100", (gint) alt_d);

              set_tag_string (g_metadata, default_metadata_tags[i].tag,
                              alt_str, FALSE);
            }
          else
            {
              gint         index;
              const gchar *text_value = gtk_entry_get_text (entry);

              if (default_metadata_tags[i].xmp_type == GIMP_XMP_TEXT ||
                  default_metadata_tags[i].xmp_type == GIMP_XMP_NONE)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                          default_metadata_tags[i].tag,
                                                          GEXIV2_STRUCTURE_XA_NONE,
                                                          NULL);
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  text_value, FALSE);
                }
              else
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  text_value, FALSE);
                }

              index = default_metadata_tags[i].other_tag_index;
              if (index == SPECIAL_PROCESSING_DATE_CREATED)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 "Iptc.Application2.DateCreated",
                                                 NULL);
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 "Iptc.Application2.TimeCreated",
                                                 NULL);
                  if (text_value)
                    {
                      gchar **date_time_split = NULL;

                      date_time_split = g_strsplit (text_value, "T", 2);
                      if (date_time_split[0] != NULL)
                        {
                          /* We can't use : as date delimiter for IPTC */
                          g_strdelimit (date_time_split[0], ":/.", '-');

                          set_tag_string (g_metadata, "Iptc.Application2.DateCreated",
                                          date_time_split[0], FALSE);

                          if (date_time_split[1] != NULL)
                            {
                              set_tag_string (g_metadata, "Iptc.Application2.TimeCreated",
                                              date_time_split[1], FALSE);
                            }
                          else
                            {
                              set_tag_string (g_metadata, "Iptc.Application2.TimeCreated",
                                              "00:00:00+00:00", FALSE);
                            }
                        }
                      g_strfreev (date_time_split);
                    }
                }
              else if (index > -1)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 equivalent_metadata_tags[index].tag,
                                                 NULL);
                  if (*text_value)
                    set_tag_string (g_metadata, equivalent_metadata_tags[index].tag,
                                    text_value, FALSE);

                  if (equivalent_metadata_tags[index].exif_tag_index > -1)
                    {
                      gint         i_exif       = equivalent_metadata_tags[index].exif_tag_index;
                      const gchar *exif_tag_str = exif_equivalent_tags[i_exif].tag;

                      gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                     exif_tag_str,
                                                     NULL);
                      if (*text_value)
                        set_tag_string (g_metadata, exif_tag_str,
                                        text_value, FALSE);
                    }
                }
            }
        }

      /* MULTI TAGS */

      else if (default_metadata_tags[i].mode == MODE_MULTI)
        {
          GtkTextView   *text_view = GTK_TEXT_VIEW (widget);
          GtkTextBuffer *buffer;
          GtkTextIter    start;
          GtkTextIter    end;
          gchar         *text;
          gint           index;
          GError    *error = NULL;

          buffer = gtk_text_view_get_buffer (text_view);
          gtk_text_buffer_get_start_iter (buffer, &start);
          gtk_text_buffer_get_end_iter (buffer, &end);

          text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

          gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                         default_metadata_tags[i].tag,
                                         NULL);

          if (text && *text)
            {
              if (default_metadata_tags[i].xmp_type == GIMP_XMP_TEXT ||
                  default_metadata_tags[i].xmp_type == GIMP_XMP_NONE)
                {
                  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                          default_metadata_tags[i].tag,
                                                          GEXIV2_STRUCTURE_XA_NONE,
                                                          NULL);
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  text, FALSE);
                }
              else
                {
                  gchar **multi;

                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);

                  /* We have one value per line. */
                  multi = g_strsplit (text, "\n", 0);

                  if (! gexiv2_metadata_try_set_tag_multiple (GEXIV2_METADATA (g_metadata),
                                                              default_metadata_tags[i].tag,
                                                              (const gchar **) multi,
                                                              &error))
                    {
                      set_tag_failed (default_metadata_tags[i].tag, error->message);
                      g_clear_error (&error);
                    }
                  g_strfreev (multi);
                }
            }

            index = default_metadata_tags[i].other_tag_index;
            if (index > -1)
              {
                gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                               equivalent_metadata_tags[index].tag,
                                               NULL);

                if (text && *text)
                  {
                    if (equivalent_metadata_tags[index].mode == MODE_MULTI)
                      {
                        gchar **multi;

                        multi = g_strsplit (text, "\n", 0);

                        if (! gexiv2_metadata_try_set_tag_multiple (GEXIV2_METADATA (g_metadata),
                                                                    equivalent_metadata_tags[index].tag,
                                                                    (const gchar **) multi,
                                                                    &error))
                          {
                            set_tag_failed (equivalent_metadata_tags[index].tag, error->message);
                            g_clear_error (&error);
                          }

                        g_strfreev (multi);
                      }
                    else if (equivalent_metadata_tags[index].mode == MODE_SINGLE)
                      {
                        /* Convert from multiline to single line: keep the \n and just add the whole text. */
                        set_tag_string (g_metadata, equivalent_metadata_tags[index].tag,
                                        text, FALSE);
                      }
                    else
                      {
                        g_warning ("Copying from multiline tag %s to tag %s not implemented!",
                                   default_metadata_tags[i].tag,
                                   equivalent_metadata_tags[index].tag);
                      }
                  }

                if (equivalent_metadata_tags[index].exif_tag_index > -1)
                  {
                    gint         i_exif       = equivalent_metadata_tags[index].exif_tag_index;
                    const gchar *exif_tag_str = exif_equivalent_tags[i_exif].tag;

                    gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                   exif_tag_str,
                                                   NULL);
                    if (text && *text)
                      set_tag_string (g_metadata, exif_tag_str, text, FALSE);
                  }
              }

          if (text)
            g_free (text);
        }
      else if (default_metadata_tags[i].mode == MODE_LIST)
        {
          /* MIGHT DO SOMETHING HERE */
        }

      /* COMBO TAGS */

      else if (default_metadata_tags[i].mode == MODE_COMBO)
        {
          GtkComboBoxText *combo;
          gint32           value;

          combo = GTK_COMBO_BOX_TEXT (widget);
          value = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

          if (! strcmp ("Xmp.photoshop.Urgency", default_metadata_tags[i].tag))
            {
              /* IPTC tab - Urgency */
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 "Iptc.Application2.Urgency",
                                                 NULL);
                }
              else
                {
                  gchar *save;

                  save = g_strdup_printf ("%d", value);

                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  save, FALSE);
                  set_tag_string (g_metadata, "Iptc.Application2.Urgency",
                                  save, FALSE);
                  g_free (save);
                }
            }
          else if (! strcmp ("Xmp.xmpRights.Marked",
                             default_metadata_tags[i].tag))
            {
              /* Description tab - Copyright Status */
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  gchar *save_value;

                  if (value == 1)
                    save_value = g_strdup_printf ("%s", "True");
                  else /* (value == 2) */
                    save_value = g_strdup_printf ("%s", "False");

                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  save_value, FALSE);
                  g_free (save_value);
                }
            }
          else if (! strcmp ("Xmp.xmp.Rating", default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  gchar *save;

                  save = g_strdup_printf ("%d", value);

                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  save, FALSE);
                  g_free (save);
                }
            }
          else if (! strcmp ("Xmp.DICOM.PatientSex",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  break;

                case 1:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "male", FALSE);
                  break;

                case 2:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "female", FALSE);
                  break;

                case 3:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "other", FALSE);
                   break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSLongitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  break;

                case 1:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "E", FALSE);
                  break;

                case 2:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "W", FALSE);
                  break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSLatitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  break;

                case 1:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "N", FALSE);
                  break;

                case 2:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "S", FALSE);
                  break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSAltitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                  break;

                case 1:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "0", FALSE);
                  break;

                case 2:
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  "1", FALSE);
                  break;
                }
            }
          else if (! strcmp ("Xmp.plus.ModelReleaseStatus",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  modelreleasestatus[value].data, FALSE);
                }
            }
          else if (! strcmp ("Xmp.plus.PropertyReleaseStatus",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  propertyreleasestatus[value].data, FALSE);
                }
            }
          else if (! strcmp ("Xmp.plus.MinorModelAgeDisclosure",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  minormodelagedisclosure[value].data, FALSE);
                }
            }
          else if (! strcmp ("Xmp.iptcExt.DigitalSourceType",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_try_clear_tag (GEXIV2_METADATA (g_metadata),
                                                 default_metadata_tags[i].tag,
                                                 NULL);
                }
              else
                {
                  set_tag_string (g_metadata, default_metadata_tags[i].tag,
                                  digitalsourcetype[value].data, FALSE);
                }
            }
        }
    }

  gimp_image_set_metadata (image, g_metadata);
}

/* ============================================================================
 * ==[ METADATA IMPORT / EXPORT FILE DIALOG UI ]===============================
 * ============================================================================
 */

static void
import_dialog_metadata (metadata_editor *args)
{
  GtkWidget *file_dialog;
  gchar     *filename;

  file_dialog = gtk_file_chooser_dialog_new (_("Import Metadata File"),
                                             NULL,
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Import"), GTK_RESPONSE_ACCEPT,
                                             NULL);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_dialog),
                                 args->filename);

  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));

      if (filename)
        {
          if (args->filename)
            {
              g_free (args->filename);
            }

          args->filename = g_strdup (filename);
          import_file_metadata (args);
        }
    }

  gtk_widget_destroy (file_dialog);
}

static void
export_dialog_metadata (metadata_editor *args)
{
  GtkWidget *file_dialog;
  gchar     *filename;

  file_dialog = gtk_file_chooser_dialog_new (_("Export Metadata File"),
                                             NULL,
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Export"), GTK_RESPONSE_ACCEPT,
                                             NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_dialog),
                                                  TRUE);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_dialog),
                                 args->filename);

  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));

      if (filename)
        {
          if (args->filename)
            {
              g_free (args->filename);
            }

          args->filename = g_strdup (filename);
          export_file_metadata (args);
        }
    }

  gtk_widget_destroy (file_dialog);
}

static void
impex_combo_callback (GtkComboBoxText *combo,
                      gpointer         data)
{
  metadata_editor *args;
  gint32           selection;

  args = data;
  selection = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  switch (selection)
    {
    case 1: /* Import */
      import_dialog_metadata (args);
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      break;

    case 2: /* Export */
      export_dialog_metadata (args);
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      break;
    }
}


static void
gpsaltsys_combo_callback (GtkComboBoxText *combo,
                          gpointer         data)
{
  metadata_editor *meta_info = data;
  GtkWidget       *entry;
  gint32           selection;
  gchar            alt_str[256];
  double           alt_d;

  selection = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  entry = metadata_editor_get_widget (meta_info, "Exif.GPSInfo.GPSAltitude");

  switch (selection)
    {
    case 0: /* Meters */
      if (last_gpsaltsys_sel != 0)
        {
          alt_d = atof (gtk_entry_get_text (GTK_ENTRY (entry)));
          alt_d = (alt_d * (12 * 2.54)) / 100;

          g_snprintf (alt_str, sizeof (alt_str), "%.2f", (float)alt_d);

          gtk_entry_set_text (GTK_ENTRY (entry), alt_str);
        }
      break;

    case 1: /* Feet */
      if (last_gpsaltsys_sel != 1)
        {
          alt_d = atof (gtk_entry_get_text (GTK_ENTRY (entry)));
          alt_d = alt_d * 3.28;

          g_snprintf (alt_str, sizeof (alt_str), "%.2f", (float)alt_d);

          gtk_entry_set_text (GTK_ENTRY (entry), alt_str);
        }
      break;
    }

  last_gpsaltsys_sel = selection;
}
