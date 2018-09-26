#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tape {
	bool *values;
	int length, head;
} tape;

struct instruction {
	char operation;
	int link1, link2;
} *instructions;

int currentStep = 0, currentInstructionNumber = 0, nextInstructionNumber = 1;
char *outputFileName;

void error(int code) {
	char *errorMessage;
	switch(code) {
		case 101: errorMessage = "Invalid number of arguments."; break;
		case 102: errorMessage = "Input file not found."; break;
		case 103: errorMessage = "Memory allocation error."; break;
		case 201: errorMessage = "Invalid tape file. Head not found."; break;
		case 202: errorMessage = "Invalid tape file. Several heads found."; break;
		case 203: errorMessage = "Invalid tape file. Invalid position of head."; break;
		case 204: errorMessage = "Invalid tape file. Invalid position of tape."; break;
		case 205: errorMessage = "Invalid tape file. Invalid character found."; break;
		case 301: errorMessage = "Invalid instructions file. Invalid instruction format."; break;
		case 302: errorMessage = "Invalid instructions file. Invalid instruction number."; break;
		case 303: errorMessage = "Invalid instructions file. Invalid operation found."; break;
		case 304: errorMessage = "Invalid instructions file. Invalid transition direction."; break;
		case 305: errorMessage = "Invalid instructions file. Stop instruction not found."; break;
	}
	fprintf(stderr, "Error %d: %s",code, errorMessage);
	if (code == 101) fprintf(stderr, "\npost-machine.exe <tape file> <instructions file> <output file> [-d]\n -d: enter debug mode\n");
	exit(code);
}

void readTape(char *fileName) {
	tape.values = (bool*) malloc(sizeof(bool));
	tape.length = 0;
	tape.head = 0;
	FILE *tapeFile = fopen(fileName, "r");
	if (tapeFile == NULL) error(102);
	bool isHeadFound = false;
	int line = 1;
	char c;
	while ((c = fgetc(tapeFile)) != EOF) {
		switch (c) {
			case ' ':
				if (!isHeadFound) tape.head++;
				if (line == 2) error(205);
				break;
			case 'v':
				if (isHeadFound) error(202);
				else if (line == 1) isHeadFound = true;
				else error(203);
				break;
			case '0':
			case '1':
				if (line == 2) {
					tape.values = (bool*) realloc(tape.values, (tape.length + 1) * sizeof(bool));
					if (tape.values == NULL) error(103);
					tape.values[tape.length] = (c == '1');
					tape.length++;
				} else error(204);
				break;
			case '\n':
				line++;
				break;
			default:
				error(205);
		}
	}
	if (!isHeadFound) error(201);
	fclose(tapeFile);
}

void readInstructions(char *fileName) {
	instructions = (struct instruction*) malloc(2 * sizeof(struct instruction));
	FILE *instructionsFile = fopen(fileName, "r");
	if (instructionsFile == NULL) error(102);
	int i = 1;
	int index;
	bool isStopFound = false;
	int f;
	while ((f = fscanf(instructionsFile, "%d. %c", &index, &instructions[i].operation)) != EOF) {
		if (f < 2) error(301);
		if (index != i) error(302);
		switch(instructions[i].operation) {
			case '<':
			case '>':
			case '0':
			case '1':
				if (fgetc(instructionsFile) != '\n') {
					fscanf(instructionsFile, "%d", &instructions[i].link1);
					if (instructions[i].link1 <= 0) error(304);
				}
				else instructions[i].link1 = i + 1;
				instructions[i].link2 = 0;
				break;
			case '?':
				if (fscanf(instructionsFile, "%d; %d", &instructions[i].link1, &instructions[i].link2) < 2) error(301);
				if (instructions[i].link1 <= 0 || instructions[i].link2 <= 0) error(304);
				break;
			case '!':
				isStopFound = true;
				instructions[i].link1 = 0;
				instructions[i].link2 = 0;
				break;
			default:
				error(303);
		}
		fscanf(instructionsFile, "\n");
		i++;
		instructions = (struct instruction*) realloc(instructions, (i + 1) * sizeof(struct instruction));
		if (instructions == NULL) error(103);
	}
	if (!isStopFound) error(305);
	i--;
	for (int k = 1; k <= i; k++) {
		if ((instructions[k].link1 > i) || (instructions[k].link2 > i)) error(304);
	}
	fclose(instructionsFile);
}

