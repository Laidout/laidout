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

#include "plaintext.h"
#include "stylemanager.h"
#include "../language.h"

#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/interfaces/interfacemanager.h>

#include <sys/times.h>


using namespace Laxkit;


namespace Laidout {



//to ward off segfaults in calls to times()
static struct tms tmptimestruct;


// //------------------------------ FileRef -------------------------------
// /*! \class FileRef 
//  * \ingroup misc
//  * \brief Reference counted pointer to an external file.
//  *
//  * At some point, this class may become the basis for an external link manager.
//  * Then again, it might not.
//  *
//  * \todo put me somewhere useful, or remove me!!
//  */
// 
// FileRef::FileRef(const char *file)
// { filename=newstr(file); }

//------------------------------ PlainText ------------------------------

/*! \enum PlainTextType
 * \brief Enum for PlainText::texttype.
 */

/*! \class PlainText
 * \brief Holds plain text, which is text with no formatting.
 *
 * These are for holding random notes and scripts. Actual text loading can 
 * be defered. This means that the actual text may or may
 * not be in PlainText::thetext. If filename!=nullptr, then calls to GetText() will
 * then load the text in.
 *
 * Ultimately, they might contain something like Latex code that an EPS
 * grabber might run to get formulas.... Big todo!!
 *
 * All text is assumed to be utf8, even that contained in files currently.
 */
/*! \var int PlainText::textsubtype
 * If texttype is TEXT_Script, then this is the id of the interpreter to use
 * to run the script.  When other interpreters like Python or Perl are available in Laidout,
 * this will be more useful. Right now, it can only be run as default
 * laidout script. If this value is -1, then this text is assumed to not be runnable,
 */

PlainText::PlainText()
{
	thetext      = nullptr;
	filename     = nullptr;
	texttype     = TEXT_Plain;
	textsubtype  = -1;
	lastmodtime  = 0;
	lastfiletime = 0;
	loaded       = false;
}

PlainText::PlainText(const char *newtext)
 : PlainText()
{
	SetText(newtext);
}

PlainText::~PlainText()
{
	if (thetext) delete[] thetext;
	if (filename) delete[] filename;
}

const char *PlainText::Filename()
{
	return filename;
}

//! Cause the object to be marked as saving to the given file.
/*! This does not actually read or write anything to newfile.
 * Returns pointer to this->filename.
 */
const char *PlainText::Filename(const char *newfile)
{
	makestr(filename,newfile);
	return  filename;
}

//! Save the text to Filename(), completely overwriting what was there.
/*! Save to the file in filename. 
 * Return 0 for success, or 1 for no filename, or some other number
 * for other error.
 *
 * Updates lastfiletime to the current time.
 */
int PlainText::SaveText()
{
	if (!Filename()) return 1;
	FILE *f=fopen(Filename(),"w");
	if (!f) return 2;
	fwrite(thetext,1,strlen(thetext),f);
	fclose(f);
	lastfiletime=times(&tmptimestruct);
	return 0;
}

//! Clear whatever is in the object, and replace with file data.
/*! name will become something like "file whatever.txt".
 *
 * Return 0 for success, nonzero for error and nothing changed.
 */
int PlainText::LoadFromFile(const char *fname)
{
	char *filetext = read_in_whole_file(fname,nullptr);
	if (!filetext) return 1;
	
	makestr(filename, fname);
	if (isblank(Id())) {
		Id(lax_basename(fname));
	}
	if (thetext) delete[] thetext;
	thetext = filetext;
	texttype = TEXT_Plain;
	textsubtype = -1;
	lastmodtime = lastfiletime = times(&tmptimestruct);
	loaded = true;
	return 0;
}

//! Update the text, and set lastmodtime to the current time.
/*! Return 0 for success, or nonzero for error.
 */
int PlainText::SetText(const char *newtext)
{
	makestr(thetext,newtext);
	lastmodtime=times(&tmptimestruct);
	return 0;
}

//! Return relevant text.
/*! If thetext==nullptr, and filename!=nullptr, then read in all of file into
 * thetext, and return thetext, also setting lastmodtime.
 */
const char *PlainText::GetText()
{
	if (thetext) return thetext;

	if (!S_ISREG(file_exists(Filename(),1,nullptr))) return nullptr;

	int size = file_size(Filename(),1,nullptr);
	if (size <= 0) return nullptr;

	FILE *f = fopen(Filename(),"r");
	if (!f) return nullptr;
	thetext = new char[size+1];
	int actuallyread = fread(thetext,1,size,f);
	if (actuallyread > size || actuallyread <= 0) {
		delete thetext;
		thetext = nullptr;
	} else thetext[actuallyread]='\0';

	fclose(f);
	lastmodtime = lastfiletime = times(&tmptimestruct);
	loaded = true;
	return thetext;
}

/*! If filename!=nullptr, then filename is output, but thetext is ignored, and 
 * assumed to have been saved in filename already.
 *
 * <pre>
 *  name tname   #an id for this text object
 *  filename     #the file containing the text
 *  text \       #if no filename, then the text is stored here
 *    some text
 *    .
 *    more text after blank line
 *    final line of text
 * </pre>
 */
void PlainText::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%stexttype     #the type of text. this is just a hint\n",spc);
		fprintf(f,"%stextsubtype  #the subtype for texttype. this is just a hint\n",spc);
		fprintf(f,"%sname tname   #an id for this text object\n",spc);
		fprintf(f,"%sfilename     #the file containing the text\n",spc);
		fprintf(f,"%stext \\       #if no filename, then the text is stored here\n",spc);
		fprintf(f,"%s  text... the following line is actually blank\n",spc);
		fprintf(f,"%s  .\n",spc);
		fprintf(f,"%s  the final line of text\n",spc);
		return;
	}
	fprintf(f,"%stexttype %d\n",spc,texttype);
	fprintf(f,"%stextsubtype %d\n",spc,textsubtype);
	if (!isblank(Id())) fprintf(f,"%sname %s\n",spc,Id());
	if (!isblank(Filename())) fprintf(f,"%sfilename %s\n",spc,Filename());
	else if (thetext) {
		fprintf(f,"%stext \\\n",spc);
		Laxkit::dump_out_indented(f,indent+2,thetext);
	}
}

