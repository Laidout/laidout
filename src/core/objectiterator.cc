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

#include <lax/interfaces/viewportwindow.h>

#include "objectiterator.h"

//template implementation:
#include <lax/lists.cc>


#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {

/*! \class ObjectIterator
 * Class to help searching Selection or ObjectContainer objects, optionally
 * matching against various kinds of patterns.
 */


bool MatchContains(Laxkit::anObject *obj, ObjectIterator::SearchPattern *pattern)
{
	return strstr(obj->Id(), pattern->pattern.c_str()) != nullptr;
}
bool MatchContainsCaseless(Laxkit::anObject *obj, ObjectIterator::SearchPattern *pattern)
{
	return strcasestr(obj->Id(), pattern->pattern.c_str()) != nullptr;
}
bool MatchRegex(Laxkit::anObject *obj, ObjectIterator::SearchPattern *pattern)
{
	return std::regex_search(obj->Id(), pattern->rex);
}


/*! If newfunc provided, is_regex and do_caseless are ignored.
 * If not_match, then fail a match if pattern matches.
 * If must_match, then fail a match if pattern is not matched.
 * if !not_match and !must_match, then matching will continue down the pattern stack.
 */
ObjectIterator::SearchPattern::SearchPattern(const char *str, bool is_regex, bool do_caseless, bool _must_match, bool _must_not_match, MatchFunc newfunc)
{
	must_not_match = _must_not_match;
	must_match = _must_match;
	func = newfunc;
	pattern = str;

	if (!func) {
		if (!is_regex) {
			if (do_caseless) func = MatchContainsCaseless;
			else func = MatchContains;
		} else {
			//if (do_caseless) rex.assign(str, std::regex::ECMAScript | std::regex::icase );
			//else rex.assign(str, std::regex::ECMAScript);
			if (do_caseless) rex.assign(str, std::regex::grep | std::regex::icase );
			else rex.assign(str, std::regex::grep);
			func = MatchRegex;
		}
	}
}

ObjectIterator::ObjectIterator()
{
	where = SEARCH_ObjectContainer;
	root = nullptr;
	// oroot = nullptr;
	selection = nullptr;

	max_matches = -1;
	finished = true;
}

ObjectIterator::~ObjectIterator()
{
	if (selection) selection->dec_count();
	if (root) root->dec_count();
	// if (oroot) oroot->dec_count();
}

void ObjectIterator::Clear()
{
	if (root) { root->dec_count(); root = nullptr; }
	// if (oroot) { oroot->dec_count(); oroot = nullptr; }
	if (selection) { selection->dec_count(); selection = nullptr; }
	patterns.flush();
	finished = true;
}

int ObjectIterator::Pattern(MatchFunc func, bool must_match, bool must_not_match, bool keep_current)
{
	if (!keep_current) patterns.flush();

	patterns.push(new SearchPattern(nullptr, false, false, must_match, must_not_match, func));
	return 0;
}

/*! Pattern is either wholely one regex, or is a space separated list of patterns.
 * In the latter, if the chunk starts with a '-' then matches must not have that.
 * If the chunk starts with a '+' then match fails immediately if pattern not matched.
 * If keep_current, don't flush current patterns.
 */
int ObjectIterator::Pattern(const char *str, bool is_regex, bool do_caseless, bool must_match, bool must_not_match, bool keep_current)
{
	if (!keep_current) patterns.flush();

	if (is_regex) patterns.push(new SearchPattern(str, true, do_caseless, must_match, must_not_match, nullptr));
	else {
		char *endptr;
		char *pat;
		do {
			pat = QuotedAttribute(str, &endptr);
			if (!pat) break;
			if (*pat == '-') {
				//matches to the pattern must be ignored
				if (pat[1] != '\0') { //make sure pattern is non-empty
					patterns.push(new SearchPattern(pat+1, false, do_caseless, true, false, nullptr));
				}
			} else if (*pat == '+') {
				//matches to the pattern must be ignored
				if (pat[1] != '\0') { //make sure pattern is non-empty
					patterns.push(new SearchPattern(pat+1, false, do_caseless, false, true, nullptr));
				}
			} else {
				if (*pat) patterns.push(new SearchPattern(pat, false, do_caseless, must_match, must_not_match, nullptr));
			}
			delete[] pat;
			str = endptr;
			while (isspace(*str)) str++;
		} while (str && *str);
	}
	return 0;
}

