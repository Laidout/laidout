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
// Copyright (C) 2004-2007,2010 by Tom Lechner
//

#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <lax/lists.h>
#include <lax/laximages.h>

//----------------------------- IconNode ---------------------------

class IconNode
{
 public:
	char *name;
	int id;
	Laxkit::LaxImage *image;
	IconNode(const char *nname, int nid, Laxkit::LaxImage *img);
	~IconNode();
};

//----------------------------- IconManager ---------------------------

class IconManager : public Laxkit::PtrStack<IconNode>
{
  protected:
	Laxkit::PtrStack<char> icon_path;
	Laxkit::LaxImage *findicon(const char *name);
  public:
	IconManager();
	int InstallIcon(const char *nname, int nid, const char *file);
	int InstallIcon(const char *nname, int nid, Laxkit::LaxImage *img);
	Laxkit::LaxImage *GetIconByIndex(int index);
	Laxkit::LaxImage *GetIcon(int id);
	Laxkit::LaxImage *GetIcon(const char *name);
	int HowMany();
	void addpath(const char *newpath);
};



#endif


