/* radare - LGPL - Copyright 2009-2014 - pancake, nibble */
/* baleful plugin by SkUaTeR */

#include <stdio.h>
#include <string.h>
#include <r_types.h>
#include <r_lib.h>
#include <r_asm.h>
int asm_baleful_getregs(const ut8 *buf,ut8 * b,char * oper,int type) {
	const ut8 * c;
	const ut8  *r0;
	const ut8  *r1;
	const ut8  *r2;
	const ut8  *r3;
	const ut32 *imm;
	const ut32 *imm1;
	
	int size=0;
	c   = buf+1;
	switch(type) {
	case 0: // 8 8 11 5
		r0  = buf + 2;
		switch(*c) {
		case 1:
			r1  = buf + 3;
			imm = buf + 4;
			snprintf(b, 64,  "r_%02x = r_%02x %s 0x%04x",*r0,*r1,oper,*imm);
			//snprintf(b, 64,  "%s",oper);
			size=8;
			break;
		case 2:
			imm  = buf + 3;
			r1   = buf + 4;
			snprintf(b, 64,  "r_%02x = 0x%04x %s r_%02x",*r0,*imm,oper,*r1);		
			//snprintf(b, 64,  "%s",oper);
			size=8;
			break;
		case 4:
			imm  = buf + 3;
			imm1 = buf + 7;
			snprintf(b, 64,  "r_%02x = 0x%04x %s 0x%04x",*r0,*imm,oper,*imm1);	
			//snprintf(b, 64,  "%s",oper);
			size=11;
			break;
		case 0:
			r1  = buf + 3;
			r2  = buf + 4;
			snprintf(b, 64,  "r_%02x = r_%02x %s r_%02x",*r0,*r1,oper,*r2);	
			//snprintf(b, 64,  "%s",oper);
			size=5;
			break;
		default:
			r1  = buf + 3;
			r2  = buf + 4;
			snprintf(b, 64,  "r_%02x = r_%02x %s r_%02x",*r0,*r1,oper,*r2);		
			//snprintf(b, 64,  "%s",oper);
			size=5;
			break;
		}
		break;
	case 1: // 9 9 12 6
		r0  = buf + 2;
		r3  = buf +3; // guarda aki el resto
		switch(*c) {
		case 1:
			r1  = buf + 4;
			imm = buf + 5;
			snprintf(b, 64,  "r_%02x = r_%02x %s 0x%04x",*r0,*r1,oper,*imm);
			//snprintf(b, 64,  "%s",oper);
			size=9;
			break;
		case 2:
			r1   = buf + 5;
			snprintf(b, 64,  "r_%02x = 0x%04x %s r_%02x",*r0,*imm,oper,*r1);		
			//snprintf(b, 64,  "%s",oper);
			size=9;
			break;
		case 4:
			imm  = buf + 4;
			imm1 = buf + 8;
			snprintf(b, 64,  "r_%02x = 0x%04x %s 0x%04x",*r0,*imm,oper,*imm1);	
			//snprintf(b, 64,  "%s",oper);
			size=12;
			break;
		case 0:
			r1  = buf + 4;
			r2  = buf + 5;
			snprintf(b, 64,  "r_%02x = r_%02x %s r_%02x",*r0,*r1,oper,*r2);	
			//snprintf(b, 64,  "%s",oper);
			size=6;
			break;
		default:
			r1  = buf + 4;
			r2  = buf + 5;
			snprintf(b, 64,  "r_%02x = r_%02x %s r_%02x",*r0,*r1,oper,*r2);		
			//snprintf(b, 64,  "%s",oper);
			size=6;
			break;
		}		
		break;
	case 2: // 7 7 10 4
		switch(*c) {
		case 1:
			r1  = buf + 2;
			imm = buf + 3;
			snprintf(b, 64,  "r_%02x %s 0x%04x",*r1,oper,*imm);
			//snprintf(b, 64,  "%s",oper);
			size=7;
			break;
		case 2:
			imm  = buf + 2;
			r1   = buf + 6;
			snprintf(b, 64,  "0x%04x %s r_%02x",*imm,oper,*r1);		
			//snprintf(b, 64,  "%s",oper);
			size=7;
			break;
		case 4:
			imm  = buf + 2;
			imm1 = buf + 6;
			snprintf(b, 64,  "0x%04x %s 0x%04x",*imm,oper,*imm1);	
			//snprintf(b, 64,  "%s",oper);
			size=10;
			break;
		case 0:
			r1  = buf + 2;
			r2  = buf + 3;
			snprintf(b, 64,  "r_%02x %s r_%02x",*r1,oper,*r2);	
			//snprintf(b, 64,  "%s",oper);
			size=4;
			break;
		default:
			r1  = buf + 2;
			r2  = buf + 3;
			snprintf(b, 64,  "r_%02x %s r_%02x",*r1,oper,*r2);		
			//snprintf(b, 64,  "%s",oper);
			size=4;
			break;
		}	
		break;
	case 3:// 7 4
		switch(*c) {
		case 1:
			r1  = buf + 2;
			imm = buf + 3;
			snprintf(b, 64,  "%s r_%02x,0x%04x",oper,*r1,*imm);
			//snprintf(b, 64,  "%s",oper);
			size=7;
			break;
		case 0:
			r1  = buf + 2;
			r2 = buf + 3;
			snprintf(b, 64,  "%s r_%02x,r_%02x",oper,*r1,*r2);
			//snprintf(b, 64,  "%s",oper);
			size=4;
			break;
		default:
			r1  = buf + 2;
			r2 = buf + 3;
			snprintf(b, 64,  "%s r_%02x,r_%02x",oper,*r1,*r2);
			//snprintf(b, 64,  "%s",oper);
			size=4;
			break;
		}
		break;
	case 4: // 6 3
		switch(*c) {
		case 1:
			imm = buf + 2;
			snprintf(b, 64, "%s 0x%04x",oper,*imm);			  							
			//snprintf(b, 64,  "%s",oper);
			size=6;
			break;
		case 0:
			r0  = buf + 2;
			snprintf(b, 64, "%s r_%02x",oper,*r0);			  							
			//snprintf(b, 64,  "%s",oper);
			size=3;
			break;
		default:
			r0  = buf + 2;
			snprintf(b, 64, "%s r_%02x",oper,*r0);			  							
			//snprintf(b, 64,  "%s",oper);
			size=3;
			break;
		}
		break;
	case 5: //5
		imm  = buf + 1;
		snprintf(b, 64, "%s 0x%04x",oper,*imm);			  							
	    //snprintf(b, 64,  "%s",oper);
		size=5;		
		break;
	case 6://2
		r0  = buf + 1;
		snprintf(b, 64, "%s r_%02x",oper,*r0);			  							
	    //snprintf(b, 64,  "%s",oper);
		size=2;		
		break;
	break;
	}
	return size;
}

