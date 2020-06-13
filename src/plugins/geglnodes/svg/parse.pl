#!/usr/bin/perl

#
# Parse svg filters
#
# in:  svg.rnc
# out: stdout
#
#


my $file = "svg.rnc";

open ($INFILE, "<$file") || die "Can't open $file!\n";


$stage = 0;

@primitives = ();
@primitiveatts = ();

@classes = ( "FilterPrimitive", "feFuncR", "feFuncG", "feFuncB", "feFuncA",
			 "feDistantLight",  "fePointLight", "feSpotLight", #feSpecularLighting, feDiffuseLighting
			 "feMergeNode" #for feMerge
			);
#@classes = ( "FilterPrimitive", "FilterPrimitiveWithIn" );
#print "  classes: ".(scalar (@classes))."\n";

while ($line = <$INFILE>) {

	if ($line =~ m/^SVG.FilterPrimitive.class/) {
		while ($line = <$INFILE> and $line =~ m/^ /) {
			$line =~ m/([a-zA-Z0-9.]+)/;
#			print "primitive: $1\n";
			$class = $1;

			if (!($line =~ m/^SVG./)) {
				push(@primitives, $class);
				push(@classes, $class);
#				print "  classes: ".(scalar (@classes))."\n";
			}
		}
		$stage = 1;
		break;
	}
}


#for my $class (@classes) {
#	print "looking for class: $class...\n";
#}

#print "\n";
print "#Laidout namespace\n";
print "name SvgFilters\n";
print "format class\n";



for my $class (@classes) {
#	print "looking for $class...\n";

	seek $INFILE, 0, SEEK_SET;

	$contents = "";
	@atts = ();

	while ($line = <$INFILE>) {

		#if ($line =~ m/^SVG.FilterPrimitive.attrib/) {
		if ($line =~ m/^SVG.$class.content(.*)/) {
			$contents = $1;

			while ($line = <$INFILE> and $line =~ m/^ /) {
				$line =~ m/^  (.*)$/;
				#$line =~ s/\,//;
				#print "$1\n";

				$lline = $1;
				$lline =~ s/,//;
				if ($line =~ m/{/) {
					while ($line =~ m/^ / and !($line =~ m/\}/)) {
						if ($line = <$INFILE>) {
							$line =~ m/^  (.*)$/;
							$lline .= $1;
						}
					}
				}
				$contents .= $lline;
			}

#			print "  contents of $class: $contents\n";


		}

		if ($line =~ m/^SVG.$class.attrib/ or $line =~ m/^attlist.$class /) {

			while ($line = <$INFILE> and $line =~ m/^ /) {
				$line =~ m/^  (.*)$/;
				#$line =~ s/\,//;
				#print "$1\n";

				$lline = $1;
				$lline =~ s/,//;
				if ($line =~ m/{/) {
					while ($line =~ m/^ / and !($line =~ m/\}/)) {
						if ($line = <$INFILE>) {
							$line =~ m/^  (.*)$/;
							$lline .= $1;
						}
					}
				}
				push(@atts, $lline);
#				unshift(@atts, $lline);
			}
		}
	} #while more lines


	print "class $class\n";
#			print "  name $class\n";
#			print "  format class\n";
	@extends = ();

	$hasin = 0;
	for my $att (@atts) {
		if ($class ne 'FilterPrimitiveWithIn' and $class ne 'FilterPrimitive'
				and $att =~ m/FilterPrimitiveWithIn/) {
			$hasin = 1;
			#push(@extends, "FilterPrimitiveWithIn");
			push(@extends, "FilterPrimitive");

		} elsif ($class ne 'FilterPrimitiveWithIn' and $class ne 'FilterPrimitive'
				and $att =~ m/FilterPrimitive/) {
			push(@extends, "FilterPrimitive");
		}
	}
	if (scalar @extends > 0) {
		print "  extends ".(join(', ', @extends))."\n";
	}

	$contents =~ s/^[ =,|*()?]+//;
	$contents =~ s/[ =,|*()?]+$//;
#	print "contents: $contents\n";

	if ($contents ne "") {
		@elements = split(/[ =,|*()?]+/, $contents);
		@els = ();
		
		for my $element (@elements) {
			#print "check $element\n";
			if (scalar grep(/^$element$/, @classes) > 0) {
				push(@els, $element);
			}
		}

		if (scalar @els > 0) {
			print "  uihint kids(".join(", ", @els).")\n";
		}
	}


	if ($hasin) {
		print "  string in\n";
#				print "  field string\n";
#				print "    name in\n";
#				print "    format string\n";
	}

	$defaultValue = "";

	if ($class eq "feFlood") {
		#for simplicity, manually add flood-color and flood-opacity
		print"  color flood-color\n";
		print"  color flood-opacity\n";
	}

	for my $att (@atts) {
#		print "    att: $att\n";

		if ($att =~ m/attribute ([a-zA-Z0-9]*) \{([.a-zA-Z0-9 ]*)\}/) {

			$name   = $1;

			$format = $2;
			$format =~ s/^\s+|\s+$//g;
			if ($format eq "NumberOptionalNumber.datatype") { $format = "number"; }
			elsif ($format eq "Coordinate.datatype") { $format = "number"; }
			elsif ($format eq "Length.datatype")     { $format = "number"; }
			elsif ($format eq "Number.datatype")     { $format = "number"; }
			elsif ($format eq "text")                { $format = "string"; }
			elsif ($format eq "Integer.datatype")    { $format = "int";    }
			elsif ($format eq "Boolean.datatype")    { $format = "boolean";}
			elsif ($format eq "PreserveAspectRatioSpec.datatype") { $format = "string";}


#			print "  field $format\n";
#			print "    name   $name\n";
#			-----
			print "  $format $name\n";
			if ($defaultValue ne "") {
				print "    defaultValue $defaultValue\n";
				$defaultValue = "";
			}
#			print "      format $format\n";


		} elsif ($att =~ m/attribute ([a-zA-Z0-9]*) \{(.*)\}/) {

			my $name = $1;
			my $names = $2;

			print "  enum $name\n";
			if ($defaultValue ne "") {
				print "    defaultValue $defaultValue\n";
				$defaultValue = "";
			}
#			print "      format enum\n";

			if ($names =~ /|/) {
				#print "split: $type\n";
				@enums = split('\|', $names);

				#for $enum (@enums) {
				#	$enum =~ s/^\s+|\s+$//g
				#}
				for $enum (@enums) {
					$enum = trimws($enum);
					print "    enumval $enum\n";
#					print "      name   $enum\n";
#					print "      format enumval\n";
				}
			}

		} elsif ($att =~ m/defaultValue = "([ a-zA-Z0-9]*)"/) {
			$defaultValue = $1;
#			print "    setting defaultValue: $1\n";

		} else {
#			print "    field OTHER: $att\n";
		}
	}
}

$stage = 0;


sub trimws {
	my $str = $_[0];
	$str =~ s/^\s*//;
	$str =~ s/\s*$//;
	return $str;
}

#print "\n\n";
#print "Filter primitives: \n";
#for my $fe (@primitives) {
#	#print "  $fe\n";
#
#	seek $INFILE, 0, SEEK_SET;
#
#	while ($line = <$INFILE>) {
#		if ($line =~ m/^attlist\.$fe/) {
#			break;
#		}
#	}
#}





close(INFILE);


