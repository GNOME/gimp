XJT Fileformat specification:   
------------------------------

 (XJT 1.1,   1998.10.31 - 1999.03.16)
       
----------------------------------------------------------------  

   XJT Fileformat was designed to save compressed GIMP Images with
   all the properties that are available in GIMP.

   XJT uses JPEG compression foreach layer or channel and TAR to
   collect all layers in one file. (Layers with alphachannels
   are splitted into 2 jpeg files)

   Additional property informations about offsets, opacity, layernames etc..
   are stored in the readable textfile called "PRP" (Properties)

   The resulting tar-file is optionaly compressed a 2.nd time
   using GZIP or BZIP2.
   (depends on the filename ending "gz" or "bz2")

   Filename Extensions are:
     image.xjt                # plain tar file
     image.xjtgz              # gzip compressed tar file
     image.xjbz2              # bzip2 compressed tar file
     
Restrictions:
-------------
   ** XJT does not support INDEXED Images. **



Why should anyone use the new XJT Fileformat ?
---------------------------------------------

   XJT Advantages:

   - The high JPEG compression rates (1:10 and more) helps
     to save a lot of Diskspace.
     (especially if you are using GIMP and GAP to
      store many images as AnimFrames )
     
   - Saved image keeps all layers channels and properties
   
   - single Layers can be extracted using tar
     and then be accessed by many other 
     jpeg-compatible programs.


   XJT Disadvantages:
   
   - JPEG is a lossy compression.
     After save and load the bitmapdata
     differs a little from the original,
     depending on the used quality setting.
     
     ==> use GIMP's native xcf format
         where you need exactly the original.


Example:
________

    Example of PRP file:
    --------------------
       GIMP_XJ_IMAGE ver:"1.0" w/h:256,256
       L0 acl fsl pt o:35,116 n:"Pasted Layer"
       L1 ln pt aml eml o:0,7 n:"Pasted Layer"
       m1 smc o:0,7 n:"Pasted Layer mask"
       l2 fa n:"Background"
       c0 op:33.725 iv smc c:255,0,0 n:"saved selection mask"


    TAR Contents of the example file
    --------------------------------
      >tar -tvf example.xjt
      -rw-r--r-- hof/users       228 Nov  1 11:19 1998 PRP
      -rw-r--r-- hof/users      2352 Nov  1 11:19 1998 c0.jpg
      -rw-r--r-- hof/users      3789 Nov  1 11:19 1998 l0.jpg
      -rw-r--r-- hof/users      8569 Nov  1 11:19 1998 l1.jpg
      -rw-r--r-- hof/users      9098 Nov  1 11:19 1998 l2.jpg
      -rw-r--r-- hof/users      3926 Nov  1 11:19 1998 la0.jpg
      -rw-r--r-- hof/users      6095 Nov  1 11:19 1998 la1.jpg
      -rw-r--r-- hof/users      1623 Nov  1 11:19 1998 lm1.jpg


  The example image has dimensions of 256x256 pixels. 
  The Image Type is RGB (default Property typ:0)
  
  L0 The Image has a floating selection (stored as l0.jpg) at offset 35/116.
     The floating selection has an alpha channel (stored in la0.jpg)
     The floating selection is the active Layer.

  There are 2 further Layers.
  L1 at offset 077 is named "Pasted Layer" and is stored in l1.jpg. 
     This Layer has both an alpha channel (stored in la1.jpg)
     and a LayerMask (stored in lm1.jpg)
     
     (the Properties of the LayerMask are stored in an extra Line
     of the PRP file beginning with m1)
     
  l2 is the Background Layer without alpha channel. (stored in l2.jpg)
     The Floating selection is attached to this layer ("fa" Property)
  
  c0 The image has one extra channel named "saved selection mask"
     This channel has full red color (c:255,0,0 property) and an opacity
     value of 33.725 % but is invisible ("iv" property)


