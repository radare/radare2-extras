/* radare - LGPL3 - Copyright 2016-2020 - c0riolis, x0urc3 */

#ifndef OPCODE_H
#define OPCODE_H

#include <r_types.h>
#include <r_list.h>
#include <r_util.h>

typedef enum {
	HASCOMPARE   = 0x1,
	HASCONDITION = 0x2,    // conditional operator; has jump offset
	HASCONST     = 0x4,
	HASFREE      = 0x8,
	HASJABS      = 0x10,
	HASJREL      = 0x20,
	HASLOCAL     = 0x40,
	hASNAME      = 0x80,
	HASNARGS     = 0x100,  // For function-like calls
	HASSTORE     = 0x200,  // Some sort of store operation
	HASVARGS     = 0x400,  // Similar but for operators BUILD_xxx
	NOFOLLOW     = 0x800,  // Instruction doesn't fall to the next opcode
	HASNAME      = 0x1000,
} pyc_opcode_type;

typedef enum {
	NAME_OP   = 0x1,
	LOCAL_OP  = 0x2,    
	FREE_OP   = 0x4,
	DEF_OP    = 0x8,
} pyc_store_op_func;

typedef struct {
	char *op_name;
	ut16 type;
	ut8 op_code;
	st8 op_push;
	st8 op_pop;
} pyc_opcode_object;

typedef struct {
	ut8 extended_arg; 
	ut8 have_argument;
	ut8 bits;
    void *(*version_sig) ();
    RList *opcode_arg_fmt;
    pyc_opcode_object *opcodes;
} pyc_opcodes;

typedef struct {
	char *op_name;
	const char *(*formatter) (ut32 oparg);
} pyc_arg_fmt;

typedef struct {
    char *version;
    pyc_opcodes *(*opcode_func) ();
} version_opcode;

pyc_opcodes *opcode_2x();
pyc_opcodes *opcode_3x();
pyc_opcodes *opcode_10();
pyc_opcodes *opcode_11();
pyc_opcodes *opcode_12();
pyc_opcodes *opcode_13();
pyc_opcodes *opcode_14();
pyc_opcodes *opcode_15();
pyc_opcodes *opcode_16();
pyc_opcodes *opcode_20();
pyc_opcodes *opcode_21();
pyc_opcodes *opcode_22();
pyc_opcodes *opcode_23();
pyc_opcodes *opcode_24();
pyc_opcodes *opcode_25();
pyc_opcodes *opcode_26();
pyc_opcodes *opcode_27();
pyc_opcodes *opcode_30();
pyc_opcodes *opcode_31();
pyc_opcodes *opcode_32();
pyc_opcodes *opcode_33();
pyc_opcodes *opcode_34();
pyc_opcodes *opcode_35();
pyc_opcodes *opcode_36();
pyc_opcodes *opcode_37();
pyc_opcodes *opcode_38();
pyc_opcodes *opcode_39();

pyc_opcodes *get_opcode_by_version(char *version);

pyc_opcodes *new_pyc_opcodes();
void free_opcode(pyc_opcodes *opcodes);
bool pyc_opcodes_equal (pyc_opcodes *op, const char *version);

void add_arg_fmt(pyc_opcodes *ret, const char *op_name, const char *(*formatter) (ut32 oparg));

const char *format_MAKE_FUNCTION_arg_3x (ut32 oparg);
const char *format_extended_arg (ut32 oparg);
const char *format_CALL_FUNCTION_pos_name_encoded (ut32 oparg);
const char *format_CALL_FUNCTION_KW_36 (ut32 oparg);
const char *format_CALL_FUNCTION_EX_36 (ut32 oparg);
const char *format_MAKE_FUNCTION_arg_36 (ut32 oparg);
const char *format_value_flags_36 (ut32 oparg);
const char *format_extended_arg_36 (ut32 oparg);

struct op_parameter { pyc_opcode_object* op_obj; const char *op_name; ut8 op_code; st8 pop; st8 push; pyc_store_op_func func; bool conditional; bool fallthrough; };

#define def_op(...) def_op ((struct op_parameter) {.pop = -2, .push = -2, .fallthrough = true, __VA_ARGS__})
void (def_op) (struct op_parameter par);

#define name_op(...) name_op ((struct op_parameter) {.pop = -2, .push = -2, __VA_ARGS__})
void (name_op) (struct op_parameter par);

#define local_op(...) local_op ((struct op_parameter) {.pop = 0, .push = 1, __VA_ARGS__})
void (local_op) (struct op_parameter par);

#define free_op(...) free_op ((struct op_parameter) {.pop = 0, .push = 1, __VA_ARGS__})
void (free_op) (struct op_parameter par);

#define store_op(...) store_op ((struct op_parameter) {.pop = 0, .push = 1, .func = DEF_OP, __VA_ARGS__})
void (store_op) (struct op_parameter par);

#define varargs_op(...) varargs_op ((struct op_parameter) {.pop = -1, .push = 1, __VA_ARGS__})
void (varargs_op) (struct op_parameter par);

#define const_op(...) const_op ((struct op_parameter) {.pop = 0, .push = 1, __VA_ARGS__})
void (const_op) (struct op_parameter par);

#define compare_op(...) compare_op ((struct op_parameter) {.pop = 2, .push = 1, __VA_ARGS__})
void (compare_op) (struct op_parameter par);

#define jabs_op(...) jabs_op ((struct op_parameter) {.pop = 0, .push = 0, .conditional = false, .fallthrough = true, __VA_ARGS__})
void (jabs_op) (struct op_parameter par);

#define jrel_op(...) jrel_op ((struct op_parameter) {.pop = 0, .push = 0, .conditional = false, .fallthrough = true, __VA_ARGS__})
void (jrel_op) (struct op_parameter par);

#define nargs_op(...) nargs_op ((struct op_parameter) {.pop = -2, .push = -2, __VA_ARGS__})
void (nargs_op) (struct op_parameter par);

#define rm_op(...) rm_op ((struct op_parameter) {__VA_ARGS__})
void (rm_op) (struct op_parameter par);

#endif
