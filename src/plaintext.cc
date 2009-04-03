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
// Copyright (C) 2004-2009 by Tom Lechner
//

#include "plaintext.h"
#include "language.h"

#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <sys/times.h>

using namespace LaxFiles;


//to ward off segfaults in calls to times()
static struct tms tmptimestruct;


//------------------------------ FileRef -------------------------------
/*! \class FileRef 
 * \ingroup misc
 * \brief RefCounted pointer to an external file.
 *
 * At some point, this class may become the basis for an external link manager.
 * Then again, it might not.
 *
 * \todo put me somewhere useful, or remove me!!
 */

FileRef::FileRef(const char *file)
{ filename=newstr(file); }

//------------------------------ PlainText ------------------------------

/*! \enum PlainTextType
 * \brief Enum for PlainText::texttype.
 */

/*! \class PlainText
 * \brief Holds plain text, which is text with no formatting.
 *
 * These are for holding random notes and scripts. Actual text loading can 
 * be defered. This means that the actual text may or may
 * not be in PlainText::thetext. If filename!=NULL, then calls to GetText() will
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
	thetext=NULL;
	name=NULL;
	filename=NULL;
	owner=NULL;
	texttype=TEXT_Temporary;
	textsubtype=-1;
	lastmodtime=lastfiletime=0;
}

PlainText::~PlainText()
{
	if (thetext) delete[] thetext;
	if (filename) delete[] name;
	if (name) delete[] name;
	if (owner && dynamic_cast<RefCounted *>(owner))
		dynamic_cast<RefCounted *>(owner)->dec_count();
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
	char *filetext=read_in_whole_file(fname,NULL);
	if (!filetext) return 1;
	
	makestr(filename,fname);
	makestr(name,lax_basename(fname));
	prependstr(name,_("file: "));
	if (thetext) delete[] thetext;
	thetext=filetext;
	texttype=TEXT_Temporary;
	textsubtype=-1;
	lastmodtime=lastfiletime=times(&tmptimestruct);
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
/*! If thetext==NULL, and filename!=NULL, then read in all of file into
 * thetext, and return thetext, also setting lastmodtime.
 */
const char *PlainText::GetText()
{
	if (thetext) return thetext;

	if (!S_ISREG(file_exists(Filename(),1,NULL))) return NULL;

	int size=file_size(Filename(),1,NULL);
	if (size<=0) return NULL;

	FILE *f=fopen(Filename(),"r");
	if (!f) return NULL;
	thetext=new char[size+1];
	int actuallyread=fread(thetext,1,size,f);
	if (actuallyread>size || actuallyread<=0) {
		delete thetext;
		thetext=NULL;
	} else thetext[actuallyread]='\0';

	fclose(f);
	lastmodtime=lastfiletime=times(&tmptimestruct);
	return thetext;
}

/*! If filename!=NULL, then filename is output, but thetext is ignored, and 
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
void PlainText::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
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
	if (!isblank(name)) fprintf(f,"%sname %s\n",spc,name);
	if (!isblank(Filename())) fprintf(f,"%sfilename %s\n",spc,Filename());
	else if (thetext) {
		fprintf(f,"%stext \\\n",spc);
		LaxFiles::dump_out_indented(f,indent+2,thetext);
	}
}

void PlainText::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *nme,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		nme  =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(nme,"name")) {
			makestr(name,value);
		} else if (!strcmp(nme,"text")) {
			makestr(thetext,value);
		} else if (!strcmp(nme,"filename")) {
			if (isblank(value)) continue;
			Filename(value);
		}
	}
}

