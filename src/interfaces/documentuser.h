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
// Copyright (C) 2011 by Tom Lechner
//
#ifndef INTERFACES_DOCUMENTUSER_H
#define INTERFACES_DOCUMENTUSER_H



namespace Laidout {


class Document;

/*! \class DocumentUser
 * \brief Class to facilitate updating interfaces to use particular documents.
 */
class DocumentUser
{
  public:
	DocumentUser() {}
	virtual int UseThisDocument(Document *doc) = 0;
};

} //namespace Laidout

#endif

