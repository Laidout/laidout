#$Id$

include ../../Makefile-toinclude

	#$(foreach NAME, `ls *.po`, $(shell msgfmt $(NAME) -o `basename $(NAME) .po`.mo))
compile:
	@echo "Compiling po files... $(wildcard *.po)."
	$(foreach NAME, $(basename $(shell ls *.po)), $(shell msgfmt $(NAME).po -o $(NAME).mo))
	@echo "Done!"

install:
	@echo Installing language files: $(wildcard *.mo)...
	$(foreach NAME, $(basename $(wildcard *.mo)), \
		$(shell	install -D -m644 $(NAME).mo $(PO) $(LANGUAGE_PATH)/locale/$(NAME)/LC_MESSAGES/laidout-$(LAIDOUTVERSION).mo))
	@echo Done installing language files.

#PO=$(foreach NAME, $(basename $(wildcard *.mo)), $(shell echo temp-install/locale/$(NAME)/LC_MESSAGES/laidout.mo >> temp))
installhere:
	@echo Installing language files...
	$(foreach NAME, $(basename $(wildcard *.mo)), \
		$(shell	install -D -m755 $(NAME).mo $(PO) temp-install/locale/$(NAME)/LC_MESSAGES/laidout-$(LAIDOUTVERSION).mo))
	@echo Done installing language files.

uninstall:
	@echo Un-installing language files based on: $(wildcard *.mo)...
	$(foreach NAME, $(basename $(wildcard *.mo)), \
		$(shell	rm -vf $(LANGUAGE_PATH)/locale/$(NAME)/LC_MESSAGES/laidout-$(LAIDOUTVERSION).mo))
	@echo Done uninstalling language files.

update: freshdump
	msgmerge -U laidout.pot laidout-freshdump.pot

 #generate POTFILES
potfiles:
	find .. -name '*.cc' > POTFILES

potfiles-withlax:
	find .. -name '*.cc' > POTFILES
	find ../../laxkit/lax -name '*.cc' >> POTFILES

freshdump:
	xgettext -C -d laidout  --files-from POTFILES -o laidout-freshdump.pot --keyword=_ --flag=_:1:pass-c-format

clean:
	rm -f *.mo
	rm -f laidout-freshdump.pot


.PHONY: potfiles potfiles-withlax rawdump update compile clean all install installhere
