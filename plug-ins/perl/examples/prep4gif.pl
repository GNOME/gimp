#!/usr/bin/perl

use Gimp;
use Gimp::Fu;

# This script requires a Gimp version >= 0.96 (I haven't checked - ymmv)
# small changes by Marc Lehmann <pcg@goof.com>

# prep4gif.pl
# by Seth Burgess <sjburges@gimp.org>
# June 29, 1998
#
# This perl plug-in prepares a multilayer RGB image for use as a 
# transparent gif.  To use this prpoerly, you want to have something 
# close to the intended background as the bottom layer.  If convert
# to indexed is not selected, the bottom two options are unused.
# 
# TODO: Write a nicer GUI than Gimp::Fu provides (learn some gtk)
#       Anything else that seems useful 

# Gimp::set_trace(TRACE_ALL);

sub prep {
	my ($img, $drawable, $threshold, $growth, $index, $dither, $colors) = @_;
	
# Duplicate this image, and work on the duplicate for the rest of the
# procedure.
	my $out = gimp_channel_ops_duplicate($img);

# @layers is the ordered list, from top to bottom, of all layers in the
# duplicated image.  To find length of the list, use $#layers
	my @layers = gimp_image_get_layers($out);

# if there's not enough layers, abort.	
	if ($#layers <= 0) {
		gimp_message("You need at least 2 layers to perform prep4gif");
		print "Only ", scalar(@layers), " layers found!(", $layers[0],")\n";
		return 0;
		}

# Show the image early - this makes debugging a breeze	
	my $newdisplay = gimp_display_new($out);

# Hide the bottom layer, so it doesn't get into the merge visible later.

	my $bottomlayer = $layers[$#layers];
	gimp_layer_set_visible($bottomlayer, 0);
	gimp_layer_add_alpha($bottomlayer);

# NOTE TO PERL NEWBIES - 'my' variables should be declared in their outermost
# scope - if defined inside the if statement, will disappear to program.

	my $foreground; 

	if ($#layers > 1) {
		$foreground = gimp_image_merge_visible_layers($out, 0);
	}
	else {	
		$foreground = $layers[0];
	};

	my $layer_mask = gimp_layer_create_mask($foreground,2);
	gimp_image_add_layer_mask ($out, $foreground, $layer_mask);	
	gimp_threshold($layer_mask,$threshold,255);

# Transfer layer mask to selection, and grow the selection
	gimp_selection_layer_alpha($foreground);
	gimp_selection_grow($out,$growth);

# Apply this selection to the background
	gimp_layer_set_visible($bottomlayer, 1);
	gimp_image_set_active_layer($out, $bottomlayer);
	gimp_selection_invert($out);
	gimp_edit_cut($bottomlayer);

# Clean up after yourself
	gimp_image_remove_layer_mask($out, $foreground, 1);
	my $outlayer = gimp_image_merge_visible_layers($out,0);

# Convert to indexed
	if ($index) {
		gimp_convert_indexed($out,$dither,$colors);
		}

# Show all the changes.
	gimp_displays_flush();
	
	undef;
	}

register
	"prep4gif",
	"Prep for gif",
	"Make the image a small-cut-out of the intended background, so your transparent text doesn't look blocky.",
	"Seth Burgess",
	"Seth Burgess <sjburges\@gimp.org>",
	"2-15-98",
	"<Image>/Filters/Misc/Prepare for GIF",
	"RGB*",
	[
	 [PF_INT32, "Lower Threshold", "Lower Alpha Threshold", 64],
	 [PF_INT32, "Growth", "How Much growth for safety ",1],
	 [PF_TOGGLE, "Convert To Indexed", "Convert Image to indexed", 0],
	 [PF_TOGGLE, "Dither", "Floyd-Steinberg Dithering?", 1],
	 [PF_INT32, "Colors", "Colors to quantize to", "255"],
	],
	\&prep;

exit main;