Syntax of the PRP -file
-----------------------

  Image Properties (must be the 1. line in the PRP file)
  ================

       The line starts with 
       GIMP_XJ_IMAGE Image-fileformat-Identifier

       followed by a List of Image properties seperated by Blank:
	  PROP_VERSION
	  PROP_DIMENSION
	  PROP_RESOLUTION
	  PROP_TYPE            0 == RGB, 1 = GRAY
	  PROP_GUIDES         position, orientation     (can occure more than 1 time)
          PROP_PARASITES

 
  Layer Properties:
  =================
      The line starts with
       l<nr>     Layer description of layer <nr> without alpha channel.
                     the layers bitmapdata is stored in a jpeg encoded
		     file named l<nr>.jpg
       L<nr>     Layer description of layer <nr> with alpha channel.
                     the layers bitmapdata is stored in a jpeg encoded
		     file named l<nr>.jpg,
		     the alpha channel is stored in an additional jpeg encoded
		     file named la<nr>.jpg,
 
       followed by a List of Layer properties seperated by Blank:
       (properties for the default values are not written)

           PROP_ACTIVE_LAYER
           PROP_FLOATING_SELECTION
           PROP_FLOATING_ATTACHED
           PROP_OPACITY
           PROP_MODE
           PROP_VISIBLE
           PROP_LINKED
           PROP_PRESERVE_TRANSPARENCY
           PROP_APPLY_MASK
           PROP_EDIT_MASK
           PROP_SHOW_MASK
           PROP_OFFSETS
           PROP_TATTOO
           PROP_PARASITES
           PROP_NAME
 
  Channel Properties:
  ===================
      
      The line starts with
       c<nr>     Channel description of channel <nr>.
                     the channels bitmapdata is stored in a jpeg encoded
		     file named c<nr>.jpg
       m<nr>     Layermask-channel of layer <nr>
                     the layermask-channels bitmapdata is tored in a jpeg encoded
		     file named c<nr>.jpg
      
       followed by a List of Channel properties seperated by Blank:
       (properties for the default values are not written)

          PROP_ACTIVE_CHANNEL
          PROP_SELECTION
          PROP_FLOATING_ATTACHED
          PROP_OPACITY
          PROP_VISIBLE
          PROP_SHOW_MASKED
          PROP_COLOR
          PROP_TATTOO
          PROP_PARASITES
          PROP_NAME

  Parasite Properties:
  ====================
      The line starts with
       p<nr>     Parasite description of parasite <nr>.
                     the parasite data is stored 1:1 in a
		     file named p<nr>.pte
 
       followed by a List of Layer properties seperated by Blank:
       (properties for the default values are not written)

           PROP_NAME

--------------------------
Properties Summary
--------------------------


Property types:
  PTYP_BOOLEAN
         mnemonic
  PTYP_INT
         mnemonic:int_value
  PTYP_2xINT
         mnemonic:int_value,int_value
  PTYP_3xINT
         mnemonic:int_value,int_value,int_value
  PTYP_FLT
         mnemonic:float_value
  PTYP_2xFLT
         mnemonic:float_value,float_value
  PTYP_STRING
         mnemonic:"string_value"

Properties are written as short mnemonics (1 upto 3 characters) to identify the Property.
Non-boolean Property-mnemonics require a Value seperated by ':'. 

For each property there is a defined Defaultvalue. (usually 0, or "" for strings)
The Defaultvalue is assumed when the Token is not specified,
Boolean tokens default always to FALSE and become TRUE when specified.



  /* propery                   mnemonic   type                     default values */
   PROP_END,		       "*",	 PTYP_NOT_SUPPORTED,	   0, 
   PROP_COLORMAP,	       "*",	 PTYP_NOT_SUPPORTED,	   0, 
   PROP_ACTIVE_LAYER,	       "acl",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_ACTIVE_CHANNEL,        "acc",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_SELECTION,	       "sel",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_FLOATING_SELECTION,    "fsl",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_OPACITY,	       "op",	 PTYP_FLT,		   100.0,
   PROP_MODE,		       "md",	 PTYP_INT,		   0, 
   PROP_VISIBLE,	       "iv",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_LINKED, 	       "ln",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_PRESERVE_TRANSPARENCY, "pt",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_APPLY_MASK,	       "aml",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_EDIT_MASK,	       "eml",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_SHOW_MASK,	       "sml",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_SHOW_MASKED,	       "smc",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_OFFSETS,	       "o",	 PTYP_2xINT,		   0, 0,
   PROP_COLOR,  	       "c",	 PTYP_3xINT,		   0, 0, 0, 
   PROP_COMPRESSION,	       "*",	 PTYP_NOT_SUPPORTED,	   0,
   PROP_GUIDES, 	       "g",	 PTYP_2xINT,		   0, 0,
   PROP_RESOLUTION,            "res",    PTYP_2xFLT,               72.0, 72.0,
   PROP_TATTOO,                "tto",    PTYP_INT,                 0,
   PROP_PARASITES,             "pte",	 PTYP_INT,		   0,


   PROP_FLOATING_ATTACHED,     "fa",	 PTYP_BOOLEAN,  	   FALSE, 
   PROP_NAME,		       "n",	 PTYP_STRING,		   "",
   PROP_DIMENSION,	       "w/h",	 PTYP_2xINT,		   0, 0,
   PROP_TYPE,		       "typ",	 PTYP_INT,		   0,
   PROP_VERSION,	       "ver",	 PTYP_STRING,		   0,


