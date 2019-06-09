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

// file structures
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
	while (isspace((unsigned char) *s)) {
		s++;
	}

	return (*s == '\0');
}

// strdup is non-standard - reimplemented here
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

	TAILQ_INIT(&file.lineList);

	FILE *f;
	char buffer[BUFLEN];

	if ((f = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "%s: %s\nCannot open input file\n",
			fileName, strerror(errno));
		return (false);
	}

	while (fgets(buffer, sizeof (buffer), f) != NULL) {
		// add line to list
		struct Line *item = malloc(sizeof (struct Line));
		item->line = strdup(buffer);

		totalLen += strlen(item->line);

		TAILQ_INSERT_TAIL(&file.lineList, item, pointers);

		// keep pointer to the last line
		currentIt = item;

		++lines;
		if (feof(f)) {
			break;
		}
	}

	// set currentLine index to the last line
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

// parses address string and convert to line index
// expects a single address, not range, i.e. ".,20"
int
toAbsoluteAddress(char * addrStr)
{
	if (addrStr == NULL || isEmpty(addrStr)) {
		return (0);
	}
	int len = strlen(addrStr);
	if (len > 0 && isspace(addrStr[0])) {
		return (0); // no leading space allowed
	}

	int addr = atoi(addrStr);
	if (addr && strchr(addrStr, '.') != NULL) { // discard decimal numbers
		return (0);
	}

	if (!addr && (len == 1 || isspace(addrStr[1]))) {
		switch (addrStr[0]) {
			case '.':
				return (currentLine);
			case '$':
				return (lines);
			default:
				return (0);
		}
	}

	if (addr < 0) {
		return (currentLine + addr);
	} else {
		return (addr);
	}
}

// parses full address string and assigns indeces to from and to vars
// returns true if string was a valid address, false otherwise
bool
parseAddress(char * address, int * from, int * to)
{
	if (strchr(address, ',') == NULL) {
		*from = *to = toAbsoluteAddress(address);
	} else {
		char *fromStr;
		char *toStr;
		fromStr = strtok(address, ",");
		toStr = strtok(NULL, ",");

		*from = toAbsoluteAddress(fromStr);
		*to = toAbsoluteAddress(toStr);
	}

	if (!*from || !*to || *from > *to || *from > lines || *to > lines) {
		return (false);
	}

	return (true);
}

void
printCurrentLine()
{
	if (currentLine > 0 && currentLine <= lines) {
		printf("%s", currentIt->line);
	} else {
		handleError("Invalid address");
	}
}

// sets current line index and pointer to the specified line
void
setCurrentLine(int lineNumber)
{
	int i = 1;
	TAILQ_FOREACH(lineIt, &file.lineList, pointers) {
		if (i == lineNumber) {
			currentIt = lineIt;
			currentLine = i;
			return;
		}

		++i;
	}
}

// moves current line index and pointer to the next line
void
setNextLine()
{
	if (currentLine > lines) {
		return;
	}

	currentIt = TAILQ_NEXT(currentIt, pointers);
	++currentLine;
}

// prints all lines between from and to indeces (inclusive)
// optionally adds line numbers
void
printRange(int from, int to, bool withLineNumbers)
{
	setCurrentLine(from);
	if (withLineNumbers) {
		printf("%d\t", from);
	}
	printCurrentLine();

	for (int i = from + 1; i <= to; ++i) {
		setNextLine();
		if (withLineNumbers) {
			printf("%d\t", i);
		}

		printCurrentLine();
	}
}

// parses and validated command string
// executes command if valid
void
processCommand(char * inputStr)
{
	char command;
	char address[COMMAND_LEN] = "";

	char commandStr[COMMAND_LEN] = "";
	char suffix[COMMAND_LEN] = "";

	int count = sscanf(inputStr, "%[^a-zA-Z%]%c%s",
		address, commandStr, suffix);

	if (count == 0) {
		count = sscanf(inputStr, "%c%s", commandStr, suffix);
	}

	command = commandStr[0];
	bool invalidSuffix = !isEmpty(suffix);

	int addrFrom;
	int addrTo;

	bool unexpectedAddress = false;
	bool invalidAddress = false;

	if (command == 0) {
		// just <enter> detected - print next line
		if (isEmpty(address)) {
			setNextLine();
			printCurrentLine();
			return;
		} else if (!parseAddress(address, &addrFrom, &addrTo)) {
			handleError("Invalid address");
			return;
		}

		printRange(addrFrom, addrTo, false);
		return;
	} else {
		// validate input
		invalidAddress = !isEmpty(address) &&
			!parseAddress(address, &addrFrom, &addrTo);
		if (invalidAddress) {
			handleError("Invalid address");
			return;
		}

		// validate command code
		switch (command) {
			case 'q':
			case 'h':
			case 'H':
				if (!isEmpty(address)) {
					unexpectedAddress = true;
				}

				break;

			case 'n':
			case 'p':
				break;

			default:
				handleError("Unknown command");
				return;
		}
	}

	// print errors with the expected priority
	if (invalidAddress) {
		handleError("Invalid address");
		return;
	}

	if (invalidSuffix) {
		handleError("Invalid command suffix");
		return;
	}

	if (unexpectedAddress) {
		handleError("Unexpected address");
		return;
	}

	// execute command
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
			printRange(addrFrom, addrTo, true);
			break;

		case 'p':
			printRange(addrFrom, addrTo, false);
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

	// continue even if load fails - list is empty
	loadFile(fn);

	for (;;) {
		char input[COMMAND_LEN];
		if (!fgets(input, BUFLEN, stdin)) {
			break;
		}

		if (strlen(input) > 0) {
			processCommand(input);
		}
	}

	return (lastError == NULL ? 0 : 1);
}
