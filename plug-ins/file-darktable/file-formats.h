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

  const gchar *load_thumb_proc;
  const gchar *load_thumb_blurb;
  const gchar *load_thumb_help;
};

/* some magic numbers taken from http://www.garykessler.net/library/file_sigs.html
 * see also http://fileformats.archiveteam.org/wiki/Cameras_and_Digital_Image_Sensors
 */
static const FileFormat file_formats[] =
{
  {
    N_("Canon raw"),
    "image/x-canon-cr2,image/x-canon-crw,image/tiff", // FIXME: only one mime type
    "cr2,crw,tif,tiff",
    "0,string,II*\\0\\020\\0\\0\\0CR,"             // cr2
    "0,string,II\\024\\0\\0\\0HEAPCCDR,"           // crw
    "0,string,MM\\0*\\0\\0\\0\\020\\0272\\0260,"   // tiff
    "0,string,MM\\0*\\0\\0\\021\\064\\0\\04,"      // tiff
    "0,string,II*\\0\\0\\03\\0\\0\\0377\\01",      // tiff

    "file-raw-canon-load",
    "Load files in the Canon raw formats via darktable",
    "This plug-in loads files in Canon's raw formats by calling darktable.",

    "file-raw-canon-load-thumb",
    "Load thumbnail from a Canon raw image via darktable",
    "This plug-in loads a thumbnail from Canon's raw images by calling darktable-cli."
  },

  {
    N_("Nikon raw"),
    "image/x-nikon-nef,image/x-nikon-nrw", // FIXME: only one mime type
    "nef,nrw",
    NULL,

    "file-raw-nikon-load",
    "Load files in the Nikon raw formats via darktable",
    "This plug-in loads files in Nikon's raw formats by calling darktable.",

    "file-raw-nef-load-thumb",
    "Load thumbnail from a Nikon raw image via darktable",
    "This plug-in loads a thumbnail from Nikon's raw images by calling darktable-cli."
  },

  {
    N_("Hasselblad raw"),
    "image/x-hasselblad-3fr,image/x-hasselblad-fff", // FIXME: only one mime type
    "3fr,fff",
    NULL,

    "file-raw-hasselblad-load",
    "Load files in the Hasselblad raw formats via darktable",
    "This plug-in loads files in Hasselblad's raw formats by calling darktable.",

    "file-hasselblad-load-thumb",
    "Load thumbnail from a Hasselblad raw image via darktable",
    "This plug-in loads a thumbnail from Hasselblad raw images by calling darktable-cli."
  },

  {
    N_("Sony raw"),
    "image/x-sony-arw,image/x-sony-srf,image/x-sony-sr2", // FIXME: only one mime type
    "arw,srf,sr2",
    NULL,

    "file-raw-sony-load",
    "Load files in the Sony raw formats via darktable",
    "This plug-in loads files in Sony's raw formats by calling darktable.",

    "file-raw-sony-load-thumb",
    "Load thumbnail from a Sony raw image via darktable",
    "This plug-in loads a thumbnail from Sony raw images by calling darktable-cli."
  },

  {
    N_("Casio BAY raw"),
    "image/x-casio-bay",
    "bay",
    NULL,

    "file-raw-bay-load",
    "Load files in the BAY raw format via darktable",
    "This plug-in loads files in Casio's raw BAY format by calling darktable.",

    "file-raw-bay-load-thumb",
    "Load thumbnail from a BAY raw image via darktable",
    "This plug-in loads a thumbnail from Casio's raw BAY images by calling darktable-cli."
  },

  {
    N_("Phantom Software CINE raw"),
    "", // FIXME: find a mime type
    "cine,cin",
    NULL,

    "file-raw-cine-load",
    "Load files in the CINE raw format via darktable",
    "This plug-in loads files in Phantom Software's raw CINE format by calling darktable.",

    "file-raw-cine-load-thumb",
    "Load thumbnail from a CINE raw image via darktable",
    "This plug-in loads a thumbnail from Phantom Software's raw CINE images by calling darktable-cli."
  },

  {
    N_("Sinar raw"),
    "", // FIXME: find a mime type
    "cs1,ia,sti",
    NULL,

    "file-raw-sinar-load",
    "Load files in the Sinar raw formats via darktable",
    "This plug-in loads files in Sinar's raw formats by calling darktable.",

    "file-raw-sinar-load-thumb",
    "Load thumbnail from a Sinar raw image via darktable",
    "This plug-in loads a thumbnail from Sinar raw images by calling darktable-cli."
  },

  {
    N_("Kodak raw"),
    "image/x-kodak-dc2,image/x-kodak-dcr,image/x-kodak-kdc,image/x-kodak-k25,image/x-kodak-kc2,image/tiff", // FIXME: only one mime type
    "dc2,dcr,kdc,k25,kc2,tif,tiff",
    "0,string,MM\\0*\\0\\0\\021\\0166\\0\\04,"    // tiff
    "0,string,II*\\0\\0\\03\\0\\0\\0174\\01",     // tiff

    "file-raw-kodak-load",
    "Load files in the Kodak raw formats via darktable",
    "This plug-in loads files in Kodak's raw formats by calling darktable.",

    "file-raw-kodak-load-thumb",
    "Load thumbnail from a Kodak raw image via darktable",
    "This plug-in loads a thumbnail from Kodak raw images by calling darktable-cli."
  },

  {
    N_("Adobe DNG Digital Negative raw"),
    "image/x-adobe-dng",
    "dng",
    NULL,

    "file-raw-dng-load",
    "Load files in the DNG raw format via darktable",
    "This plug-in loads files in the Adobe Digital Negative DNG format by calling darktable.",

    "file-raw-dng-load-thumb",
    "Load thumbnail from a DNG raw image via darktable",
    "This plug-in loads a thumbnail from Adobe's Digital Negative DNG images by calling darktable-cli."
  },

  {
    N_("Epson ERF raw"),
    "image/x-epson-erf",
    "erf",
    NULL,

    "file-raw-erf-load",
    "Load files in the ERF raw format via darktable",
    "This plug-in loads files in Epson's raw ERF format by calling darktable.",

    "file-raw-erf-load-thumb",
    "Load thumbnail from an ERF raw image via darktable",
    "This plug-in loads a thumbnail from Epson's raw ERF images by calling darktable-cli."
  },

  {
    N_("Phase One raw"),
    "image/x-phaseone-cap,image/x-phaseone-iiq", // FIXME: only one mime type
    "cap,iiq",
    NULL,

    "file-raw-phaseone-load",
    "Load files in the Phase One raw formats via darktable",
    "This plug-in loads files in Phase One's raw formats by calling darktable.",

    "file-raw-phaseone-load-thumb",
    "Load thumbnail from a Phase One raw image via darktable",
    "This plug-in loads a thumbnail from Phase One's raw images by calling darktable-cli."
  },

  {
    N_("Minolta raw"),
    "image/x-minolta-mdc,image/x-minolta-mrw", // FIXME: only one mime type
    "mdc,mrw",
    NULL,

    "file-raw-minolta-load",
    "Load files in the Minolta raw formats via darktable",
    "This plug-in loads files in Minolta's raw formats by calling darktable.",

    "file-raw-minolta-load-thumb",
    "Load thumbnail from a Minolta raw image via darktable",
    "This plug-in loads a thumbnail from Minolta raw images by calling darktable-cli."
  },

  {
    N_("Mamiya MEF raw"),
    "image/x-mamiya-mef",
    "mef", NULL,

    "file-raw-mef-load",
    "Load files in the MEF raw format via darktable",
    "This plug-in loads files in Mamiya's raw MEF format by calling darktable.",

    "file-raw-mef-load-thumb",
    "Load thumbnail from a MEF raw image via darktable",
    "This plug-in loads a thumbnail from Mamiya's raw MEF images by calling darktable-cli."
  },

  {
    N_("Leaf MOS raw"),
    "image/x-leaf-mos",
    "mos",
    NULL,

    "file-raw-mos-load",
    "Load files in the MOS raw format via darktable",
    "This plug-in loads files in Leaf's raw MOS format by calling darktable.",

    "file-raw-mos-load-thumb",
    "Load thumbnail from a MOS raw image via darktable",
    "This plug-in loads a thumbnail from Leaf's raw MOS images by calling darktable-cli."
  },

  {
    N_("Olympus ORF raw"),
    "image/x-olympus-orf",
    "orf",
    "0,string,IIRO,0,string,MMOR,0,string,IIRS",

    "file-raw-orf-load",
    "Load files in the ORF raw format via darktable",
    "This plug-in loads files in Olympus' raw ORF format by calling darktable.",

    "file-raw-orf-load-thumb",
    "Load thumbnail from an ORF raw image via darktable",
    "This plug-in loads a thumbnail from Olympus' raw ORF images by calling darktable-cli."
  },

  {
    N_("Pentax PEF raw"),
    "image/x-pentax-pef,image/x-pentax-raw", // FIXME: only one mime type
    "pef,raw",
    NULL,

    "file-raw-pef-load",
    "Load files in the PEF raw format via darktable",
    "This plug-in loads files in Pentax' raw PEF format by calling darktable.",

    "file-raw-pef-load-thumb",
    "Load thumbnail from a PEF raw image via darktable",
    "This plug-in loads a thumbnail from Pentax' raw PEF images by calling darktable-cli."
  },

  {
    N_("Logitech PXN raw"),
    "image/x-pxn", // FIXME: is that the correct mime type?
    "pxn",
    NULL,

    "file-raw-pxn-load",
    "Load files in the PXN raw format via darktable",
    "This plug-in loads files in Logitech's raw PXN format by calling darktable.",

    "file-raw-pxn-load-thumb",
    "Load thumbnail from a PXN raw image via darktable",
    "This plug-in loads a thumbnail from Logitech's raw PXN images by calling darktable-cli."
  },

  {
    N_("Apple QuickTake QTK raw"),
    "", // FIXME: find a mime type
    "qtk",
    NULL,

    "file-raw-qtk-load",
    "Load files in the QTK raw format via darktable",
    "This plug-in loads files in Apple's QuickTake QTK raw format by calling darktable.",

    "file-raw-qtk-load-thumb",
    "Load thumbnail from a QTK raw image via darktable",
    "This plug-in loads a thumbnail from Apple's QuickTake QTK raw images by calling darktable-cli."
  },

  {
    N_("Fujifilm RAF raw"),
    "image/x-fuji-raf",
    "raf",
    "0,string,FUJIFILMCCD-RAW",

    "file-raw-raf-load",
    "Load files in the RAF raw format via darktable",
    "This plug-in loads files in Fujifilm's raw RAF format by calling darktable.",

    "file-raw-raf-load-thumb",
    "Load thumbnail from a RAF raw image via darktable",
    "This plug-in loads a thumbnail from Fujifilm's raw RAF images by calling darktable-cli."
  },

  {
    N_("Panasonic raw"),
    "image/x-panasonic-raw,image/x-panasonic-rw2", // FIXME: only one mime type
    "raw,rw2",
    "0,string,IIU\\0",

    "file-raw-panasonic-load",
    "Load files in the Panasonic raw formats via darktable",
    "This plug-in loads files in Panasonic's raw formats by calling darktable.",

    "file-raw-panasonic-load-thumb",
    "Load thumbnail from a Panasonic raw image via darktable",
    "This plug-in loads a thumbnail from Panasonic's raw images by calling darktable-cli."
  },

  {
    N_("Digital Foto Maker RDC raw"),
    "", // FIXME: find a mime type
    "rdc",
    NULL,

    "file-raw-rdc-load",
    "Load files in the RDC raw format via darktable",
    "This plug-in loads files in Digital Foto Maker's raw RDC format by calling darktable.",

    "file-raw-rdc-load-thumb",
    "Load thumbnail from a RDC raw image via darktable",
    "This plug-in loads a thumbnail from Digital Foto Maker's raw RDC images by calling darktable-cli."
  },

  {
    N_("Leica RWL raw"),
    "image/x-leica-rwl",
    "rwl",
    NULL,

    "file-raw-rwl-load",
    "Load files in the RWL raw format via darktable",
    "This plug-in loads files in Leica's raw RWL format by calling darktable.",

    "file-raw-rwl-load-thumb",
    "Load thumbnail from a RWL raw image via darktable",
    "This plug-in loads a thumbnail from Leica's raw RWL images by calling darktable-cli."
  },

  {
    N_("Samsung SRW raw"),
    "image/x-samsung-srw",
    "srw",
    NULL,

    "file-raw-srw-load",
    "Load files in the SRW raw format via darktable",
    "This plug-in loads files in Samsung's raw SRW format by calling darktable.",

    "file-raw-srw-load-thumb",
    "Load thumbnail from a SRW raw image via darktable",
    "This plug-in loads a thumbnail from Samsung's raw SRW images by calling darktable-cli."
  },

  {
    N_("Sigma X3F raw"),
    "image/x-sigma-x3f",
    "x3f",
    "0,string,FOVb",

    "file-raw-x3f-load",
    "Load files in the X3F raw format via darktable",
    "This plug-in loads files in Sigma's raw X3F format by calling darktable.",

    "file-raw-x3f-load-thumb",
    "Load thumbnail from a X3F raw image via darktable",
    "This plug-in loads a thumbnail from Sigma's raw X3F images by calling darktable-cli."
  },

  {
    N_("Arriflex ARI raw"),
    "",
    "ari",
    NULL,

    "file-raw-ari-load",
    "Load files in the ARI raw format via darktable",
    "This plug-in loads files in Arriflex' raw ARI format by calling darktable.",

    "file-raw-ari-load-thumb",
    "Load thumbnail from an ARI raw image via darktable",
    "This plug-in loads a thumbnail from Arriflex' raw ARI images by calling darktable-cli."
  }

};
