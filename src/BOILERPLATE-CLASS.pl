#!/usr/bin/perl

##
## ./BOILERPLATE-CLASS.pl ClassName
##   -> makes classname.h, classname.cc
##


$Name=$ARGV[0];

$name=lc($Name);
$NAME=uc($name);


$hfile="$name.h";
$ccfile="$name.cc";

$NAME_H=uc($hfile);
$NAME_H =~ s/\./_/;

die  "$hfile already exists. Aborting!" if (-e $hfile);
die "$ccfile already exists. Aborting!" if (-e $ccfile);


###--------- header file -----------

open (HFILE, ">$hfile") or die "can't open $hfile";

print HFILE << "END";
//
//  
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2017 by Tom Lechner
//
//
#ifndef $NAME_H
#define $NAME_H

#include <lax/anobject.h>
#include <lax/dump.h>


namespace Laidout {

//----------------------------- $Name -------------------------------

class $Name : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  protected:
  public:
	$Name();
	virtual ~$Name();
	const char *whattype() { return "$Name"; }

	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} //namespace Laidout

#endif
END

close(HFILE);





###--------- cc file -----------

open (CCFILE, ">$ccfile") or die "can't open $ccfile";

print CCFILE << "END";
//
//  
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2017 by Tom Lechner
//
//

#include <lax/$name.h>


namespace Laidout {

//----------------------------- $Name -------------------------------

/*! \\class $Name
 */

$Name\:\:$Name()
{
}

$Name\:\:~$Name()
{
}

void $Name\:\:dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{ ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sAField value #comment\n",spc);
		return;
	}
						
	fprintf(f,"%sAField %d\n",spc, value);

	-------- OR:  piggy back on dump_out_atts() ------------

	LaxFiles::Attribute att;
	dump_out_atts(&att,what,savecontext);
	att.dump_out(f,indent);
}

LaxFiles::Attribute *$Name\:\:dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{ ***
	if (what==-1) {
		if (!att) att = new Attribute;

		return att;
	}

	if (!att) att = new Attribute;

	return att;
}

void $Name\:\:dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ ***
	char *name;
    char *value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"AField")) {
			*** //do stuff
		}
	} 
}


} // namespace Laidout

END


close(CCFILE);


print "Created $hfile\nCreated $ccfile\n";

