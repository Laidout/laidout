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
// Copyright (C) 2007-2013 by Tom Lechner
//

//--------------------------------------- MysteryData ---------------------------------

#include <lax/strmanip.h>
#include "mysterydata.h"


#define DBG
#include <iostream>
using namespace std;

using namespace LaxFiles;
using namespace LaxInterfaces;

namespace Laidout {

/*! \class MysteryData
 * \brief Holds fragments of objects of an imported file.
 *
 * This class facilitates preserving data from files of non-native formats, so that
 * when a Laidout document is exported to the same format, those objects that Laidout
 * doesn't understand can still be exported. Other formats will generally ignore the
 * MysteryData.
 */
/*! \var MysteryData::outline
 * 
 * A list of bezier points: c-v-c - c-v-c..., to be displayed in the representative box.
 * This can give more hints about what it is.
 */


MysteryData::MysteryData(const char *gen)
{
	numpoints=0;
	outline=NULL;

	name=NULL;
	nativeid=-1;
	importer=newstr(gen);
	attributes=NULL;
}

MysteryData::~MysteryData()
{
	if (name) delete[] name;
	if (importer) delete[] importer;
	if (attributes) delete attributes;
	if (outline) delete[] outline;
}

//! Move the att to this->attributes.
/*! This object takes possession of att. The calling code should not delete att.
 * If this->attributes existed, it is deleted.
 *
 * If att==NULL, then this->attributes becomes NULL.
 */
int MysteryData::installAtts(LaxFiles::Attribute *att)
{
	if (attributes) delete attributes;
	attributes=att;
	return 0;
}

void MysteryData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%simporter Passepartout  #the name of the importer that created this data\n",spc);
		fprintf(f,"%sname     Raster x.jpg  #some name that the importer gave this data\n",spc);
        fprintf(f,"%smaxx 100               #width of the data\n",spc);
        fprintf(f,"%smaxy 200               #height of the data\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0     #affine transform to apply to the data\n",spc);
		fprintf(f,"%snativeid 1             #A numeric id grabbed from the original file, if any\n",spc);
		fprintf(f,"%sattributes             #the imported hints for this data\n",spc);
		fprintf(f,"%s  ...                  #  what these are depend on the importer\n",spc);
		return;
	}
	
	if (importer) fprintf(f,"%simporter \"%s\"\n",spc,importer);
	if (name)     fprintf(f,"%sname \"%s\"\n",spc,name);
	fprintf(f,"%smaxx %.10g\n",spc,maxx);
	fprintf(f,"%smaxy %.10g\n",spc,maxy);
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%snativeid %ld\n",spc,nativeid);
	if (attributes) {
		fprintf(f,"%sattributes\n",spc);
		attributes->dump_out(f,indent+2);
	}
}
	
/*! When the image listed in the attribute cannot be loaded,
 * image is set to NULL, and the width and height attributes
 * are used if present. If the image can be loaded, then width and
 * height as given in the file are curretly ignored, and the actual pixel 
 * width and height of the image are used instead.
 */
void MysteryData::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *nname,*value;
	minx=miny=0;
	maxx=maxy=-1;
	double x2,y2;
	x2=y2=0;
	for (int c=0; c<att->attributes.n; c++) {
		nname=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(nname,"matrix")) {
			double mm[6];
			if (DoubleListAttribute(value,mm,6)==6) m(mm);
		} else if (!strcmp(nname,"importer")) {
			makestr(importer,value);
		} else if (!strcmp(nname,"name")) {
			makestr(name,value);
		} else if (!strcmp(nname,"maxx")) {
			DoubleAttribute(value,&x2);
		} else if (!strcmp(nname,"maxy")) {
			DoubleAttribute(value,&y2);
		} else if (!strcmp(nname,"nativeid")) {
			LongAttribute(value,&nativeid);
		} else if (!strcmp(nname,"attributes")) {
			if (attributes) delete attributes;
			attributes=att->attributes.e[c]->duplicate();
		}
	}
	minx=0;
	miny=0;
	maxx=x2;
	maxy=y2;
}

