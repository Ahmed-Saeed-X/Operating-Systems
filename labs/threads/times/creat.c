#include <stdio.h>

#define ROWS 100
#define COLS 100

int main() {
    FILE *file = fopen("matrix_100x100.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Write header
    fprintf(file, "row=%d col=%d\n", ROWS, COLS);

    // Write matrix values
    for (int i = 1; i <= ROWS; i++) {
        for (int j = 1; j <= COLS; j++) {
            fprintf(file, "%d ", (i - 1) * COLS + j);  // Filling the matrix with numbers incrementally
        }
        fprintf(file, "\n");
    }

    fclose(file);
    printf("Matrix written to matrix_100x100.txt\n");
    return 0;
}
