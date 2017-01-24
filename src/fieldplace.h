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
// Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef FIELDPLACE_H
#define FIELDPLACE_H

#include <lax/lists.h>



namespace Laidout {

//------------------------------ FieldExtPlace -------------------------------------------

class FieldExtPlace : protected Laxkit::PtrStack<char>
{
	class ExtNode {
	  public:
		char *ext;
		int exti;
		ExtNode(const char *str, int i);
		~ExtNode();
	};
	Laxkit::PtrStack<ExtNode> ext;
 public:
	FieldExtPlace() {}
	FieldExtPlace(const char *str,int len=-1);
	FieldExtPlace(const FieldExtPlace &place);
	virtual ~FieldExtPlace() {}
	int Set(const char *str, int len, const char **next);
	virtual int n() const { return ext.n; }
	virtual char *e(int i, int *ei=NULL) const;
	virtual int e(int i,const char *val, int ei);
	virtual int operator==(const FieldExtPlace &place) const;
	virtual FieldExtPlace &operator=(const FieldExtPlace &place);
	virtual int push(const char *nd,int where=-1);
	virtual int push(int i,int where=-1);
	virtual char *pop(int *i, int which=-1);
	virtual int remove(int which=-1);
	virtual void flush() { ext.flush(); }
	virtual void out(const char *str);//for debugging
};


//------------------------------ FieldPlace -------------------------------------------

class FieldPlace : protected Laxkit::NumStack<int>
{
 public:
	FieldPlace() {}
	FieldPlace(const FieldPlace &place);
	virtual ~FieldPlace() {}
	virtual int n() const { return Laxkit::NumStack<int>::n; }
	virtual int e(int i) const { if (i>=0 && i<Laxkit::NumStack<int>::n) { return Laxkit::NumStack<int>::e[i]; }  return -1; }
	virtual int e(int i,int val) { if (i>=0 && i<Laxkit::NumStack<int>::n) 
		{ return Laxkit::NumStack<int>::e[i]=val; }  return -1; }
	virtual int operator==(const FieldPlace &place) const;
	virtual FieldPlace &operator=(const FieldPlace &place);
	virtual int push(int nd,int where=-1) { return Laxkit::NumStack<int>::push(nd,where); }
	virtual int pop(int which=-1) { return Laxkit::NumStack<int>::pop(which); }
	virtual void flush() { Laxkit::NumStack<int>::flush(); }
	virtual const int *list() { return (const int *)Laxkit::NumStack<int>::e; }
	virtual void out(const char *str);//for debugging
};


//------------------------------ FieldMask -------------------------------------------

class FieldMask : public Laxkit::PtrStack<FieldPlace>
{
 protected:
	int *chartoint(const char *ext,const char **next_ret);
 public:
	FieldMask();
	FieldMask(const char *ext);
	FieldMask(const FieldMask &mask);
	FieldMask &operator=(FieldMask &mask);
	FieldMask &operator=(FieldPlace &place);
	virtual int operator==(int what);
	virtual int operator==(const FieldMask &mask);
	virtual FieldMask &operator=(int f); // shorthand for single level fields
	//virtual int push(FieldMask &f);
	virtual int push(int where,int n, ...);
	virtual int push(int n,int *list,int where=-1);
	virtual int has(const char *ext,int *index=NULL); // does ext correspond to any in mask?
	virtual int value(int f,int where);
	virtual int value(int f,int where,int newval);
};



} // namespace Laidout
#endif

