/* radare2-keystone - GPL - Copyright 2016 - pancake */

#include <r_asm.h>
#include <r_lib.h>
#include <keystone/keystone.h>
#include <keystone/hexagon.h>
#include "keystone.c"

static int assemble(RAsm *a, RAsmOp *ao, const char *str) {
	ks_mode mode = (ks_mode)0;
	if (a->big_endian) {
		mode = (ks_mode)((int)mode | KS_MODE_BIG_ENDIAN);
	}
	return keystone_assemble (a, ao, str, KS_ARCH_HEXAGON, mode);
}

RAsmPlugin r_asm_plugin_hexagon_ks = {
	.name = "hexagon.ks",
	.desc = "Hexagon keystone assembler",
	.license = "GPL",
	.arch = "hexagon",
	.bits = 32,
	.assemble = &assemble,
};

#ifndef CORELIB
struct r_lib_struct_t radare_plugin = {
	.type = R_LIB_TYPE_ASM,
	.data = &r_asm_plugin_hexagon_ks,
	.version = R2_VERSION
};
#endif
