.PHONY: all start victim attacking reset docs docs-serve clean

all: start

start: victim attacking

victim:
	$(MAKE) -C vms/victim start

attacking:
	$(MAKE) -C vms/attacking start

reset: victim-reset attacking-reset

victim-reset:
	$(MAKE) -C vms/victim reset

attacking-reset:
	$(MAKE) -C vms/attacking reset

docs:
	$(MAKE) -C docs build

docs-serve:
	$(MAKE) -C docs serve

clean:
	$(MAKE) -C docs clean
	$(MAKE) -C vms/victim clean
	$(MAKE) -C vms/attacking clean
