
/* Generated data (by ligma-mkenums) */

#include "stamp-plug-in-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "plug-in-enums.h"
#include "ligma-intl.h"

/* enumerations from "plug-in-enums.h" */
GType
ligma_plug_in_image_type_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { LIGMA_PLUG_IN_RGB_IMAGE, "LIGMA_PLUG_IN_RGB_IMAGE", "rgb-image" },
    { LIGMA_PLUG_IN_GRAY_IMAGE, "LIGMA_PLUG_IN_GRAY_IMAGE", "gray-image" },
    { LIGMA_PLUG_IN_INDEXED_IMAGE, "LIGMA_PLUG_IN_INDEXED_IMAGE", "indexed-image" },
    { LIGMA_PLUG_IN_RGBA_IMAGE, "LIGMA_PLUG_IN_RGBA_IMAGE", "rgba-image" },
    { LIGMA_PLUG_IN_GRAYA_IMAGE, "LIGMA_PLUG_IN_GRAYA_IMAGE", "graya-image" },
    { LIGMA_PLUG_IN_INDEXEDA_IMAGE, "LIGMA_PLUG_IN_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const LigmaFlagsDesc descs[] =
  {
    { LIGMA_PLUG_IN_RGB_IMAGE, "LIGMA_PLUG_IN_RGB_IMAGE", NULL },
    { LIGMA_PLUG_IN_GRAY_IMAGE, "LIGMA_PLUG_IN_GRAY_IMAGE", NULL },
    { LIGMA_PLUG_IN_INDEXED_IMAGE, "LIGMA_PLUG_IN_INDEXED_IMAGE", NULL },
    { LIGMA_PLUG_IN_RGBA_IMAGE, "LIGMA_PLUG_IN_RGBA_IMAGE", NULL },
    { LIGMA_PLUG_IN_GRAYA_IMAGE, "LIGMA_PLUG_IN_GRAYA_IMAGE", NULL },
    { LIGMA_PLUG_IN_INDEXEDA_IMAGE, "LIGMA_PLUG_IN_INDEXEDA_IMAGE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("LigmaPlugInImageType", values);
      ligma_type_set_translation_context (type, "plug-in-image-type");
      ligma_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_plug_in_call_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PLUG_IN_CALL_NONE, "LIGMA_PLUG_IN_CALL_NONE", "none" },
    { LIGMA_PLUG_IN_CALL_RUN, "LIGMA_PLUG_IN_CALL_RUN", "run" },
    { LIGMA_PLUG_IN_CALL_QUERY, "LIGMA_PLUG_IN_CALL_QUERY", "query" },
    { LIGMA_PLUG_IN_CALL_INIT, "LIGMA_PLUG_IN_CALL_INIT", "init" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PLUG_IN_CALL_NONE, "LIGMA_PLUG_IN_CALL_NONE", NULL },
    { LIGMA_PLUG_IN_CALL_RUN, "LIGMA_PLUG_IN_CALL_RUN", NULL },
    { LIGMA_PLUG_IN_CALL_QUERY, "LIGMA_PLUG_IN_CALL_QUERY", NULL },
    { LIGMA_PLUG_IN_CALL_INIT, "LIGMA_PLUG_IN_CALL_INIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPlugInCallMode", values);
      ligma_type_set_translation_context (type, "plug-in-call-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_file_procedure_group_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_FILE_PROCEDURE_GROUP_NONE, "LIGMA_FILE_PROCEDURE_GROUP_NONE", "none" },
    { LIGMA_FILE_PROCEDURE_GROUP_ANY, "LIGMA_FILE_PROCEDURE_GROUP_ANY", "any" },
    { LIGMA_FILE_PROCEDURE_GROUP_OPEN, "LIGMA_FILE_PROCEDURE_GROUP_OPEN", "open" },
    { LIGMA_FILE_PROCEDURE_GROUP_SAVE, "LIGMA_FILE_PROCEDURE_GROUP_SAVE", "save" },
    { LIGMA_FILE_PROCEDURE_GROUP_EXPORT, "LIGMA_FILE_PROCEDURE_GROUP_EXPORT", "export" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_FILE_PROCEDURE_GROUP_NONE, "LIGMA_FILE_PROCEDURE_GROUP_NONE", NULL },
    { LIGMA_FILE_PROCEDURE_GROUP_ANY, "LIGMA_FILE_PROCEDURE_GROUP_ANY", NULL },
    { LIGMA_FILE_PROCEDURE_GROUP_OPEN, "LIGMA_FILE_PROCEDURE_GROUP_OPEN", NULL },
    { LIGMA_FILE_PROCEDURE_GROUP_SAVE, "LIGMA_FILE_PROCEDURE_GROUP_SAVE", NULL },
    { LIGMA_FILE_PROCEDURE_GROUP_EXPORT, "LIGMA_FILE_PROCEDURE_GROUP_EXPORT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaFileProcedureGroup", values);
      ligma_type_set_translation_context (type, "file-procedure-group");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

