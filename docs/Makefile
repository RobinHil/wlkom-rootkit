VENV   = .venv
PIP    = $(VENV)/bin/pip
MKDOCS = $(VENV)/bin/mkdocs
DIST   = ./build

.PHONY: all venv build serve clean

all: build

venv:
	python3 -m venv $(VENV)
	$(PIP) install -r requirements.txt

build: venv
	$(MKDOCS) build -d $(DIST)

serve: venv
	$(MKDOCS) serve

clean:
	$(RM) -r $(VENV) $(DIST)