PROP_OPACITY  valid values are 
              0.0        ( full transparent)
	        upto 
	      100.0      (full opaque)
	      
PROP_TYPE     valid values are:
              0 ... RGB 
	      1 ... GRAY

PROP_MODE     valid values are:
              0  ...  NORMAL_MODE
              1  ...  DISSOLVE_MODE
              3  ...  MULTIPLY_MODE
              4  ...  SCREEN_MODE
              5  ...  OVERLAY_MODE
              6  ...  DIFFERENCE_MODE
              7  ...  ADDITION_MODE
              8  ...  SUBTRACT_MODE
              9  ...  DARKEN_ONLY_MODE
              10 ...  LIGHTEN_ONLY_MODE
              11 ...  HUE_MODE
              12 ...  SATURATION_MODE
              13 ...  COLOR_MODE
              14 ...  VALUE_MODE
 


-----------------------------------------------------------
changelog:
------------------------------------------------
 - XJT Version 1.1
 
     XJT Format 1.1 was extended for
            RESOLUTION, TATTOO's, and PARASITES
            
            Please note that TATTOO's and PARASITES are only defined
            but not implemented.
            (Gimp 1.1.3 has no PDB-Interfaces to enable Plug-Ins
            to Load/Save TATTOO's and PARASITES)



  PARASITES: The parasite data is stored in a seperate file for each
             parasite. The file is named p<id>.pte, where
             <id> is a unique integer parasite Id.
             
             A Layer, Channel or Image can have 0 or more PROP_PARASITE
             Properties (pte:1 pte:2 ...), where the integer parameter
             refers to the unique parasite id.
             
             Parasite entries can have a Name Property. 
  

    Extended Example of PRP file with resolution, and parasites:
    -------------------------------------------------------------
       GIMP_XJ_IMAGE ver:"1.1" w/h:256,256 res:72.0,100.5 pte:1
       L0 acl fsl pt o:35,116 n:"Pasted Layer"
       L1 ln pt aml eml o:0,7 n:"Pasted Layer" pte:2 pte:3
       m1 smc o:0,7 n:"Pasted Layer mask"
       l2 fa n:"Background"
       c0 op:33.725 iv smc c:255,0,0 n:"saved selection mask" pte:4
       p1 n:"image parasite"
       p2 n:"layer parasite A"
       p3 n:"layer parasite B"
       p4 n:"channel parasite"

    TAR Contents of the example file
    --------------------------------
      >tar -tvf example.xjt
      -rw-r--r-- hof/users       228 Nov  1 11:19 1998 PRP
      -rw-r--r-- hof/users      2352 Nov  1 11:19 1998 c0.jpg
      -rw-r--r-- hof/users      3789 Nov  1 11:19 1998 l0.jpg
      -rw-r--r-- hof/users      8569 Nov  1 11:19 1998 l1.jpg
      -rw-r--r-- hof/users      9098 Nov  1 11:19 1998 l2.jpg
      -rw-r--r-- hof/users      3926 Nov  1 11:19 1998 la0.jpg
      -rw-r--r-- hof/users      6095 Nov  1 11:19 1998 la1.jpg
      -rw-r--r-- hof/users      1623 Nov  1 11:19 1998 lm1.jpg
                                                       p1.pte  
                                                       p2.pte
                                                       p3.pte
                                                       p4.pte
	      
