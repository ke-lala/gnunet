OUT=pdf_build_artifacts
BUILD=texi_tmp
TEXI=texi2pdf -q
#--build-dir=$(BUILD)

bilanz/pdf:
	mkdir -p $(BUILD)
	mkdir -p $(OUT)
	$(TEXI) --output=$(OUT)/bilanz2014.pdf bilanz2014.tex
	$(TEXI) --output=$(OUT)/bilanz2015.pdf bilanz2015.tex
	$(TEXI) --output=$(OUT)/bilanz2016.pdf bilanz2016.tex
	$(TEXI) --output=$(OUT)/bilanz2017.pdf bilanz2017.tex

letters/pdf:
	mkdir -p $(BUILD)
	mkdir -p $(OUT)/letters
	$(TEXI) --output=$(OUT)/letters/telekomiker.pdf letters/telekomiker.tex

misc/pdf:
	mkdir -p $(BUILD)
	mkdir -p $(OUT)
	$(TEXI) --output=$(OUT)/copyright.pdf copyright.tex
	$(TEXI) --output=$(OUT)/eroeffnungsbilanz.pdf eroeffnungsbilanz.tex
	$(TEXI) --output=$(OUT)/gnunet_pep_agreement.pdf gnunet_pep_agreement.tex
	$(TEXI) --output=$(OUT)/gnunet_taler_agreement.pdf gnunet_taler_agreement.tex
	$(TEXI) --output=$(OUT)/satzung.pdf satzung.tex

references/pdf:
	mkdir -p $(BUILD)
	mkdir -p $(OUT)/references
	$(TEXI) --output=$(OUT)/references/reference-nlnet.pdf references/reference-nlnet.tex

all: references/pdf misc/pdf letters/pdf bilanz/pdf

clean:
	rm *.aux *.log *.out
	rm -rf $(OUT) $(BUILD)