void PlainText::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *nme,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		nme  =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(nme,"name")) {
			if (!isblank(value)) Id(value);
		} else if (!strcmp(nme,"text")) {
			makestr(thetext,value);
		} else if (!strcmp(nme,"filename")) {
			if (isblank(value)) continue;
			Filename(value);
		}
	}
}

/*! Does NOT copy file or owner.
 */
Value *PlainText::duplicateValue()
{
	PlainText *obj = new PlainText(thetext);
	obj->texttype = texttype;
	obj->textsubtype = textsubtype;
	//makestr(obj->name, name);
	return obj;
}

Value *NewPlainText()
{
	return new PlainText();
}

ObjectDef *PlainText::makeObjectDef()
{
	ObjectDef *def = stylemanager.FindDef("PlainText");
    if (def) {
        def->inc_count();
        return def;
    }

	def = new ObjectDef(nullptr,
			"PlainText",
            _("Plain Text"),
            _("Plain Text"),
            NewPlainText,nullptr);
	stylemanager.AddObjectDef(def, 0);

	def->pushVariable("text",_("Text"),_("Text"), "string",0,   NULL,0);
	def->pushVariable("file",_("File"),_("File that contains the text"), "File",0,   NULL,0);
	def->pushFunction("IsLoaded", _("Is loaded"), _("If from file, whether text is loaded. Always true if not from file."), NULL, NULL);

	return def;
}

int PlainText::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log)
{
	if (isName(function, len, "IsLoaded")) {
		*value_ret = new BooleanValue(isblank(filename) ? true : loaded);
		return 0;
	}
	return -1;
}

Value *PlainText::dereference(const char *extstring, int len)
{
	if (isName(extstring,len, "text")) return new StringValue(thetext);
	if (isName(extstring,len, "file")) return new FileValue(filename);
	
	return nullptr;
}


//------------- static func:

//! Make obj->name be a new name not found in the project.
void PlainText::uniqueName(PlainText *obj)
{
	if (!obj) return;
	if (isblank(obj->Id())) obj->Id(_("Text object"));

	LaxInterfaces::InterfaceManager *imanager = LaxInterfaces::InterfaceManager::GetDefault(true);
	ResourceManager *rm = imanager->GetResourceManager();
	ResourceType *objs = rm->FindType("PlainText");
	if (!objs) return; //no PlainText resources yet, so name definitely unique
	objs->MakeNameUnique(obj->object_idstr);
}


} //namespace Laidout

