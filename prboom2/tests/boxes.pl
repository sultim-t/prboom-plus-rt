#!/usr/bin/perl

use strict;
use warnings;

my $SQUARES = 100;
my $WAD = "boxes.wad";
my @LMPS = ("MAP01", "THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES",
	"SEGS", "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP");

my %lmp;

for (my ($s,$i) = (0,0); $i < $SQUARES; $i++) {
	for (my $j = 0; $j < $SQUARES; $j++) {
		rect($s++, 16*$i, 16*$j, 16*$i+8, 16*$j+8);
	}
}

$lmp{"THINGS"} = pack("S5", 4, 4, 0, 1, 7);

open(F, ">$WAD");
my $ptr = 12 + 16*scalar @LMPS;
print F pack("a4L2", "PWAD", scalar @LMPS, 12);
for (@LMPS) {
	if (exists $lmp{$_}) {
		print F pack("L2a8", $ptr, length $lmp{$_}, $_);
		$ptr += length $lmp{$_};
	} else {
		print F pack("L2a8", $ptr, 0, $_);
	}
}
for (@LMPS) { print F $lmp{$_} if exists $lmp{$_}; }
close F;

sub rect
{
	my ($n, $x0, $y0, $x1, $y1) = @_;

	$lmp{"SECTORS"}.=pack("S2a8a8S3",
		0, 128, "FLOOR4_8", "CEIL3_5", 192, 0, 0);

	$lmp{"VERTEXES"}.=pack("S2"x4,
		$x0, $y0,
		$x0, $y1,
		$x1, $y1,
		$x1, $y0);

	$lmp{"LINEDEFS"}.=pack("S7"x4,
		4*$n,   4*$n+1, 1, 0, 0, 4*$n,   65535,
		4*$n+1, 4*$n+2, 1, 0, 0, 4*$n+1, 65535,
		4*$n+2, 4*$n+3, 1, 0, 0, 4*$n+2, 65535,
		4*$n+3, 4*$n,   1, 0, 0, 4*$n+3, 65535);

	$lmp{"SIDEDEFS"}.=pack("S2a8a8a8S"x4,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n);
}

__END__
