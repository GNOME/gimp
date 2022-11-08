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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pdb-types.h"

#include "gimppdb.h"
#include "gimp-pdb-compat.h"


/*  public functions  */

void
gimp_pdb_compat_procs_register (GimpPDB           *pdb,
                                GimpPDBCompatMode  compat_mode)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    { "gimp-blend",                         "gimp-edit-blend"                 },
    { "gimp-brushes-list",                  "gimp-brushes-get-list"           },
    { "gimp-bucket-fill",                   "gimp-edit-bucket-fill"           },
    { "gimp-channel-delete",                "gimp-item-delete"                },
    { "gimp-channel-get-name",              "gimp-item-get-name"              },
    { "gimp-channel-get-tattoo",            "gimp-item-get-tattoo"            },
    { "gimp-channel-get-visible",           "gimp-item-get-visible"           },
    { "gimp-channel-set-name",              "gimp-item-set-name"              },
    { "gimp-channel-set-tattoo",            "gimp-item-set-tattoo"            },
    { "gimp-channel-set-visible",           "gimp-item-set-visible"           },
    { "gimp-color-picker",                  "gimp-image-pick-color"           },
    { "gimp-convert-grayscale",             "gimp-image-convert-grayscale"    },
    { "gimp-convert-indexed",               "gimp-image-convert-indexed"      },
    { "gimp-convert-rgb",                   "gimp-image-convert-rgb"          },
    { "gimp-crop",                          "gimp-image-crop"                 },
    { "gimp-drawable-bytes",                "gimp-drawable-bpp"               },
    { "gimp-drawable-image",                "gimp-drawable-get-image"         },
    { "gimp-image-floating-selection",      "gimp-image-get-floating-sel"     },
    { "gimp-layer-delete",                  "gimp-item-delete"                },
    { "gimp-layer-get-name",                "gimp-item-get-name"              },
    { "gimp-layer-get-tattoo",              "gimp-item-get-tattoo"            },
    { "gimp-layer-get-visible",             "gimp-item-get-visible"           },
    { "gimp-layer-mask",                    "gimp-layer-get-mask"             },
    { "gimp-layer-set-name",                "gimp-item-set-name"              },
    { "gimp-layer-set-tattoo",              "gimp-item-set-tattoo"            },
    { "gimp-layer-set-visible",             "gimp-item-set-visible"           },
    { "gimp-palette-refresh",               "gimp-palettes-refresh"           },
    { "gimp-patterns-list",                 "gimp-patterns-get-list"          },
    { "gimp-temp-PDB-name",                 "gimp-procedural-db-temp-name"    },
    { "gimp-undo-push-group-end",           "gimp-image-undo-group-end"       },
    { "gimp-undo-push-group-start",         "gimp-image-undo-group-start"     },

    /*  deprecations since 2.0  */
    { "gimp-brushes-get-opacity",           "gimp-context-get-opacity"        },
    { "gimp-brushes-get-paint-mode",        "gimp-context-get-paint-mode"     },
    { "gimp-brushes-set-brush",             "gimp-context-set-brush"          },
    { "gimp-brushes-set-opacity",           "gimp-context-set-opacity"        },
    { "gimp-brushes-set-paint-mode",        "gimp-context-set-paint-mode"     },
    { "gimp-channel-ops-duplicate",         "gimp-image-duplicate"            },
    { "gimp-channel-ops-offset",            "gimp-drawable-offset"            },
    { "gimp-gradients-get-active",          "gimp-context-get-gradient"       },
    { "gimp-gradients-get-gradient",        "gimp-context-get-gradient"       },
    { "gimp-gradients-set-active",          "gimp-context-set-gradient"       },
    { "gimp-gradients-set-gradient",        "gimp-context-set-gradient"       },
    { "gimp-image-get-cmap",                "gimp-image-get-colormap"         },
    { "gimp-image-set-cmap",                "gimp-image-set-colormap"         },
    { "gimp-palette-get-background",        "gimp-context-get-background"     },
    { "gimp-palette-get-foreground",        "gimp-context-get-foreground"     },
    { "gimp-palette-set-background",        "gimp-context-set-background"     },
    { "gimp-palette-set-default-colors",    "gimp-context-set-default-colors" },
    { "gimp-palette-set-foreground",        "gimp-context-set-foreground"     },
    { "gimp-palette-swap-colors",           "gimp-context-swap-colors"        },
    { "gimp-palettes-set-palette",          "gimp-context-set-palette"        },
    { "gimp-patterns-set-pattern",          "gimp-context-set-pattern"        },
    { "gimp-selection-clear",               "gimp-selection-none"             },

    /*  deprecations since 2.2  */
    { "gimp-layer-get-preserve-trans",      "gimp-layer-get-lock-alpha"       },
    { "gimp-layer-set-preserve-trans",      "gimp-layer-set-lock-alpha"       },

    /*  deprecations since 2.6  */
    { "gimp-drawable-is-valid",             "gimp-item-is-valid"              },
    { "gimp-drawable-is-layer",             "gimp-item-is-layer"              },
    { "gimp-drawable-is-text-layer",        "gimp-item-is-text-layer"         },
    { "gimp-drawable-is-layer-mask",        "gimp-item-is-layer-mask"         },
    { "gimp-drawable-is-channel",           "gimp-item-is-channel"            },
    { "gimp-drawable-delete",               "gimp-item-delete"                },
    { "gimp-drawable-get-image",            "gimp-item-get-image"             },
    { "gimp-drawable-get-name",             "gimp-item-get-name"              },
    { "gimp-drawable-set-name",             "gimp-item-set-name"              },
    { "gimp-drawable-get-visible",          "gimp-item-get-visible"           },
    { "gimp-drawable-set-visible",          "gimp-item-set-visible"           },
    { "gimp-drawable-get-tattoo",           "gimp-item-get-tattoo"            },
    { "gimp-drawable-set-tattoo",           "gimp-item-set-tattoo"            },
    { "gimp-drawable-parasite-find",        "gimp-item-get-parasite"          },
    { "gimp-drawable-parasite-attach",      "gimp-item-attach-parasite"       },
    { "gimp-drawable-parasite-detach",      "gimp-item-detach-parasite"       },
    { "gimp-drawable-parasite-list",        "gimp-item-get-parasite-list"     },
    { "gimp-image-get-layer-position",      "gimp-image-get-item-position"    },
    { "gimp-image-raise-layer",             "gimp-image-raise-item"           },
    { "gimp-image-lower-layer",             "gimp-image-lower-item"           },
    { "gimp-image-raise-layer-to-top",      "gimp-image-raise-item-to-top"    },
    { "gimp-image-lower-layer-to-bottom",   "gimp-image-lower-item-to-bottom" },
    { "gimp-image-get-channel-position",    "gimp-image-get-item-position"    },
    { "gimp-image-raise-channel",           "gimp-image-raise-item"           },
    { "gimp-image-lower-channel",           "gimp-image-lower-item"           },
    { "gimp-image-get-vectors-position",    "gimp-image-get-item-position"    },
    { "gimp-image-raise-vectors",           "gimp-image-raise-item"           },
    { "gimp-image-lower-vectors",           "gimp-image-lower-item"           },
    { "gimp-image-raise-vectors-to-top",    "gimp-image-raise-item-to-top"    },
    { "gimp-image-lower-vectors-to-bottom", "gimp-image-lower-item-to-bottom" },
    { "gimp-vectors-is-valid",              "gimp-item-is-valid"              },
    { "gimp-vectors-get-image",             "gimp-item-get-image"             },
    { "gimp-vectors-get-name",              "gimp-item-get-name"              },
    { "gimp-vectors-set-name",              "gimp-item-set-name"              },
    { "gimp-vectors-get-visible",           "gimp-item-get-visible"           },
    { "gimp-vectors-set-visible",           "gimp-item-set-visible"           },
    { "gimp-vectors-get-tattoo",            "gimp-item-get-tattoo"            },
    { "gimp-vectors-set-tattoo",            "gimp-item-set-tattoo"            },
    { "gimp-vectors-parasite-find",         "gimp-item-get-parasite"          },
    { "gimp-vectors-parasite-attach",       "gimp-item-attach-parasite"       },
    { "gimp-vectors-parasite-detach",       "gimp-item-detach-parasite"       },
    { "gimp-vectors-parasite-list",         "gimp-item-get-parasite-list"     },
    { "gimp-image-parasite-find",           "gimp-image-get-parasite"         },
    { "gimp-image-parasite-attach",         "gimp-image-attach-parasite"      },
    { "gimp-image-parasite-detach",         "gimp-image-detach-parasite"      },
    { "gimp-image-parasite-list",           "gimp-image-get-parasite-list"    },
    { "gimp-parasite-find",                 "gimp-get-parasite"               },
    { "gimp-parasite-attach",               "gimp-attach-parasite"            },
    { "gimp-parasite-detach",               "gimp-detach-parasite"            },
    { "gimp-parasite-list",                 "gimp-get-parasite-list"          },

    /*  deprecations since 2.8  */
    { "gimp-edit-paste-as-new",             "gimp-edit-paste-as-new-image"    },
    { "gimp-edit-named-paste-as-new", "gimp-edit-named-paste-as-new-image"    }
 };

  g_return_if_fail (GIMP_IS_PDB (pdb));

  if (compat_mode != GIMP_PDB_COMPAT_OFF)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
        gimp_pdb_register_compat_proc_name (pdb,
                                            compat_procs[i].old_name,
                                            compat_procs[i].new_name);
    }
}
