.DEFAULT_GOAL := run
.PHONY: build run

build:
	docker build --no-cache -t afpacket .
run: build
	docker run -it afpacket bash
