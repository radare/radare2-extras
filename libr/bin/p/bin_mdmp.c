/* radare2 - LGPL - Copyright 2016 - Davis, Alex Kornitzer */

#include <r_types.h>
#include <r_util.h>
#include <r_lib.h>
#include <r_bin.h>

#include "mdmp/pe/pe.h"
#include "mdmp/mdmp.h"


static int check(RBinFile *arch);
static int check_bytes(const ut8 *buf, ut64 length);


static ut64 baddr(RBinFile *arch) {
	return 0LL;
}

static Sdb *get_sdb(RBinObject *o) {
	struct r_bin_mdmp_obj *bin;

	if (!o) return NULL;

	bin = (struct r_bin_mdmp_obj *) o->bin_obj;
	if (bin && bin->kv) return bin->kv;

	return NULL;
}

static int destroy(RBinFile *arch) {
	r_bin_mdmp_free ((struct r_bin_mdmp_obj*)arch->o->bin_obj);

	return true;
}

static RList* entries(RBinFile *arch) {
	struct minidump_module *module;
	struct r_bin_mdmp_obj *obj;
	struct r_bin_pe_addr_t *entry = NULL;
	struct PE_(r_bin_pe_obj_t) *pe_bin;
	ut64 offset, paddr;
	RBinAddr *ptr = NULL;
	RListIter *it;
	RList* ret;
	RBuffer *pe_buf;

	if (!(ret = r_list_newf (free))) {
		return NULL;
	}

	obj = (struct r_bin_mdmp_obj *)arch->o->bin_obj;

	r_list_foreach (obj->streams.modules, it, module) {
		/* TODO: Don't initialise the whole pe structure, we only need
		** to init the header and the sections, but both of these
		** functions are static! */
		if (!(paddr = r_bin_get_paddr(obj, module->base_of_image))) {
			continue;
		}
		pe_buf = r_buf_new_with_bytes(obj->b->buf + paddr, module->size_of_image);
		pe_bin = PE_(r_bin_pe_new_buf) (pe_buf);
		r_buf_free(pe_buf);

		if (!(entry = PE_(r_bin_pe_get_entrypoint) (pe_bin))) {
			PE_(r_bin_pe_free) (pe_bin);
			continue;
		}

		if ((ptr = R_NEW0 (RBinAddr))) {
			/* Hacky! We need to use the vaddr to calculate the
			** correct offset for the entry. We must cater for
			** correctly resolved calculations and incorrectly
			** resolved! */
			/* FIXME: Does this work for all cases? */
			offset = entry->vaddr;
			if (offset > module->base_of_image) {
				offset -= module->base_of_image;
			}
			ptr->paddr = offset + paddr;
			ptr->vaddr = offset + module->base_of_image;
			ptr->type  = R_BIN_ENTRY_TYPE_PROGRAM;
			r_list_append (ret, ptr);
		}

		// TODO: TLS Callback
		// get TLS callback addresses
		//add_tls_callbacks (arch, ret);

		free (entry);
		PE_(r_bin_pe_free) (pe_bin);
	}

	return ret;
}

