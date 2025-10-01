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
// Copyright (C) 2022 by Tom Lechner
//
#ifndef SINGLES_INTERFACE_H
#define SINGLES_INTERFACE_H

#include "singles.h"
#include "../interfaces/paperinterface.h"
#include "../ui/papersizewindow.h"


namespace Laidout {


class SinglesInterface : virtual public ImpositionInterface, virtual public PaperInterface
{

  protected:
	virtual void send();

	int first_paper = 0;
	bool showsplash = false;

	Singles *singles = nullptr;
	Document *document = nullptr;
	PaperStyle *papertype = nullptr;

  public:
 	SinglesInterface(LaxInterfaces::anInterface *nowner,int nid,Laxkit::Displayer *ndp);
	virtual ~SinglesInterface();
	virtual const char *whattype() { return "SinglesInterface"; }
	virtual const char *IconId() { return "SinglePageView"; }
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual const char *Name();

	//virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int InterfaceOn();

	// override from PaperInterface:
	virtual const char *whatdatatype() { return nullptr; }

	// from ImpositionInterface:
    virtual const char *ImpositionType() { return whattype(); }
    virtual Imposition *GetImposition() { return singles; }
    virtual int SetTotalDimensions(double width, double height); //set default paper size
    virtual int GetDimensions(double &width, double &height); //Return default paper size
    virtual int SetPaper(PaperStyle *paper);
    virtual int UseThisDocument(Document *doc);
    virtual int UseThisImposition(Imposition *imp);

    virtual int ShowThisPaperSpread(int index);
    virtual void ShowSplash(int yes) { showsplash = yes; needtodraw = 1; }
};

} // namespace Laidout

#endif

