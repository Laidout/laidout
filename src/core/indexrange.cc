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


#include "indexrange.h"

#include <lax/strmanip.h>


//template implementation:
#include <lax/lists.cc>


using namespace Laxkit;


namespace Laidout {


/*! Class to simplify parsing and access to noncontinuous rangess
 * defined by ints, such as "1-4, 10-20".
 *
 * Syntax being: s* [s* i s* [ rangechar s* i s* ] ]*
 * where s is whitespace and i is a positive or negative integer.
 * Negative integers are from the end, so -1 is the final index (max-1),
 * rangechar is "-" or ":", but you can also use a custom one
 * by setting range_marker, such as "..".
 *
 * Ranges are stored by integer pairs in indices.
 */

IndexRange::IndexRange()
{
	parse_from_one = false;
	curi = 0;
	cur = 0;
	max = -1;
	//min = 0;
	range_marker = newstr("-"); //used in ToString, and ALSO checked in Parse(), along with '-' and ':'
}

IndexRange::~IndexRange()
{
	delete[] range_marker;
}

const char *IndexRange::RangeMarker(const char *marker)
{
	if (isblank(marker)) return range_marker;
	makestr(range_marker, marker);
	return range_marker;
}

/*! Return 0 for succes, or -1 for which not found.
 */
int IndexRange::GetRange(int which, int *start, int *end)
{
	if (which*2 >= indices.n) return -1;
	*start = indices.e[which*2];
	*end   = indices.e[which*2+1];
	return 0;
}

/*! Sum of the number of indices in each range.
 * Note that if ranges overlap, the overlapping elements are counted more than once.
 */
int IndexRange::NumInRanges()
{
	int n=0;
	for (int c=0; c<indices.n; c+=2) {
		if (indices[c+1] >= indices[c])
			n += indices[c] - indices[c+1] + 1;
		else
			n += indices[c+1] - indices[c] + 1;
	}

	return n;
}

int IndexRange::Start()
{
	curi = 0;
	curi2 = 0;
	cur = indices[0];
	return cur;
}

/*! Return -1 if all done.
 */
int IndexRange::Next()
{
	if (cur == indices[curi+1]) {
		//advance to next section
		curi += 2;
		curi2 = 0;
		if (curi >= indices.n) return -1;
		cur = indices[curi];

	} else {
		//next in current range
		if (indices[curi+1] > indices[curi]) {
			cur++;
			curi2++;

		} else {
			cur--;
			curi2++;
		}
	}

	return cur;
}

int IndexRange::Current()
{
	return cur;
}

int IndexRange::End()
{
	if (indices.n == 0) return -1;
	return indices[indices.n-1];
}

void IndexRange::Clear()
{
	cur = -1;
	curi = curi2 = 0;
	str.Clear();
}

/*! Clears current ranges before parsing. Return 0 for success, or nonzero for error.
 * Expects string like "0-5, 9-7, 34". Any whitespace ok. Commas are ignored.
 * If second number is below first, then that range will count down instead of up.
 * Negative numbers mean count from end, so -1 means (max-1).
 *
 * If parse_from_one, then a number 1 gets translated to index 0.
 *
 * Return 0 for success, nonzero for parse error.
 */
int IndexRange::Parse(const char *range, const char **end_ptr, bool use_labels)
{
	const char *p = range;
	char *endptr;
	const char *cendptr;
	int i,i2;
	NumStack<int> newi;

	while (*p) {
		while (isspace(*p)) p++;
		if (!(*p)) break;

		if (use_labels) {
			int ii = LabelToIndex(p, &cendptr);
			if (cendptr == p) { //none found!
				break;
			}
			p = cendptr;
			i = ii;
		} else {
			i = strtol(p, &endptr, 10);
			if (endptr == p) { //number not found
				//if (*p == '-' && max >= 0) i = -1;  <-- should we allow like "-10" or "10-" ??
				//else break;
				break;
			}
			p = endptr;
		}

		while (isspace(*p)) p++;

		if (!strncmp(range_marker, p, strlen(range_marker))) {
			p += strlen(range_marker);
		} else if (*p == '-' || *p == ':') {
			p++;
		} else {
			//no range marker found!
			newi.push(i);
			newi.push(i);
			while (isspace(*p) || *p == ',') p++;
			continue; //no range marker found, assume end of current range
		}

		while (isspace(*p)) p++;

		//there is a second number
		if (use_labels) {
			int ii = LabelToIndex(p, &cendptr);
			if (cendptr == p) { //none found!
				break;
			}
			p = cendptr;
			i2 = ii;
		} else {
			i2 = strtol(p, &endptr, 10);
			if (endptr == p) { //number not found, this is an error!
				newi.flush();
				break;
			}
			p = endptr;
		}

		newi.push(i);
		newi.push(i2);

		while (isspace(*p) || *p == ',') p++;
		//if (*p != ',') break; //no more ranges
	}

	while (isspace(*p)) p++;
	if (*p != '\0') {
		//error! string contains something that couldn't be parsed
		newi.flush();
	}
	if (newi.n == 0) { //parse problem!
		if (end_ptr) *end_ptr = range;
		return -1; 
	}

	str.Clear();
	indices = newi;
	return 0;
}

/*! If absolute, translate negative numbers i to (max+i).
 * If use_labels, negative numbers will be output as the same negative number.
 */
const char *IndexRange::ToString(bool absolute, bool use_labels)
{
	if (!str.IsEmpty()) return str.c_str();

	Utf8String scratch;

	if (use_labels) {
		Utf8String l1, l2;
		int i1, i2;

		for (int c=0; c<indices.n; c+=2) {
			i1 = indices[c];
			i2 = indices[c+1];
			IndexToLabel(i1, l1, absolute);
			IndexToLabel(i2, l2, absolute);

			if (i1 == i2) scratch.Sprintf("%s", l1.c_str());
			else scratch.Sprintf("%s%s%s", l1.c_str(), range_marker, l2.c_str());
			str.Append(scratch);
			if (c < indices.n-2) str.Append(", ");
		}

	} else { //output as ints
		int i1,i2;
		for (int c=0; c<indices.n; c+=2) {
			i1 = indices[c];
			i2 = indices[c+1];

			if (absolute) {
				if (i1 < 0) i1 = max + i1;
				if (i2 < 0) i2 = max + i2;
			}
			if (parse_from_one) {
				if (i1 >= 0) i1++;
				if (i2 >= 0) i2++;
			}

			if (i1 == i2) scratch.Sprintf("%d", i1);
			else scratch.Sprintf("%d%s%d", i1, range_marker, i2);
			str.Append(scratch);
			if (c < indices.n-2) str.Append(", ");
		}
	}

	return str.c_str();
}

/*! From absolute index i, return corresponding string.
 * Default is same as i, or i+1 if parse_from_one.
 * Subclasses might redefine to some other label translation.
 *
 * If absolute and i<0, then make string max-i, or max-i if parse_from_one.
 */
void IndexRange::IndexToLabel(int i, Utf8String &str, bool absolute)
{
	if (i<0 && absolute) i = max+i;
	if (parse_from_one && i >= 0) i++;
	str.Sprintf("%d", i);
}

/*! From label string, return 
 * Default is same as i, or i+1 if parse_from_one. Negative numbers passed as is.
 * Subclasses might redefine to some other label translation.
 *
 * If parse_from_one, subtract 1 from number read.
 */
int IndexRange::LabelToIndex(const char *label, const char **end_ptr)
{
	char *endptr;
	int i = strtol(label, &endptr, 10);
	*end_ptr = endptr;
	if (parse_from_one && i > 0) i--;
	return i;
}

} //namespace Laidout