static RBinInfo *info(RBinFile *arch) {
	struct r_bin_mdmp_obj *obj;
	RBinInfo *ret;

	obj = (struct r_bin_mdmp_obj *)arch->o->bin_obj;

	if (!(ret = R_NEW0 (RBinInfo))) {
		return NULL;
	}
	ret->big_endian = obj->endian;
	ret->claimed_checksum = strdup (sdb_fmt (0, "0x%08x", obj->hdr->check_sum));
	ret->file = arch->file ? strdup (arch->file) : NULL;
	ret->has_va = true;
	ret->rpath = strdup ("NONE");
	ret->type = strdup ("MDMP (MiniDump crash report data)");

	if (obj->streams.system_info)
	{
		switch (obj->streams.system_info->processor_architecture) {
		case PROCESSOR_ARCHITECTURE_INTEL:
			ret->machine = strdup ("i386");
			ret->arch = strdup ("x86");
			ret->bits = 32;
			break;
		case PROCESSOR_ARCHITECTURE_ARM:
			ret->machine = strdup ("ARM");
			ret->big_endian = false;
			break;
		case PROCESSOR_ARCHITECTURE_IA64:
			ret->machine = strdup ("IA64");
			ret->arch = strdup ("IA64");
			ret->bits = 64;
			break;
		case PROCESSOR_ARCHITECTURE_AMD64:
			ret->machine = strdup ("AMD64");
			ret->arch = strdup ("x86");
			ret->bits = 64;
			break;
		default:
			strncpy (ret->machine, "Unknown", R_BIN_SIZEOF_STRINGS);
		}

		switch (obj->streams.system_info->product_type) {
		case VER_NT_WORKSTATION:
			ret->os = r_str_newf ("Windows NT Workstation %d.%d.%d",
			obj->streams.system_info->major_version,
			obj->streams.system_info->minor_version,
			obj->streams.system_info->build_number);
			break;
		case VER_NT_DOMAIN_CONTROLLER:
			ret->os = r_str_newf ("Windows NT Server Domain Controller %d.%d.%d",
			obj->streams.system_info->major_version,
			obj->streams.system_info->minor_version,
			obj->streams.system_info->build_number);
			break;
		case VER_NT_SERVER:
			ret->os = r_str_newf ("Windows NT Server %d.%d.%d",
			obj->streams.system_info->major_version,
			obj->streams.system_info->minor_version,
			obj->streams.system_info->build_number);
			break;
		default:
			ret->os = strdup ("Unknown");
		}
	}

	return ret;
}

static void *load_bytes(RBinFile *arch, const ut8 *buf, ut64 sz, ut64 loadaddr, Sdb *sdb) {
	RBuffer *tbuf;
	struct r_bin_mdmp_obj *res;

	if (!buf || !sz || sz == UT64_MAX) {
		return NULL;
	}

	tbuf = r_buf_new ();
	r_buf_set_bytes (tbuf, buf, sz);
	res = r_bin_mdmp_new_buf (tbuf);
	if (res) {
		sdb_ns_set (sdb, "info", res->kv);
	}
	r_buf_free (tbuf);
	return res;
}

static int load(RBinFile *arch) {
	const ut8 *bytes = arch ? r_buf_buffer (arch->buf) : NULL;
	ut64 sz = arch ? r_buf_size (arch->buf) : 0;

	if (!arch || !arch->o) {
		return false;
	}

	arch->o->bin_obj = load_bytes (arch, bytes, sz, arch->o->loadaddr, arch->sdb);

	return arch->o->bin_obj ? true : false;
}

static RList *sections(RBinFile *arch) {
	struct minidump_memory_descriptor *memory;
	struct minidump_memory_descriptor64 *memory64;
	struct minidump_module *module;
	struct minidump_string *str;
	struct r_bin_mdmp_obj *obj;
	RList *ret;
	RListIter *it;
	RBinSection *ptr;
	ut64 index;

	obj = (struct r_bin_mdmp_obj *)arch->o->bin_obj;

	if (!(ret = r_list_newf (free))) {
		return NULL;
	}

	/* In order to resolve virtual and physical addresses correctly, the
	** memories list must also be resolved. FIXME?: As a further note it
	** seems that r2 will not resolve the addresses unless memory
	** permissions contain R_BIN_SCN_MAP and add==true!!! */

	r_list_foreach (obj->streams.memories, it, memory) {
		if (!(ptr = R_NEW0 (RBinSection))) {
			return ret;
		}

		strncpy(ptr->name, "Memory_Section", 14);
		ptr->paddr = (memory->memory).rva;
		ptr->size = (memory->memory).data_size;
		ptr->vaddr = memory->start_of_memory_range;
		ptr->vsize = (memory->memory).data_size;
		ptr->add = true;

		ptr->srwx = R_BIN_SCN_MAP;
		ptr->srwx |= r_bin_mdmp_get_srwx (obj, ptr->vaddr);

		r_list_append (ret, ptr);
	}

	index = obj->streams.memories64.base_rva;
	r_list_foreach (obj->streams.memories64.memories, it, memory64) {
		if (!(ptr = R_NEW0 (RBinSection))) {
			return ret;
		}

		strncpy(ptr->name, "Memory_Section", 14);
		ptr->paddr = index;
		ptr->size = memory64->data_size;
		ptr->vaddr = memory64->start_of_memory_range;
		ptr->vsize = memory64->data_size;
		ptr->add = true;

		ptr->srwx = R_BIN_SCN_MAP;
		ptr->srwx |= r_bin_mdmp_get_srwx (obj, ptr->vaddr);

		r_list_append (ret, ptr);

		index += memory64->data_size;
	}

	r_list_foreach (obj->streams.modules, it, module) {
		if (!(ptr = R_NEW0 (RBinSection))) {
			return ret;
		}

		str = (struct minidump_string *)(obj->b->buf + module->module_name_rva);
		r_str_utf16_to_utf8 ((ut8 *)ptr->name, R_BIN_SIZEOF_STRINGS, (const ut8 *)&(str->buffer), str->length, obj->endian);
		ptr->vaddr = module->base_of_image;
		ptr->vsize = module->size_of_image;
		ptr->paddr = r_bin_get_paddr (obj, ptr->vaddr);
		ptr->size = module->size_of_image;
		ptr->add = true;

		/* FIXME?: Will only set the permissions for the first section,
		** i.e. header. Should we group all the permissions together
		** and report as lets say rwx as we will contain header, .text,
		** .data, etc... */
		ptr->srwx = R_BIN_SCN_MAP;
		ptr->srwx |= r_bin_mdmp_get_srwx (obj, ptr->vaddr);

		r_list_append (ret, ptr);
	}

	return ret;
}

