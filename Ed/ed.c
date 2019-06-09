#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

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
} *lineIt, *currentIt;

struct File {
    TAILQ_HEAD(line_list, Line) lineList;
} file;

bool printErrors = false;

int lines = 0;
int currentLine = 0;

char * lastError = NULL;

bool
isEmpty(const char * s)
{
	while(isspace((unsigned char) *s)) {
		s++;
	}
	
	return (*s == '\0');
}

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
	int totalLen = 0;
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

		totalLen += strlen(item->line);
		
		TAILQ_INSERT_TAIL(&file.lineList, item, pointers);
		currentIt = item;
		
		++lines;
		if (feof(f)) {
			break;
		}
	}

	currentLine = lines;

	if (fclose(f) == EOF) {
		fprintf(stderr, "fclose f");
	}

	printf("%d\n", totalLen);
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
	if (!isEmpty(address)) {
		handleError("Unexpected address");
		return (false);
	}

	return (true);
}

int
toAbsoluteAddress(char * addrStr)
{
	int len = strlen(addrStr);
	if (len > 0 && isspace(addrStr[0])) {
		return (0); // no leading space allowed
	}
	
	int addr = atoi(addrStr);
	if (!addr && len == 1) {
		switch (addrStr[0]) {
			case '.':
				return currentLine;
			case '$':
				return lines;
			default:
				return 0;
		}
	}
	
	if (addr < 0) {
		return (currentLine + addr);
	} else {
		return (addr);
	}
}

bool
parseAddress(char * address, int * from, int * to)
{
	if (strlen(address) == 0) {
		*from = *to = currentLine;
	} else if (strchr(address, ',') == NULL) {
		*from = *to = toAbsoluteAddress(address);
	} else {
		char *fromStr;
		char *toStr;
		fromStr = strtok(address, ",");
		toStr = strtok(NULL, ",");
				
		*from = toAbsoluteAddress(fromStr);
		*to = toAbsoluteAddress(toStr);
	}
	
	//printf("(%d) (%d)\n", *from, *to);
		
	if (!*from || !*to || *from > *to || *from > lines || *to > lines) {
		handleError("Invalid address");
		return (false);
	}

	return (true);
}

void
printCurrentLine()
{
	
}

void
setCurrentLine(int lineNumber)
{
	int i = 1;
	TAILQ_FOREACH(lineIt, &file.lineList, pointers) {
		if (i == lineNumber) {
			currentIt = lineIt;
			puts(currentIt->line);
			return;
		}
		++i;
	}
}

void
processCommand(char * inputStr)
{
	char command;
	char address[COMMAND_LEN] = "";
	
	char commandStr[COMMAND_LEN] = "";
	char suffix[COMMAND_LEN] = "";
	
	int count = sscanf(inputStr, "%[^qhHnp%]%c%s", address, commandStr, suffix);
	
	if (count == 0) {
		count = sscanf(inputStr, "%c%s", commandStr, suffix);
	}
	
	command = commandStr[0];
	//printf("addr (%s), command(%s) suffix(%s) count(%d)\n", address, commandStr, suffix, count);

	bool invalidSuffix = !isEmpty(suffix);
	
	int addrFrom;
	int addrTo;
	
	if (command == 0) {
		if (!parseAddress(address, &addrFrom, &addrTo)) {
			return;
		}
		
		setCurrentLine(addrTo);
		return;
	} else {	
		switch (command) {
			case 'q':
			case 'h':
			case 'H':
				if (!checkEmptyAddress(address)) {
					return;
				}

				break;
				
			case 'n':
			case 'p':
				if (!parseAddress(address, &addrFrom, &addrTo)) {
					return;
				}

				break;
				
		}
	}

	if (invalidSuffix) {
		handleError("Invalid command suffix");
		return;
	}
	
	switch (command) {
		case 'q':
			exit(0);
			break;

		case 'h':
			printLastError();
			break;

		case 'H':
			printErrors = !printErrors;
			printLastError();
			break;
		
		case 'n':
			break;
			
		case 'p':
		
			break;

		default:
			puts("def");
			break;
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

	for (;;) {
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
