.PHONY: clean

all: doc manual.pdf

doc:
	doxygen doxyfiles/Doxyfile

manual.pdf: doc
	make -C doc/latex
	mv doc/latex/refman.pdf $@
	
clean:
	rm -rf doc
