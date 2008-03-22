#!/usr/bin/perl

use strict;
use warnings;

use Getopt::Std; $Getopt::Std::STANDARD_HELP_VERSION = 1;

sub VERSION_MESSAGE
{
  print <<"EOF";
lmpwatch.pl: a clone of PrBoom-Plus's automated demo playback system.
EOF
}

sub HELP_MESSAGE
{
  print <<"EOF";
Usage: $0 [OPTIONS] DEMO_FILES...

Options:
    -d x    name of doom executable (try "echo" to test the program)
    -e x    any extra options to pass to doom
    -p x    name of patterns file from lmpwatch.zip
    -s x    colon- or space-separated search path, may contain wildcards
    -t      test demo sync (not fully implemented, needs engine-side support)
    -v      print lots of debugging output

EOF
}

getopts('d:e:p:s:tv', \my %opts) or do {
  HELP_MESSAGE();
  die "$0: bad options\n"
};
if (!@ARGV) {
  HELP_MESSAGE();
  die "$0: no demos?\n";
}

my $verbose = defined($opts{'v'});

# name of doom executable
my $DOOM = (defined $opts{'d'}) ? $opts{'d'} : "prboom";
# any extra options to pass?
my $extra = (defined $opts{'e'}) ? $opts{'e'} : "";

# are we playing back demos or testing them for demosync?
my $testing = defined($opts{'t'});
# "batch mode" if you're testing a demo (turns off the renderer)
my $demo_opts = $testing ? "-nodraw -nosound -timedemo" : "-playdemo";

# paths to search for files, these will be subject to filename globbing
my @paths = do {
  # expand colon- or space-separated string into list
  my @s = defined($opts{'s'}) ? split(/[: ]+/, $opts{'s'}) : ();

  # expand paths that contain wildcards
  map { glob $_ } grep { defined $_ } (@s, $ENV{"DOOMWADDIR"},
    "/usr/local/share/games/doom", "/usr/share/games/doom");
};
print "DIRS: @paths\n\n" if $verbose;

# name of the patterns file from lmpwatch.zip
my $patsfile = do {
  my $p = (defined $opts{'p'}) ? $opts{'p'} : "patterns.txt";
  path_expand($p, @paths);
} or die "$0: lmpwatch.zip patterns file is needed to use this program\n";
print "PATTERNS: $patsfile\n\n" if $verbose;
open my ($PATTERNS), $patsfile;

while (@ARGV) {
  my $demo = path_expand(shift @ARGV, @paths) or next;
  my ($comment, $iwad, @files) = parse_patterns($PATTERNS, $demo);
  print "DEMO: $demo\nIWAD: $iwad\nFILES: @files\n" if $verbose;

  # filename globbing and switch prepending for wad files
  $iwad = path_expand($iwad, @paths) or next; # not strictly necessary
  @files = map { path_expand($_, @paths) or next; } @files;
  my @deh = grep { /\.(deh|bex)$/i } @files;
  unshift @deh, "-deh" if @deh;
  my @wad = grep { /\.wad$/i } @files;
  unshift @wad, "-file" if @wad; # should be "-merge" for chocolate-doom?

  my $command = sprintf("%s -iwad %s %s %s %s %s %s",
    $DOOM, $iwad, "@wad", "@deh", $demo_opts, $demo, $extra);
  print "RUNNING: $command\n" if $verbose;

  if ($testing) {
    my @finished = test_demo($command);
    # unfortunately there is not yet an easy way to test if the
    # list of maps the player exited is correct. lmpwatch.zip is
    # only meant for demo playback, not testing.
    # just print out the names of maps that were completed
    print "FINISHED: " if $verbose;
    print join(" ", (split(m!/!, $demo))[-1], @finished), "\n";
  } else {
    system($command);
  }
  print "\n" if $verbose;
}

close $PATTERNS;

exit 0;

# --------------------------------------------------------------------------

# play back a demo, return list of maps the player finishes
# requires that the engine prints "FINISHED: <mapname>" when a map is beaten
sub test_demo
{
  my ($command) = @_;
  my @finished;

  # run the game and capture its output
  open my ($doom), "$command 2>&1 |";
  # make a list of maps the player managed to exit
  # this needs engine-side support
  while (<$doom>) {
    push @finished, $1 if m/^FINISHED: (.*)$/
  }
  close $doom;

  return @finished;
}

# parse lmpwatch.zip's patterns file to determine wads required to play a demo
sub parse_patterns
{
  my ($PATTERNS, $demofile) = @_;
  my ($mask, $comment, $pattern, $iwad, @files);

  my ($demo) = (split m!/!,$demofile)[-1];

  seek $PATTERNS, 0, 0; # rewind file to start
  while (<$PATTERNS>) {
    chomp;
    # read valid key/value pairs
    my ($key, $value) = m/^(\w+)\s+\"(.+)\"/;
    next if !defined($key) || !defined($value);

    # set the pattern mask
    $mask = $value, next if $key eq 'demo_patterns_mask';

    # ignore keys that don't start with the pattern mask
    next if !$mask || $key !~ m/^$mask/;

    # decode the pattern
    ($comment, $pattern, my $files) = split(m!/!, $value);
    # the first file is always the iwad, apparently
    ($iwad, @files) = split(m!\|!, $files);
    next if !$iwad; # no iwad??

    # stop at the first match (conflict avoidance)
    last if $demo =~ m/$pattern/
  }

  return ($comment, $iwad, @files);
}

# from the list supplied, find the first directory a given filename is in
sub path_expand
{
  my $file = shift;

  # if it's already a valid filename, skip the searching
  return $file if -f $file;

  print " SEARCHING: $file\n" if $verbose;
  for my $dir (@_) {
    my $path = "$dir/$file";
    print "  TRYING: $path\n" if $verbose;
    if (-f $path) {
      print " FOUND: $file => $path\n" if $verbose;
      return $path;
    }
  }
  warn "$0: cannot find $file\n";
  return undef;
}

# vim:set sts=2 sw=2 ts=8 et:
