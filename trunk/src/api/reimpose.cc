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
// Copyright (C) 2009 by Tom Lechner
//

#include "reimpose.h"
#include "../language.h"
#include "../laidout.h"

using namespace Laxkit;


namespace Laidout {


/*! \ingroup api */
StyleDef *makeReimposeStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Reimpose",
			_("Reimpose"),
			_("Replace a document's imposition"),
			"function",
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			ReImposeFunction);

	 //define parameters
	sd->push("document",
			_("Document"),
			_("The document to reimpose. Pulled from context if none given"),
			"string",
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("imposition",
			_("New Imposition"),
			_("The new imposition to use"),
			"any", //VALUE_DynamicEnum, ***
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("scalepages",
			_("Scale Pages"),
			_("Yes or no, whether to scale the old pages to fit the new pages"),
			"boolean",
			NULL,
			"yes",
			0,NULL);
	sd->push("paper",
			_("Paper Size"),
			_("New paper size to use in the new imposition. Use a Paper object or a known paper name. "
			  "A string such as \"tabloid,landscape\" will also work."),
			"any",
			NULL,
			NULL,
			0,NULL);
	sd->pushEnum("orientation",
			_("Orientation"),
			_("Orientation of the paper. Either portrait (0) or landscape (1)."),
			"portrait",
			NULL,NULL,
			"portrait", _("Portrait"), _("Long direction is vertical"),
			"landscape", _("Landscape"), _("Short direction is vertical"),
			NULL
		);
	sd->push("createnew",
			_("Create new"),
			_("Reimpose to a new document"),
			"boolean",
			NULL,
			"no",
			0,NULL);

	return sd;
}

//! General function to reimpose a document.
/*! \ingroup api
 *
 * Return 0 for success, -1 for success with warnings, or 1 for unredeemable failure.
 *
 * Something like "doc=Document(); doc.Reimpose(...)" would not go through this function
 * but instead call Document::FunctionCall("Reimpose",context,parameters).
 *
 * parameters MUST have the exact order of available parameters as specified in the
 * StyleDef for this function. This is normally done in Calculator::parseParameters().
 *
 * \todo *** this is major hack stage, needs to be automated more, with automatic parsing
 *    based on StyleDef objects
 */
int ReImposeFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 ErrorLog &log)
{
	 //grab document from context, and call doc->reimpose() using parameters from the styledef;
	int scalepages=1, createnew=0;
	Imposition *imp=NULL;
	Document *doc=NULL;
	try {
		if (!parameters) throw _("Reimpose needs parameters!");
		//if (!context) return 1; //it's ok if context is NULL

		 //we need:
		 //  1. a document
		 //  2. a new imposition object
		 //  3. optional new paper size
		 //  4. whether to scale pages if they don't fit the new papersize
		 //  5. whether to impose to a new document

		//--------- in an ideal world, it would be as easy as:
		//	 //1.
		//	Document *doc=dynamic_cast<Document*>(parameters->findObject(NULL,0));
		//	if (!doc) doc=dynamic_cast<Document*>context->findObject("document");
		//	if (!doc) return 1;
		//
		//	 //2.
		//	Imposition *imp=NULL;
		//	imp=dynamic_cast<Imposition*>(parameters->findObject(NULL,1));
		//	if (!imp) return 2;
		//
		//	 //3. 
		//	int scalepages=parameters->findInt(NULL,2);
		//
		//	 //4.
		//	PaperStyle *paper=dynamic_cast<PaperStyle*>(parameters->findObject(NULL,3));
		//
		//
		//	if (paper) imp->SetPaperSize(paper);
		//	if (doc->ReImpose(imp,scalepages)==0) return 0;
		//	return 10; //error reimposing!
		//----------------------------------

		 //1.
		const char *str=NULL;
		int i;

		str=parameters->findString("document",-1,&i);
		if (str) {
			 //look in parameters
			if (!str) throw _("You must specify a document to reimpose."); //no doc to reimpose!
			doc=laidout->findDocument(str);
			if (!doc) throw _("Cannot find that document!"); //doc not found!
		}
		if (!doc && context) doc=dynamic_cast<Document*>(context->findObject("document"));
		if (!doc) throw _("You must specify a document to reimpose."); //no doc to impose!

		 //2.
		int ii=parameters->findIndex("imposition");
		if (ii<0) throw _("You must specify an imposition!"); //no new imposition!
		str=parameters->findString("imposition",ii,&i);
		if (i==0) {
			int c2;
			for (c2=0; c2<laidout->impositionpool.n; c2++) {
				if (!strncasecmp(str,laidout->impositionpool.e[c2]->name,strlen(laidout->impositionpool.e[c2]->name))) {
					break;
				}
			}
			if (c2==laidout->impositionpool.n) _("Imposition not found!"); //no imposition to use!
			imp=laidout->impositionpool.e[c2]->Create();
		} else {
			anObject *obj=parameters->findObject("imposition",ii,&i);
			Imposition *impmaybe=dynamic_cast<Imposition*>(obj);
			if (!impmaybe) throw _("Wrong format for imposition!");

			imp=(Imposition*)impmaybe->duplicate();
		}

		 //3.

		 //----orientation
		int orientation=parameters->findInt("orientation",-1,&i);
		if (i==1) orientation=-1;
		if (i==2) throw _("Invalid format for orientation!");

		 //----paper
		PaperStyle *paper=NULL;
		str=parameters->findString("paper",-1,&i);
		if (i==2) {
			paper=dynamic_cast<PaperStyle*>(parameters->findObject("paper"));
			if (paper) paper=(PaperStyle*)paper->duplicate();
		} else if (i==1) {
			//***use a papersize that is big enough to fit;
			cout <<"*** must implement auto compute paper size for reimposition"<<endl;
			//figure out w and h of current document
			//step through list of papers, find one that will fit either WxH or HxW
			//if no fit, then custom size
		} else if (str) {
			 //establish papersize;
			for (int c2=0; c2<laidout->papersizes.n; c2++) {
				if (!strncasecmp(str,laidout->papersizes.e[c2]->name,strlen(laidout->papersizes.e[c2]->name))) {
					paper=laidout->papersizes.e[c2];
					break;
				}
			}
			if (paper) {
				paper=(PaperStyle*)paper->duplicate();
				if (strcasestr(str,"landscape")) paper->flags=1;
				else if (strcasestr(str,"portrait")) paper->flags=0;
			}
		}
		if (!paper) { delete imp; throw _("You must provide a paper size!"); }

		 //apply orientation
		if (orientation>=0) {
			if (orientation) paper->flags|=1;
			else paper->flags&=~1;
		}
		imp->SetPaperSize(paper); // makes a duplicate of paper
		paper->dec_count();

		 //4. scale pages to fit
		i=parameters->findIndex("scalepages");
		if (i>=0) scalepages=parameters->findInt("scalepages", i);

		 //5. create a new document
		i=parameters->findIndex("createnew");
		if (i>=0) createnew=parameters->findInt("createnew", i);


	} catch (const char *error) {
		log.AddMessage(error,ERROR_Fail);
		if (value_ret) *value_ret=NULL;
		return 1;
	}
	
	 //so parameters passed, so now try to actually reimpose...
	if (createnew) {
		 //must duplicate the document, install in laidout, then call reimpose on it
		cerr <<" *** must implement createnew in Reimpose()!!"<<endl;
	}
	int c=doc->ReImpose(imp, scalepages);
	imp->dec_count();

	if (value_ret) *value_ret=NULL;
	return c?1:0;
}


} // namespace Laidout

