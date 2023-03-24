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
// Copyright (C) 2004-2013 by Tom Lechner
//

#ifndef PLAINTEXT_H
#define PLAINTEXT_H

#include <lax/resources.h>

#include "../calculator/values.h"

#include <cstdlib>


namespace Laidout {


// //------------------------------ FileRef -------------------------------
// class FileRef : public Laxkit::anObject
// {
//  public:
// 	char *filename;
// 	FileRef(const char *file);
// 	virtual ~FileRef() { if (filename) delete[] filename; }
// 	virtual const char *whattype() { return "FileRef"; }
// };

//------------------------------ PlainText -------------------------------

enum PlainTextType {
	TEXT_Plain,
	TEXT_Component,
	TEXT_Script
};

class PlainText : virtual public Laxkit::Resourceable, virtual public Value, virtual public FunctionEvaluator
{
 public:
	int texttype;
	int textsubtype;
	clock_t lastmodtime;
	clock_t lastfiletime;
	char *thetext;
	char *filename;
	bool loaded;

	PlainText();
	PlainText(const char *newtext);
	virtual ~PlainText();
	virtual const char *whattype() { return "PlainText"; }
	virtual const char *Filename();
	virtual const char *Filename(const char *newfile);
	virtual int SaveText();
	virtual int SetText(const char *newtext);
	virtual const char *GetText();
	virtual int LoadFromFile(const char *fname);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

	virtual bool IsTemporary() { return ResourceOwner() == nullptr; }

	//from Value:
	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
	virtual Value *dereference(const char *extstring, int len);

	//from FunctionEvaluator:
	virtual int Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log);

	static void uniqueName(PlainText *obj);
};


} //namespace Laidout

#endif