/*! If o != current root, then flush first and current.
 * Note you still have to Start() or End() to fully initialize the search.
 * If o == current root, then first and current are not flushed, nor must you re-initialize search.
 */
void ObjectIterator::SearchIn(ObjectContainer *o)
{
	if (root != o) {
		if (root) root->dec_count();
		root = o;
		if (root) root->inc_count();

		where = SEARCH_ObjectContainer;
		first.flush();
		current.flush();
	}
}

void ObjectIterator::SearchIn(DrawableObject *o)
{
	if (root != o) {
		if (root) root->dec_count();
		root = o;
		if (root) root->inc_count();

		where = SEARCH_Drawable;
		first.flush();
		current.flush();
	}
}

/*! note: Selections are not currently ObjectContainers, it's flat and easy to search, so we treat it specially.
 *
 *  If o != current root, then flush first and current.
 * Note you still have to Start() or End() to fully initialize the search.
 * If o == current root, then first and current are not flushed, nor must you re-initialize search.
 */
void ObjectIterator::SearchIn(LaxInterfaces::Selection *sel)
{
	if (selection != sel) {
		if (selection) selection->dec_count();
		selection = sel;
		if (selection) selection->inc_count();

		where = SEARCH_Selection;
		first.flush();
		current.flush();
	}
}

/*! Check obj against current patterns.
 */
bool ObjectIterator::Match(anObject *obj)
{
	if (!obj) return false;
	if (patterns.n == 0) return true; //accept everything, such as when simply stepping

	bool matching = false;
	for (int c=0; c<patterns.n; c++) {
		if (matching && !patterns.e[c]->must_not_match && !patterns.e[c]->must_match) continue; //we are already true, or'ing with true is still true
		bool match = patterns.e[c]->func(obj, patterns.e[c]);
		if (patterns.e[c]->must_not_match && match) {
			return false;
		} else if (patterns.e[c]->must_match && !match) {
			return false;
		} else matching |= match;
	}

	return matching;
}


/*! Initialize for stepping forward with Next(), and return the first object, or null if no match.
 */
Laxkit::anObject *ObjectIterator::Start(FieldPlace *where_ret)
{
	if (where == SEARCH_Selection) {
		if (!selection || selection->n() == 0) { finished = true; return nullptr; }

		first.flush();
		current.flush();
		for (int i = 0; i < selection->n(); i++) {
			anObject *obj = selection->e(i)->obj;
			if (Match(obj)) {
				first.push(i);
				current = first;
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				finished = false;
				return obj;
			}
		}
		finished = true;
		return nullptr;
	}

	if (where == SEARCH_ObjectContainer || where == SEARCH_Drawable) {
		if (!root || root->n() == 0) { finished = true; return nullptr; }

		//root is taken to be a "zone" head, and will not ever be matched.
		//When Next() or Previous() iterates to root, search skips it

		first.flush();
		current.flush();
		anObject *obj = nullptr;

		int status;
		do {
			status = root->nextObject(first, 0, Next_Increment, &obj);
			DBG cerr << "ObjectIterator objc status: "<<status<<", obj: "<<(obj ? (obj->Id() ? obj->Id(): "null id") : "null")<<endl;
			if (status != Next_Success || obj == root) break; //note obj might be null, but place is still valid

			if (Match(obj)) {
				current = first;
				cur_obj = obj;
				finished = false;
				if (where_ret) *where_ret = current;
				return obj;
			}
		} while (first.n() && status == Next_Success); //if !n, then we are back at root

		finished = true;
		return nullptr;
	}

	return nullptr;
}

/*! Initialize for stepping with Previous(), and return the first object, or null if no match.
 */
