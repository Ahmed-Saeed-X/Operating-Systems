#include <stdio.h>   // Standard I/O functions
#include <stdlib.h>  // Memory management
#include <string.h>  // String management
#include <pthread.h> // Thread management
#include <sys/time.h>    // For clock_gettime()

// Maximum matrix dimensions (not larger than 20x20)
#define MAX_DIM 105

//------------------------------
// Matrix Structure and Functions
//------------------------------
typedef struct {
    int rows;          // Number of rows
    int cols;          // Number of columns
    int **data;        // 2D array to store matrix elements
} Matrix;

// Allocate a new matrix with given dimensions; memory is zero‐initialized
Matrix *create_matrix(int rows, int cols) {
    Matrix *mat = malloc(sizeof(Matrix)); // Dynamically allocates memory for the Matrix struct
    if (!mat) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Set the matrix dimensions using the input arguments
    mat->rows = rows;
    mat->cols = cols;
    mat->data = malloc(rows * sizeof(int *)); // Create an array of pointers, where each pointer will later point to a row of integers
    if (!mat->data) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) {
        mat->data[i] = calloc(cols, sizeof(int)); // Allocate memory for a row of cols integers and initializes them to 0
        if (!mat->data[i]) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }
    }
    return mat;
}

// Free a matrix created by create_matrix()
void free_matrix(Matrix *mat) {
    if (mat) { // Check if matrix exists
        // Free each row's data array
        for (int i = 0; i < mat->rows; i++) {
            free(mat->data[i]);  // Release memory for individual rows
        }
        free(mat->data);    // Free the array of row pointers
        free(mat);          // Free the matrix structure itself
    }
}

// Reads a matrix from a text file
Matrix *read_matrix_from_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);  // Print system error message
        return NULL;
    }

    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);  // Close file before returning
        return NULL; // File empty or read error
    }

    int rows, cols;
    if (sscanf(buffer, "row=%d col=%d", &rows, &cols) != 2) {
        fprintf(stderr, "Error: invalid header format in %s\n", filename);
        fclose(fp);
        return NULL;
    }

    if (rows > MAX_DIM || cols > MAX_DIM) {
        fprintf(stderr, "Error: matrix dimensions exceed maximum allowed (%d)\n", MAX_DIM);
        fclose(fp);
        return NULL;
    }

    Matrix *mat = create_matrix(rows, cols);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(fp, "%d", &(mat->data[i][j])) != 1) {
                fprintf(stderr, "Error reading matrix element (%d, %d) from %s\n", i, j, filename);
                free_matrix(mat);
                fclose(fp);
                return NULL;
            }
        }
    }

    fclose(fp);
    return mat;
}

// Writes a matrix to a text file in the specified format
void write_matrix_to_file(const char *filename, Matrix *mat) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror(filename);  // Display system error message
        return;            // Abort operation if file can't be opened
    }

    fprintf(fp, "row=%d col=%d\n", mat->rows, mat->cols);

    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
            fprintf(fp, "%d ", mat->data[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp); // Close the file stream
}

//------------------------------
// Structures for Thread Arguments
//------------------------------

// For method 1 (thread per matrix)
typedef struct {
    Matrix *A;
    Matrix *B;
    Matrix *C;
} MatMultArgs;

// For method 2 (thread per row)
typedef struct {
    Matrix *A;
    Matrix *B;
    Matrix *C;
    int row;
} RowMultArgs;

// For method 3 (thread per element)
typedef struct {
    Matrix *A;
    Matrix *B;
    Matrix *C;
    int row;
    int col;
} ElementMultArgs;

//------------------------------
// Thread Worker Functions
//------------------------------

// Method 1: One thread computes the entire matrix multiplication.
void *multiply_matrix(void *arg) {
    MatMultArgs *args = (MatMultArgs *)arg;
    Matrix *A = args->A;
    Matrix *B = args->B;
    Matrix *C = args->C;

    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            int sum = 0;
            for (int k = 0; k < A->cols; k++) {
                sum += A->data[i][k] * B->data[k][j];
            }
            C->data[i][j] = sum;
        }
    }
    free(args);
    return NULL;
}

// Method 2: thread per row.
void *multiply_row(void *arg) {
    RowMultArgs *args = (RowMultArgs *)arg;
    Matrix *A = args->A;
    Matrix *B = args->B;
    Matrix *C = args->C;
    const int i = args->row;

    for (int j = 0; j < B->cols; j++) {
        int sum = 0;
        for (int k = 0; k < A->cols; k++) {
            sum += A->data[i][k] * B->data[k][j];
        }
        C->data[i][j] = sum;
    }
    free(args);
    return NULL;
}

