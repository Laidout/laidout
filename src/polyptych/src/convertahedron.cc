//
//	
// Convertahedron, convert beteen what polyptyth/poly can understand.
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2011-2012 by Tom Lechner


#include <lax/fileutils.h>
#include "poly.h"

using namespace LaxFiles;
using namespace Laxkit;
using namespace Polyptych;

//Right now, convert between "idat", obj, and off

void usage()
{
	Polyhedron poly;

	cerr <<"convertahedron -h"<<endl;
	cerr <<"convertahedron [-i informat] infilename [-o outformat] [-c] outfilename"<<endl;

	cerr <<"\nThe \"-h\" option displays this help."<<endl;
	cerr <<"Current input formats are:  "<<poly.InFileFormats()<<endl;
	cerr <<"Current output formats are: "<<poly.OutFileFormats()<<endl;
	cerr <<"\nFor the second run format, options must be given in specified order."<<endl;
	cerr <<"The format for infilename is hopefully found automatically, but can be specified by -i."<<endl;
	cerr <<"Format for out is determined by the extension, or can be explicitly specified with -o."<<endl;
	cerr <<"If outfilename exists, it will not be clobbered, unless -c is specified."<<endl;
}

void Arrg()
{
	cerr <<"Not enough arguments!!"<<endl<<endl;
	usage();
	exit(1);
}

int main(int argc, char **argv) 
{
	if (argc==1 || (argc>1 && (!strcasecmp(argv[1],"-h") || !strcasecmp(argv[1],"--help")))) {
		usage();
		exit(0);
	}

	int i=1;
	const char *infile=NULL, *outfile=NULL, *informat=NULL, *outformat=NULL;
	int clobber=0;
	if (argc<=i) Arrg();

	if (!strcmp(argv[i],"-i")) {
		informat=argv[i];
		i++;
	}
	if (argc<=i) Arrg();
	infile=argv[i];
	i++;


	Polyhedron poly;
	int status=1;
	char *error=NULL;
	if (informat) {
		if (!strcasecmp(informat,"idat")) status=poly.dumpInFile(infile,&error);
		else {
			FILE *f=fopen(infile,"r");
			if (!f) {
				cerr <<"Could not open "<<infile<<" for reading!"<<endl;
				exit(1);
			}
			if (!strcasecmp(informat,"off")) status=poly.dumpInOFF(f,&error);
			else if (!strcasecmp(informat,"obj")) status=poly.dumpInObj(f,&error);
			fclose(f);
		}
	} else status=poly.dumpInFile(infile,&error);

	if (status!=0) {
		if (error) cerr<<error<<endl;
		cerr <<"Error reading in "<<infile<<"!"<<endl;
		exit(1);
	}

	cerr <<"Done reading in.."<<endl;

	if (argc<=i) Arrg();
	if (!strcmp(argv[i],"-o")) {
		outformat=argv[i];
		i++;
	}
	if (argc<=i) Arrg();
	if (!strcmp(argv[i],"-c")) {
		clobber=1;
		i++;
	}

	if (argc<=i) Arrg();
	outfile=argv[i];
	if (!outformat) outformat=strrchr(outfile,'.');
	if (outformat) outformat++;

	if (!outfile) Arrg();
	if (!outformat) {
		cerr <<"Missing or unknown output format!"<<endl;
		exit(1);
	}

	if (!clobber && file_exists(outfile,0,NULL)) {
		cerr <<"Not overwriting "<<outfile<<", use -c if you must."<<endl;
		exit(1);
	}

	if (error) { delete[] error; error=NULL; }
	status=poly.dumpOutFile(outfile,outformat,&error);
	if (error) cerr <<"Errors encountered:\n"<<error<<endl;

	if (status) {
		cerr <<"Error interrupted output."<<endl;
		return 1;
	}
	cout <<"Done!"<<endl;
	return 0;
}



