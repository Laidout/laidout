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
#ifndef INDEXRANGE_H
#define INDEXRANGE_H


#include <lax/utf8string.h>
#include <lax/lists.h>


namespace Laidout {


class IndexRange
{
    int curi, curi2; //range, index in that range
	int cur; //current actual index
	//int min;
	int max;
	char *range_marker;
	Laxkit::Utf8String str;
	Laxkit::NumStack<int> indices;

  public:
	bool parse_from_one; //whether int range when parsing/tostring starts at 1 (true) or 0 (false, default)

	IndexRange();
	virtual ~IndexRange();

	int NumRanges() { return indices.n/2; }
	int NumInRanges();
	int Start();
	int Next();
	int Current();
	int End();
	virtual int Max(int nmax) { max = nmax; return max; }
	virtual int Max() { return max; }
	//virtual int Min(int nmin) { min = nmin; }
	//virtual int Min() { return min; }
	virtual int GetRange(int which, int *start, int *end);
	virtual const char *RangeMarker() { return range_marker; }
	virtual const char *RangeMarker(const char *marker);

	virtual void Clear();
	virtual int Parse(const char *range, const char **end_ptr, bool use_labels);
	virtual const char *ToString(bool absolute, bool use_labels);

	virtual void IndexToLabel(int i, Laxkit::Utf8String &str, bool absolute);
	virtual int LabelToIndex(const char *label, const char **end_ptr);
};

} //namespace Laidout



#endif

