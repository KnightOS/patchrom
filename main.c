#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#define PAGE_SIZE 0x4000

struct entry {
	char *symbol;
	unsigned short address;
	int found;
	struct entry *next;
};

struct {
	FILE *config;
	FILE *rom;
	unsigned char page;
	struct entry *symbols;
	int c_headers;
} context;

void show_help() {
	puts(
		"patchrom - Patch jump table into a ROM dump\n"
		"\n"
		"Usage: patchrom [-c] CONFIG ROM PAGE < SYMBOLS\n"
		"See `man 1 patchrom` for details.\n"
		"\n"
		"Creates a jump table in PAGE of ROM and outputs index definitions as\n"
		"configured CONFIG from symbols from stdin. '-c' instructs patchrom to output a C header.\n"
		"\n"
		"Examples:\n"
		"\tpatchrom 00.config example.rom 0x00 <00.sym >00.inc\n"
		"\t\tPatch example.rom with a jump table containing symbols listed in 00.config\n"
		"\t\tand output the jump table index definitions to 00.inc"
	);
}

void parse_context(int argc, char **argv) {
	const char *errorMessage = "Invalid usage - see `patchrom --help`";
	context.c_headers = 0;
	int i;
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') {
			if (strcasecmp(argv[i], "--help") == 0) {
				show_help();
				exit(0);
			} else if (strcmp(argv[i], "-c") == 0) {
				context.c_headers = 1;
			} else {
				fputs(errorMessage, stderr);
				exit(1);
			}
		}
	}
	if (argc > 5) {
		fputs(errorMessage, stderr);
		exit(1);
	}

	context.config = fopen(argv[1 + context.c_headers], "r");
	if (context.config == NULL) {
		fprintf(stderr, "Unable to open %s\n", argv[1 + context.c_headers]);
		exit(1);
	}
	context.rom = fopen(argv[2 + context.c_headers], "r+");
	if (context.rom == NULL) {
		fprintf(stderr, "Unable to open %s\n", argv[2 + context.c_headers]);
		exit(1);
	}
	if (sscanf(argv[3 + context.c_headers], "0x%hhX", &context.page) != 1 &&
		sscanf(argv[3 + context.c_headers], "%hhu", &context.page) != 1) {
		fputs(errorMessage, stderr);
		exit(1);
	}
}

void load_config() {
	int comment;
	char buf[256];
	char *pbuf;
	int c;
	struct entry *ent, *prev = NULL;

	while(1) { // config lines
		comment = 0;
		pbuf = buf;
		while(pbuf < buf+255) { // parse line
			c = fgetc(context.config);
			if (c == EOF) return;
			if (c == '#') {
				comment = 1;
				continue;
			}
			if (c == ' ' || c == '\t' || c == '\r') continue;
			if (c == '\n') {
				if(!comment && pbuf == buf) continue; // empty line
				else break;
			}
			if (!comment) {
				*pbuf++ = (char)toupper(c);
			}
		} // parse line
		if (comment) {
			continue;
		}
		*pbuf = '\0';

		ent = malloc(sizeof(struct entry));
		ent->symbol = malloc(strlen(buf)+1);
		strcpy(ent->symbol, buf);
		ent->found = 0;
		ent->next = NULL;

		if (prev == NULL) {
			context.symbols = ent;
		} else {
			prev->next = ent;
		}
		prev = ent;
	} // config lines
}

void load_symbols() {
	int comment, sym_not_found;
	char buf[256], sym[256];
	unsigned short address;
	char *pbuf;
	int c;
	struct entry *ent;

	while (1) { // symbol lines
		comment = 0;
		pbuf = buf;
		while (pbuf < buf+255) { // parse line
			c = fgetc(stdin);
			if (c == EOF) goto load_symbols_end;
			if (c == '\n') {
				if(!comment && pbuf == buf) continue; // empty line
				else break;
			}
			if (c == ';') {
				comment = 1;
				continue;
			}
			if (c == '\r') continue;
			if(!comment) {
				*pbuf++ = (char)c;
			}
		} // parse line
		if (comment) {
			continue;
		}
		*pbuf = '\0';

		if (sscanf(buf, ".equ %s 0x%hX", sym, &address) != 2) {
			fprintf(stderr, "Failed to parse symbol data line: %s\n", buf);
			exit(1);
		}
		for (ent = context.symbols; ent; ent = ent->next) {
			if (strcasecmp(ent->symbol, sym) == 0) {
				ent->address = address;
				ent->found = 1;
				break;
			}
		}
	} // symbol lines
	load_symbols_end:

	sym_not_found = 0;
	for (ent = context.symbols; ent; ent = ent->next) {
		if (!ent->found) {
			fprintf(stderr, "Symbol not found: %s\n", ent->symbol);
			sym_not_found = 1;
		}
	}
	if (sym_not_found) {
		exit(1);
	}
}

int main(int argc, char **argv) {
	struct entry *ent;
	int index = 0;
	parse_context(argc, argv);
	load_config();
	fclose(context.config);
	load_symbols();

	fseek(context.rom, (context.page+1)*PAGE_SIZE - 3, SEEK_SET);

	for (ent = context.symbols; ent; ent = ent->next) {
		fputc(0xC3, context.rom);
		fputc(ent->address & 0xff, context.rom);
		fputc(ent->address >> 8, context.rom);
		fseek(context.rom, -6, SEEK_CUR);
		if (context.c_headers) {
			printf("#define %s 0x%.2X%.2hX\n", ent->symbol, index++, context.page);
		} else {
			printf(".equ %s 0x%.2X%.2hX\n", ent->symbol, index++, context.page);
		}
	}

	fclose(context.rom);

	return 0;
}
