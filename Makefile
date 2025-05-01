
# Build path
BUILD_DIR = build

startbuild:
	make -f ./build.mk  startbuild  BUILD_DIR=$(BUILD_DIR)  DEBUG=$(DEBUG)

clean:
	-rm -fR $(BUILD_DIR)

dbg_server:
	@openocd --search /usr/share/openocd/scripts \
		--file board/st_nucleo_f4.cfg \
		--file interface/stlink.cfg \
		--command init --command "reset init"

dbg_client:
	@make -f  Inc/build-cfg/mk/toolchain/gcc-arm.mk  dbg_client

help:
	@cat ./helpdoc.txt

