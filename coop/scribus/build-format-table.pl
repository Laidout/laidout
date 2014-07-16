#!/usr/bin/perl


#This file will convert an indented format of essentially xml to an html table.
#You may also import and export wiki table formatted data.
#This is mainly to make it easy to make documention of xml formatted files.
#It will adjust html column and row spans to give a basically meaningful display of
#the relationships of elements and attributes.
#
#So for instance, something like:
#name
#  name2 #Description of 2
#  name3 #Description of 3
#  name3 default-value #(optional) Description of 3
#  elements:
#    name4
#    name5
#...would correspond to this xml:
#<name name2="whatever" name3="whatever">
#  <name4/>
#  <name5/>
#</name>
#The comments after the '#' will be put alongside an html table of the elements and attributes. 
#Comments beginning with "##" at the start of a line will be ignored.
#
# usage:
#  Input formats can be:   .att  
#  Output formats can be:  .html .wiki .dtd
#
#  ./build-format-table.pl  infile.att   outfile.html 
#  ./build-format-table.pl  infile.att   outfile.wiki 
#
#Written by Tom Lechner (tomlechner.com).
#Copyright (c) 2009,2010, 2013
#
#Permission is hereby granted, free of charge, to any person
#obtaining a copy of this software and associated documentation
#files (the "Software"), to deal in the Software without
#restriction, including without limitation the rights to use,
#copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the
#Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be
#included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#OTHER DEALINGS IN THE SOFTWARE.
#
#
#Todo:
#  adapt to also use with Laidout file format dump
#
#---------------------------------------------------------------------

if ($ARGV[0] eq "--pure-att" or $ARGV[0] eq "-a") {
	$allelements=1;
	$ARGIN  = $ARGV[1];
	$ARGOUT = $ARGV[2];
} else {
	$allelements=0;
	$ARGIN  = $ARGV[0];
	$ARGOUT = $ARGV[1];
}


#$infile="testformat.att";
$infile=$ARGIN;

if ($infile eq "") {
	die << "THEEND";
usage:
 --pure-att    <- Use for pure att. Otherwise, att is formatted similar to xml

 Input formats can be:   .att  
 Output formats can be:  .html .wiki .dtd

 ./build-format-table.pl  infile.att  outfile.html
 ./build-format-table.pl  infile.att  outfile.wiki
 ./build-format-table.pl  infile.att  outfile.dtd
THEEND
}


#---what kind of thing to output to
$outfile=$ARGOUT;
if ($outfile eq "") { $outfile="$infile.html"; }

if ($outfile =~ m/\.html$/) {
	$htmlfile=$outfile;
} else { $htmlfile=""; }

if ($outfile =~ m/\.wiki$/) {
	$wikifile=$outfile;
} else { $wikifile=""; }

if ($outfile =~ m/\.dtd$/) {
	$dtdfile=$outfile;
} else { $dtdfile=""; }


#---what kind of thing to input from
if ($infile =~ m/\.wiki$/) {
	$inwiki=$infile;
	$infile="";
} else { $inwiki=""; }

if ($infile =~ m/\.dtd$/) {
	$indtd=$infile;
	$infile="";
} else { $indtd=""; }

#else assume it is an att


#---summary
if ($infile ne "") { print "In: $infile\n"; }
if ($inwiki ne "") { print "In: $inwiki (wiki)\n"; }
if ($indtd  ne "") { print "In: $indtd (dtd)\n"; }

if ($htmlfile ne "") { print "htmlout: $htmlfile\n"; }
if ($wikifile ne "") { print "wikiout: $wikifile\n"; }
if ($dtdfile  ne "") { print "dtdout:  $dtdfile\n"; }
print "\n";
die "Output where!?\n" if ($dtdfile eq "" and $htmlfile eq "" and $wikifile eq "") ;




# 1. read in whole file to nested objects with elements as children, and attributes as string list
#  It is necessary to read it all in to get rowspans, and depth of tree
# 2. output the table!

