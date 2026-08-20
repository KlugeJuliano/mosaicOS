PKGDIR	?= .
L4DIR	?= ../../../.lab/l4
TARGET		= mosaic-display
# No SRC_C or SRC_CC, we use the rust library
SRC_C		= ../../sdk/l4-rust/shim/l4_shim.c
EXTRA_LIBS	= -lmosaic_display_server
REQUIRES_LIBS	= l4re l4re-util libio-vbus l4sys l4re_c libc_be_l4re libc_be_l4refile libl4re-vfs libpthread libc

# Path to the cargo-built static library
LDFLAGS		+= -L$(PKGDIR)/target/x86_64-unknown-none/release

all:: cargo-build

cargo-build:
	cd $(PKGDIR) && cargo build --target x86_64-unknown-none --release

include $(L4DIR)/mk/prog.mk
