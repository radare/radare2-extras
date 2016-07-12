/* radare - LGPLv3 - Copyright 2014-2015 - pancake, jvoisin, jfrankowski */

#include <dirent.h>
#include <r_core.h>
#include <r_lib.h>
#include <yara.h>

#undef R_API
#define R_API static
#undef R_IPI
#define R_IPI static

static int loaded_version = 3;

// R_TRUE if the plugin has been initialized.
static int initialized = R_FALSE;

static int callback(int message, void* rule, void* data);
static int r_cmd_yara_add (const RCore* core, const char* input);
static int r_cmd_yara_add_file (const char* rules_path);
static int r_cmd_yara_call(void *user, const char *input);
static int r_cmd_yara_clear();
static int r_cmd_yara_help(const RCore* core);
static int r_cmd_yara_init(const RCore* core);
static int r_cmd_yara_process(const RCore* core, const char* input);
static int r_cmd_yara_scan(const RCore* core);
static int r_cmd_yara_load_default_rules (const RCore* core);

static const char* yara_rule_template = "rule RULE_NAME {\n\tstrings:\n\n\tcondition:\n}";

/* Because of how the rules are compiled, we are not allowed to add more
 * rules to a compiler once it has compiled. That's why we keep a list
 * of those compiled rules.
 */
static RList* rules_list;

static int callback (int message, void *msg_data, void *user_data) {
	YR_RULE* rule = msg_data;
	if (message == CALLBACK_MSG_RULE_MATCHING)
		r_cons_printf ("%s\n", rule->identifier);
	return CALLBACK_CONTINUE;
}

static void compiler_callback(int error_level, const char* file_name,
		int line_number, const char* message, void* user_data) {
	eprintf ("file: %s line_number: %d.\n%s", file_name, line_number, message);
	return;
}

static int r_cmd_yara_scan(const RCore* core) {
	RListIter* rules_it;
	YR_RULES* rules;
	void* to_scan;
	int result;
	const unsigned int to_scan_size = r_io_size (core->io);

	if (to_scan_size < 1) {
		eprintf ("Invalid file size\n");
		return R_FALSE;
	}

	to_scan = malloc (to_scan_size);
	if (!to_scan) {
		eprintf ("Something went wrong during memory allocation\n");
		return R_FALSE;
	}

	result = r_io_read_at (core->io, 0L, to_scan, to_scan_size);
	if (!result) {
		eprintf ("Something went wrong during r_io_read_at\n");
		free (to_scan);
		return R_FALSE;
	}

	r_list_foreach (rules_list, rules_it, rules) {
		yr_rules_scan_mem (rules, to_scan, to_scan_size, 0, callback, NULL, 0);
	}

	free (to_scan);

	return R_TRUE;
}

static int r_cmd_yara_show(const char * name) {
	/* List loaded rules containing name */
	RListIter* rules_it;
	YR_RULES* rules;
	YR_RULE* rule;

	r_list_foreach (rules_list, rules_it, rules) {
		yr_rules_foreach (rules, rule) {
			if (r_str_casestr (rule->identifier, name)) {
				r_cons_printf ("%s\n", rule->identifier);
			}
		}
	}

	return R_TRUE;
}

static int r_cmd_yara_tags() {
	/* List tags from all the different loaded rules */
	RListIter* rules_it;
	RListIter *tags_it;
	YR_RULES* rules;
	YR_RULE* rule;
	char* tag_name;
	RList *tag_list = r_list_new();
	tag_list->free = free;

	r_list_foreach (rules_list, rules_it, rules) {
		yr_rules_foreach(rules, rule) {
			yr_rule_tags_foreach(rule, tag_name) {
				if (! r_list_find (tag_list, tag_name, (RListComparator)strcmp)) {
					r_list_add_sorted (tag_list,
							strdup (tag_name), (RListComparator)strcmp);
				}
			}
		}
	}

	r_cons_printf ("[YARA tags]\n");
	r_list_foreach (tag_list, tags_it, tag_name) {
		r_cons_printf ("%s\n", tag_name);
	}

	r_list_free (tag_list);

	return R_TRUE;
}

static int r_cmd_yara_tag (const char * search_tag) {
	/* List rules with tag search_tag */
	RListIter* rules_it;
	YR_RULES* rules;
	YR_RULE* rule;
	char* tag_name;

	r_list_foreach (rules_list, rules_it, rules) {
		yr_rules_foreach (rules, rule) {
			yr_rule_tags_foreach(rule, tag_name) {
				if (r_str_casestr (tag_name, search_tag)) {
					r_cons_printf("%s\n", rule->identifier);
					break;
				}
			}
		}
	}

	return R_TRUE;
}

