//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2025 by Tom Lechner
//

#include "streaminterface.h"
#include "../language.h"


using namespace LaxInterfaces;

namespace Laidout {


//--------------------------- StreamInterface -------------------------------------


StreamInterface::StreamInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp)
  : TextStreamInterface(nowner, nid, ndp)
{}

StreamInterface::~StreamInterface()
{}

const char *StreamInterface::Name()
{
	return _("Streams");
}

const char *StreamInterface::whatdatatype()
{
	return nullptr;
}

anInterface *StreamInterface::duplicate(anInterface *dup)
{
	if (dup == nullptr) dup = new StreamInterface(nullptr,-1,nullptr);
	else if (!dynamic_cast<StreamInterface *>(dup)) return nullptr;
	return TextStreamInterface::duplicate(dup);
}

bool StreamInterface::AttachStream()
{
	PostMessage("IMP ME!!");
	return false;
}


} // namespace Laidout


