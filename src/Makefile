###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces

LIBINTL=-lintl
ifeq ($(shell uname -s),Linux)
	LIBINTL=
endif

LD=g++
#LDFLAGS= $(EXTRA_LDFLAGS) -L/usr/local/lib -L/usr/X11R6/lib -rdynamic -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` `pkg-config --libs freetype2`\
#		 `pkg-config --libs GraphicsMagick++` `cups-config --libs` -ldl -lXft -lXrandr $(LIBINTL)
LDFLAGS= $(EXTRA_LDFLAGS) -L/usr/local/lib -L/usr/X11R6/lib -rdynamic `pkg-config --libs $(LAXKIT_PC)`\
		 `cups-config --libs` -ldl -lreadline $(LIBINTL)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS)  -I$(LAXDIR)/.. `pkg-config --cflags freetype2` -I$(POLYPTYCHBASEDIR)


#---optional libs:
ifeq ($(LAIDOUT_USES_SQLITE), yes)
OPTIONALLIBS= -lsqlite3
endif


#---polyptych related:
ifeq ($(LAIDOUT_USING_GL), yes)
#---if gl:
POLYPTYCHLIBS=  -ltiff -lGL -lGLU -lftgl
POLYPTYCH_GL_OBJS=polyptych/src/hedronwindow.o  polyptych/src/glbase.o  polyptych/src/gloverlay.o  polyptych/src/polyrender.o
POLYPTYCHFORLAIDOUT=polyptychgl

else
#---if no gl:
POLYPTYCHLIBS= 
POLYPTYCH_GL_OBJS= 
POLYPTYCHFORLAIDOUT=polyptych
endif

#---either
POLYPTYCHOBJS=polyptych/src/nets.o polyptych/src/poly.o


dirs= \
	api \
	calculator \
	core \
	dataobjects \
	filetypes \
	impositions \
	interfaces \
	nodes \
	plugins \
	printing \
	text \
	ui

objs= \
	laidout-more.o \
	laidout.o 

#object files from other directories. Listing them all explicitly to prevent
#spurious inclusion of temporary testing stuff.
otherobjs= \
	api/buildicons.o \
	api/functions.o \
	api/importexport.o \
	api/openandnew.o \
	api/reimpose.o \
	calculator/calculator.o \
	calculator/curvevalue.o \
	calculator/interpreter.o \
	calculator/shortcuttodef.o \
	calculator/values.o \
	core/document.o \
	core/drawdata.o \
	core/externaltools.o \
	core/fieldplace.o \
	core/guides.o \
	core/importimage.o \
	core/laidoutprefs.o \
	core/objectiterator.o \
	core/page.o \
	core/papersizes.o \
	core/plaintext.o \
	core/project.o \
	core/spreadview.o \
	core/stylemanager.o \
	core/utils.o \
	dataobjects/affinevalue.o \
	dataobjects/bboxvalue.o \
	dataobjects/datafactory.o \
	dataobjects/drawableobject.o \
	dataobjects/epsdata.o \
	dataobjects/fontvalue.o \
	dataobjects/group.o \
	dataobjects/groupinterface.o \
	dataobjects/helpertypes.o \
	dataobjects/imagevalue.o \
	dataobjects/lcaptiondata.o \
	dataobjects/lellipsedata.o \
	dataobjects/lengraverfilldata.o \
	dataobjects/lgradientdata.o \
	dataobjects/limagedata.o \
	dataobjects/limageinterface.o \
	dataobjects/limagepatch.o \
	dataobjects/lmirrorinterface.o \
	dataobjects/lpathsdata.o \
	dataobjects/lperspectiveinterface.o \
	dataobjects/lroundedrectdata.o \
	dataobjects/lsimplepath.o \
	dataobjects/lsomedataref.o \
	dataobjects/ltextonpath.o \
	dataobjects/lvoronoidata.o \
	dataobjects/mysterydata.o \
	dataobjects/objectcontainer.o \
	dataobjects/objectfilter.o \
	dataobjects/pointsetvalue.o \
	dataobjects/printermarks.o \
	filetypes/exportdialog.o \
	filetypes/filefilters.o \
	filetypes/filters.o \
	filetypes/htmlgallery.o \
	filetypes/image-gs.o \
	filetypes/image.o \
	filetypes/importdialog.o \
	filetypes/laidoutimport.o \
	filetypes/pageatlas.o \
	filetypes/pdf.o \
	filetypes/podofoimposeplan.o \
	filetypes/postscript.o \
	filetypes/ppt.o \
	filetypes/scribus.o \
	filetypes/svg.o \
	impositions/box.o \
	impositions/dodecahedron.o \
	impositions/imposition.o \
	impositions/impositioneditor.o \
	impositions/impositions.o \
	impositions/netdialog.o \
	impositions/netimposition.o \
	impositions/polyptychwindow.o \
	impositions/signatureinterface.o \
	impositions/signatures.o \
	impositions/simplenet.o \
	impositions/singles.o \
	impositions/singleseditor.o \
	interfaces/actionarea.o \
	interfaces/aligninterface.o \
	interfaces/anchorinterface.o \
	interfaces/animationinterface.o \
	interfaces/cloneinterface.o \
	interfaces/graphicalshell.o \
	interfaces/nupinterface.o \
	interfaces/objectfilterinterface.o \
	interfaces/objectindicator.o \
	interfaces/pagemarkerinterface.o \
	interfaces/pagerangeinterface.o \
	interfaces/paperinterface.o \
	interfaces/pathintersectionsinterface.o \
	nodes/nodeeditor.o \
	nodes/nodeinterface.o \
	nodes/nodes-dataobjects.o \
	nodes/nodes.o \
	plugins/addonaction.o \
	plugins/plugin.o \
	printing/epsutils.o \
	printing/print.o \
	printing/pscolorpatch.o \
	printing/pseps.o \
	printing/psfilters.o \
	printing/psgradient.o \
	printing/psimage.o \
	printing/psimagepatch.o \
	printing/psout.o \
	printing/pspathsdata.o \
	text/streaminterface.o \
	ui/about.o \
	ui/autosavewindow.o \
	ui/buttonbox.o \
	ui/commandwindow.o \
	ui/externaltoolwindow.o \
	ui/findwindow.o \
	ui/headwindow.o \
	ui/helpwindow.o \
	ui/importimagesdialog.o \
	ui/interfaces.o \
	ui/metawindow.o \
	ui/newdoc.o \
	ui/objecttree.o \
	ui/palettes.o \
	ui/papersizewindow.o \
	ui/plaintextwindow.o \
	ui/pluginwindow.o \
	ui/settingswindow.o \
	ui/spreadeditor.o \
	ui/valuewindow.o \
	ui/viewwindow.o \


