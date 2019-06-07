
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>

#define	BUFLEN 1024

struct Line {
	char* line;
	LIST_ENTRY(Line) pointers;
};

struct File {
    LIST_HEAD(line_list, Line) lineList;
} file;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr,"ed: not enough arguments\nusage: ed file");			
		return 1;
	}

	char* fileName = argv[1];
	if (fileName[0] == '-') {
		fprintf(stderr, "ed: illegal option -- %s\nusage: ed file", fileName[1] + "");
		return 1;
	}
	
	FILE * fileStream = fopen(fileName, "r");
	if (!fileStream) {
		fprintf(stderr, "%s", strerror(errno));
		return 1;
	}
	
	LIST_INIT(&file.lineList);

	size_t n;
	FILE *f;
	char line[BUFLEN];

	if ((f = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "fopen");
	}

	while ((n = fread(line, 1, BUFLEN, f)) > 0) {
		struct Line *item = malloc(sizeof(struct Line));

		LIST_INSERT_HEAD(&file.lineList, item, pointers);
		if (feof(f)) {
			break;
		}
	}

	if (fclose(f) == EOF) {
		fprintf(stderr, "fclose f");
	}
	
	struct Line * lineIt = NULL;
	
	// TODO remove
	LIST_FOREACH(lineIt, &file.lineList, pointers) {
		printf(line);
	}
	
    return 0;
}

