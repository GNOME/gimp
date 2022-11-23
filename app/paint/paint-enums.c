
/* Generated data (by ligma-mkenums) */

#include "stamp-paint-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libligmabase/ligmabase.h"
#include "paint-enums.h"
#include "ligma-intl.h"

/* enumerations from "paint-enums.h" */
GType
ligma_brush_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_BRUSH_HARD, "LIGMA_BRUSH_HARD", "hard" },
    { LIGMA_BRUSH_SOFT, "LIGMA_BRUSH_SOFT", "soft" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_BRUSH_HARD, "LIGMA_BRUSH_HARD", NULL },
    { LIGMA_BRUSH_SOFT, "LIGMA_BRUSH_SOFT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaBrushApplicationMode", values);
      ligma_type_set_translation_context (type, "brush-application-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_perspective_clone_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST, "LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST", "adjust" },
    { LIGMA_PERSPECTIVE_CLONE_MODE_PAINT, "LIGMA_PERSPECTIVE_CLONE_MODE_PAINT", "paint" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST, NC_("perspective-clone-mode", "Modify Perspective"), NULL },
    { LIGMA_PERSPECTIVE_CLONE_MODE_PAINT, NC_("perspective-clone-mode", "Perspective Clone"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaPerspectiveCloneMode", values);
      ligma_type_set_translation_context (type, "perspective-clone-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
ligma_source_align_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { LIGMA_SOURCE_ALIGN_NO, "LIGMA_SOURCE_ALIGN_NO", "no" },
    { LIGMA_SOURCE_ALIGN_YES, "LIGMA_SOURCE_ALIGN_YES", "yes" },
    { LIGMA_SOURCE_ALIGN_REGISTERED, "LIGMA_SOURCE_ALIGN_REGISTERED", "registered" },
    { LIGMA_SOURCE_ALIGN_FIXED, "LIGMA_SOURCE_ALIGN_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const LigmaEnumDesc descs[] =
  {
    { LIGMA_SOURCE_ALIGN_NO, NC_("source-align-mode", "None"), NULL },
    { LIGMA_SOURCE_ALIGN_YES, NC_("source-align-mode", "Aligned"), NULL },
    { LIGMA_SOURCE_ALIGN_REGISTERED, NC_("source-align-mode", "Registered"), NULL },
    { LIGMA_SOURCE_ALIGN_FIXED, NC_("source-align-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("LigmaSourceAlignMode", values);
      ligma_type_set_translation_context (type, "source-align-mode");
      ligma_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