all: laidout

#laidout: lax laxinterface $(objs) $(POLYPTYCHFORLAIDOUT)
laidout: lax $(objs) $(POLYPTYCHFORLAIDOUT)
	for NAME in $(dirs); do $(MAKE) -C $$NAME || exit ; done
	$(LD) $(otherobjs) $(objs) $(POLYPTYCH_GL_OBJS) $(POLYPTYCHOBJS) -L$(LAXIDIR) -L$(LAXDIR) $(LDFLAGS) $(OPTIONALLIBS) $(POLYPTYCHLIBS) -o $@

icons:
	cd icons && $(MAKE)
	cd $(LAXDIR)/icons && $(MAKE)
									 
polyptych:
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) forlaidout

polyptychgl:
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) forlaidoutgl

alldirs:
	for NAME in $(dirs); do $(MAKE) -C $$NAME ; done

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME hidegarbage; done

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage -0 *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME unhidegarbage; done

lax:
	cd $(LAXDIR) && $(MAKE)

#laxinterface:
#	cd $(LAXIDIR) && $(MAKE)

#test2: lax laxinterface interfaces test2.o papersizes.o
test2: lax interfaces test2.o papersizes.o
	$(LD) test2.o styles.o dataobjects/group.o dataobjects/objectcontainer.o papersizes.o interfaces/paperinterface.o $(LDFLAGS)  -llaxinterfaces -llaxkit -o $@

testobjs= \
	calculator/calculator.o \
	calculator/curvevalue.o \
	calculator/interpreter.o \
	calculator/shortcuttodef.o \
	calculator/values.o \
	core/fieldplace.o \
	core/plaintext.o \
	core/utils.o \
	dataobjects/affinevalue.o \
	dataobjects/bboxvalue.o \
	dataobjects/datafactory.o \
	dataobjects/drawableobject.o \
	dataobjects/epsdata.o \
	dataobjects/fontvalue.o \
	dataobjects/group.o \
	dataobjects/groupinterface.o \
	dataobjects/helpertypes.o \
	dataobjects/imagevalue.o \
	dataobjects/lcaptiondata.o \
	dataobjects/lellipsedata.o \
	dataobjects/lengraverfilldata.o \
	dataobjects/lgradientdata.o \
	dataobjects/limagedata.o \
	dataobjects/limageinterface.o \
	dataobjects/limagepatch.o \
	dataobjects/lmirrorinterface.o \
	dataobjects/lpathsdata.o \
	dataobjects/lperspectiveinterface.o \
	dataobjects/lroundedrectdata.o \
	dataobjects/lsimplepath.o \
	dataobjects/lsomedataref.o \
	dataobjects/ltextonpath.o \
	dataobjects/lvoronoidata.o \
	dataobjects/mysterydata.o \
	dataobjects/objectcontainer.o \
	dataobjects/objectfilter.o \
	dataobjects/pointsetvalue.o \
	dataobjects/printermarks.o \
	filetypes/exportdialog.o \
	filetypes/filefilters.o \
	filetypes/filters.o \

test: lax $(objs) $(POLYPTYCHFORLAIDOUT) test.o
	for NAME in $(dirs); do $(MAKE) -C $$NAME || exit ; done
	$(LD) $(testobjs) test.o $(POLYPTYCH_GL_OBJS) $(POLYPTYCHOBJS) -L$(LAXIDIR) -L$(LAXDIR) $(LDFLAGS) $(OPTIONALLIBS) $(POLYPTYCHLIBS) text/*.o -o $@

docs:
	cd ../docs && doxygen 

alldocs:
	cd ../docs && doxygen Doxyfile-with-laxkit

depends:
	utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME depends; done
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) depends

include makedepend


.PHONY: clean lax docs $(dirs) alldirs addons hidegarbage unhidegarbage polyptych polyptychgl
clean:
	rm -f laidout *.o
	for NAME in $(dirs); do $(MAKE) -C $$NAME clean; done
	$(MAKE) -C polyptych/src clean

