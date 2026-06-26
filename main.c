#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

const size_t ROW_SIZE = 16;

typedef struct File File;
struct File {
	FILE* fh;
	size_t length;
	bool exists;
	char const *name;
};

File fileOpen(const char* filename) {
	File file = {};
	file.fh = fopen(filename, "rb");
	if (file.fh != NULL) {
		file.exists = true;
		fseek(file.fh, 0, SEEK_END);
		file.length = ftell(file.fh);
		fseek(file.fh, 0, SEEK_SET);
	}
	file.name = filename;
	return file;
}

void fileClose(File *file) {
	if (file && file->fh) {
		fclose(file->fh);
	}
}

void printRow(size_t offset, uint8_t* buffer, size_t length, const char* name) {
	printf("0x%08lx: ", offset);
	for (size_t i = 0; i < length; ++i) {
		printf("%02x ", buffer[i]);
	}
	while (length < ROW_SIZE) {
		printf("   ");
		++length;
	}
	printf("(%s)\n", name);
}

#define min(v1, v2) (v2 > v1 ? v2 : v1)

static bool g_verify_identical = false;
static File g_file1;
static File g_file2;

bool parseCommandLine(int argc, char **argv);
void printUsage();

void quit(int exitCode) {
	fileClose(&g_file1);
	fileClose(&g_file2);
	exit(exitCode);
}

int main(int argc, char** argv) {
	if (!parseCommandLine(argc, argv)) {
		printUsage();
		quit(1);
	}

	if (!g_file1.exists) { printf("%s doesn't exist!\n", g_file1.name); }
	if (!g_file2.exists) { printf("%s doesn't exist!\n", g_file2.name); }
	if (!g_file1.exists || !g_file2.exists) { quit(1); }

	if (g_file1.length > g_file2.length) {
		printf("'%s' is longer than '%s' by %ld bytes.\n", g_file1.name, g_file2.name, g_file1.length - g_file2.length);
	} else if (g_file1.length < g_file2.length) {
		printf("'%s' is shorter than '%s' by %ld bytes.\n", g_file1.name, g_file2.name, g_file2.length - g_file1.length);
	} else {
		printf("'%s' and '%s' are both %ld bytes long.\n", g_file1.name, g_file2.name, g_file1.length);
	}

	bool diverged = false;
	for (size_t offset = 0; (offset < g_file1.length || offset < g_file2.length) && !diverged; offset += ROW_SIZE) {
		uint8_t buffer1[ROW_SIZE];
		uint8_t buffer2[ROW_SIZE];
		size_t len1 = fread(buffer1, 1, ROW_SIZE, g_file1.fh);
		size_t len2 = fread(buffer2, 1, ROW_SIZE, g_file2.fh);
		size_t diffOffset = 0;
		size_t minLength = min(len1, len2);

		for (size_t j = 0; j < minLength; ++j) {
			if (buffer1[j] != buffer2[j]) {
				diverged = true;
				diffOffset = j;
				break;
			}
		}

		if (len1 != len2 && !diverged) {
			diverged = true;
			diffOffset = minLength;
		}

		if (diverged) {
			printf("Files diverge at 0x%08lx:\n", offset + diffOffset);
			printRow(offset, buffer1, len1, g_file1.name);
			printRow(offset, buffer2, len2, g_file2.name);
			printf("            ");
			for (size_t j = 0; j < diffOffset; ++j) {
				printf("   ");
			}
			printf("^\n");
		}
	}
	if (!diverged) {
		printf("Files are identical and do not diverge.\n");
	}

	quit(diverged && g_verify_identical ? 2 : 0);
}

bool parseCommandLine(int argc, char **argv) {
	static struct option long_options[] = {
		{"verify-identical", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	int option_index = 0;
	int c;
	while ((c = getopt_long(argc, argv, "v", long_options, &option_index)) != -1) {
		switch (c) {
			case 'v':
				g_verify_identical = true;
				break;
			default:
				printf("qbdiff: ?? : %d\n", c);
		}
	}

	int file_argc = argc - optind;

	if (file_argc != 2) {
		printf("qbdiff: 2 files expected, got %d\n", file_argc);
		return false;
	}

	g_file1 = fileOpen(argv[optind+0]);
	g_file2 = fileOpen(argv[optind+1]);

	return true;
}

void printUsage() {
	puts("Usage:");
	puts("   qbdiff [options] <input file1> <input file2>");
	puts("");
	puts("Options:");
	puts("   -v,--verify-identical Verify files are identical -- exit non-zero when files differ");
}
