PKGDIR	?= .
L4DIR	?= ../../../.lab/l4
TARGET		= mosaic-input
EXTRA_LIBS	= -lmosaic_input_server
REQUIRES_LIBS	= l4re l4re-util

LDFLAGS		+= -L$(PKGDIR)/target/x86_64-unknown-none/release

all:: cargo-build

cargo-build:
	cd $(PKGDIR) && cargo build --target x86_64-unknown-none --release

include $(L4DIR)/mk/prog.mk
