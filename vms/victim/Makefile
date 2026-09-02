UBUNTU_IMG  = focal-server-cloudimg-amd64.img

.PHONY: all start reset clean

all: start

start:
	sh run.sh $(UBUNTU_IMG)

reset: clean start

clean:
	rm -rf build