#attribute has:
#  name
#  description
#  optional (can be "zero or one" or "optional" or "required")
#  owner
#
#element has:
#  name
#  description
#  optional (can be "one only", "zero or one", "zero or more", "one or more", or "required")
#  owner
#  subelements
#  attributes

#------------------------ READ IN --------------------------------
$initialnotes="";

%mainnode={};

$maxdepth=0;
$depth=0;
$linenum=0;
$maybemoredesc=0; #if line indented with more comment: /^ +#(.*)$/, append to desc
$lastindent=0;
$reading_attributes=0;
$currentelement=0;
$currentindent=-1; #set to -1 with each new element or an "elements:" section
$desc="";

#------------------------ read in att file --------------------------------

if ($infile ne "") {
	open(INFILE,$infile) or die "cannot open $infile\n";

	while (defined($line = <INFILE>)) {
		$linenum++;

		if ($line =~ /^##/) {
			#print "skipping comment starting with \"##\"...\n";
			next;
		}

		if ($line =~ /^\s*$/) {
			#print "skipping blank line $linenum...\n";
			next;
		}

		 #intercept continuing comments before doing anything else
		if ($maybemoredesc==0) { 
			#print "skipping check for more comments\n"; 
		}
		if ($maybemoredesc!=0) {
			#print "--maybe adding comment to ".$maybemoredesc->{"name"}."\n";
			if ($line =~ /^ +#(.*)$/) {
				#add to comment
				#print "---adding to comment: $1\n";
				$desc=$1;

				 #for elements that were something like: 
				 #  blah option
				 #  0 = 1st
				 #  1 = second
				 #this substition provides the proper <br> tags
				$desc=~ s/([0-9]*)\s*=/<br\/>\n\1 =/g;
				$maybemoredesc->{"comment"}=$maybemoredesc->{"comment"}." $desc";
				#print "---updated comment: ".$maybemoredesc->{"comment"}."\n";

				$desc="";
				next;
			}
			#print "---didn't add to comment\n";
			$maybemoredesc=0;
		}


		 #append any initial notes
		if ($line =~ /^#(.*)$/) {
			my $add=trim($1);
			if ($add eq "") { $add="<br/>\n<br/>\n"; }
			$initialnotes.="\n$add";
			#print "Adding initial comments: $add";
			next;
		}

		#if last indent was less than this indent, then we have attributes or elements of the current element
		#if (name=="elements:") we are done reading in attributes, so now add sub elements
		#else *** add more attributes

		$desc="(none)";
		if ($currentelement!=0) { $desc=$currentelement->{"name"}; }
		#print "line lastindent:$lastindent, curE:\"$desc\"  $linenum:\"$line\"";

		$line =~ /(\s*)(\S.*)$/;  #get (indent) (non-indent)
		$lineindent=length($1);
		$line = trim($2);

		#print "indent:$lineindent   rest of line:\"$line\"\n";

		if ($line =~ /([^#]*)(#.*)$/) {  #get (name) (#final comment)
			$linename   =rtrim($1); #if "" then append any comment found to current name
			$linecomment=$2; #comment could be "#blah" or "#(optional) blah" 
			
			if ($linename =~ /(\S*)\s+(.*)/) {
				$defaultvalue=trim($2);
				$linename=$1;
			} else { $defaultvalue=""; }

			if ($linecomment =~ /^#\(([^)]*)\)(.*)$/) {
				$linecomment=$2;

				if ($1 eq "optional") { $optional="zero or one"; }
				elsif ($1 eq "zero or one")  { $optional="zero or one"; }
				elsif ($1 eq "one or more")  { $optional="one or more"; }
				elsif ($1 eq "zero or more") { $optional="zero or more"; }
				elsif ($1 eq "required")     { $optional="one only"; }
				elsif ($1 eq "one only")     { $optional="one only"; }
				else { $optional="zero or one"; }
			#} elsif ($linecomment =~ /^#\(default="blah blah"\)(.*)$/) {
			#} elsif ($linecomment =~ /^#\(fixed "blah blah"\)(.*)$/) {
			} else {
				$linecomment=substr $linecomment, 1; #everything after initial '#'
				$optional="one only";
			}
		} else {  #make sure comment is "" if none given
			#print "--no comment for $linename\n";
			$optional="one only";
			$linecomment="";
			$linename=$line;

			if ($linename =~ /(\S*)\s+(.*)/) {
				$defaultvalue=trim($2);
				$linename=$1;
			} else { $defaultvalue=""; }

		}

		# we have now scanned the line, and parsed into indent, name, and comment
		# we just need to deal with those now!
		#print "line:$linenum, name:$linename  optional:$optional  indent:$lineindent  comment:\"$linecomment\"\n";

		if ($linename eq "") {
			 #no name
			$lineindent=1000000;
			#note: shouldn't ever come here....
			#print $currentelement;
			die "***after:".$currentelement->{"name"}."**************shouldn't be here!\n";
			next;
		} else {
			if ($linename eq "elements:") {
				#print "curi:$currentindent   li:$lineindent\n";
				if ($reading_attributes==1 && $lineindent!=$currentindent && $currentindent!=-1) {
					die "**********should not be reading attributes of an attribute!\n";
				}
				#print "--found elements section for ".$currentelement->{"name"}."\n";
				$currentindent=-1;
				$reading_attributes=0;
				$maybemoredesc=0;
				$depth++;
				if ($depth>$maxdepth) { $maxdepth=$depth; }
				next;
			}

			if ($currentelement==0) {
				 #create first node
				%mainnode={};
				$mainnode{"name"}=$linename;
				$mainnode{"owner"}=0;
				$mainnode{"indent"}=$lineindent;
				$mainnode{"childindent"}=-1;
				$mainnode{"isattribute"}=0;
				$mainnode{"optional"}=$optional;
				$mainnode{"comment"}=$linecomment;
				$mainnode{"subelements"}=[];
				$mainnode{"attributes"}=[];
				$currentelement=\%mainnode;
				$depth=1;
				$maxdepth=1;

				$lastindent=$lineindent;

				$maybemoredesc=$currentelement;
				if ($allelements!=1) { $reading_attributes=1; }
				#print "created first node from line $linenum.\n";
				next;
			}
			#print "line:$linenum, line data: $line\n";

			 #now stand alone comments have been dealt with, so we need to
			 #append either a new attribute or a new element, and we have to figure
			 #out where to append it
			if ($lineindent<$lastindent) {
				#print "--moving up from:".$currentelement->{"name"}."\n";
				#close off current element, make parent element the current element

				#print "curi:".$currentelement->{"indent"}."   li:$lineindent\n";
				while ($lineindent<$currentelement->{"indent"}) {
					$currentelement=$currentelement->{"owner"};
					$depth--;
					#print "----back 1 to ".$currentelement->{"name"}."\n";
				}
				#if ($lineindent==$currentelement->{"indent"}}  {
				#	$currentelement
				#} else {
				#	$lastindent=$currentelement->{"indent"};
				#}
				$lastindent=$currentelement->{"indent"};
				$reading_attributes=0;
				#print "---moved up to: ".$currentelement->{"name"}."\n";
			} elsif ($lineindent>$lastindent) {
				if ($currentindent<0) {
					 #we have a 1st attribute for currentelement
					 # or 1st element in "elements:" section
					$currentindent=$lineindent;
					$lastindent=$lineindent;
				} elsif ($reading_attributes==1) { 
					 #if we are reading attributes, and the attribute appears to have an
					 #an attribute, it is an error
					die "error inputting: attributes cannot have sub elements!!"; 
				}
			} else { # $lineindent==$lastindent
				if ($reading_attributes==1 && $currentelement->{"indent"}==$lineindent
						&& $currentelement->{"isattribute"}==0) { $reading_attributes=0; }
			}

			#print "Current element: ".$currentelement->{"name"}."\n";
			 #append more elements or attributes
			$newelement={};
			$newelement->{"name"}=$linename;
			$newelement->{"indent"}=$lineindent;
			$newelement->{"childindent"}=-1;
			$newelement->{"optional"}=$optional;
			$newelement->{"comment"}=$linecomment;
			$newelement->{"default"}=$defaultvalue;
			$newelement->{"subelements"}=[];
			$newelement->{"attributes"}=[];
			#print "Current element: ".$currentelement->{"name"}."\n";

			if ($reading_attributes==0) { #another element of $currentelement->owner
				$newelement->{"isattribute"}=0;
				#print "Current element a: ".$currentelement->{"name"}."\n";
				if ($lineindent==$currentelement->{"indent"}) { $currentelement=$currentelement->{"owner"}; }
				#$currentelement=$currentelement->{"owner"};
				#print "Current element b: ".$currentelement->{"name"}."\n";
				$newelement->{"owner"}=$currentelement;
				#print "----new element \"".$newelement->{'name'}."\", parent:".$currentelement->{"name"}."----\n";
				$arrayref=$currentelement->{"subelements"};
				push(@{$arrayref},$newelement);
				$currentelement=$newelement;
				$maybemoredesc=$currentelement;
				$currentindent=-1;
				if ($allelements!=1) { $reading_attributes=1; }
			} else { #push attribute
				#print "----new attribute ".$newelement->{"name"}." for ".$currentelement->{"name"}."----\n";
				$newelement->{"owner"}=$currentelement;
				$newelement->{"isattribute"}=1;
				$arrayref=$currentelement->{"attributes"};
				push(@{$arrayref}, $newelement);
				$maybemoredesc=$newelement;
				$currentindent=$lineindent;
				$reading_attributes=1;
			}
		}
	}
	close (INFILE);
}



#------------------------ read in wiki file --------------------------------
if ($inwiki ne "") {
	open(INWIKI,$inwiki) or die "cannot open $inwiki!!\n";


	close (INWIKI);

	die " **** wiki text input todo!\n";
}




#------------------------ read in dtd file --------------------------------
if ($inwiki ne "") {
	open(INDTD,$indtd) or die "cannot open $indtd!!\n";


	close (INDTD);

	die " **** dtd input todo!\n";
}






$depth=$maxdepth;
computedepth(\%mainnode);


#------------------------ write out att to screen --------------------------------
#FOR DEBUGGING:
#print "\n\n----------------Attribute out---------------\nDepth:$maxdepth\n";
#dumpatt(0,\%mainnode);


#------------------------ write out html file --------------------------------
if ($htmlfile ne "") {
	print "Writing out html table to \"$htmlfile\"...";
	open(OUTHTML, ">$htmlfile");

	print OUTHTML "<html>\n<head>\n<title>$infile</title>\n\n";

	print OUTHTML << "INHTML";
<style type="text/css">
table.formattable {
	border-width: 1px 1px 1px 1px;
	border-spacing: 0px;
	border-style: outset outset outset outset;
	border-color: gray gray gray gray;
	border-collapse: collapse;
	#background-color: white;
}
table.formattable th {
	border-width: 1px 1px 1px 1px;
	padding: 1px 1px 1px 1px;
	border-style: inset inset inset inset;
	border-color: gray gray gray gray;
	#background-color: white;
	-moz-border-radius: 0px 0px 0px 0px;
}
table.formattable td {
	border-width: 1px 1px 1px 1px;
	padding: 1px 1px 1px 1px;
	border-style: inset inset inset inset;
	border-color: gray gray gray gray;
	padding: 3px 3px 3px 3px;
	#background-color: white;
	-moz-border-radius: 0px 0px 0px 0px;
}
</style>
INHTML
	print OUTHTML "</head>\n<body>\n";

	print OUTHTML "<div>\n$initialnotes\n</div>\n\n";
	print OUTHTML "<br/><div>\n<table class=\"formattable\">\n".
				  "  <tr valign=\"top\">\n".
				  "    <td colspan=\"$depth\" align=\"center\"><b>Tags</b></td>\n".
				  "	   <td><b>Attributes</b></td>\n".
				  "    <td><b>Meaning</b></td>\n".
				  "  </tr>\n";

	$rowcol=0;
	$tdcolor0="#000000";
	$tdcolor1="#222222";
	$currenttdcolor=$tdcolor0;
	dumphtmlatt(\%mainnode);


	print OUTHTML "</div>\n</table>\n</body>\n</html>";

	close (OUTHTML);
	print "done!\n";
}

 #called for each element needing to be output
sub dumphtmlatt
{
	my $att=$_[0];

	my $numatts=scalar(@{$att->{"attributes"}});

	print OUTHTML "  <tr>\n";
	my $currentdepth = dumpPreviousElements($att,1+$numatts);
	my $bgcolor="#ffffff"; #"#A9D1FF"
	my $cbgcolor="#ffffff"; #"#A9D1FF"

	 #print out 1 row for element name and comment if any
	if ($att->{"optional"} eq "one only" or $att->{"optional"} eq "one or more" ) { $bgcolor="#eeeeee"; } else { $bgcolor="#A9D1FF"; }
	print OUTHTML "    <td colspan=\"".($depth-$currentdepth+2)."\" style=\"background:$bgcolor;color:$currenttdcolor\">";
	print OUTHTML $att->{"name"}."</td>\n";
	print OUTHTML "    <td colspan=\"1\" style=\"background:$bgcolor;\">";
	if ($att->{"comment"} ne "") { print OUTHTML $att->{"comment"}; }
	else { print OUTHTML "&nbsp;"; }
	print OUTHTML "</td>\n";
	print OUTHTML "  </tr>\n"; #end of element name row
	
	 #output all attributes of the element
	if ($att->{"optional"} eq "one only" or $att->{"optional"} eq "one or more" ) { $bgcolor="#ffffff"; } else { $bgcolor="#c9e4FF"; }
	if (scalar(@{$att->{"attributes"}})>0) {
		print OUTHTML "  <tr>\n      <td style=\"background:$bgcolor\" colspan=\"".($depth-$currentdepth+1)
					 ."\" rowspan=\"".$numatts."\">&nbsp;&nbsp;&nbsp;</td>\n";
	}
	my $c=0;
	foreach $a (@{$att->{"attributes"}}) {
		if ($c>0) { print OUTHTML "  <tr>\n"; }
		$c++;
		if ($a->{"optional"} eq "one only" or $a->{"optional"} eq "one or more" ) { $bgcolor="#ffffff"; } else { $bgcolor="#c9e4FF"; }
		if ($a->{"comment"} eq "") { $cbgcolor="#ffaaaa"; } else { $cbgcolor=$bgcolor; }
		print OUTHTML "    <td style=\"background:$bgcolor\">".$a->{"name"}."</td><td style=\"background:$cbgcolor\">";
		if ($a->{"default"} ne "") { print OUTHTML "Default value: ".$a->{"default"}.".<br/>\n"; }
		if ($a->{"comment"} ne "") { print OUTHTML $a->{"comment"}; }
		else { print OUTHTML "<blink>DOCUMENT ME!!</blink>"; }
		#else { print OUTHTML "&nbsp;"; }
		print OUTHTML "</td></tr>\n";
	}

	#output all sub elements of the element
	if (scalar(@{$att->{"subelements"}})>0) {
		foreach $a (@{$att->{"subelements"}}) {
			dumphtmlatt($a);
		}
	}

}

 #dump out any <td> that are in front of the current element, if any
sub dumpPreviousElements
{
	my $att=$_[0];
	my $span=$_[1];
	my $d=1;

	$att=$att->{"owner"};
	if ($att==0) { return 1; }

	$d+=dumpPreviousElements($att,$span);
	print OUTHTML "    <td valign=\"top\" style=\"background:#eeeeee;color:#aaaaaa\" rowspan=\"$span\">".$att->{"name"}."</td>\n";

	return $d;
}


#------------------------ write out dtd file --------------------------------
if ($dtdfile ne "") {
	print "Writing out dtd to \"$dtdfile\"...";
	open(OUTDTD, ">$dtdfile");

	print OUTDTD "<!-- $initialnotes -->\n\n";

	dumpdtdatt(\%mainnode);

	close (OUTDTD);
	print "done!\n";

}

 #called for each element needing to be output
sub dumpdtdatt
{
# Todo:
#  specify enums: <!ATTLIST element attribute (val1|val2|val3) "val1">
#  <!ATTLIST element att #FIXED "1.4">
#  #IMPLIED atts?
#
	my $att=$_[0];

	my $numatts=scalar(@{$att->{"attributes"}});

	my $optional="";

	 #print out 1 row for element name and comment if any
	#if ($att->{"optional"}!=0) { $optional="\"".$att->{"default"}."\""; } else { $optional="#REQUIRED"; }
	
	print OUTDTD "\n<!ELEMENT ".$att->{"name"};
	if (scalar(@{$att->{"subelements"}})==0) {
		print OUTDTD " EMPTY";
	} else {
		print OUTDTD " (";
		my $c=0;
		foreach $a (@{$att->{"subelements"}}) {
			if ($c>0) { print OUTDTD ", "; }
			print OUTDTD $a->{"name"};
			if ($a->{"optional"} eq "zero or one")   { print OUTDTD "?"; }
			if ($a->{"optional"} eq "one or more")   { print OUTDTD "+"; }
			if ($a->{"optional"} eq "zero or more")  { print OUTDTD "*"; }
			$c++;
		}
		print OUTDTD ")";
	}
	print OUTDTD ">\n";
	#print OUTDTD "> <!-- ".$att->{"comment"}." -->\n";
	
	 #output all attributes of the element
	if ($numatts>0) {
		#----output att list in separate ATTLIST:
		my $c=0;
		foreach $a (@{$att->{"attributes"}}) {
			$c++;
			if ($a->{"optional"} eq "one only" or $a->{"optional"} eq "one or more" )
				{ $optional="#REQUIRED"; } else { $optional="\"".$att->{"default"}."\""; }

			printf OUTDTD ("<!ATTLIST ".$att->{"name"}." %-18s CDATA %s >\n", $a->{"name"}, $optional);
			#print OUTDTD "<!-- ".$a->{"comment"}." -->\n";
		}

	}
#	----output att list in single ATTLIST:
#	if ($numatts>0) {
#		print OUTDTD "<!ATTLIST ".$att->{"name"}."\n";
#
#		my $c=0;
#		foreach $a (@{$att->{"attributes"}}) {
#			$c++;
#			if ($a->{"optional"} eq "one only" or $a->{"optional"} eq "one or more" )
#				{ $optional="#REQUIRED"; } else { $optional="\"".$att->{"default"}."\""; }
#
#			printf OUTDTD ("  %-18s CDATA %s\n", $a->{"name"}, $optional);
#			#print OUTDTD "<!-- ".$a->{"comment"}." -->\n";
#		}
#
#		print OUTDTD "> <!-- attributes of ".$att->{"name"}." -->\n";
#	}

	#output all sub elements of the element
	if (scalar(@{$att->{"subelements"}})>0) {
		foreach $a (@{$att->{"subelements"}}) {
			dumpdtdatt($a);
		}
	}
}





#------------------------ write out wiki table --------------------------------

if ($wikifile ne "") {

	print "Writing out wiki table to \"$wikifile\"...";
	open(OUTWIKI, ">$wikifile");

	print OUTWIKI "$initialnotes\n\n";

	print OUTWIKI << "TOPWIKI";
{| class="wikitable" border="1" cellpadding="3"
| <center>TAGS</center>
| <center>ATTRIBUTE</center>
| <center>EXPLANATION</center>

TOPWIKI

	dumpwikiatt(\%mainnode);

	print OUTWIKI "|}\n";

	close (OUTWIKI);
	print "done!\n";

}

 #called for each element needing to be output
sub dumpwikiatt
{
	my $att=$_[0];

	my $numatts=scalar(@{$att->{"attributes"}});

	print OUTWIKI "|-\n";
	my $currentdepth = dumpPreviousElementsWiki($att,1+$numatts);
	my $bgcolor="#ffffff"; #"#A9D1FF"
	my $cbgcolor="#ffffff"; #"#A9D1FF"
	my $optional="";

	 #print out 1 row for element name and comment if any
	#if ($att->{"optional"}==0) { $bgcolor="#eeeeee"; } else { $bgcolor="#A9D1FF"; }
	if ($att->{"optional"} eq "one only" or $att->{"optional"} eq "one or more" )
		{ $optional="(optional) "; } else { $optional=""; }
	
	print OUTWIKI "| $currentdepth".$att->{"name"}."\n";
	print OUTWIKI "|\n"; #this line is for attributes, not elements
	print OUTWIKI "| $optional ".$att->{"comment"}."\n\n";
	
	 #output all attributes of the element
	my $c=0;
	foreach $a (@{$att->{"attributes"}}) {
		$c++;
		#if ($a->{"optional"}==0) { $bgcolor="#ffffff"; } else { $bgcolor="#c9e4FF"; }
		if ($a->{"optional"} eq "one only" or $a->{"optional"} eq "one or more" ) { $optional="(optional) "; } else { $optional=""; }

		print OUTWIKI "|-\n"; #new row
		print OUTWIKI "|\n";  #element name
		print OUTWIKI "| ".$a->{"name"}."\n";
		print OUTWIKI "| $optional ".$a->{"comment"}."\n\n";
	}

	#output all sub elements of the element
	if (scalar(@{$att->{"subelements"}})>0) {
		foreach $a (@{$att->{"subelements"}}) {
			dumpwikiatt($a);
		}
	}
}

 #return a string like Parent.Child1.Child1 for all elements above the current one
sub dumpPreviousElementsWiki
{
	my $att=$_[0];
	my $span=$_[1];
	my $d="";

	$att=$att->{"owner"};
	if ($att==0) { return ""; }

	$d=dumpPreviousElementsWiki($att,$span);
	$d.=$att->{"name"}." . ";

	return $d;
}


#------------------------------------- ONLY SUBS FOLLOW ------------------------------

#----------------------compute table length for tabs----------------
sub computedepth
{
	my $att=$_[0];

	my $num=1; #1 for the att passed in

	$num+=scalar(@{$att->{"attributes"}});

	if (scalar(@{$att->{"subelements"}})>0) {
		foreach $a (@{$att->{"subelements"}}) {
			$num+=computedepth($a);
		}
	}
	$att->{"totallines"}=$num;
	return $num;
}

#----------------------stdout dump sub for debugging-------------------
sub dumpatt
{
	my $indent=$_[0];
	my $att   =$_[1];

	printindent($indent);
	print $att->{"name"}."\n";
	#$array=$att->{"attributes"};
	foreach $a (@{$att->{"attributes"}}) {
		printindent($indent+2);
		print $a->{"name"}."\n";
	}
	#print $att->{"subelements"}."\n";
	if (scalar(@{$att->{"subelements"}})>0) {
		printindent($indent+2);
		print "elements:\n";
		foreach $a (@{$att->{"subelements"}}) {
			dumpatt($indent+4, $a);
		}
	}

}

sub printindent
{
	my $indent=$_[0];
	for ($c=0; $c<$indent; $c++) { print " "; }
}

# Perl trim function to remove whitespace from the start and end of the string
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}
# Left trim function to remove leading whitespace
sub ltrim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	return $string;
}
# Right trim function to remove trailing whitespace
sub rtrim($)
{
	my $string = shift;
	$string =~ s/\s+$//;
	return $string;
}


