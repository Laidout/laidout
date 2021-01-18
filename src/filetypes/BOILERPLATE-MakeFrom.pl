#!/usr/bin/perl

##
## ./BOILERPLATE-CLASS.pl Name [boilerplate_base]
##   -> makes nameinterface.h, nameinterface.cc
##      from boilerplate template .h and .cc
##

$boilerplate_h  = "BOILERPLATE-filter.h";
$boilerplate_cc = "BOILERPLATE-filter.cc";

$Name=$ARGV[0];
if ($Name eq "") {
	print "Usage: ./BOILERPLATE-CLASS.pl NameOfThing [boilerplate_base]\n";
	print "  boilerplate_base implies templates boilerplate_base.h and boilerplate_base.cc\n";
	exit;
}

if ($ARGV > 1) {
	$boilerplate_h  = "$ARGV[1].h";
	$boilerplate_cc = "$ARGV[1].cc";
}

$Name =~ s/Interface$//;

$name=lc($Name);
$NAME=uc($name);


$hfile="${name}.h";
$ccfile="${name}.cc";

$NAME_H=uc($hfile);
$NAME_H =~ s/\./_/;

###--------- header file -----------

open (HFILE, ">$hfile") or die "can't open $hfile";
open (BOILER_H, "<$boilerplate_h") or die "can't open $boilerplate_h!";

print "Creating $hfile\n";

while (defined($line = <BOILER_H>)) {
	$line =~ s/boilerplate/$name/g;
	$line =~ s/BoilerPlate/$Name/g;
	$line =~ s/BOILERPLATE/$NAME/g;

	print HFILE $line
}


close(BOILER_H);
close(HFILE);





###--------- cc file -----------

open (CCFILE, ">$ccfile") or die "can't open $ccfile";
open (BOILER_CC, "<$boilerplate_cc") or die "can't open $boilerplate_cc!";

print "Creating $ccfile\n";

while (defined($line = <BOILER_CC>)) {
	$line =~ s/boilerplate/$name/g;
	$line =~ s/BoilerPlate/$Name/g;
	$line =~ s/BOILERPLATE/$NAME/g;

	print CCFILE $line
}


close(BOILER_CC);
close(CCFILE);