// Method 3: One thread per element.
void *multiply_element(void *arg) {
    ElementMultArgs *args = (ElementMultArgs *)arg;
    Matrix *A = args->A;
    Matrix *B = args->B;
    Matrix *C = args->C;
    const int i = args->row;
    const int j = args->col;

    int sum = 0;
    for (int k = 0; k < A->cols; k++) {
        sum += A->data[i][k] * B->data[k][j];
    }

    C->data[i][j] = sum;
    free(args);
    return NULL;
}

//------------------------------
// Main Program
//------------------------------
int main(int argc, char *argv[]) {
    char inA_filename[256], inB_filename[256], out_prefix[256];
    if (argc < 4) {
        strcpy(inA_filename, "a.txt");
        strcpy(inB_filename, "b.txt");
        strcpy(out_prefix, "c");
    } else {
        snprintf(inA_filename, sizeof(inA_filename), "%s.txt", argv[1]);
        snprintf(inB_filename, sizeof(inB_filename), "%s.txt", argv[2]);
        strncpy(out_prefix, argv[3], sizeof(out_prefix) - 1);
        out_prefix[sizeof(out_prefix) - 1] = '\0';
    }

    Matrix *A = read_matrix_from_file(inA_filename);
    Matrix *B = read_matrix_from_file(inB_filename);
    if (!A || !B) {
        fprintf(stderr, "Error reading input matrices.\n");
        exit(EXIT_FAILURE);
    }

    if (A->cols != B->rows) {
        fprintf(stderr, "Error: Incompatible matrix dimensions for multiplication.\n");
        free_matrix(A);
        free_matrix(B);
        exit(EXIT_FAILURE);
    }

    int rows = A->rows;
    int cols = B->cols;

    Matrix *C_matrix = create_matrix(rows, cols);  // For method 1
    Matrix *C_row = create_matrix(rows, cols);     // For method 2
    Matrix *C_element = create_matrix(rows, cols); // For method 3

    struct timeval start, end;
    double duration;

    // ------------------------------
    // Method 1: One thread per entire matrix.
    // ------------------------------
    gettimeofday(&start, NULL);

    pthread_t thread_matrix;
    MatMultArgs *args_matrix = malloc(sizeof(MatMultArgs));
    if (!args_matrix) { perror("malloc"); exit(EXIT_FAILURE); }
    args_matrix->A = A;
    args_matrix->B = B;
    args_matrix->C = C_matrix;

    pthread_create(&thread_matrix, NULL, multiply_matrix, args_matrix);
    pthread_join(thread_matrix, NULL);

    gettimeofday(&end, NULL);
    duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Method 1 (one thread total) took %.6f seconds.\n", duration);

    // ------------------------------
    // Method 2: One thread per row.
    // ------------------------------
    gettimeofday(&start, NULL);

    pthread_t threads_row[rows];
    for (int i = 0; i < rows; i++) {
        RowMultArgs *args_row = malloc(sizeof(RowMultArgs));
        if (!args_row) { perror("malloc"); exit(EXIT_FAILURE); }
        args_row->A = A;
        args_row->B = B;
        args_row->C = C_row;
        args_row->row = i;
        pthread_create(&threads_row[i], NULL, multiply_row, args_row);
    }

    for (int i = 0; i < rows; i++) {
        pthread_join(threads_row[i], NULL);
    }

    gettimeofday(&end, NULL);
    duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Method 2 (one thread per row) took %.6f seconds.\n", duration);

    // ------------------------------
    // Method 3: One thread per element.
    // ------------------------------
    gettimeofday(&start, NULL);

    pthread_t threads_element[rows * cols];
    int index = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            ElementMultArgs *args_elem = malloc(sizeof(ElementMultArgs));
            if (!args_elem) { perror("malloc"); exit(EXIT_FAILURE); }
            args_elem->A = A;
            args_elem->B = B;
            args_elem->C = C_element;
            args_elem->row = i;
            args_elem->col = j;
            pthread_create(&threads_element[index], NULL, multiply_element, args_elem);
            index++;
        }
    }

    for (int i = 0; i < rows * cols; i++) {
        pthread_join(threads_element[i], NULL);
    }

    gettimeofday(&end, NULL);
    duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Method 3 (one thread per element) took %.6f seconds.\n", duration);
    
    // Write the results to files
    write_matrix_to_file("C_matrix.txt", C_matrix);
    write_matrix_to_file("C_row.txt", C_row);
    write_matrix_to_file("C_element.txt", C_element);

    // Free allocated memory
    free_matrix(A);
    free_matrix(B);
    free_matrix(C_matrix);
    free_matrix(C_row);
    free_matrix(C_element);

    return 0;
}
