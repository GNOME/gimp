#!/usr/bin/perl

# Blow In/Out
# John Pitney

use Gimp 1.06;
use Gimp::Fu;

# print "hello there\n";

# Gimp::set_trace(TRACE_CALL);


sub blowinout { 
    my ($img, $drawable, $angle, $nsteps, $distance, $inmode, $arithmode) = @_;
    # bail out if $drawable isn't a layer
#    print "Starting\n";
    if( gimp_selection_is_empty($img) == 0) { return };
#    if ($nsteps == 0) return;
    eval { $img->undo_push_group_start };
    # save the background color for later restoration
    my $oldbg = gimp_palette_get_background();
    #get the drawable dimensions
    my $xsize = gimp_drawable_width($drawable);
    my $ysize = gimp_drawable_height($drawable);
    
    # Set background color to 128, for clearing dm
    gimp_palette_set_background([128,128,128]);

    # Create a grayscale workspace image for displacement map
    my $dm = gimp_image_new($xsize, $ysize, 1);
    eval { $dm->undo_push_group_start($dm) };
    # It needs to have 2 layers
    my $dmlayer = gimp_layer_new($dm, $xsize, $ysize, GRAY_IMAGE, "newlayer", 
        100, NORMAL_MODE);
    gimp_image_add_layer($dm, $dmlayer, 0);
    
    # Create the layers, one-by-one 
    my $i = 1;
    my $xdist = ($arithmode) ? 
        $i * $distance / $nsteps * -cos($angle * 3.14159 / 180) : 
        $distance ** ($i/$nsteps) * -cos($angle * 3.14159 / 180);
    my $ydist = ($arithmode) ? 
        $i * $distance / $nsteps * sin($angle * 3.14159 / 180) : 
        $distance ** ($i/$nsteps) * sin($angle * 3.14159 / 180);
    gimp_edit_clear($dmlayer);
    plug_in_noisify(1, $dm, $dmlayer, 0, 255, 255, 255, 0);
    gimp_levels($dmlayer, 0, 0, 255, 1.0, 128, 255); 
    $drawable = gimp_layer_copy($drawable, 0);
    gimp_image_add_layer($img, $drawable, -1);
    plug_in_displace(1, $img, $drawable, $xdist, $ydist, 1, 1, $dmlayer, 
        $dmlayer, 1);
    if ( $inmode == 1 )  
    {
        gimp_image_lower_layer($img, $drawable);
    };
    for ( $i = 2; $i <= $nsteps; $i++ ) {
        $xdist = ($arithmode) ? 
            $i * $distance / $nsteps * -cos($angle * 3.14159 / 180) : 
            $distance ** ($i/$nsteps) * -cos($angle * 3.14159 / 180);
        $ydist = ($arithmode) ? 
            $i * $distance / $nsteps * sin($angle * 3.14159 / 180) : 
            $distance ** ($i/$nsteps) * sin($angle * 3.14159 / 180);
        gimp_edit_clear($dmlayer); 
        plug_in_noisify(1, $dm, $dmlayer, 0, 255, 255, 255, 0);
        gimp_levels($dmlayer, 0, 0, 255, 1.0, 128, 255);
        $drawable = gimp_layer_copy($drawable, 0);
        gimp_image_add_layer($img, $drawable, -1);
        plug_in_displace(1, $img, $drawable, $xdist, $ydist, 1, 1, $dmlayer, 
            $dmlayer, 1);
        if ( $inmode == 1 )  
        {
            gimp_image_lower_layer($img, $drawable);
        };
    } 

    eval { $dm->undo_push_group_end };
#    gimp_image_remove_layer($dm, $dmlayer);
#    gimp_image_delete ($dm);
    gimp_palette_set_background($oldbg);
    eval { $img->undo_push_group_end };
#    gimp_displays_flush(); unneccessary (and dangerous ;)
    
    (); # I like smileys ;)
}

register
	"blowinout",
	"Blow selected layer inout",
	"Generates an animation thats blows the selected layer in or out",
	"John Pitney",
	"John Pitney <pitney\@uiuc.edu>",
	"1999-03-15",
	"<Image>/Filters/Distorts/BlowInOut",
	"*",
	[
	 [PF_INT32, "angle", "Wind Angle, 0 is left", 120],
         [PF_INT32, "steps", "Number of Steps/Layers", 5],
	 [PF_VALUE, "distance", "How far to blow",30],
# What I really need here are radio buttons!  Maybe they even exist...
# You wanted them...
	 [PF_RADIO, "direction", "Blow direction", 0, [In => 1, Out => 0]],
         [PF_RADIO, "series", "Kind of series", 1, [Arithmetic => 1, Geometric => 0]]
	],
        [],
	\&blowinout;

exit main;

