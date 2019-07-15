// NPRG066 - Zapocet - phase 1 + 2
// Oskar Hybl

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

extern char *strtok_r(char *, const char *, char **);
extern char *strdup(const char *s);

// file structures
struct Line {
	char * line;
	TAILQ_ENTRY(Line) pointers;
} *lineIt, *currentIt, *toDelete;

struct File {
    TAILQ_HEAD(line_list, Line) lineList;
} file;

bool printErrors = false;
bool bufferModified = false;
bool triedToQuit = false;

int lines = 0;
int currentLine = 0;
int totalLen = 0;

char * lastError = NULL;
char * fileName = NULL;

bool
isEmpty(const char * s)
{
	while (isspace((unsigned char) *s)) {
		s++;
	}

	return (*s == '\0');
}

bool
loadFile(char * fileName)
{
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
		if (item == NULL) {
			perror("Cannot load file");
			exit(1);
		}

		item->line = strdup(buffer);
		if (item->line == NULL) {
			perror("Cannot load file");
			free(item);
			exit(1);
		}

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
		char *savePtr;
		fromStr = strtok_r(address, ",", &savePtr);
		toStr = strtok_r(NULL, ",", &savePtr);

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

// delete lines no. [from, to]
void
deleteRange(int from, int to)
{
	setCurrentLine(from);

	for (int i = from; i <= to; ++i) {
		toDelete = currentIt;
		totalLen -= strlen(currentIt->line);
		setNextLine();
		TAILQ_REMOVE(&file.lineList, toDelete, pointers);
		free(toDelete->line);
		free(toDelete);
		lines--;
	}

	setCurrentLine(from > lines ? lines : from);

	bufferModified = true;
}

// enters insert mode.
// new lines will be prepended to line addr addr
// exit edit mode by typing line containing a single dot (".") only
void
initInsertMode(int addr)
{
	bool insertingHead = false;

	// insert new lines before addr == after addr - 1
	// inserting at start won't need (and cannot have) currentLine set to 0
	if (addr > 1) {
		setCurrentLine(addr - 1);
	} else {
		insertingHead = true;
	}

	char buffer[BUFLEN];
	while (fgets(buffer, sizeof (buffer), stdin) != NULL) {
		if (strcmp(buffer, ".\n") == 0) {
			bufferModified = true;
			return;
		}

		struct Line *item = malloc(sizeof (struct Line));
		if (item == NULL) {
			perror("Cannot insert new line");
			return;
		}

		item->line = strdup(buffer);
		if (item->line == NULL) {
			perror("Cannot insert new line");
			free(item);
			return;
		}

		totalLen += strlen(item->line);

		if (insertingHead) {
			// insert as the new first line
			TAILQ_INSERT_HEAD(&file.lineList, item, pointers);
			insertingHead = false;
			currentLine = 1;
		} else {
			TAILQ_INSERT_AFTER(&file.lineList, currentIt,
				item, pointers);
			++currentLine;
		}

		// keep pointer to the last line
		currentIt = item;
		++lines;
	}
}

// dumps currently loaded text to file "target"
// if no default file is set, sets the default to "target"
void
writeFile(char * target)
{
	printf("%d\n", totalLen);
	FILE * fp = fopen(target, "w");
	if (fp == NULL) {
		handleError("Cannot open output file");
		return;
	}

	TAILQ_FOREACH(lineIt, &file.lineList, pointers) {
		fprintf(fp, "%s", lineIt->line);
	}

	bufferModified = false;

	if (fileName == NULL) {
		fileName = strdup(target);
	}

	fclose(fp);
}

char *
truncateWhitespace(char * s) {
	int startId = 0;
	while (isspace(s[0])) {
		startId++;
	}

	return (&s[startId]);
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
	char * cmdPtr = strchr(inputStr, command);

	bool invalidSuffix = command != 'w' && !isEmpty(suffix);

	// check if 'w' is followed by a space
	if (command == 'w' && !isspace(*(cmdPtr + 1))) {
		handleError("Unexpected command suffix");
		return;
	}

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
		if (command == 'i' && strcmp(address, "0") == 0) {
			// exception for 'i' command which is the only one
			// accepting 0 as address (same behaviour as 1)
			addrFrom = 0;
		} else {
			invalidAddress = !isEmpty(address) &&
				!parseAddress(address, &addrFrom, &addrTo);
			if (invalidAddress) {
				handleError("Invalid address");
				return;
			}
		}

		bool singlePartAddr;

		// validate command code
		switch (command) {
			case 'q':
			case 'h':
			case 'H':
			case 'w':
				if (!isEmpty(address)) {
					unexpectedAddress = true;
				}

				break;

			case 'n':
			case 'p':
			case 'd':
				break;

			case 'i':
				singlePartAddr = (strchr(address, ',') == 0);
				if (!singlePartAddr) {
					handleError("Unknown command");
				}
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

	char * targetName = NULL;

	// execute command
	switch (command) {
		case 'q':
			if (bufferModified) {
				if (triedToQuit) {
					exit(1);
				}
				handleError("Warning: buffer modified");
			} else {
				exit(lastError == NULL ? 0 : 1);
			}
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

		case 'd':
			deleteRange(addrFrom, addrTo);
			break;

		case 'i':
			initInsertMode(addrFrom);
			break;

		case 'w':
			targetName = truncateWhitespace(suffix);
			if (strlen(targetName) == 0) {
				if (fileName == NULL) {
					handleError("No current filename");
					break;
				}
				targetName = fileName;
			}

			writeFile(targetName);
			break;
	}

	triedToQuit = command == 'q';
}

int
main(int argc, char *argv[])
{
	if (argc > 1) {
		fileName = argv[1];
		if (fileName[0] == '-') {
			fprintf(stderr,
				"ed: illegal option -- %s\nusage: ed file",
				&fileName[1]);
			return (1);
		}

		// continue even if load fails - list is empty
		loadFile(fileName);
	}


	for (;;) {
		char input[COMMAND_LEN];
		if (!fgets(input, COMMAND_LEN, stdin)) {
			break;
		}

		if (strlen(input) > 0) {
			processCommand(input);
		}
	}

	return (lastError == NULL ? 0 : 1);
}
