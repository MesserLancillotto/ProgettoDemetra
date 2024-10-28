#!/bin/sh


pdflatex $1.tex
rm *.aux *.log *.toc
firefox $1.pdf