static int disassemble(RAsm *a, RAsmOp *op, const ut8 *buf, int len) {
	const ut8 *p;
	const ut8  *r;
	const ut8  *r1;
	const ut32 *imm;
	const ut32 *imm1;

	switch (*buf) {
	  case 2://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"+",0);
		break;
      case 3://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"-",0);
		break;     
      case 4://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"*",0);
		break;  
      case 6://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"^",0);
		break; 
      case 9://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"&",0);
		break; 
      case 10://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"|",0);
		break; 
      case 12://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"<<",0);
		break; 
      case 13://8 8 11 5
		op->size = asm_baleful_getregs(buf,op->buf_asm,">>",0);
		break;
      case 5: // //9 9 12 6
		op->size = asm_baleful_getregs(buf,op->buf_asm,"/",1);
		break;
      case 22: // 7 7 10 4
		op->size = asm_baleful_getregs(buf,op->buf_asm,"and",2);
		break;
      case 23: // 7 7 10 4
		op->size = asm_baleful_getregs(buf,op->buf_asm,"cmp",2);
		break;
	  case 24: //7 4
        op->size = asm_baleful_getregs(buf,op->buf_asm,"mov",3);
		break;
	  case 30: // 6 3
        op->size = asm_baleful_getregs(buf,op->buf_asm,"push",4);
		break;
      case 15: //5
  	    op->size = asm_baleful_getregs(buf,op->buf_asm,"call",5);
		break;
      case 14: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jmp",5);
		break;
      case 16: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jz",5);
		break;
      case 17: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"js",5);
		break;
      case 18: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jbe",5);
		break;
      case 19: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jg",5);
		break;
      case 20: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jns",5);
		break;
      case 21: //5
		op->size = asm_baleful_getregs(buf,op->buf_asm,"jnz",5);
		break;
      case 27: //3
		op->size = 3;
  	    r  = buf + 1;
		r1 = buf + 2;
		snprintf (op->buf_asm, R_ASM_BUFSIZE, "mov r_%02x,[r_%02x]",*r,*r1);
		break;
      case 28: //3                                                  
		r  = buf + 1;
		r1 = buf + 2;
		op->size = 3;
		snprintf (op->buf_asm, R_ASM_BUFSIZE, "mov [r_%02x],r_%02x",*r,*r1);
		break;
	  case 11: //3
		op->size = 3;
        snprintf (op->buf_asm, R_ASM_BUFSIZE, "regX= regY==0");
		break;	
      case 7: //3
		op->size = 3;
		snprintf (op->buf_asm, R_ASM_BUFSIZE, "regX= NEG regY");
		break;
	  case 8: //3
		op->size = 3;
		snprintf (op->buf_asm, R_ASM_BUFSIZE, "regX= NOT regY");
		break;
	  case 25: //2
		op->size = asm_baleful_getregs(buf,op->buf_asm,"++",6);
		break;
      case 26: //2
		op->size = asm_baleful_getregs(buf,op->buf_asm,"--",6);
		break;
      case 31: //2
		op->size = asm_baleful_getregs(buf,op->buf_asm,"pop",6);
		break;
      case 32: // 2
        op->size = asm_baleful_getregs(buf,op->buf_asm,"apicall",6);
		break;
	  case 1:
		op->size = 1;
        strcpy (op->buf_asm, "ret");			  							  
		break;
      case 0:
		op->size = 1;
		strcpy (op->buf_asm, "nop");			  							  
		break;
	  case 29:
		op->size = 1;
		strcpy (op->buf_asm, "end virtual");
		break;
	  default:
		op->size = 1;
		strcpy (op->buf_asm, "nop");
		break;
	}
	return op->size;
}

RAsmPlugin r_asm_plugin_baleful = {
	.name = "baleful",
	.arch = "baleful",
	.license = "LGPL3",
	.bits = 32,
	.desc = "Baleful",
	.init = NULL,
	.fini = NULL,
	.disassemble = &disassemble,
	//.assemble =null// &assemble 
};

#ifndef CORELIB
struct r_lib_struct_t radare_plugin = {
	.type = R_LIB_TYPE_ASM,
	.data = &r_asm_plugin_baleful
};
#endif
