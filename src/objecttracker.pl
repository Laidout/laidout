#!/usr/bin/perl

use FileHandle;


$file     = "temp";
$outfile  = "temp-searched";
$out2file = "temp-parsed";

open(OUTFILE, ">$outfile");
if (open (INFILE, "<$file")) {
	while ($line = <INFILE>) {
		if ($line =~ m/^.*(anObject tracker .*)/) {
			print OUTFILE "$1\n";
		}
	}
	close(INFILE);
}
close(OUTFILE);

system("sort $outfile -o $outfile");


open (OUTFILE, "<$outfile")   or die "can't open $outfile";
open (OUT2FILE, ">$out2file") or die "can't open $out2file";

$line=<OUTFILE>;
while (1) {
	print "$line\n";
	if ($line =~ m/^.*(\d*).*created/) {
		$num=$1;
		print "  $num created";
		if (!($line=<OUTFILE>)) {
			print " but NOT destroyed\n";
			break;
		}
		if (!($line =~ m/^.*(\d*).*destroyed/)) {
			print " but not destroyed\n";
			print OUT2FILE "$num  created but not destroyed\n";
			if (!($line=<OUTFILE>)) { break; }
		} else {
			print " and destroyed\n";
		}
	} else {
		if (!($line=<OUTFILE>)) { break; }
	}
}
close(INFILE);

