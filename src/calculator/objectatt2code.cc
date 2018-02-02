//
// 
//  Use this to convert an indented att based class definition file
//  to c++ ObjectDef code.
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
// Copyright (C) 2009-2014 by Tom Lechner
//


#include "values.h"

#include <iostream>

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace Laidout;


int main(int argc, char **argv)
{
	if (argc<2) {
		cout <<"usage: objectatttocode infile.att > whereverYouWant"<<endl;
		exit(0);
	}

	const char *infile = argv[1];

	ObjectDef def;
	FILE *f = fopen(infile, "r");
	if (!f) {
		cout <<"Could not open "<<infile<<"!"<<endl;
		exit(1);
	}

	def.dump_in(f, 0, 0, NULL, NULL);
	fclose(f);


	//cout << "-------Attribute--------------"<<endl;
	//def.dump_out(stdout, 0, 0, NULL);

	//cout << "-------Code--------------"<<endl;
	def.dump_out(stdout, 0, DEFOUT_CPP, NULL);


	return 0;
}


