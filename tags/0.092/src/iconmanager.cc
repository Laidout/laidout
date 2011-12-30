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
// Copyright (C) 2004-2006 by Tom Lechner
//

#include "iconmanager.h"
#include <lax/strmanip.h>
#include <lax/lists.cc>

using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

//----------------------------- IconNode ---------------------------
/*! \class IconNode
 * \brief Stacked in an IconManager.
 */


//! img's count is not incremented.
IconNode::IconNode(const char *nname, int nid, LaxImage *img)
{
	id=nid;
	name=newstr(nname);
	image=img;
	image->doneForNow();
}

IconNode::~IconNode()
{
	////DBG cerr <<"IconNode destructor"<<endl;
	if (image) image->dec_count();
	if (name) delete[] name;
}


//----------------------------- IconManager ---------------------------
/*! \class IconManager
 * \brief Simplify maintenance of icons with this stack of IconNode objects.
 *
 * This is essentially a Laxkit::RefStackPtr<IconNode> with some helper functions
 * to ease lookup of icons as button boxes come and go. The stack is sorted by id.
 *
 * \todo Eventually, it might be in charge of generating pixmap icons from an icons.svg or
 *   icons.laidout or something of the kind.
 */


/*! Icons are added only when needed by GetIcon(), which uses findicon().
 */
IconManager::IconManager()
	: icon_path(2)
{}

//! Return -1 for fail to load file.
int IconManager::InstallIcon(const char *nname, int nid, const char *file)
{
	LaxImage *img=load_image(file);
	if (!img) return -1;
	return InstallIcon(nname,nid,img);
}

/*! If nid<=0, then make the id one more than the maximum nid in stack.
 * \todo *** must check that the icon is not already installed
 */
int IconManager::InstallIcon(const char *nname, int nid, LaxImage *img)
{
	int c;
	if (nid<=0) {
		if (n) nid=e[n-1]->id+1; else nid=1;
		c=n;
	} else {
		c=0;
		while (c<n && e[c]->id>nid) c++;
	}
	
	IconNode *node=new IconNode(nname,nid,img);
	return push(node,1,c);
}

//! Search for a icon file "name.png" in all the icon paths.
/*! Install and return the icon if found, else NULL.
 * 
 * This function assumes that name is not already in the stack.
 */
LaxImage *IconManager::findicon(const char *name)
{
	char *path;
	LaxImage *img=NULL;
	for (int c=0; c<icon_path.n; c++) {
		path=newstr(icon_path.e[c]);
		appendstr(path,"/");
		appendstr(path,name);
		appendstr(path,".png");
		img=load_image(path);
		delete[] path;
		if (img) break;
	}
	if (img) {
		InstallIcon(name,-1,img);
		img->inc_count();
	}
	return img;
}

//! Return how many icons are currently installed.
int IconManager::HowMany()
{
	return PtrStack<IconNode>::n;
}

//! Returns the icon. The icon's count is incremented.
Laxkit::LaxImage *IconManager::GetIconByIndex(int index)
{
	if (index<0 || index>=PtrStack<IconNode>::n) return NULL;

	PtrStack<IconNode>::e[index]->image->inc_count();
	return PtrStack<IconNode>::e[index]->image;
}

//! Returns the icon. The icon's count is incremented.
Laxkit::LaxImage *IconManager::GetIcon(int id)
{
	 //rather slow, but then, there won't be a million of them
	for (int c=1; c<PtrStack<IconNode>::n; c++)
		if (id==PtrStack<IconNode>::e[c]->id)  {
			PtrStack<IconNode>::e[c]->image->inc_count();
			return PtrStack<IconNode>::e[c]->image;
		}
	return NULL;
}

//! Returns the icon. The icon's count is incremented.
/*! If name does not exist, then the icon is searched for in the icon paths,
 * and the first one found is installed.
 */
Laxkit::LaxImage *IconManager::GetIcon(const char *name)
{
	 //rather slow, but then, there won't be a million of them
	for (int c=1; c<PtrStack<IconNode>::n; c++)
		if (strcmp(name,PtrStack<IconNode>::e[c]->name)==0) {
			PtrStack<IconNode>::e[c]->image->inc_count();
			return PtrStack<IconNode>::e[c]->image;
		}
	return findicon(name);
}

//! Add path to index 0 position of the path stack.
/*! When passed a name that is unrecognized, then the all the icon paths are searched for
 * a loadable image named name.png.
 */
void IconManager::addpath(const char *newpath)
{
	icon_path.push(newstr(newpath),2,0);
}



