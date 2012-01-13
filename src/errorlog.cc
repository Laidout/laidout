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

#include "errorlog.h"
#include <lax/lists.cc>


//---------------------------------- ErrorLog -----------------------------


//! Dump to cout.
int dumperrorlog(const char *mes,ErrorLog &log)
{
	if (mes) cout <<mes<<"("<<log.Total()<<")"<<endl;
	ErrorLogNode *e;
	for (int c=0; c<log.Total(); c++) {
		e=log.Message(c);
		if (e->severity==ERROR_Ok) cout <<"Ok: ";
		else if (e->severity==ERROR_Warning) cout <<"Warning: ";
		else if (e->severity==ERROR_Fail) cout <<"Error! ";

		cout <<e->description<<", id:"<<e->object_id<<","<<(e->objectstr_id?e->objectstr_id:"(no str)")<<" ";
		if (place.n()) place.out(NULL);
		cout <<endl;

	}
}

//---------------------------------- ErrorLogNode
/*! \class ErrorLogNode
 * \brief Stack node for ErrorLog class.
 */

ErrorLogNode::ErrorLogNode(unsigned int objid, const char *objidstr, FieldPlace *nplace, const char *desc, int nseverity, int ninfo)
{
	if (nplace) place=*nplace;
	description=newstr(desc);
	severity=nseverity;
	object_id=objid;
	objectstr_id=newstr(objidstr);
	info=ninfo;
}

ErrorLogNode::~ErrorLogNode()
{
	if (description) delete[] description;
	if (objectstr_id) delete[] objectstr_id;
}


//---------------------------------- ErrorLog
/*! \class ErrorLog
 * \brief Class to simplify keeping track of offending objects.
 *
 * This is used by importers and exporters to tag and describe various incompatibilities.
 * It allows one to quickly locate problems.
 */

const char *ErrorLog::Message(int i,int *severity,int *info)
{
	if (i<0 || i>=messages.n) {
		*severity=messages.e[i]->severity;
		*info=messages.e[i]->info;
		return messages.e[i]->description;
	}

	if (severity) *severity=-1;
	if (info) *info=0;
	return NULL;
}

int ErrorLog::AddMessage(const char *desc, int severity, int ninfo)
{
	return AddMessage(0,NULL,NULL,desc,severity,ninfo)
}

/*! Returns number of messages including this one.
 */
int ErrorLog::AddMessage(unsigned int objid, const char *objidstr, FieldPlace *place, const char *desc, int severity, int ninfo)
{
	messages.push(new ErrorLogNode(objid,objidstr,place, desc,severity,ninfo));
	return messages.n;
}

ErrorLogNode *ErrorLog::Message(int i)
{
	if (i>=0 && i<messages.n) return messages.e[i];
	return NULL;
}

//! Return the number of ok notes.
int ErrorLog::Oks()
{
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Ok) n++;
	return n;
}

//! Return the number of warnings.
int ErrorLog::Warnings()
{
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Warning) n++;
	return n;
}

//! Return the number of failing errors.
int ErrorLog::Errors()
{
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Fail) n++;
	return n;
}