static RList *mem (RBinFile *arch) {
	struct minidump_location_descriptor *location;
	struct minidump_memory_descriptor *module;
	struct minidump_memory_descriptor64 *module64;
	struct r_bin_mdmp_obj *obj;
	RList *ret;
	RListIter *it;
	RBinMem *ptr;
	ut64 index;

	if (!(ret = r_list_new ()))
		return NULL;

	obj = (struct r_bin_mdmp_obj *)arch->o->bin_obj;

	r_list_foreach (obj->streams.memories, it, module) {
		if (!(ptr = R_NEW0 (RBinMem))) {
			return ret;
		}

		/* FIXME: Hacky approach to match memory from virtual address to location in buffer */
		location = &(module->memory);
		ptr->name = strdup (sdb_fmt (0, "paddr=0x%08x Memory_Section", location->rva));
		ptr->addr = module->start_of_memory_range;
		ptr->size = (location->data_size);
		ptr->perms = R_BIN_SCN_MAP;
		ptr->perms |= r_bin_mdmp_get_srwx (obj, ptr->addr);

		r_list_append (ret, ptr);
	}

	index = obj->streams.memories64.base_rva;
	r_list_foreach (obj->streams.memories64.memories, it, module64) {
		if (!(ptr = R_NEW0 (RBinMem))) {
			return ret;
		}

		/* FIXME: Hacky approach to match memory from virtual address to location in buffer */
		ptr->name = strdup (sdb_fmt (0, "paddr=0x%08x Memory_Section", index));
		ptr->addr = module64->start_of_memory_range;
		ptr->size = module64->data_size;
		ptr->perms = R_BIN_SCN_MAP;
		ptr->perms |= r_bin_mdmp_get_srwx (obj, ptr->addr);

		index += module64->data_size;

		r_list_append (ret, ptr);
	}

	return ret;
}

static int check(RBinFile *arch) {
	const ut8 *bytes;
	ut64 sz;

	bytes = arch ? r_buf_buffer (arch->buf) : NULL;
	sz = arch ? r_buf_size (arch->buf) : 0;

	return check_bytes (bytes, sz);
}

static int check_bytes(const ut8 *buf, ut64 length) {
	return buf && (length > sizeof (struct minidump_header))
		&& (!memcmp (buf, MDMP_MAGIC, 6));
}

RBinPlugin r_bin_plugin_mdmp = {
	.name = "mdmp",
	.desc = "Minidump format r_bin plugin",
	.license = "LGPL3",
	.baddr = &baddr,
	.check = &check,
	.check_bytes = &check_bytes,
	.destroy = &destroy,
	.entries = entries,
	.get_sdb = &get_sdb,
	.info = &info,
	.load = &load,
	.load_bytes = &load_bytes,
	.mem = &mem,
	.sections = &sections,
};

#ifndef CORELIB
struct r_lib_struct_t radare_plugin = {
	.type = R_LIB_TYPE_BIN,
	.data = &r_bin_plugin_mdmp,
	.version = R2_VERSION
};
#endif
