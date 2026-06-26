#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int ROW_SIZE = 16;

typedef struct File File;
struct File {
	FILE* fh;
	size_t length;
	bool exists;
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
	return file;
}

void fileClose(File *file) {
	if (file && file->fh) {
		fclose(file->fh);
	}
}

void printRow(size_t offset, uint8_t* buffer, int length, const char* name) {
	printf("0x%08lx: ", offset);
	for (int i = 0; i < length; ++i) {
		printf("%02x ", buffer[i]);
	}
	while (length < ROW_SIZE) {
		printf("   ");
		++length;
	}
	printf("(%s)\n", name);
}

int min(int v1, int v2) { 
	return v2 > v1 ? v2 : v1; 
}

static File g_file1;
static File g_file2;

void quit(int exit_code) {
	fileClose(&g_file1);
	fileClose(&g_file2);
	exit(exit_code);
}

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("%s: missing operand after '%s'\n", argv[0], argv[argc-1]);
		return 1;
	} else if (argc > 3) {
		printf("%s: extra operand '%s'\n", argv[0], argv[3]);
		return 1;
	}

	g_file1 = fileOpen(argv[1]);
	if (!g_file1.exists) { printf("%s doesn't exist!\n", argv[1]); }

	g_file2 = fileOpen(argv[2]);
	if (!g_file2.exists) { printf("%s doesn't exist!\n", argv[2]); }
	if (!g_file1.exists || !g_file2.exists) { quit(1); }

	if (g_file1.length > g_file2.length) {
		printf("'%s' is longer than '%s' by %ld bytes.\n", argv[1], argv[2], g_file1.length - g_file2.length);
	} else if (g_file1.length < g_file2.length) {
		printf("'%s' is shorter than '%s' by %ld bytes.\n", argv[1], argv[2], g_file2.length - g_file1.length);
	} else {
		printf("'%s' and '%s' are both %ld bytes long.\n", argv[1], argv[2], g_file1.length);
	}

	bool diverged = false;
	for (size_t offset = 0; (offset < g_file1.length || offset < g_file2.length) && !diverged; offset += ROW_SIZE) {
		uint8_t buffer1[ROW_SIZE];
		uint8_t buffer2[ROW_SIZE];
		int len1 = fread(buffer1, 1, ROW_SIZE, g_file1.fh);
		int len2 = fread(buffer2, 1, ROW_SIZE, g_file1.fh);
		int diffOffset = 0;
		int minLength = min(len1, len2);

		for (int j = 0; j < minLength; ++j) {
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
			printRow(offset, buffer1, len1, argv[1]);
			printRow(offset, buffer2, len2, argv[2]);
			printf("            ");
			for (int j = 0; j < diffOffset; ++j) {
				printf("   ");
			}
			printf("^\n");
		}
	}
	if (!diverged) {
		printf("Files are identical and do not diverge.\n");
	}

	quit(0);
}
