VENV    = .venv
PIP     = $(VENV)/bin/pip
ROOTKIT = $(VENV)/bin/rootkit

.PHONY: all venv start install glob-install clean

all: start

venv:
	python3 -m venv $(VENV)
	$(PIP) install -r requirements.txt

install: venv
	$(PIP) install -e .

glob-install:
	pip3 install -r requirements.txt
	pip3 install .

start: install
	$(ROOTKIT)

clean:
	$(RM) -r $(VENV) *.egg-info build __pycache__
