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

namespace SphereMap {

//------------------------ Enums -------------------------------
enum OutFormat {
	OUT_NONE,
	OUT_LAIDOUT,
	OUT_SVG,
	OUT_QTVR,
	OUT_QTVR_FACES,
	OUT_IMAGE
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
					  char **error_ret);

int SphereToPoly(const char *spherefile,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,
				 const char *filebase,
				 int output,
				 int oversample,
				 int generate_images,
				 basis *extra_basis,
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
				 basis *extra_basis,
				 char **error_ret 
				);

} //namespace SphereMap



#endif



