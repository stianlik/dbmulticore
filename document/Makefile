name = 'fordypning'
tmp_files = $(name).aux $(name).log $(name).bbl $(name).blg $(name).out $(name).toc

ALL:
	pdflatex $(name).tex
	bibtex $(name).aux
	pdflatex $(name).tex
	pdflatex $(name).tex
clean:
	rm $(tmp_files)
