#!/usr/app/bin/perl

use Gimp qw( :auto );
use Gimp::Fu;

# alpha2color.pl
# by Seth Burgess <sjburges@gimp.org>
# Version 0.02
# Oct 16th, 1998
#
# This script simply changes the current alpha channel to a given color
# instead.  I'm writing it primarily for use with the displace plugin, 
# but I imagine it'll have other uses.

# TODO: Selection is currently ignored.  It'd be better if it remembered
#       what the previous selection was.
#       Also, it needs to find a happier home than in the Filters/Misc menu.

# Gimp::set_trace(TRACE_ALL);

# Revision History
# v0.02 - fixed up @color (should be $color) and undef; (should be return();)

sub save_layers_state ($)  {
	$img = shift;
	my @layers = $img->get_layers;
	$i = 0;
	foreach $lay (@layers) {
		if ($lay->get_visible){
			$arr[$i] = 1;
			}
		else {
			$arr[$i] = 0;
			}
		$i++;
		}
	return @arr;
	}

sub restore_layers_state($@) {
	$img = shift;
	@arr = @_;
	my @layers = $img->get_layers;
	$i = 0;
	foreach $lay (@layers) {
		$lay->set_visible($arr[$i]);
		$i++;
		}
	}
	
				

sub alpha2col {
	my ($img, $drawable, $color) = @_;

	my $oldcolor = gimp_palette_get_background();

	my @layers = gimp_image_get_layers($img);

# if there's not enough layers, abort.	
	if ($#layers < 0) {
		gimp_message("You need at least 1 layer to perform alpha2color!");
		print "Only ", scalar(@layers), " layers found!(", $layers[0],")\n";
		return 0;
		}

# Hide the bottom layer, so it doesn't get into the merge visible later.

	@layer_visibilities = save_layers_state ($img);
	# foreach $visible (@layer_visibilities) {
		# print $visible, "\n";
	#	}
	$target_layer = gimp_image_get_active_layer($img);
	@offsets=$target_layer->offsets;
	# print $target_layer, "\n";
	foreach $eachlay (@layers) {
		$eachlay->set_visible(0);
		}		
	$target_layer->set_visible(1);
	gimp_palette_set_background($color);
#	$newlay = gimp_layer_new (  $img, 
#								$target_layer->width, 
#								$target_layer->height, 
#								1, "NewLayer", 100, NORMAL);
#	$img->add_layer($newlay, scalar(@layers));	
	$newlay = $target_layer->copy(1);
	$img->add_layer($newlay, 0);	
	$newlay->set_offsets(@offsets);
	$target_layer->set_active_layer;

	$img->selection_all;
	$target_layer->edit_fill;
	$img->selection_none;

	$foreground = gimp_image_merge_visible_layers($img,0);
	
	restore_layers_state($img, @layer_visibilities);	
	
	gimp_palette_set_background($oldcolor);
	gimp_displays_flush();
	return();
	}

register
	"plug_in_alpha2color",
	"Alpha 2 Color",
	"Change the current alpha to a selected color.",
	"Seth Burgess",
	"Seth Burgess<sjburges\@gimp.org>",
	"1998-10-18",
	"<Image>/Filters/Misc/Alpha2Color",
	"RGBA",
	[
	 [PF_COLOR, "Color", "Color for current alpha", [127,127,127]]
	],
	\&alpha2col;

exit main();	
	


