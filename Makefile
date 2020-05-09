.DEFAULT_GOAL := run
.PHONY: build run

build:
	docker build -t afpacket .
run: build
	docker run --cap-add NET_RAW --cap-add NET_ADMIN --cap-add IPC_LOCK -it --network host -v ${PWD}/src/:/src/ afpacket bash
