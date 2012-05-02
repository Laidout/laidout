//
// $Id$
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
// Copyright (C) 2011 by Tom Lechner
//


#ifndef POLYRENDER_H
#define POLYRENDER_H


#include </usr/include/GraphicsMagick/Magick++.h>

namespace Polyptych {

//------------------------ Enums -------------------------------
enum OutFormat {
	OUT_NONE,
	OUT_LAIDOUT,
	OUT_SVG,
	OUT_QTVR,
	OUT_QTVR_FACES,
	OUT_IMAGE
};

//--------------------------- PaperBound -------------------------------
class PaperBound
{
  public:
	char *name;
	char *units;
	double width,height;
	double r,g,b;
	PaperBound(const char *nname,double w,double h,const char *unit);
	PaperBound(const PaperBound &p);
	~PaperBound();
	PaperBound &operator=(PaperBound &p);
};

//------------------------ RenderConfig -------------------------------
class RenderConfig
{
 public:
	Polyhedron *poly;
	Net *net;
	Net **nets;
	int numnets;
	int maxwidth;
	char *filebase;
	int output;
	int oversample;
	int generate_images;
	basis extra_basis;
	Laxkit::PtrStack<PaperBound> papers;

	RenderConfig();
	virtual ~RenderConfig();
};

//--------------------------------- Functions ------------------------------------
int SaveSvgWithImages(Net *net,
					  const char *filename,
					  const char *filebase,
					  flatpoint *imagedims,
					  flatpoint *imageoffset,
					  double pixPerUnit,
				 	  int net_only,
					  int num_papers,
					  PaperBound **papers,
					  char **error_ret);

int SphereToPoly(const char *spherefile,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,
				 const char *filebase,
				 int output,
				 int oversample,
				 int generate_images,
				 int net_only,
				 basis *extra_basis,
				 int num_papers,
				 PaperBound **papers,
				 char **error_ret
				);

int SphereToPoly(Magick::Image spheremap,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,
				 const char *filebase,
				 int output,
				 int oversample,
				 int generate_images,
				 int net_only,
				 basis *extra_basis,
				 int num_papers,
				 PaperBound **papers,
				 char **error_ret 
				);

} //namespace Polyptych



#endif



