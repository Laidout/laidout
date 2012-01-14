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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef ERRORLOG_H
#define ERRORLOG_H


#include "fieldplace.h"
#include <lax/lists.h>

//---------------------------------- ErrorLog -----------------------------

/*! ERROR_Ok, means everything checks out.
 * ERROR_Fail is an extreme error which should interrupt whatever you are doing.
 * ERROR_Warning is a generic error which does not halt anything, but users can attend to it.
 * Other values above ERROR_MAX can be used for other warnings.
 */
enum ErrorSeverity {
		ERROR_Unknown =-1,
		ERROR_Ok      =0,
		ERROR_Fail    =1,
		ERROR_Warning =2,
		//ERROR_Missing_Glyphs,
		//ERROR_Unexposed_Text,
		//ERROR_Has_Transparency,
		//ERROR_Broken_Image,
		//ERROR_Broken_Resource,
		//ERROR_Image_Not_At_Desired_Resolution,
		ERROR_MAX

	};

class ErrorLogNode
{
  public:
	FieldPlace place;
	char *objectstr_id;
	unsigned int object_id;
	char *description;
	int severity; //"ok", "warning", "fail", "version fail"
	int info; //extra info
	ErrorLogNode(unsigned int objid, const char *objidstr, FieldPlace *nplace, const char *desc, int nseverity,int ninfo);
	~ErrorLogNode();
};

class ErrorLog
{
  public:
	Laxkit::PtrStack<ErrorLogNode> messages;

	ErrorLog() {}
	virtual ~ErrorLog() {}
	virtual int AddMessage(const char *desc, int severity, int ninfo=0);
	virtual int AddMessage(unsigned int objid, const char *objidstr, FieldPlace *place, const char *desc, int severity, int ninfo=0);
	virtual const char *Message(int i,int *severity,int *info);
	virtual int Total() { return messages.n; }
	virtual ErrorLogNode *Message(int i);
	virtual const char *MessageStr(int i);
	virtual int Warnings();
	virtual int Errors();
	virtual int Oks();
	virtual void Clear();
};

void dumperrorlog(const char *mes,ErrorLog &log);


#endif