Laxkit::anObject *ObjectIterator::End(FieldPlace *where_ret)
{
	if (where == SEARCH_Selection) {
		if (!selection || selection->n() == 0) { finished = true; return nullptr; }

		first.flush();
		current.flush();
		for (int i = selection->n()-1; i >= 0; i--) {
			anObject *obj = selection->e(i)->obj;
			if (Match(obj)) {
				first.push(i);
				current = first;
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				finished = false;
				return obj;
			}
		}
		finished = true;
		return nullptr;
	}

	if (where == SEARCH_ObjectContainer || where == SEARCH_Drawable) {
		if (!root || root->n() == 0) { finished = true; return nullptr; }

		//root is taken to be a "zone" head, and will not ever be matched.
		//When Next() or Previous() iterates to root, search skips it

		first.flush();
		current.flush();
		anObject *obj = nullptr;

		int status;
		do {
			status = root->nextObject(first, 0, Next_Decrement, &obj);
			DBG cerr << "ObjectIterator objc status: "<<status<<", obj: "<<(obj ? (obj->Id() ? obj->Id(): "null id") : "null")<<endl;
			if (status != Next_Success || obj == root) break;

			if (Match(obj)) {
				current = first;
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				finished = false;
				return obj;
			}
		} while (status == Next_Success);

		finished = true;
		return nullptr;
	}

	return nullptr;
}

/*! Make first = current. Search will wrap around until this object is encountered again.
 * Returns current.
 */
Laxkit::anObject *ObjectIterator::StartFromCurrent(FieldPlace *where_ret)
{
	first = current;
	finished = false;
	if (where_ret) *where_ret = current;
	return cur_obj;
}

/*! Iterator stepper, used after Start().
 */
Laxkit::anObject *ObjectIterator::Next(FieldPlace *where_ret)
{
	if (finished) return nullptr;

	if (where == SEARCH_Selection) {
		if (!selection || selection->n() == 0) { finished = true; return nullptr; }

		int curi = current.e(0);
		int firsti = first.e(0);
		for (int i = (curi+1)%selection->n(); i != firsti; i = (i+1)%selection->n()) {
			anObject *obj = selection->e(i)->obj;
			if (Match(obj)) {
				current.e(0, i);
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				return obj;
			}
		}
		//only way to be here is if (i == firsti)
		finished = true;
		return nullptr;
	}

	if (where == SEARCH_ObjectContainer || where == SEARCH_Drawable) {
		if (!root || root->n() == 0) { finished = true; return nullptr; }

		anObject *obj = nullptr;

		FieldPlace prevcurrent = current;
		int status;
		do {
			status = root->nextObject(current, 0, Next_Increment, &obj);
			DBG cerr << "ObjectIterator objc status: "<<status<<", obj: "<<(obj ? (obj->Id() ? obj->Id(): "null id") : "null")<<endl;
			if (status != Next_Success) break;

			if (current == prevcurrent) break;
			if (obj == root) continue;

			if (Match(obj)) {
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				return obj;
			}
		} while (status == Next_Success);
		//only way to be here is if stepping failed, or search ended
		finished = true;
		return nullptr;
	}

	finished = true;
	return nullptr;
}

/*! Iterator stepper, used after End().
 */
Laxkit::anObject *ObjectIterator::Previous(FieldPlace *where_ret)
{
	if (finished) return nullptr;

	if (where == SEARCH_Selection) {
		if (!selection || selection->n() == 0) return nullptr;

		int curi = current.e(0);
		int firsti = first.e(0);
		for (int i = (curi+selection->n()-1)%selection->n(); i != firsti; i = (i+selection->n()-1)%selection->n()) {
			anObject *obj = selection->e(i)->obj;
			if (Match(obj)) {
				current.e(0, i);
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				return obj;
			}
		}
		//only way to be here is if (i == firsti)
		finished = true;
		return nullptr;
	}

	if (where == SEARCH_ObjectContainer || where == SEARCH_Drawable) {
		if (!root || root->n() == 0) { finished = true; return nullptr; }

		anObject *obj = nullptr;

		FieldPlace prevcurrent = current;
		int status;
		do {
			status = root->nextObject(current, 0, Next_Decrement, &obj);
			DBG cerr << "ObjectIterator objc status: "<<status<<", obj: "<<(obj ? (obj->Id() ? obj->Id(): "null id") : "null")<<endl;
			if (status != Next_Success) break;

			if (current == prevcurrent) break;
			if (obj == root) continue;

			if (Match(obj)) {
				cur_obj = obj;
				if (where_ret) *where_ret = current;
				return obj;
			}
		} while (status == Next_Success);
		//only way to be here is if stepping failed, or search ended
		finished = true;
		return nullptr;
	}

	finished = true;
	return nullptr;
}



} //namespace Laidout

