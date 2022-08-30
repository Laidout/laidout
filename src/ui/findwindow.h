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
// Copyright (C) 2020 by Tom Lechner
//
#ifndef FINDWINDOW_H
#define FINDWINDOW_H

#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/messagebar.h>
#include <lax/iconselector.h>
#include <lax/draggableframe.h>
#include <lax/interfaces/selection.h>

#include "../core/objectiterator.h"


namespace Laidout {


class FindWindow : public Laxkit::DraggableFrame
{
  public:
	enum FindThings {
		FIND_Anywhere = 1,
		FIND_InView = 2,
		FIND_InSelection = 3,
		FIND_MAX
	};
	
  private:
	class FindResult
	{
	  public:
	  	FieldPlace where;
	  	anObject *obj;
	  	FindResult(FieldPlace &place, anObject *o) {
	  		where = place;
	  		obj = o;
	  		if (o) o->inc_count();
	  	}
	  	~FindResult() { if (obj) obj->dec_count(); }
	};

	Laxkit::PtrStack<FindResult> selection;
	int most_recent;

	Laxkit::MessageBar *how_many;
	Laxkit::LineInput *pattern;
	Laxkit::IconSelector *founditems;

	bool regex;
	bool caseless;

	ObjectIterator iterator;
	bool search_started;
	
	FindThings where;

	virtual void send(int info);
	virtual void UpdateFound();

	virtual void SearchNext();
	virtual void SearchPrevious();
	virtual void SearchAll();

	virtual void InitiateSearch();

 public:

 	FindWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 			int x, int y, int w, int h, unsigned long owner, const char *msg);
	virtual ~FindWindow();
	virtual const char *whattype() { return "FindWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual void SetFocus();
	virtual int GetCurrent(FieldPlace &place_ret, Laxkit::anObject *&obj_ret);
	virtual FindThings Where() { return where; }
};

} //namespace Laidout

#endif

