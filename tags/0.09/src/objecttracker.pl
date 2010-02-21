#!/usr/bin/perl

#this file is used to track unmatched reference counting
#do laidout 2> temp, then run:
#./objecttracker.pl > temp-not-deleted, which will
#then contain a list of what was deleted or not. Go through that looking
#for "NOT".....

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
	#print "$line\n";
	if ($line =~ /^[a-zA-Z ]*([0-9]*).*created/) {
		$num=$1;
		#print "num:".$num;
		#print "  $num created";
		if (!defined($line=<OUTFILE>)) {
			 #end of file reached
			print " $num created but NOT destroyed\n";
			break;
		}
		if (!($line =~ /^.*(\d*).*destroyed/)) {
			print " $num  created but NOT destroyed\n";
			print OUT2FILE "$num  created but NOT destroyed\n";
			next;
			#if (!($line=<OUTFILE>)) { break; }
		} else {
			#print " and destroyed\n";
			print " $num  created and destroyed\n";
		}
	} else {
		if (!defined($line=<OUTFILE>)) { break; }
	}
}
close(INFILE);

