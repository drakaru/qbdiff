#!/usr/bin/env -S tcc -run

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

typedef struct File File;
struct File {
	FILE *handle;
	size_t length;
	bool exists;
	char const *name;
};

size_t const ROW_SIZE = 16;

static bool g_verifyIdentical = false;
static File g_file1;
static File g_file2;

File fileOpen(char const *filename) {
	File file = {};
	file.handle = fopen(filename, "rb");
	if (file.handle != NULL) {
		file.exists = true;
		fseek(file.handle, 0, SEEK_END);
		file.length = ftell(file.handle);
		fseek(file.handle, 0, SEEK_SET);
	}
	file.name = filename;
	return file;
}

void fileClose(File const *file) {
	if (file && file->handle) { fclose(file->handle); }
}

#define min(v1, v2) (v2 > v1 ? v2 : v1)

bool parseCommandLine(int argc, char **argv);
void printUsage();
bool diff(File const *file1, File const *file2);
void printRow(size_t offset, uint8_t const *buffer, size_t length, char const *name);

void quit(int const exitCode) {
	fileClose(&g_file1);
	fileClose(&g_file2);
	exit(exitCode);
}

int main(int argc, char** argv) {
	if (!parseCommandLine(argc, argv)) {
		printUsage();
		quit(1);
	}

	if (!g_file1.exists) { printf("qbdiff: %s doesn't exist!\n", g_file1.name); }
	if (!g_file2.exists) { printf("qbdiff: %s doesn't exist!\n", g_file2.name); }
	if (!g_file1.exists || !g_file2.exists) { quit(1); }

	bool const diverged = diff(&g_file1, &g_file2);
	if (!diverged) {
		printf("Files are identical and do not diverge.\n");
	}

	quit(diverged && g_verifyIdentical ? 2 : 0);
}

bool parseCommandLine(int argc, char **argv) {
	static struct option longOptions[] = {
		{"verify-identical", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	int optionIndex = 0;
	int c;
	while ((c = getopt_long(argc, argv, "v", longOptions, &optionIndex)) != -1) {
		switch (c) {
			case 'v':
				g_verifyIdentical = true;
				break;
			default:
				printf("qbdiff: ?? : %d\n", c);
		}
	}

	int const fileArgc = argc - optind;

	if (fileArgc != 2) {
		printf("qbdiff: 2 files expected, got %d\n", fileArgc);
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

bool diff(File const *file1, File const *file2) {
	assert(file1 && file1->handle && "diff assumes valid files");
	assert(file2 && file2->handle && "diff assumes valid files");

	if (file1->length > file2->length) {
		printf("'%s' is longer than '%s' by %ld bytes.\n", file1->name, file2->name, file1->length - file2->length);
	} else if (file1->length < file2->length) {
		printf("'%s' is shorter than '%s' by %ld bytes.\n", file1->name, file2->name, file2->length - file1->length);
	} else {
		printf("'%s' and '%s' are both %ld bytes long.\n", file1->name, file2->name, file1->length);
	}

	bool diverged = false;
	for (size_t offset = 0; (offset < file1->length || offset < file2->length) && !diverged; offset += ROW_SIZE) {
		uint8_t buffer1[ROW_SIZE];
		uint8_t buffer2[ROW_SIZE];
		size_t const len1 = fread(buffer1, 1, ROW_SIZE, file1->handle);
		size_t const len2 = fread(buffer2, 1, ROW_SIZE, file2->handle);
		size_t const minLength = min(len1, len2);
		size_t diffOffset = 0;

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
			printRow(offset, buffer1, len1, file1->name);
			printRow(offset, buffer2, len2, file2->name);
			printf("            ");
			for (size_t j = 0; j < diffOffset; ++j) {
				printf("   ");
			}
			printf("^\n");
		}
	}
	
	return diverged;
}

void printRow(size_t const offset, uint8_t const *buffer, size_t length, char const *name) {
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
