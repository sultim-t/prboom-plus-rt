#!/usr/bin/perl
$write = 0;
while ($line = <>) {
  chop($line);
  if ($write == 1) {
    if ($line =~ /^?END BEX/) {
      $write = 0;
    }
    else
    {
      print $line, "\n";
    }
  }
  else
  {
    if ($line =~ /^?BEX FORMAT/) {
      $write = 1;
    }
  }
}