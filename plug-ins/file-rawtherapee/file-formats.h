/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-rawtherapee.c -- raw file format plug-in that uses rawtherapee
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
 * Copyright (C) 2017 Alberto Griggio <alberto.griggio@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* These are the raw formats that file-rawtherapee will register */

typedef struct _FileFormat FileFormat;

struct _FileFormat
{
  const gchar *file_type;
  const gchar *mime_type;
  const gchar *extensions;
  const gchar *magic;

  const gchar *load_proc;
  const gchar *load_blurb;
  const gchar *load_help;
};

#define N_(s) s
#define _(s) s

static const FileFormat file_formats[] =
{
  {
    N_("Raw Canon"),
    "image/x-canon-cr2,image/x-canon-crw",
    "cr2,crw",
    NULL,

    "file-rawtherapee-canon-load",
    "Load files in the Canon raw formats via rawtherapee",
    "This plug-in loads files in Canon's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Nikon"),
    "image/x-nikon-nef,image/x-nikon-nrw",
    "nef,nrw",
    NULL,

    "file-rawtherapee-nikon-load",
    "Load files in the Nikon raw formats via rawtherapee",
    "This plug-in loads files in Nikon's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Hasselblad"),
    "image/x-hasselblad-3fr,image/x-hasselblad-fff",
    "3fr,fff",
    NULL,

    "file-rawtherapee-hasselblad-load",
    "Load files in the Hasselblad raw formats via rawtherapee",
    "This plug-in loads files in Hasselblad's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Sony"),
    "image/x-sony-arw,image/x-sony-srf,image/x-sony-sr2",
    "arw,srf,sr2",
    NULL,

    "file-rawtherapee-sony-load",
    "Load files in the Sony raw formats via rawtherapee",
    "This plug-in loads files in Sony's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Casio BAY"),
    "image/x-casio-bay",
    "bay",
    NULL,

    "file-rawtherapee-bay-load",
    "Load files in the BAY raw format via rawtherapee",
    "This plug-in loads files in Casio's raw BAY format by calling rawtherapee."
  },

  {
    N_("Raw Phantom Software CINE"),
    "", /* FIXME: find a mime type */
    "cine,cin",
    NULL,

    "file-rawtherapee-cine-load",
    "Load files in the CINE raw format via rawtherapee",
    "This plug-in loads files in Phantom Software's raw CINE format by calling rawtherapee."
  },

  {
    N_("Raw Sinar"),
    "", /* FIXME: find a mime type */
    "cs1,ia,sti",
    NULL,

    "file-rawtherapee-sinar-load",
    "Load files in the Sinar raw formats via rawtherapee",
    "This plug-in loads files in Sinar's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Kodak"),
    "image/x-kodak-dc2,image/x-kodak-dcr,image/x-kodak-kdc,image/x-kodak-k25,image/x-kodak-kc2",
    "dc2,dcr,kdc,k25,kc2",
    NULL,

    "file-rawtherapee-kodak-load",
    "Load files in the Kodak raw formats via rawtherapee",
    "This plug-in loads files in Kodak's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Adobe DNG Digital Negative"),
    "image/x-adobe-dng",
    "dng",
    NULL,

    "file-rawtherapee-dng-load",
    "Load files in the DNG raw format via rawtherapee",
    "This plug-in loads files in the Adobe Digital Negative DNG format by calling rawtherapee."
  },

  {
    N_("Raw Epson ERF"),
    "image/x-epson-erf",
    "erf",
    NULL,

    "file-rawtherapee-erf-load",
    "Load files in the ERF raw format via rawtherapee",
    "This plug-in loads files in Epson's raw ERF format by calling rawtherapee."
  },

  {
    N_("Raw Phase One"),
    "image/x-phaseone-cap,image/x-phaseone-iiq",
    "cap,iiq",
    NULL,

    "file-rawtherapee-phaseone-load",
    "Load files in the Phase One raw formats via rawtherapee",
    "This plug-in loads files in Phase One's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Minolta"),
    "image/x-minolta-mdc,image/x-minolta-mrw",
    "mdc,mrw",
    NULL,

    "file-rawtherapee-minolta-load",
    "Load files in the Minolta raw formats via rawtherapee",
    "This plug-in loads files in Minolta's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Mamiya MEF"),
    "image/x-mamiya-mef",
    "mef", NULL,

    "file-rawtherapee-mef-load",
    "Load files in the MEF raw format via rawtherapee",
    "This plug-in loads files in Mamiya's raw MEF format by calling rawtherapee."
  },

  {
    N_("Raw Leaf MOS"),
    "image/x-leaf-mos",
    "mos",
    NULL,

    "file-rawtherapee-mos-load",
    "Load files in the MOS raw format via rawtherapee",
    "This plug-in loads files in Leaf's raw MOS format by calling rawtherapee."
  },

  {
    N_("Raw Olympus ORF"),
    "image/x-olympus-orf",
    "orf",
    NULL,

    "file-rawtherapee-orf-load",
    "Load files in the ORF raw format via rawtherapee",
    "This plug-in loads files in Olympus' raw ORF format by calling rawtherapee."
  },

  {
    N_("Raw Pentax PEF"),
    "image/x-pentax-pef,image/x-pentax-raw",
    "pef,raw",
    NULL,

    "file-rawtherapee-pef-load",
    "Load files in the PEF raw format via rawtherapee",
    "This plug-in loads files in Pentax' raw PEF format by calling rawtherapee."
  },

  {
    N_("Raw Logitech PXN"),
    "image/x-pxn", /* FIXME: is that the correct mime type? */
    "pxn",
    NULL,

    "file-rawtherapee-pxn-load",
    "Load files in the PXN raw format via rawtherapee",
    "This plug-in loads files in Logitech's raw PXN format by calling rawtherapee."
  },

  {
    N_("Raw Apple QuickTake QTK"),
    "", /* FIXME: find a mime type */
    "qtk",
    NULL,

    "file-rawtherapee-qtk-load",
    "Load files in the QTK raw format via rawtherapee",
    "This plug-in loads files in Apple's QuickTake QTK raw format by calling rawtherapee."
  },

  {
    N_("Raw Fujifilm RAF"),
    "image/x-fuji-raf",
    "raf",
    NULL,

    "file-rawtherapee-raf-load",
    "Load files in the RAF raw format via rawtherapee",
    "This plug-in loads files in Fujifilm's raw RAF format by calling rawtherapee."
  },

  {
    N_("Raw Panasonic"),
    "image/x-panasonic-raw,image/x-panasonic-rw2",
    "raw,rw2",
    NULL,

    "file-rawtherapee-panasonic-load",
    "Load files in the Panasonic raw formats via rawtherapee",
    "This plug-in loads files in Panasonic's raw formats by calling rawtherapee."
  },

  {
    N_("Raw Digital Foto Maker RDC"),
    "", /* FIXME: find a mime type */
    "rdc",
    NULL,

    "file-rawtherapee-rdc-load",
    "Load files in the RDC raw format via rawtherapee",
    "This plug-in loads files in Digital Foto Maker's raw RDC format by calling rawtherapee."
  },

  {
    N_("Raw Leica RWL"),
    "image/x-leica-rwl",
    "rwl",
    NULL,

    "file-rawtherapee-rwl-load",
    "Load files in the RWL raw format via rawtherapee",
    "This plug-in loads files in Leica's raw RWL format by calling rawtherapee."
  },

  {
    N_("Raw Samsung SRW"),
    "image/x-samsung-srw",
    "srw",
    NULL,

    "file-rawtherapee-srw-load",
    "Load files in the SRW raw format via rawtherapee",
    "This plug-in loads files in Samsung's raw SRW format by calling rawtherapee."
  },

  {
    N_("Raw Sigma X3F"),
    "image/x-sigma-x3f",
    "x3f",
    NULL,

    "file-rawtherapee-x3f-load",
    "Load files in the X3F raw format via rawtherapee",
    "This plug-in loads files in Sigma's raw X3F format by calling rawtherapee."
  },

  {
    N_("Raw Arriflex ARI"),
    "",
    "ari",
    NULL,

    "file-rawtherapee-ari-load",
    "Load files in the ARI raw format via rawtherapee",
    "This plug-in loads files in Arriflex' raw ARI format by calling rawtherapee."
  }
};
