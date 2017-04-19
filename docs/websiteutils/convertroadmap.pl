#!/usr/bin/perl

#This program takes ../../ROADMAP
#and converts it to html in ./web/roadmap.html.
#
#Just run it, no arguments necessary.



$infile="../../ROADMAP";
open(ROADMAP,$infile)
  or die "Cannot open $infile!";
open(ROADMAPHTML, ">web/roadmap.html");

#------write header
print ROADMAPHTML << "END_OF_HEADER";
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1"> 
<meta charset="utf-8">

<title>Laidout Roadmap</title>

<meta name="description" content="Laidout, desktop publishing software.">
<meta name="keywords" content="Cartoons, Prints, Drawings, Polyhedra, Polyhedron, Calendar, 
Desktop, Publishing, Page, Layout, Multipage, Booklet, Books, Imposition, Linux, Software,
Tensor, Product, Patch, Laidout, Folded, Pamphlet, Help,
Dodecahedron, software, open, source, dtp, signature, editor">

<link rel="icon" href="laidout-icon-16x16.png" type="image/png">
<link rel="stylesheet" href="css/style.css" type="text/css">


</head>
<body>

<div class="whole">

  <div id="side">
    <ul class='sidenav'>
	   <li><a class="feeds" href="http://plus.google.com/u/0/b/117350226436132405223/117350226436132405223"><img src="images/gplus.png" alt="Google+"/></a></li>
	   <li><a class="feeds" href="rss.xml"><img src="images/rss.png" alt="RSS feed"/></a></li>
	   &nbsp;&nbsp;
      <li><a href="dev.html">Dev</a></li>
      <li><a href="index.html#download">Download</a></li>
      <li><a href="links.html">Links</a></li>
      <li><a href="faq.html">FAQ</a></li>
      <li><a href="screenshots/">Screenshots</a></li>
      <li><a href="index.html">Home</a></li>
    </ul>
  

  </div><!-- side -->



  <div id="main">

 
<h1>Laidout Roadmap</h1>


<!-- div>
Things to do before the next release.
This is a very rough draft of a roadmap, and is by no means the final word.
</div -->

END_OF_HEADER

$versions=0;
$linenum=0;
$maybemoredesc=0; #if line indented more than 6 spaces: /^       +(.*)/, append to desc
while (defined($line = <ROADMAP>)) {
	$linenum++;

	 #perhaps there is more description incoming
	if ($maybemoredesc!=0) {
		if ($maybemoredesc==1 && $line =~ /^        +(.*)/) {

			$desc="$desc $1";
			next;
		} elsif ($maybemoredesc==-1 && $line=~ /^ +(.*)/) {
			$desc="$desc $1";
			next;
		}
		 #else no more description, and dump out current stuff
		print ROADMAPHTML "<tr><td align=\"center\" valign=\"top\">$done</td><td>$bugtext$desc</td></tr>\n";

		 ## with column for bug numbers:
		#print ROADMAPHTML "<tr><td>$bug</td><td align=\"center\" valign=\"top\">$done</td><td>$desc</td></tr>\n";

		$bug="";
		$bugtext="";
		$done="";
		$desc="";
		$maybemoredesc=0;
	}

	if ($line =~ /^\s*$/) {
		print "skipping blank line $linenum...\n";
		next;
	}

   	 #skip lines like "  ------------  "
	if ($line =~ /^\s*-*\s*$/) { 
		print "skipping dashed line $linenum...\n";
		next;
   	}

	 #dump out initial description bascially verbatim
	if ($versions==0) {
		if ($line =~/LAIDOUT/) { next; }
		if ($line =~/^VERSION/) {
			print ROADMAPHTML "<table>\n\n";
			$versions=1;
		} else {
			print ROADMAPHTML $line;
			next;
		}
	}

	 #start new section
	if ($line =~ /^VERSION/) {
		$line =~ /^VERSION\s*(\S*)\s*-*\s*(.*)/;
		print ROADMAPHTML "<tr><td colspan=3><br><h2>Version $1 -- $2</h2></td></tr>\n";
		next;
	}

	 #start old release section
	if ($line =~ /OLD RELEASE/) {
		print ROADMAPHTML "<tr><td colspan=\"3\" align=\"center\"><hr/><br/><h2>Old Releases</h2></td></tr>\n";
		next;
	}

	 #last line should be the svn id line
	if ($line =~/^\$Id/) {
		$datestring=localtime();
		print ROADMAPHTML "</table>\n<br/>\n<br/>\nLast change: $datestring\n\n";
		break;
	}

	if ($line =~ /bug #(\d*)/) { $bug=$1; } else { $bug=""; }
	#$bug=$1;
	if ($bug ne "") { 
		# $bug contains the number of the bug
		$bugtext="<a title=\"Laidout bug number $bug\" "
			."href=\"http://sourceforge.net/tracker/?func=detail&aid=$bug&group_id=160598&atid=816489\">bug #$bug</a>, ";
	} else {
		$bug="";
		$bugtext="";
	}
	if ($line =~ /^\((done)([^)]*)\)\s*(.*)$/ ) { #match: ^(done *) desc
		$done="($1)&nbsp;";
		if ($2 ne "") { 
			$tmp=$2;
			$desc=$3;
			$tmp =~ /^\W*(.*)/;
			$desc="($1) $desc";
	   	} else { $desc=$3; }
		$maybemoredesc=1;
		print "found: (done) ...\n";
	} else {
		if ($line =~ /^       (.*)$/) { #match: (7 spaces)* 
			$maybemoredesc=1;
			#print "$linenum: found: \"       ...\" :$1\n";
			$desc=$1;
		} else {
			$line =~ /^(\w.*)$/;   #match: ^(word char)*
			$maybemoredesc=-1;
			#print "$linenum: found: \"...\": $1\n";
			$desc=$1;
		}
		
		$done="<span style=\"color:red;font-weight=bold;\">&bull;</span>";
		#print "$linenum: found: \"...\": $1\n";
	}


	print "line:$linenum   bug:$bug   done:$done   desc:$desc\n";
}


print ROADMAPHTML << "END_OF_FOOTER";

</div><!-- main -->
</div><!-- whole -->

</body>
</html>
END_OF_FOOTER


close (ROADMAP);
close (ROADMAPHTML);

