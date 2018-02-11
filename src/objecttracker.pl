#!/usr/bin/perl

#this file is used to track unmatched reference counting
#do laidout 2> temp, then run:
#./objecttracker.pl, which will make a file temp-parsed
#that contains a list of what objects were not deleted.

use FileHandle;


$temp           = "temp";
$temp_searched  = "temp-searched";
$temp_parsed    = "temp-parsed";

open(TEMPSEARCHED, ">$temp_searched");
if (open (INFILE, "<$temp")) {
	while ($line = <INFILE>) {
		if ($line =~ m/^.*(anObject tracker .*)/) {
			print TEMPSEARCHED "$1\n";
		}
	}
	close(INFILE);
}
close(TEMPSEARCHED);

system("sort $temp_searched -o $temp_searched");


open (TEMPSEARCHED, "<$temp_searched")   or die "can't open $temp_searched";
open (TEMPPARSED, ">$temp_parsed") or die "can't open $temp_parsed";

$line=<TEMPSEARCHED>;
while (defined($line)) {
	#print "$line\n";
	if ($line =~ /^[a-zA-Z ]*([0-9]*).*created/) {
		$num=$1;
		#print "num:".$num;
		#print "  $num created";
		if (!defined($line=<TEMPSEARCHED>)) {
			 #end of file reached
			print " $num created but NOT destroyed\n";
			break;
		}
		if (!($line =~ /^.*(\d*).*destroyed/)) {
			print " $num  created but NOT destroyed\n";
			print TEMPPARSED "$num  created but NOT destroyed\n";
			next;
			#if (!($line=<TEMPSEARCHED>)) { break; }
		} else {
			#print " and destroyed\n";
			print " $num  created and destroyed\n";
		}
	} else {
		if (!defined($line=<TEMPSEARCHED>)) { break; }
	}
}
close(INFILE);


print "\nNow look in temp-parsed for which objects were not deleted!\n";

