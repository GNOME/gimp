/* LIGMA - The GNU Image Manipulation Program
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

#include "ligmapdb.h"
#include "ligma-pdb-compat.h"


/*  public functions  */

void
ligma_pdb_compat_procs_register (LigmaPDB           *pdb,
                                LigmaPDBCompatMode  compat_mode)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    { "ligma-blend",                         "ligma-edit-blend"                 },
    { "ligma-brushes-list",                  "ligma-brushes-get-list"           },
    { "ligma-bucket-fill",                   "ligma-edit-bucket-fill"           },
    { "ligma-channel-delete",                "ligma-item-delete"                },
    { "ligma-channel-get-name",              "ligma-item-get-name"              },
    { "ligma-channel-get-tattoo",            "ligma-item-get-tattoo"            },
    { "ligma-channel-get-visible",           "ligma-item-get-visible"           },
    { "ligma-channel-set-name",              "ligma-item-set-name"              },
    { "ligma-channel-set-tattoo",            "ligma-item-set-tattoo"            },
    { "ligma-channel-set-visible",           "ligma-item-set-visible"           },
    { "ligma-color-picker",                  "ligma-image-pick-color"           },
    { "ligma-convert-grayscale",             "ligma-image-convert-grayscale"    },
    { "ligma-convert-indexed",               "ligma-image-convert-indexed"      },
    { "ligma-convert-rgb",                   "ligma-image-convert-rgb"          },
    { "ligma-crop",                          "ligma-image-crop"                 },
    { "ligma-drawable-bytes",                "ligma-drawable-bpp"               },
    { "ligma-drawable-image",                "ligma-drawable-get-image"         },
    { "ligma-image-floating-selection",      "ligma-image-get-floating-sel"     },
    { "ligma-layer-delete",                  "ligma-item-delete"                },
    { "ligma-layer-get-name",                "ligma-item-get-name"              },
    { "ligma-layer-get-tattoo",              "ligma-item-get-tattoo"            },
    { "ligma-layer-get-visible",             "ligma-item-get-visible"           },
    { "ligma-layer-mask",                    "ligma-layer-get-mask"             },
    { "ligma-layer-set-name",                "ligma-item-set-name"              },
    { "ligma-layer-set-tattoo",              "ligma-item-set-tattoo"            },
    { "ligma-layer-set-visible",             "ligma-item-set-visible"           },
    { "ligma-palette-refresh",               "ligma-palettes-refresh"           },
    { "ligma-patterns-list",                 "ligma-patterns-get-list"          },
    { "ligma-temp-PDB-name",                 "ligma-procedural-db-temp-name"    },
    { "ligma-undo-push-group-end",           "ligma-image-undo-group-end"       },
    { "ligma-undo-push-group-start",         "ligma-image-undo-group-start"     },

    /*  deprecations since 2.0  */
    { "ligma-brushes-get-opacity",           "ligma-context-get-opacity"        },
    { "ligma-brushes-get-paint-mode",        "ligma-context-get-paint-mode"     },
    { "ligma-brushes-set-brush",             "ligma-context-set-brush"          },
    { "ligma-brushes-set-opacity",           "ligma-context-set-opacity"        },
    { "ligma-brushes-set-paint-mode",        "ligma-context-set-paint-mode"     },
    { "ligma-channel-ops-duplicate",         "ligma-image-duplicate"            },
    { "ligma-channel-ops-offset",            "ligma-drawable-offset"            },
    { "ligma-gradients-get-active",          "ligma-context-get-gradient"       },
    { "ligma-gradients-get-gradient",        "ligma-context-get-gradient"       },
    { "ligma-gradients-set-active",          "ligma-context-set-gradient"       },
    { "ligma-gradients-set-gradient",        "ligma-context-set-gradient"       },
    { "ligma-image-get-cmap",                "ligma-image-get-colormap"         },
    { "ligma-image-set-cmap",                "ligma-image-set-colormap"         },
    { "ligma-palette-get-background",        "ligma-context-get-background"     },
    { "ligma-palette-get-foreground",        "ligma-context-get-foreground"     },
    { "ligma-palette-set-background",        "ligma-context-set-background"     },
    { "ligma-palette-set-default-colors",    "ligma-context-set-default-colors" },
    { "ligma-palette-set-foreground",        "ligma-context-set-foreground"     },
    { "ligma-palette-swap-colors",           "ligma-context-swap-colors"        },
    { "ligma-palettes-set-palette",          "ligma-context-set-palette"        },
    { "ligma-patterns-set-pattern",          "ligma-context-set-pattern"        },
    { "ligma-selection-clear",               "ligma-selection-none"             },

    /*  deprecations since 2.2  */
    { "ligma-layer-get-preserve-trans",      "ligma-layer-get-lock-alpha"       },
    { "ligma-layer-set-preserve-trans",      "ligma-layer-set-lock-alpha"       },

    /*  deprecations since 2.6  */
    { "ligma-drawable-is-valid",             "ligma-item-is-valid"              },
    { "ligma-drawable-is-layer",             "ligma-item-is-layer"              },
    { "ligma-drawable-is-text-layer",        "ligma-item-is-text-layer"         },
    { "ligma-drawable-is-layer-mask",        "ligma-item-is-layer-mask"         },
    { "ligma-drawable-is-channel",           "ligma-item-is-channel"            },
    { "ligma-drawable-delete",               "ligma-item-delete"                },
    { "ligma-drawable-get-image",            "ligma-item-get-image"             },
    { "ligma-drawable-get-name",             "ligma-item-get-name"              },
    { "ligma-drawable-set-name",             "ligma-item-set-name"              },
    { "ligma-drawable-get-visible",          "ligma-item-get-visible"           },
    { "ligma-drawable-set-visible",          "ligma-item-set-visible"           },
    { "ligma-drawable-get-tattoo",           "ligma-item-get-tattoo"            },
    { "ligma-drawable-set-tattoo",           "ligma-item-set-tattoo"            },
    { "ligma-drawable-parasite-find",        "ligma-item-get-parasite"          },
    { "ligma-drawable-parasite-attach",      "ligma-item-attach-parasite"       },
    { "ligma-drawable-parasite-detach",      "ligma-item-detach-parasite"       },
    { "ligma-drawable-parasite-list",        "ligma-item-get-parasite-list"     },
    { "ligma-image-get-layer-position",      "ligma-image-get-item-position"    },
    { "ligma-image-raise-layer",             "ligma-image-raise-item"           },
    { "ligma-image-lower-layer",             "ligma-image-lower-item"           },
    { "ligma-image-raise-layer-to-top",      "ligma-image-raise-item-to-top"    },
    { "ligma-image-lower-layer-to-bottom",   "ligma-image-lower-item-to-bottom" },
    { "ligma-image-get-channel-position",    "ligma-image-get-item-position"    },
    { "ligma-image-raise-channel",           "ligma-image-raise-item"           },
    { "ligma-image-lower-channel",           "ligma-image-lower-item"           },
    { "ligma-image-get-vectors-position",    "ligma-image-get-item-position"    },
    { "ligma-image-raise-vectors",           "ligma-image-raise-item"           },
    { "ligma-image-lower-vectors",           "ligma-image-lower-item"           },
    { "ligma-image-raise-vectors-to-top",    "ligma-image-raise-item-to-top"    },
    { "ligma-image-lower-vectors-to-bottom", "ligma-image-lower-item-to-bottom" },
    { "ligma-vectors-is-valid",              "ligma-item-is-valid"              },
    { "ligma-vectors-get-image",             "ligma-item-get-image"             },
    { "ligma-vectors-get-name",              "ligma-item-get-name"              },
    { "ligma-vectors-set-name",              "ligma-item-set-name"              },
    { "ligma-vectors-get-visible",           "ligma-item-get-visible"           },
    { "ligma-vectors-set-visible",           "ligma-item-set-visible"           },
    { "ligma-vectors-get-tattoo",            "ligma-item-get-tattoo"            },
    { "ligma-vectors-set-tattoo",            "ligma-item-set-tattoo"            },
    { "ligma-vectors-parasite-find",         "ligma-item-get-parasite"          },
    { "ligma-vectors-parasite-attach",       "ligma-item-attach-parasite"       },
    { "ligma-vectors-parasite-detach",       "ligma-item-detach-parasite"       },
    { "ligma-vectors-parasite-list",         "ligma-item-get-parasite-list"     },
    { "ligma-image-parasite-find",           "ligma-image-get-parasite"         },
    { "ligma-image-parasite-attach",         "ligma-image-attach-parasite"      },
    { "ligma-image-parasite-detach",         "ligma-image-detach-parasite"      },
    { "ligma-image-parasite-list",           "ligma-image-get-parasite-list"    },
    { "ligma-parasite-find",                 "ligma-get-parasite"               },
    { "ligma-parasite-attach",               "ligma-attach-parasite"            },
    { "ligma-parasite-detach",               "ligma-detach-parasite"            },
    { "ligma-parasite-list",                 "ligma-get-parasite-list"          },

    /*  deprecations since 2.8  */
    { "ligma-edit-paste-as-new",             "ligma-edit-paste-as-new-image"    },
    { "ligma-edit-named-paste-as-new", "ligma-edit-named-paste-as-new-image"    }
 };

  g_return_if_fail (LIGMA_IS_PDB (pdb));

  if (compat_mode != LIGMA_PDB_COMPAT_OFF)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
        ligma_pdb_register_compat_proc_name (pdb,
                                            compat_procs[i].old_name,
                                            compat_procs[i].new_name);
    }
}
