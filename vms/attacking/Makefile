ROCKY_QCOW = Rocky-10-GenericCloud-Base.latest.x86_64.qcow2

.PHONY: all start reset clean

all: start

start:
	sh run.sh $(ROCKY_QCOW)

reset: clean start

clean:
	rm -rf build
