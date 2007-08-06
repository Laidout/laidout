#$Id$


	#$(foreach NAME, `ls *.po`, $(shell msgfmt $(NAME) -o `basename $(NAME) .po`.mo))
compile:
	@echo "Compiling po files..."
	$(foreach NAME, $(basename $(shell ls *.po)), $(shell msgfmt $(NAME).po -o $(NAME).mo))
	@echo "Done!"

update: freshdump
	msgmerge -U laidout.pot laidout-freshdump.pot

 #generate POTFILES
potfiles:
	find .. -name '*.cc' > POTFILES

freshdump:
	xgettext -C -d laidout  --files-from POTFILES -o laidout-freshdump.pot --keyword=_ --flag=_:1:pass-c-format

clean:
	rm -f *.mo
	rm -f laidout-freshdump.pot


.PHONY: potfiles rawdump update compile clean all
