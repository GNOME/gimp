/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-darktable.c -- raw file format plug-in that uses darktable
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
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

/* These are the raw formats that file-darktable will register */

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

/* some magic numbers taken from
 * http://www.garykessler.net/library/file_sigs.html
 *
 * see also
 * http://fileformats.archiveteam.org/wiki/Cameras_and_Digital_Image_Sensors
 */
static const FileFormat file_formats[] =
{
  {
    N_("Raw Canon"),
    "image/x-canon-cr2,image/x-canon-crw,image/tiff", // FIXME: only one mime type
    "cr2,crw,tif,tiff",
    "0,string,II*\\0\\020\\0\\0\\0CR,"             // cr2
    "0,string,II\\024\\0\\0\\0HEAPCCDR,"           // crw
    "0,string,MM\\0*\\0\\0\\0\\020\\0272\\0260,"   // tiff
    "0,string,MM\\0*\\0\\0\\021\\064\\0\\04,"      // tiff
    "0,string,II*\\0\\0\\03\\0\\0\\0377\\01",      // tiff

    "file-darktable-canon-load",
    "Load files in the Canon raw formats via darktable",
    "This plug-in loads files in Canon's raw formats by calling darktable."
  },

  {
    N_("Raw Nikon"),
    "image/x-nikon-nef,image/x-nikon-nrw", // FIXME: only one mime type
    "nef,nrw",
    NULL,

    "file-darktable-nikon-load",
    "Load files in the Nikon raw formats via darktable",
    "This plug-in loads files in Nikon's raw formats by calling darktable."
  },

  {
    N_("Raw Hasselblad"),
    "image/x-hasselblad-3fr,image/x-hasselblad-fff", // FIXME: only one mime type
    "3fr,fff",
    NULL,

    "file-darktable-hasselblad-load",
    "Load files in the Hasselblad raw formats via darktable",
    "This plug-in loads files in Hasselblad's raw formats by calling darktable."
  },

  {
    N_("Raw Sony"),
    "image/x-sony-arw,image/x-sony-srf,image/x-sony-sr2", // FIXME: only one mime type
    "arw,srf,sr2",
    NULL,

    "file-darktable-sony-load",
    "Load files in the Sony raw formats via darktable",
    "This plug-in loads files in Sony's raw formats by calling darktable."
  },

  {
    N_("Raw Casio BAY"),
    "image/x-casio-bay",
    "bay",
    NULL,

    "file-darktable-bay-load",
    "Load files in the BAY raw format via darktable",
    "This plug-in loads files in Casio's raw BAY format by calling darktable."
  },

  {
    N_("Raw Phantom Software CINE"),
    "", // FIXME: find a mime type
    "cine,cin",
    NULL,

    "file-darktable-cine-load",
    "Load files in the CINE raw format via darktable",
    "This plug-in loads files in Phantom Software's raw CINE format by calling darktable."
  },

  {
    N_("Raw Sinar"),
    "", // FIXME: find a mime type
    "cs1,ia,sti",
    NULL,

    "file-darktable-sinar-load",
    "Load files in the Sinar raw formats via darktable",
    "This plug-in loads files in Sinar's raw formats by calling darktable."
  },

  {
    N_("Raw Kodak"),
    "image/x-kodak-dc2,image/x-kodak-dcr,image/x-kodak-kdc,image/x-kodak-k25,image/x-kodak-kc2,image/tiff", // FIXME: only one mime type
    "dc2,dcr,kdc,k25,kc2,tif,tiff",
    "0,string,MM\\0*\\0\\0\\021\\0166\\0\\04,"    // tiff
    "0,string,II*\\0\\0\\03\\0\\0\\0174\\01",     // tiff

    "file-darktable-kodak-load",
    "Load files in the Kodak raw formats via darktable",
    "This plug-in loads files in Kodak's raw formats by calling darktable."
  },

  {
    N_("Raw Adobe DNG Digital Negative"),
    "image/x-adobe-dng",
    "dng",
    NULL,

    "file-darktable-dng-load",
    "Load files in the DNG raw format via darktable",
    "This plug-in loads files in the Adobe Digital Negative DNG format by calling darktable."
  },

  {
    N_("Raw Epson ERF"),
    "image/x-epson-erf",
    "erf",
    NULL,

    "file-darktable-erf-load",
    "Load files in the ERF raw format via darktable",
    "This plug-in loads files in Epson's raw ERF format by calling darktable."
  },

  {
    N_("Raw Phase One"),
    "image/x-phaseone-cap,image/x-phaseone-iiq", // FIXME: only one mime type
    "cap,iiq",
    NULL,

    "file-darktable-phaseone-load",
    "Load files in the Phase One raw formats via darktable",
    "This plug-in loads files in Phase One's raw formats by calling darktable."
  },

  {
    N_("Raw Minolta"),
    "image/x-minolta-mdc,image/x-minolta-mrw", // FIXME: only one mime type
    "mdc,mrw",
    NULL,

    "file-darktable-minolta-load",
    "Load files in the Minolta raw formats via darktable",
    "This plug-in loads files in Minolta's raw formats by calling darktable."
  },

  {
    N_("Raw Mamiya MEF"),
    "image/x-mamiya-mef",
    "mef", NULL,

    "file-darktable-mef-load",
    "Load files in the MEF raw format via darktable",
    "This plug-in loads files in Mamiya's raw MEF format by calling darktable."
  },

  {
    N_("Raw Leaf MOS"),
    "image/x-leaf-mos",
    "mos",
    NULL,

    "file-darktable-mos-load",
    "Load files in the MOS raw format via darktable",
    "This plug-in loads files in Leaf's raw MOS format by calling darktable."
  },

  {
    N_("Raw Olympus ORF"),
    "image/x-olympus-orf",
    "orf",
    "0,string,IIRO,0,string,MMOR,0,string,IIRS",

    "file-darktable-orf-load",
    "Load files in the ORF raw format via darktable",
    "This plug-in loads files in Olympus' raw ORF format by calling darktable."
  },

  {
    N_("Raw Pentax PEF"),
    "image/x-pentax-pef,image/x-pentax-raw", // FIXME: only one mime type
    "pef,raw",
    NULL,

    "file-darktable-pef-load",
    "Load files in the PEF raw format via darktable",
    "This plug-in loads files in Pentax' raw PEF format by calling darktable."
  },

  {
    N_("Raw Logitech PXN"),
    "image/x-pxn", // FIXME: is that the correct mime type?
    "pxn",
    NULL,

    "file-darktable-pxn-load",
    "Load files in the PXN raw format via darktable",
    "This plug-in loads files in Logitech's raw PXN format by calling darktable."
  },

  {
    N_("Raw Apple QuickTake QTK"),
    "", // FIXME: find a mime type
    "qtk",
    NULL,

    "file-darktable-qtk-load",
    "Load files in the QTK raw format via darktable",
    "This plug-in loads files in Apple's QuickTake QTK raw format by calling darktable."
  },

  {
    N_("Raw Fujifilm RAF"),
    "image/x-fuji-raf",
    "raf",
    "0,string,FUJIFILMCCD-RAW",

    "file-darktable-raf-load",
    "Load files in the RAF raw format via darktable",
    "This plug-in loads files in Fujifilm's raw RAF format by calling darktable."
  },

  {
    N_("Raw Panasonic"),
    "image/x-panasonic-raw,image/x-panasonic-rw2", // FIXME: only one mime type
    "raw,rw2",
    "0,string,IIU\\0",

    "file-darktable-panasonic-load",
    "Load files in the Panasonic raw formats via darktable",
    "This plug-in loads files in Panasonic's raw formats by calling darktable."
  },

  {
    N_("Raw Digital Foto Maker RDC"),
    "", // FIXME: find a mime type
    "rdc",
    NULL,

    "file-darktable-rdc-load",
    "Load files in the RDC raw format via darktable",
    "This plug-in loads files in Digital Foto Maker's raw RDC format by calling darktable."
  },

  {
    N_("Raw Leica RWL"),
    "image/x-leica-rwl",
    "rwl",
    NULL,

    "file-darktable-rwl-load",
    "Load files in the RWL raw format via darktable",
    "This plug-in loads files in Leica's raw RWL format by calling darktable."
  },

  {
    N_("Raw Samsung SRW"),
    "image/x-samsung-srw",
    "srw",
    NULL,

    "file-darktable-srw-load",
    "Load files in the SRW raw format via darktable",
    "This plug-in loads files in Samsung's raw SRW format by calling darktable."
  },

  {
    N_("Raw Sigma X3F"),
    "image/x-sigma-x3f",
    "x3f",
    "0,string,FOVb",

    "file-darktable-x3f-load",
    "Load files in the X3F raw format via darktable",
    "This plug-in loads files in Sigma's raw X3F format by calling darktable."
  },

  {
    N_("Raw Arriflex ARI"),
    "",
    "ari",
    NULL,

    "file-darktable-ari-load",
    "Load files in the ARI raw format via darktable",
    "This plug-in loads files in Arriflex' raw ARI format by calling darktable."
  }
};