void leftShift() {
	tape.head--;
	if (tape.head < 0) {
		tape.values = (bool*) realloc(tape.values, (tape.length + 1) * sizeof(bool));
		if (tape.values == NULL) error(103);
		tape.length++;
		for (int i = tape.length; i > 0; i--) tape.values[i] = tape.values[i - 1];
		tape.values[0] = false;
		tape.head = 0;
	}
}

void rightShift() {
	tape.head++;
	if (tape.head > tape.length - 1) {
		tape.values = (bool*) realloc(tape.values, (tape.length + 1) * sizeof(bool));
		if (tape.values == NULL) error(103);
		tape.values[tape.length] = false;
		tape.length++;
	}
}

void mark() {
	tape.values[tape.head] = true;
}

void unmark() {
	tape.values[tape.head] = false;
}

bool isMarked() {
	return tape.values[tape.head];
}

void printCurrentInstruction(char *fileName) {
	FILE *outputFile;
	if (strcmp(fileName,"stdout") == 0) outputFile = stdout;
	else outputFile = fopen(fileName, "w");
	fprintf(outputFile, "Step: %d, Instruction: %d. %c", currentStep, currentInstructionNumber, instructions[currentInstructionNumber].operation);
	switch (instructions[currentInstructionNumber].operation) {
		case '?':
			fprintf(outputFile, " %d; %d\n", instructions[currentInstructionNumber].link1, instructions[currentInstructionNumber].link2);
			break;
		case '!':
			fprintf(outputFile, "\n");
			break;
		default:
			fprintf(outputFile, " %d\n", instructions[currentInstructionNumber].link1);
	}
}

void printTape(char *fileName) {
	FILE *outputFile;
	if (strcmp(fileName,"stdout") == 0) outputFile = stdout;
	else outputFile = fopen(fileName, "w");
	if (outputFile == stdout) printCurrentInstruction("stdout");
	int length = 100;
	for (int i = 0; i < length/2; i++) {
		fprintf(outputFile, " ");
	}
	fprintf(outputFile, "v\n");
	for (int i = 0; i < length/2 - tape.head; i++) {
		fprintf(outputFile, "%d", 0);
	}
	for (int i = 0; i < tape.length; i++) {
		fprintf(outputFile, "%d", tape.values[i]);
	}
	for (int i = 0; i < length/2 - tape.length + tape.head + 1; i++) {
		fprintf(outputFile, "%d", 0);
	}
	fprintf(outputFile, "\n");
	if (outputFile != stdout) fclose(outputFile);
}

bool run(bool isStep) {
	currentStep++;
	currentInstructionNumber = nextInstructionNumber;
	nextInstructionNumber = instructions[currentInstructionNumber].link1;
	switch (instructions[currentInstructionNumber].operation) {
		case '<':
			leftShift();
			break;
		case '>':
			rightShift();
			break;
		case '0':
			unmark();
			break;
		case '1':
			mark();
			break;
		case '?':
			if (isMarked()) nextInstructionNumber = instructions[currentInstructionNumber].link2;
			break;
		case '!':
			printTape(outputFileName);
			return true;
	}; 
	printTape(outputFileName);
	if (!isStep) run(false);
	return false;
}

void debug() {
	printf("     S: Step\n  C<N>: N steps\n     R: Run\n     E: Exit\n");
	bool isEnd = false;
	char command;
	while (!isEnd) {
		scanf("%c", &command);
		switch(command) {
			case 'S':
			case 's':
				isEnd = run(true);
				printTape("stdout");
				if (isEnd) exit(0);
				break;
			case 'C':
			case 'c':
				;
				int n;
				scanf("%d", &n);
				for (int i = 0; i < n; i++) {
					isEnd = run(true);
					printTape("stdout");
					if (isEnd) exit(0);
				}
				break;
			case 'R':
			case 'r':
				while (!isEnd) {
					isEnd = run(true);
					printTape("stdout");
				}
				if (isEnd) exit(0);
				break;
			case 'E':
			case 'e':
				exit(0);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 4) error(101);
	readTape(argv[1]);
	readInstructions(argv[2]);
	outputFileName = argv[3];
	if (argc == 5 && strcmp(argv[4], "-d") == 0) debug(); else run(false);
	free(tape.values);
	free(instructions);
	return 0;
}