LaxInterfaces::SomeData *MysteryData::duplicate(LaxInterfaces::SomeData *dup)
{
	MysteryData *mdata=dynamic_cast<MysteryData*>(dup);
	if (!mdata && dup) return NULL;

	char set=1;
	if (!mdata) {
		mdata=new MysteryData();
		dup=mdata;
	}
	if (set) {
		makestr(mdata->importer,importer);
		makestr(mdata->name,name);
		mdata->numpoints=numpoints;
		if (numpoints) {
			mdata->outline=new flatpoint[numpoints];
			//memcpy(mdata->outline,outline,numpoints*sizeof(flatpoint));
			for (int c=0; c<numpoints; c++) mdata->outline[c] = outline[c];
		} else {
			if (mdata->outline) delete[] mdata->outline;
			mdata->outline=NULL;
		}
		mdata->attributes=attributes->duplicate();
	}

	 //somedata elements:
	dup->bboxstyle=bboxstyle;
	dup->setbounds(this);
	dup->m(m());
	return dup;
}


//-------------------------------- MysteryInterface ----------------------------------
/*! \class MysteryInterface
 * \brief Interface to manipulate placement of eps files.
 *
 * NOTE: *** currently not used.
 *
 * If there is an epsi style preview in the eps, then that is what is put
 * on screen. Otherwise, the title/file/date are put on.
 */
class MysteryInterface : public LaxInterfaces::RectInterface
{
 public:
	MysteryInterface(int nid,Laxkit::Displayer *ndp);
	LaxInterfaces::anInterface *duplicate(anInterface *dup);
	virtual const char *whattype() { return "MysteryInterface"; }
	virtual const char *whatdatatype() { return "MysteryData"; }
	virtual int draws(const char *what);
	virtual int Refresh();
};



MysteryInterface::MysteryInterface(int nid,Laxkit::Displayer *ndp)
	: RectInterface(nid,ndp)
{
	style|=RECT_CANTCREATE;
}

//! Return whether this interface can draw the given type of object.
int MysteryInterface::draws(const char *what)
{
	if (!strcmp(what,"MysteryData")) return 1;
	return 0;		
}

//! Return new MysteryInterface.
/*! If dup!=NULL and it cannot be cast to MysteryInterface, then return NULL.
 */
LaxInterfaces::anInterface *MysteryInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=new MysteryInterface(id,NULL);
	else if (!dynamic_cast<MysteryInterface *>(dup)) return NULL;
	return RectInterface::duplicate(dup);
}

//! Draw name, box around it, and a bunch of question marks.
int MysteryInterface::Refresh()
{
	needtodraw=0;

	 // draw question marks and name if any...
	MysteryData *mdata=dynamic_cast<MysteryData *>(somedata);
	if (!mdata) return 0;

	flatpoint fp;
	if (mdata->name) {
		fp=dp->realtoscreen(flatpoint((mdata->maxx+mdata->minx)/2,(mdata->maxy+mdata->miny)));
		dp->textout((int)fp.x,(int)fp.y,mdata->name,0);
	}

	 //draw question marks in random spots
	for (int c=0; c<10; c++) {
		fp=dp->realtoscreen(flatpoint(mdata->minx+(mdata->maxx+mdata->minx)*((double)random()/RAND_MAX),
									  mdata->miny+(mdata->maxy+mdata->miny)*((double)random()/RAND_MAX)));
		dp->textout((int)fp.x,(int)fp.y,"?",0);
	}

	 //draw box around it
	flatpoint ul=dp->realtoscreen(flatpoint(mdata->minx,mdata->miny)), 
			  ur=dp->realtoscreen(flatpoint(mdata->maxx,mdata->miny)), 
			  ll=dp->realtoscreen(flatpoint(mdata->minx,mdata->maxy)), 
			  lr=dp->realtoscreen(flatpoint(mdata->maxx,mdata->maxy));
	dp->drawline(ul,ur);
	dp->drawline(ur,lr);
	dp->drawline(lr,ll);
	dp->drawline(ll,ul);

	return 0;
}

} //namespace Laidout

