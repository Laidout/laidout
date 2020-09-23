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
#ifndef CORE_OBJECTITERATOR_H
#define CORE_OBJECTITERATOR_H


#include <lax/lists.h>
#include <lax/utf8string.h>
#include <lax/interfaces/selection.h>

#include <regex>

#include "../dataobjects/objectcontainer.h"
#include "../dataobjects/drawableobject.h"


namespace Laidout {


class ObjectIterator
{
  public:

	class SearchPattern;
	typedef bool (*MatchFunc)(Laxkit::anObject *obj, SearchPattern *pattern);

	class SearchPattern {
	  public:
		Laxkit::Utf8String pattern;
		MatchFunc func;
		std::regex rex;
		bool must_not_match; //if true, reject any matching pattern
		bool must_match; //immediate fail if no match
		SearchPattern(const char *str, bool is_regex, bool do_caseless, bool _must_match, bool _must_not_match, MatchFunc newfunc);
	};
	Laxkit::PtrStack<SearchPattern> patterns;


	enum SearchDomain {
		SEARCH_Selection,
		SEARCH_ObjectContainer,
		SEARCH_Drawable,
		SEARCH_Project,
		SEARCH_Document,
		SEARCH_Viewport,
		SEARCH_Resources //warning! resources might not be SomeData!!
	};
	SearchDomain where;

	int max_matches;


	ObjectContainer *root;
	// DrawableObject *droot;
	LaxInterfaces::Selection *selection;

	FieldPlace first;
	FieldPlace current;
	Laxkit::anObject *cur_obj; //points inside root or selection, for convenience
	bool finished;

//	//on finding, do these??
//	std::function<int(SomeData*)> func;
//
//	AffineStack transforms; //maintain running transform as appropriate?


	ObjectIterator();
	virtual ~ObjectIterator();

	virtual int Pattern(const char *str, bool is_regex, bool do_caseless, bool must_match, bool must_not_match, bool keep_current);
	virtual int Pattern(MatchFunc func, bool must_match, bool must_not_match, bool keep_current);

	virtual void SearchIn(ObjectContainer *o);
	virtual void SearchIn(DrawableObject *o);
	virtual void SearchIn(LaxInterfaces::Selection *sel); //note: not an ObjectContainer, it's flat and easy to search
	virtual bool Match(Laxkit::anObject *obj);
	virtual void Clear();

	virtual Laxkit::anObject *Start(FieldPlace *where_ret);
	virtual Laxkit::anObject *StartFromCurrent(FieldPlace *where_ret);
	virtual Laxkit::anObject *End(FieldPlace *where_ret); //note this is an initializer, it resets internal tickers, don't use it for comparisons
	virtual Laxkit::anObject *Next(FieldPlace *where_ret);
	virtual Laxkit::anObject *Previous(FieldPlace *where_ret);

	//virtual int RunFunction();
};


} //namespace Laidout

#endif