static int r_cmd_yara_list () {
	/* List all loaded rules */
	RListIter* rules_it;
	YR_RULES* rules;
	YR_RULE* rule;

	r_list_foreach (rules_list, rules_it, rules) {
		yr_rules_foreach (rules, rule) {
			r_cons_printf("%s\n", rule->identifier);
		}
	}

	return R_TRUE;
}

static int r_cmd_yara_clear () {
	/* Clears all loaded rules */
	r_list_free (rules_list);
	rules_list = r_list_newf((RListFree) yr_rules_destroy);
	eprintf ("Rules cleared.\n");

	return R_TRUE;
}

static int r_cmd_yara_add(const RCore* core, const char* input) {
	/* Add a rule with user input */
	YR_COMPILER* compiler = NULL;
	char* modified_template = NULL;
	char* old_template = NULL;
	int result, i, continue_edit;

	for( i = 0; input[i] != '\0'; i++) {
		if (input[i] != ' ') {
			return r_cmd_yara_add_file (input + i);
		}
	}

	if (yr_compiler_create (&compiler) != ERROR_SUCCESS) {
		char buf[64];
		eprintf ("Error: %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

		return R_FALSE;
	}

	old_template = strdup(yara_rule_template);
	do {
		modified_template = r_core_editor (core, NULL, old_template);
		free(old_template);
		old_template = NULL;
		if (!modified_template) {
			eprintf("Something happened with the temp file");

			goto err_exit;
		}

		result = yr_compiler_add_string (compiler, modified_template, NULL);
		if( result > 0 ) {
			char buf[64];
			eprintf ("Error: %s\n",
			yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

			continue_edit = r_cons_yesno('y', "Do you want to continue editing the rule? [y]/n\n");
			if (!continue_edit) {
				goto err_exit;
			}

			old_template = modified_template;
			modified_template = NULL;
		}
	} while (result > 0);

	free(modified_template);
	yr_compiler_destroy (compiler);
	r_cons_printf ("Rule successfully added.\n");

	return R_TRUE;

err_exit:
	if (compiler) yr_compiler_destroy (compiler);
	if (modified_template) free (modified_template);
	if (old_template) free (old_template);
	return R_FALSE;
}

static int r_cmd_yara_add_file(const char* rules_path) {
	YR_COMPILER* compiler = NULL;
	YR_RULES* rules;
	FILE* rules_file = NULL;
	int result;

	if (!rules_path) {
		eprintf ("Please tell me what am I supposed to load\n");
		return R_FALSE;
	}

	rules_file = r_sandbox_fopen (rules_path, "r");
	if (!rules_file) {
		eprintf ("Unable to open %s\n", rules_path);
		return R_FALSE;
	}

	if (yr_compiler_create (&compiler) != ERROR_SUCCESS) {
		char buf[64];
		eprintf ("Error: %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

		goto err_exit;
	}

#if 0
	if (loaded_version == 2) {
		if (yr_compiler_push_file_name (compiler, rules_path) != ERROR_SUCCESS) {
			char buf[64];
			eprintf ("Error: %s : %s\n",
			yr_compiler_get_error_message (compiler, buf, sizeof (buf)),
				rules_path);

			goto err_exit;
		}
	}
#endif

	result = yr_compiler_add_file (compiler, rules_file, NULL, rules_path);
	fclose (rules_file);
	rules_file = NULL;
	if (result > 0) {
		char buf[64];
		eprintf ("Error: %s : %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)),
			rules_path);

		goto err_exit;
	}

	if (yr_compiler_get_rules (compiler, &rules) != ERROR_SUCCESS) {
		char buf[64];
		eprintf ("Error: %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

		goto err_exit;
	}

	r_list_append(rules_list, rules);

	yr_compiler_destroy (compiler);
	return R_TRUE;

err_exit:
	if (compiler) yr_compiler_destroy (compiler);
	if (rules_file) fclose (rules_file);
	return R_FALSE;
}

static int r_cmd_yara_help(const RCore* core) {
	const char * help_message[] = {
		"Usage: yara3", "", " Yara plugin",
		"add", " [file]", "Add yara rules from file, or open $EDITOR with yara rule template",
		"clear", "", "Clear all rules",
		"help", "", "Show this help",
		"list", "", "List all rules",
		"scan", "", "Scan the current file",
		"show", " name", "Show rules containing name",
		"tag", " name", "List rules with tag 'name'",
		"tags", "", "List tags from the loaded rules",
		NULL
	};

	r_core_cmd_help (core, help_message);

    return R_TRUE;
}

static int r_cmd_yara_process(const RCore* core, const char* input) {
    if (!strncmp (input, "add", 3))
        return r_cmd_yara_add (core, input + 3);
    else if (!strncmp (input, "clear", 4))
        return r_cmd_yara_clear ();
    else if (!strncmp (input, "list", 4))
        return r_cmd_yara_list ();
    else if (!strncmp (input, "scan", 4))
        return r_cmd_yara_scan (core);
    else if (!strncmp (input, "show", 4))
        return r_cmd_yara_show (input + 5);
    else if (!strncmp (input, "tags", 4))
        return r_cmd_yara_tags ();
    else if (!strncmp (input, "tag ", 4))
        return r_cmd_yara_tag (input + 4);
    else
        return r_cmd_yara_help (core);
}

static int r_cmd_yara_call(void *user, const char *input) {
	const char *args;
	const RCore* core = (RCore*) user;
	if (strncmp (input, "yara3", 5))
		return R_FALSE;
	else if (strncmp (input, "yara3 ", 6))
		return r_cmd_yara_help (core);
	args = input+5;
	if (! initialized)
		if (!r_cmd_yara_init (core))
			return R_FALSE;
	if (*args)
		args++;
	r_cmd_yara_process (core, args);

	return R_TRUE;
}

static int r_cmd_yara_load_default_rules (const RCore* core) {
#define YARA_PATH R2_PREFIX "/lib/radare2-extras/" R2_VERSION "/rules-yara3/"
	RListIter* iter = NULL;
	YR_COMPILER* compiler = NULL;
	YR_RULES* yr_rules;
	char* filename, *complete_path;
	char* rules = NULL;
	RList* list = r_sys_dir (YARA_PATH);

	if (yr_compiler_create (&compiler) != ERROR_SUCCESS) {
		char buf[64];
		eprintf ("Error: %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

		goto err_exit;
	}

	yr_compiler_set_callback(compiler, compiler_callback, NULL);

	r_list_foreach (list, iter, filename) {
		if (filename[0] != '.') { // skip '.', '..' and hidden files
			complete_path = r_str_concat (strdup (YARA_PATH), filename);
			rules = (char*)r_file_gzslurp (complete_path, NULL, R_TRUE);

			free (complete_path);
			complete_path = NULL;

			if (yr_compiler_add_string (compiler, rules, NULL) > 0) {
				char buf[64];
				eprintf ("Error: %s\n",
				yr_compiler_get_error_message (compiler, buf, sizeof (buf)));
			}

			free (rules);
			rules = NULL;
		}
	}
	r_list_free (list);

	if (yr_compiler_get_rules (compiler, &yr_rules) != ERROR_SUCCESS) {
		char buf[64];
		eprintf ("Error: %s\n",
		yr_compiler_get_error_message (compiler, buf, sizeof (buf)));

		goto err_exit;
	}

	r_list_append(rules_list, yr_rules);

	yr_compiler_destroy (compiler);
	return R_TRUE;

err_exit:
	if (compiler) yr_compiler_destroy (compiler);
	if (list) r_list_free (list);
	if (rules) free (rules);
	return R_FALSE;
}

static int r_cmd_yara_init(const RCore* core) {
	rules_list = r_list_newf((RListFree) yr_rules_destroy);
	yr_initialize ();
	r_cmd_yara_load_default_rules (core);
	initialized = R_TRUE;
	return R_TRUE;
}

static int r_cmd_yara_deinit(){
	if (initialized) {
		r_list_free (rules_list);
		yr_finalize();
		initialized = R_FALSE;
	}
	return R_TRUE;
}

RCorePlugin r_core_plugin_yara3 = {
	.name = "yara3",
	.desc = "YARA V3 integration",
	.license = "LGPL",
	.call = r_cmd_yara_call,
	.init = r_cmd_yara_init,
	.deinit = r_cmd_yara_deinit
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_CORE,
	.data = &r_core_plugin_yara3,
        .version = R2_VERSION
};
#endif
