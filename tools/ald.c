/* Copyright (C) 2020 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/
#include "common.h"
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
	puts("Usage: ald <command> [<args>]");
	puts("");
	puts("commands:");
	puts("  list     Print list of archive files");
	puts("  extract  Extract file(s) from archive");
	puts("  dump     Print hex dump of file");
	puts("  compare  Compare contents of two archives");
	puts("  help     Display help information about commands");
	puts("  version  Display version information and exit");
	puts("");
	puts("Run 'ald help <command>' for more information about a specific command.");
}

static Vector *read_alds(int *pargc, char **pargv[]) {
	int argc = *pargc;
	char **argv = *pargv;

	for (int dd = 0; dd < argc; dd++) {
		if (!strcmp(argv[dd], "--")) {
			Vector *ald = NULL;
			for (int i = 0; i < dd; i++)
				ald = ald_read(ald, argv[i]);
			*pargc -= dd + 1;
			*pargv += dd + 1;
			return ald;
		}
	}

	Vector *ald = NULL;
	for (int i = 0; i < argc; i++) {
		const char *dot = strrchr(argv[i], '.');
		if (!dot || strcasecmp(dot, ".ald"))
			break;
		ald = ald_read(ald, argv[i]);
		*pargc -= 1;
		*pargv += 1;
	}
	return ald;
}

static AldEntry *find_entry(Vector *ald, const char *num_or_name) {
	char *endptr;
	unsigned long n = strtoul(num_or_name, &endptr, 0);
	if (*endptr == '\0') {
		if (n < ald->len && ald->data[n])
			return ald->data[n];
		fprintf(stderr, "ald: Page %lu is out of range\n", n);
		return NULL;
	}

	for (int i = 0; i < ald->len; i++) {
		AldEntry *e = ald->data[i];
		if (e && !strcasecmp(num_or_name, sjis2utf(e->name)))
			return e;
	}
	fprintf(stderr, "ald: No entry for '%s'\n", num_or_name);
	return NULL;
}

static void help_list(void) {
	puts("Usage: ald list <aldfile>...");
}

static int do_list(int argc, char *argv[]) {
	if (argc == 1) {
		help_list();
		return 1;
	}
	Vector *ald = new_vec();
	for (int i = 1; i < argc; i++)
		ald_read(ald, argv[i]);
	char buf[30];
	for (int i = 0; i < ald->len; i++) {
		AldEntry *e = ald->data[i];
		if (!e) {
			printf("%4d  (missing)\n", i);
			continue;
		}
		struct tm *t = localtime(&e->timestamp);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
		printf("%4d  %d  %s  %8d  %s\n", i, e->disk, buf, e->size, sjis2utf(e->name));
	}
	return 0;
}

static const char extract_short_options[] = "d:";
static const struct option extract_long_options[] = {
	{ "directory", required_argument, NULL, 'd' },
	{ 0, 0, 0, 0 }
};

static void help_extract(void) {
	puts("Usage: ald extract [options] <aldfile>... [--] [(<n>|<file>)...]");
	puts("Options:");
	puts("    -d, --directory <dir>    Extract files into <dir>");
}

static void extract_entry(AldEntry *e, const char *directory) {
	FILE *fp = checked_fopen(path_join(directory, sjis2utf(e->name)), "wb");
	if (fwrite(e->data, e->size, 1, fp) != 1)
		error("%s: %s", sjis2utf(e->name), strerror(errno));
	fclose(fp);
}

static int do_extract(int argc, char *argv[]) {
	const char *directory = NULL;
	int opt;
	while ((opt = getopt_long(argc, argv, extract_short_options, extract_long_options, NULL)) != -1) {
		switch (opt) {
		case 'd':
			directory = optarg;
			break;
		default:
			help_extract();
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	Vector *ald = read_alds(&argc, &argv);
	if (!ald) {
		help_extract();
		return 1;
	}

	if (directory && make_dir(directory) != 0 && errno != EEXIST)
		error("cannot create directory %s: %s", directory, strerror(errno));

	if (!argc) {
		// Extract all files.
		for (int i = 0; i < ald->len; i++) {
			AldEntry *e = ald->data[i];
			if (e)
				extract_entry(e, directory);
		}
	} else {
		for (int i = 0; i < argc; i++) {
			AldEntry *e = find_entry(ald, argv[i]);
			if (e)
				extract_entry(e, directory);
		}
	}
	return 0;
}

static void help_dump(void) {
	puts("Usage: ald dump <aldfile>... [--] <n>|<file>");
}

static void print_sjis_2byte(uint8_t c1, uint8_t c2) {
	char in[3] = {c1, c2, 0};
	char *out = sjis2utf_sub(in, '.');
	fputs(out, stdout);
}

static void dump_entry(AldEntry *entry) {
	bool skip_first = false;
	for (int addr = 0; addr < entry->size; addr += 16) {
		int n = (entry->size - addr > 16) ? 16 : entry->size - addr;
		printf("%08x: ", addr);
		for (int i = 0; i < n; i++)
			printf("%02x ", entry->data[addr + i]);
		for (int i = n; i < 16; i++)
			printf("   ");
		putchar(' ');

		const uint8_t *p = entry->data + addr;
		for (int i = 0; i < n; i++) {
			if (i == 0 && skip_first) {
				putchar(' ');
				skip_first = false;
				continue;
			}
			if (is_sjis_byte1(p[i])) {
				if (addr + i + 1 < entry->size && is_sjis_byte2(p[i+1])) {
					print_sjis_2byte(p[i], p[i+1]);
					if (i == 15)
						skip_first = true;
					i++;
				} else {
					putchar('.');
				}
			} else if (is_sjis_half_kana(p[i])) {
				print_sjis_2byte(p[i], 0);
			} else if (isprint(p[i])) {
				putchar(p[i]);
			} else {
				putchar('.');
			}
		}
		putchar('\n');
	}
}

static int do_dump(int argc, char *argv[]) {
	argc--;
	argv++;
	Vector *ald = read_alds(&argc, &argv);
	if (!ald || argc != 1) {
		help_dump();
		return 1;
	}

	AldEntry *e = find_entry(ald, argv[0]);
	if (!e)
		return 1;
	dump_entry(e);
	return 0;
}

static void help_compare(void) {
	puts("Usage: ald compare <aldfile1> <aldfile2>");
}

static bool compare_entry(int page, AldEntry *e1, AldEntry *e2) {
	if (strcasecmp(e1->name, e2->name)) {
		printf("Entry %d: names differ, %s != %s\n", page, sjis2utf(e1->name), sjis2utf(e2->name));
		return true;
	}
	if (e1->size == e2->size && !memcmp(e1->data, e2->data, e1->size))
		return false;

	int i;
	for (i = 0; i < e1->size && i < e2->size; i++) {
		if (e1->data[i] != e2->data[i])
			break;
	}
	printf("%s (%d): differ at %05x\n", sjis2utf(e1->name), page, i);
	return true;
}

static int do_compare(int argc, char *argv[]) {
	if (argc != 3) {
		help_compare();
		return 1;
	}
	const char *aldfile1 = argv[1];
	const char *aldfile2 = argv[2];
	Vector *ald1 = ald_read(NULL, aldfile1);
	Vector *ald2 = ald_read(NULL, aldfile2);

	bool differs = false;
	for (int i = 0; i < ald1->len && i < ald2->len; i++) {
		if (ald1->data[i] && ald2->data[i]) {
			differs |= compare_entry(i, ald1->data[i], ald2->data[i]);
		} else if (ald1->data[i]) {
			AldEntry *e = ald1->data[i];
			printf("%s (%d) only exists in %s\n", sjis2utf(e->name), i, aldfile1);
			differs = true;
		} else if (ald2->data[i]) {
			AldEntry *e = ald2->data[i];
			printf("%s (%d) only exists in %s\n", sjis2utf(e->name), i, aldfile2);
			differs = true;
		}
	}

	for (int i = ald2->len; i < ald1->len; i++) {
		AldEntry *e = ald1->data[i];
		printf("%s (%d) only exists in %s\n", sjis2utf(e->name), i, aldfile1);
		differs = true;
	}
	for (int i = ald1->len; i < ald2->len; i++) {
		AldEntry *e = ald2->data[i];
		printf("%s (%d) only exists in %s\n", sjis2utf(e->name), i, aldfile2);
		differs = true;
	}
	return differs ? 1 : 0;
}

static void help_help(void) {
	puts("Usage: ald help <command>");
}

static int do_help(int argc, char *argv[]);

static void help_version(void) {
	puts("Usage: ald version");
}

static int do_version(int argc, char *argv[]) {
	puts("ald " VERSION);
	return 0;
}

typedef struct {
	const char *name;
	int (*func)(int argc, char *argv[]);
	void (*help)(void);
} Command;

Command commands[] = {
	{"list",    do_list,    help_list},
	{"extract", do_extract, help_extract},
	{"dump",    do_dump,    help_dump},
	{"compare", do_compare, help_compare},
	{"help",    do_help,    help_help},
	{"version", do_version, help_version},
	{NULL, NULL, NULL}
};

static int do_help(int argc, char *argv[]) {
	if (argc == 1) {
		help_help();
		return 1;
	}
	for (Command *cmd = commands; cmd->name; cmd++) {
		if (!strcmp(argv[1], cmd->name)) {
			cmd->help();
			return 0;
		}
	}
	error("ald help: Invalid subcommand '%s'", argv[1]);
}

int main(int argc, char *argv[]) {
	init();

	if (argc == 1) {
		usage();
		return 1;
	}
	for (Command *cmd = commands; cmd->name; cmd++) {
		if (!strcmp(argv[1], cmd->name))
			return cmd->func(argc - 1, argv + 1);
	}
	error("ald: Invalid subcommand '%s'", argv[1]);
}
