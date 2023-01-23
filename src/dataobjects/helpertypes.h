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
#ifndef HELPERTYPES_H
#define HELPERTYPES_H

#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/fillstyle.h>

#include "../calculator/values.h"



namespace Laidout {


//------------------------------- LLineStyle ---------------------------------------

class LLineStyle : public Value
{
  public:
	static Value *newLLineStyle();

	LaxInterfaces::LineStyle *linestyle;

	LLineStyle(LaxInterfaces::LineStyle *style = nullptr);
	virtual ~LLineStyle();
	virtual const char *whattype();
	virtual void Set(LaxInterfaces::LineStyle *style, bool absorb);

	//from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);


	//from Value:
	virtual Value *duplicate();
    virtual ObjectDef *makeObjectDef();
    virtual Value *dereference(const char *extstring, int len);
	virtual int assign(Value *v, const char *extstring);
    //virtual int assign(FieldExtPlace *ext,Value *v);
//    virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//                         Value **value_ret, Laxkit::ErrorLog *log);

    //from anObject:
	virtual anObject *duplicate(anObject *ref) { return duplicate(); }
};


//------------------------------- LFillStyle ---------------------------------------

class LFillStyle : public Value
{
  public:
	static Value *newLFillStyle();

	LaxInterfaces::FillStyle *fillstyle;

	LFillStyle(LaxInterfaces::FillStyle *style = nullptr);
	virtual ~LFillStyle();
	virtual const char *whattype();
	virtual void Set(LaxInterfaces::FillStyle *style, bool absorb);

	//from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);

	//from Value:
	virtual Value *duplicate();
    virtual ObjectDef *makeObjectDef();
    virtual Value *dereference(const char *extstring, int len);
	virtual int assign(Value *v, const char *extstring);

    //from anObject:
	virtual anObject *duplicate(anObject *ref) { return duplicate(); }
};


} //namespace Laidout

#endif


