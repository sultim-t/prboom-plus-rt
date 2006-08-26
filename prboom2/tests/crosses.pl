#!/usr/bin/perl

use strict;
use warnings;

my $grid = shift;
$grid = 75 if (!defined($grid));

my $WAD = "crosses.wad";
my @LMPS = ("MAP01", "THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES",
	"SEGS", "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP");

my %lmp;

for (my ($s,$i) = (0,0); $i < $grid; $i++) {
	for (my $j = 0; $j < $grid; $j++) {
		rect($s++, 80*$i, 80*$j, 80*$i+64, 80*$j+64);
	}
}

$lmp{"THINGS"} = pack("S5", 32, 32, 0, 1, 7);

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

	$lmp{"VERTEXES"}.=pack("S2"x5,
		$x0, $y0,
		$x0, $y1,
		$x1, $y1,
		$x1, $y0,
		($x0+$x1)/2, ($y0+$y1)/2);

	$lmp{"LINEDEFS"}.=pack("S7"x8,
		5*$n,   5*$n+1, 1, 0, 0, 12*$n,   65535,
		5*$n+1, 5*$n+2, 1, 0, 0, 12*$n+1, 65535,
		5*$n+2, 5*$n+3, 1, 0, 0, 12*$n+2, 65535,
		5*$n+3, 5*$n,   1, 0, 0, 12*$n+3, 65535,
		5*$n,   5*$n+4, 4, 0, 0, 12*$n+4, 12*$n+5,
		5*$n+1, 5*$n+4, 4, 0, 0, 12*$n+6, 12*$n+7,
		5*$n+2, 5*$n+4, 4, 0, 0, 12*$n+8, 12*$n+9,
		5*$n+3, 5*$n+4, 4, 0, 0, 12*$n+10,12*$n+11);

	$lmp{"SIDEDEFS"}.=pack("S2a8a8a8S"x12,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "STARTAN3", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n,
		0, 0, "-", "-", "-", $n);
}

__END__
