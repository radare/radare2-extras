/* radare - LGPL3 - 2016 - xarkes */

#include "swf.h"
#include "swfdis.h"
#include "swf_op_names.h"

char* get_swf_file_type(char compression, char flashVersion) {
	char* type = malloc(sizeof(SWF_FILE_TYPE) + sizeof(SWF_FILE_TYPE_ZLIB));

	switch (compression) {
		case ISWF_MAGIC_0_0:
			sprintf(type, SWF_FILE_TYPE, flashVersion, SWF_FILE_TYPE_SWF);
			break;
		case ISWF_MAGIC_0_1:
			if (flashVersion < 6)
				sprintf(type, SWF_FILE_TYPE, flashVersion, SWF_FILE_TYPE_ERROR);
			else
				sprintf(type, SWF_FILE_TYPE, flashVersion, SWF_FILE_TYPE_ZLIB);
			break;
		case ISWF_MAGIC_0_2:
			if (flashVersion < 13)
				sprintf(type, SWF_FILE_TYPE, flashVersion, SWF_FILE_TYPE_ERROR);
			else
				sprintf(type, SWF_FILE_TYPE, flashVersion, SWF_FILE_TYPE_LZMA);
			break;
		default:
			sprintf(type, SWF_FILE_TYPE, flashVersion, "");
			break;
	}

	return type;
}

swf_hdr r_bin_swf_get_header(RBinFile *arch) {
	swf_hdr header;
	ut8 nBits;

	/* First, get the rect size */
	r_buf_read_at (arch->buf, 8, (ut8*)&nBits, 1);
	nBits = (nBits & 0xf8) >> 3;
	ut32 rect_size_bits = nBits*4 + 5;
	ut32 rect_size_bytes = rect_size_bits / 8;
	if (rect_size_bits % 8) rect_size_bytes++;

	/* Read the whole header */
	memset(&header, 0, SWF_HDR_MIN_SIZE + rect_size_bytes);
	r_buf_read_at(arch->buf, 0, (ut8*)&header, 8);
	header.rect_size = rect_size_bytes;

	/* TODO: Record rectangle with xmin xmax ymin ymax if needed */

	// Do this better (swf reverts values)
	r_buf_read_at (arch->buf, 8+rect_size_bytes, (ut8*)&header.frame_rate+1, 1);
	r_buf_read_at (arch->buf, 8+rect_size_bytes+1, (ut8*)&header.frame_rate, 1);
	r_buf_read_at (arch->buf, 8+rect_size_bytes+2, (ut8*)&header.frame_count+1, 1);
	r_buf_read_at (arch->buf, 8+rect_size_bytes+3, (ut8*)&header.frame_count, 1);

	return header;
}


/* http://www.homer.com.au/webdoc/flash_file_format_specification.pdf */
int r_bin_swf_get_sections(RList *list, RBinFile *arch) {
	swf_hdr header;
	header = r_bin_swf_get_header (arch);

	if (header.signature[0] == ISWF_MAGIC_0_1) {
		// CWS file compression is not supported !
		// 2 sections in this case : header and zlib data
		// TODO: Decompress zlib data

		/* Header section */
		RBinSection *head_sect;
		if (!(head_sect = R_NEW0 (RBinSection)))
			return false;

		memset(head_sect, 0, sizeof(*head_sect));
		snprintf(head_sect->name, R_BIN_SIZEOF_STRINGS, "Header");
		head_sect->paddr = 0;
		head_sect->vaddr = 0;
		ut8 start = 0x8; // signature + version + size
		head_sect->size = start;
		head_sect->vsize = start; 
		head_sect->srwx = R_BIN_SCN_READABLE;
		r_list_append (list, head_sect);

		/* ZlibData section */
		RBinSection *data;

		if (!(data = R_NEW0 (RBinSection)))
			return false;

		snprintf(data->name, R_BIN_SIZEOF_STRINGS, "ZlibData");
		data->paddr = start;
		data->vaddr = start;
		data->size = r_buf_size(arch->buf) - start;
		data->vsize = r_buf_size(arch->buf) - start;
		data->srwx = R_BIN_SCN_READABLE;

		r_list_append (list, data);
	} else if (header.signature[0] == ISWF_MAGIC_0_2) {
		// ZWS file compression is not supported !
		// 2 sections in this case : header and Lzma data
		// TODO: Decompress lzma data

		/* Header section */
		RBinSection *head_sect;
		if (!(head_sect = R_NEW0 (RBinSection)))
			return false;

		memset(head_sect, 0, sizeof(*head_sect));
		snprintf(head_sect->name, R_BIN_SIZEOF_STRINGS, "Header");
		head_sect->paddr = 0;
		head_sect->vaddr = 0;
		ut8 start = 0x8; // signature + version + size
		head_sect->size = start;
		head_sect->vsize = start; 
		head_sect->srwx = R_BIN_SCN_READABLE;
		r_list_append (list, head_sect);

		/* LzmaData section */
		RBinSection *data;

		if (!(data = R_NEW(RBinSection)))
			return false;

		snprintf(data->name, R_BIN_SIZEOF_STRINGS, "LzmaData");
		data->paddr = start;
		data->vaddr = start;
		data->size = r_buf_size(arch->buf) - start;
		data->vsize = r_buf_size(arch->buf) - start;
		data->srwx = R_BIN_SCN_READABLE;

		r_list_append (list, data);
	} else {
		/* Header section */
		RBinSection *head_sect;
		if (!(head_sect = R_NEW0 (RBinSection)))
			return false;

		strncpy(head_sect->name, "Header", 6);
		head_sect->paddr = 0;
		head_sect->vaddr = 0;
		ut32 start = header.rect_size + SWF_HDR_MIN_SIZE; // rect + min_size
		head_sect->size = start;
		head_sect->vsize = start; 
		head_sect->srwx = R_BIN_SCN_READABLE;
		r_list_append (list, head_sect);

		/* Other sections */
		ut16 tagCodeAndLength = 0;
		ut16 tagCode = 0;
		ut8 tagLengthShort = 0;
		ut32 end = start;
		ut32 tagLengthLong;

		int section = 0;
		r_buf_read_at (arch->buf, start, (ut8*)&tagCodeAndLength, 2);

		while (tagCodeAndLength != 0) {
			start = end;

			tagLengthLong = 0;
			tagCode = tagCodeAndLength >> 6; //10 higher bytes is code
			tagLengthShort = tagCodeAndLength & 0x3f; //6 lowers bytes is length

			if (tagLengthShort >= 0x3F) {
				r_buf_read_at (arch->buf, start+2, (ut8*)&tagLengthLong, 4);
				end = start + 6 + tagLengthLong;
			} else {
				end = start + 2 + tagLengthShort;
			}

			RBinSection *new;

			if (!(new = R_NEW0 (RBinSection)))
				return false;

			swf_tag_t tag = r_asm_swf_gettag (tagCode);
			sprintf(new->name, "%s", tag.name);
			new->paddr = start;
			new->vaddr = start;

			new->size = end-start;
			new->vsize = end-start;

			switch (tagCode) {
			case TAG_DOACTION:
			case TAG_DOINITACTION:
				new->srwx = R_BIN_SCN_READABLE | R_BIN_SCN_EXECUTABLE;
				new->has_strings = true;
				break;
			default:
				new->srwx = R_BIN_SCN_READABLE;
				break;
			}
			r_list_append (list, new);

			/* Read next tag info */
			r_buf_read_at (arch->buf, end, (ut8*)&tagCodeAndLength, 2);
			section++;
		}

	}

	return true;
}

