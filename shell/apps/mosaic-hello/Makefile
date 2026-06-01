PKGDIR	?= .
L4DIR	?= ../../../.lab/l4
TARGET		= mosaic-graphical-hello
EXTRA_LIBS	= -lmosaic_hello
REQUIRES_LIBS	= l4re l4re-util

LDFLAGS		+= -L$(PKGDIR)/target/x86_64-unknown-none/release

all:: cargo-build

cargo-build:
	cd $(PKGDIR) && cargo build --target x86_64-unknown-none --release

include $(L4DIR)/mk/prog.mk
