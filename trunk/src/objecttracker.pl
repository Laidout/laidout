#!/usr/bin/perl

#this file is used to track unmatched reference counting
#do laidout > temp, then run this file, and it will result in listing the
#object numbers of undeleted objects

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
while (defined($line)) {
	print "$line\n";
	if ($line =~ m/^.*(\d*).*created/) {
		$num=$1;
		print "  $num created";
		if (!defined($line=<OUTFILE>)) {
			print " but NOT destroyed\n";
			break;
		}
		if (!($line =~ m/^.*(\d*).*destroyed/)) {
			print " but NOT destroyed\n";
			print OUT2FILE "$num  created but NOT destroyed\n";
			if (!($line=<OUTFILE>)) { break; }
		} else {
			print " and destroyed\n";
		}
	} else {
		if (!defined($line=<OUTFILE>)) { break; }
	}
}
close(INFILE);

