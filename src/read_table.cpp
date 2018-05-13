#define SEPARATOR "|"
#define LINE_LENGTH 4096

void table_read(const char* filename, int **table, int length, int attrcount) {
	FILE *fp;
	char buffer[LINE_LENGTH];
	char *token;
	int row, col;

	if ( (fp = fopen(filename, "r")) == NULL) {
		printf("unable to load table: %s \n", filename);
		exit(1);
	}

	row = 0;
	while ( fgets(buffer, LINE_LENGTH, fp) && row < length ) {
		token = strtok(buffer, SEPARATOR);
		col = 0;
		do {
			table[row][col] = atoi(token);
			++col;
		} while ( (col != attrcount) && (token = strtok(NULL, SEPARATOR)) != NULL);
		++row;
	}

	fclose(fp);
}
