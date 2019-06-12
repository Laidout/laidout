###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= $(EXTRA_LDFLAGS) -L/usr/local/lib -L/usr/X11R6/lib -rdynamic -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` `pkg-config --libs freetype2`\
		 `pkg-config --libs GraphicsMagick++` `cups-config --libs` -ldl -lXft -lXrandr
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
	ui

objs= \
	laidout-more.o \
	laidout.o 

#object files from other directories. Listing them all explicitly to prevent
#spurious inclusion of temporary testing stuff.
otherobjs= \
	core/document.o \
	core/drawdata.o \
	core/fieldplace.o \
	core/guides.o \
	core/importimage.o \
	core/laidoutprefs.o \
	core/page.o \
	core/papersizes.o \
	core/plaintext.o \
	core/project.o \
	core/spreadview.o \
	core/stylemanager.o \
	core/utils.o \
	calculator/values.o \
	calculator/calculator.o \
	calculator/interpreter.o \
	calculator/curvevalue.o \
	calculator/shortcuttodef.o \
	impositions/imposition.o \
	impositions/impositioneditor.o \
	impositions/singles.o \
	impositions/impositions.o \
	impositions/dodecahedron.o \
	impositions/box.o \
	impositions/netimposition.o \
	impositions/simplenet.o \
	impositions/signatures.o \
	impositions/signatureinterface.o \
	impositions/netdialog.o \
	impositions/polyptychwindow.o \
	impositions/singleseditor.o \
	plugins/plugin.o \
	api/reimpose.o \
	api/openandnew.o \
	api/importexport.o \
	api/functions.o \
	interfaces/actionarea.o \
	interfaces/nupinterface.o \
	interfaces/aligninterface.o \
	interfaces/anchorinterface.o \
	interfaces/pagerangeinterface.o \
	interfaces/graphicalshell.o \
	interfaces/objectindicator.o \
	interfaces/paperinterface.o \
	interfaces/cloneinterface.o \
	interfaces/animationinterface.o \
	interfaces/objecttree.o \
	interfaces/pagemarkerinterface.o \
	interfaces/objectfilterinterface.o \
	nodes/nodeinterface.o \
	nodes/nodeeditor.o \
	nodes/nodes-dataobjects.o \
	nodes/nodes.o \
	printing/print.o \
	printing/psfilters.o \
	printing/pscolorpatch.o \
	printing/psgradient.o \
	printing/psimage.o \
	printing/psimagepatch.o \
	printing/pspathsdata.o \
	printing/pseps.o \
	printing/epsutils.o \
	printing/psout.o \
	filetypes/filefilters.o \
	filetypes/filters.o \
	filetypes/exportdialog.o \
	filetypes/importdialog.o \
	filetypes/image.o \
	filetypes/image-gs.o \
	filetypes/laidoutimport.o \
	filetypes/htmlgallery.o \
	filetypes/pdf.o \
	filetypes/podofoimposeplan.o \
	filetypes/postscript.o \
	filetypes/ppt.o \
	filetypes/scribus.o \
	filetypes/svg.o \
	dataobjects/group.o \
	dataobjects/objectcontainer.o \
	dataobjects/objectfilter.o \
	dataobjects/drawableobject.o \
	dataobjects/datafactory.o \
	dataobjects/groupinterface.o \
	dataobjects/epsdata.o \
	dataobjects/limagepatch.o \
	dataobjects/limagedata.o \
	dataobjects/limageinterface.o \
	dataobjects/lgradientdata.o \
	dataobjects/lperspectiveinterface.o \
	dataobjects/lpathsdata.o \
	dataobjects/mysterydata.o \
	dataobjects/lsomedataref.o \
	dataobjects/lengraverfilldata.o \
	dataobjects/lcaptiondata.o \
	dataobjects/ltextonpath.o \
	dataobjects/lvoronoidata.o \
	dataobjects/printermarks.o \
	ui/about.o \
	ui/autosavewindow.o \
	ui/buttonbox.o \
	ui/commandwindow.o \
	ui/headwindow.o \
	ui/helpwindow.o \
	ui/metawindow.o \
	ui/importimagesdialog.o \
	ui/newdoc.o \
	ui/palettes.o \
	ui/plaintextwindow.o \
	ui/settingswindow.o \
	ui/spreadeditor.o \
	ui/viewwindow.o \
	ui/valuewindow.o \
	ui/interfaces.o \


all: laidout

laidout: lax laxinterface $(objs) $(POLYPTYCHFORLAIDOUT)
	for NAME in $(dirs); do $(MAKE) -C $$NAME || exit ; done
	$(LD) $(otherobjs) $(objs) $(POLYPTYCH_GL_OBJS) $(POLYPTYCHOBJS) -L$(LAXIDIR) -L$(LAXDIR) -llaxinterfaces -llaxkit -lharfbuzz -lcairo -lfontconfig $(LDFLAGS) $(OPTIONALLIBS) -lreadline -lcrypto $(POLYPTYCHLIBS) -o $@

icons:
	cd src/icons && $(MAKE)
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

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

test2: lax laxinterface interfaces test2.o papersizes.o
	$(LD) test2.o styles.o dataobjects/group.o dataobjects/objectcontainer.o papersizes.o interfaces/paperinterface.o $(LDFLAGS)  -llaxinterfaces -llaxkit -o $@

test: lax laxinterface $(objs) $(POLYPTYCHFORLAIDOUT)
	for NAME in $(dirs); do $(MAKE) -C $$NAME || exit ; done
	$(LD) $(CPPFLAGS) $(otherobjs) test.cc $(POLYPTYCH_GL_OBJS) $(POLYPTYCHOBJS) -L$(LAXIDIR) -L$(LAXDIR) -llaxinterfaces -llaxkit -lharfbuzz -lcairo -lfontconfig $(LDFLAGS) $(OPTIONALLIBS) -lreadline -lcrypto $(POLYPTYCHLIBS) -o $@

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

