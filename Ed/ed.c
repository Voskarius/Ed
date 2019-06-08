#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <stdbool.h>
#include <sys/queue.h>

#if __STDC_VERSION__ < 199901L
#error "You need C99 compiler or greater."
#endif

#define	BUFLEN 1024
#define	COMMAND_LEN 128

struct Line {
	char * line;
	TAILQ_ENTRY(Line) pointers;
} *lineIt;

struct File {
    TAILQ_HEAD(line_list, Line) lineList;
} file;

bool printErrors = false;

int lines = 0;
int currentLine = 0;

char * lastError = NULL;

char *
strdup(const char *s)
{
	size_t size = strlen(s) + 1;
	char *p = malloc(size);
	if (p != NULL) {
		memcpy(p, s, size);
	}
	return (p);
}

bool
loadFile(char * fileName)
{
	FILE * fileStream = fopen(fileName, "r");
	if (!fileStream) {
		fprintf(stderr, "%s", strerror(errno));
		return (false);
	}

	TAILQ_INIT(&file.lineList);

	FILE *f;
	char buffer[BUFLEN];

	if ((f = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "fopen");
	}

	while (fgets(buffer, sizeof (buffer), f) != NULL) {
		struct Line *item = malloc(sizeof (struct Line));
		item->line = strdup(buffer);

		TAILQ_INSERT_TAIL(&file.lineList, item, pointers);

		++lines;
		if (feof(f)) {
			break;
		}
	}

	currentLine = lines;

	if (fclose(f) == EOF) {
		fprintf(stderr, "fclose f");
	}

	return (true);
}

void
printLastError()
{
	if (lastError != NULL) {
		puts(lastError);
	}
}

void
handleError(char * error)
{
	lastError = error;
	puts("?");
	if (printErrors) {
		printLastError();
	}
}

bool
checkEmptyAddress(char * address)
{
	if (strlen(address) > 0) {
		handleError("Unexpected address");
		return (false);
	}

	return (true);
}

void
processCommand(char * commandString)
{
	int len = strlen(commandString);
	char command[COMMAND_LEN];
	char address[COMMAND_LEN];
	// memcpy(address, commandString, len - 1);

	sscanf(commandString, "%[^qhHnp]%s", address, command);

	bool invalidSuffix = strlen(command) > 1;
	if (invalidSuffix)
		puts("TEST - invalid suffix");

	printf("addr(%s) cmd(%s)\n", address, command);
	switch (command[0]) {
		case 'q':
			if (!checkEmptyAddress(address)) {
				break;
			}
			exit(0);
			break;

		case 'h':
			if (!checkEmptyAddress(address)) {
				break;
			}

			printLastError();
			break;

		case 'H':
			if (!checkEmptyAddress(address)) {
				break;
			}

			printErrors = !printErrors;
			printLastError();
			break;

		default:
			printf("CMD - %s", command);
			memcpy(address, commandString, len);
			puts(address);
			break;
	}

	if (invalidSuffix) {
		handleError("Invalid command suffix");
	}
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "ed: not enough arguments\nusage: ed file");
		return (1);
	}

	char * fn = argv[1];
	if (fn[0] == '-') {
		fprintf(stderr,
			"ed: illegal option -- %s\nusage: ed file", &fn[1]);
		return (1);
	}

	if (!loadFile(fn)) {
		return (1);
	}

	char input[COMMAND_LEN];

	while (true) {
		scanf("%[^\n]%*c", input);

		if (strlen(input) > 0) {
			processCommand(input);
		}
	}

	int i = 0;

	// TODO remove
	TAILQ_FOREACH(lineIt, &file.lineList, pointers) {
		printf("%d %s", i++, lineIt->line);
	}

	return (0);
}
