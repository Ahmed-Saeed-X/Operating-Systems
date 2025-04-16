#include <stdio.h>   // Standard I/O functions
#include <stdlib.h>  // Memory management
#include <string.h>  // String management
#include <pthread.h> // Thread management


// Maximum matrix dimensions (not larger than 20x20)
#define MAX_DIM 20

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

    Matrix *mat = malloc(sizeof(Matrix)); //Dynamically allocates memory for the Matrix struct
    if (!mat) 
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //Set the matrix dimensions using the input arguments
    mat->rows = rows;
    mat->cols = cols;
    mat->data = malloc(rows * sizeof(int *)); //Create an array of pointers, where each pointer will later point to a row of integers
    if (!mat->data) 
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) {
        mat->data[i] = calloc(cols, sizeof(int)); //allocate memory for a row of cols integers and initializes them to 0
        if (!mat->data[i]) 
        {
            perror("calloc");
            exit(EXIT_FAILURE);
        }
    }
    return mat;
}

// Free a matrix created by create_matrix()
void free_matrix(Matrix *mat) {

    if (mat)  // Check if matrix exists
    {
        // Free each row's data array
        for (int i = 0; i < mat->rows; i++) 
        {
            free(mat->data[i]);  // Release memory for individual rows
        }
        free(mat->data);    // Free the array of row pointers
        free(mat);          // Free the matrix structure itself
    }
}

// Reads a matrix from a text file
Matrix *read_matrix_from_file(const char *filename) {

    // open the input file
    FILE *fp = fopen(filename, "r");
    if (!fp) 
    {
        perror(filename);  // Print system error message
        return NULL;
    }

    // Read the file
    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) 
    {
        fclose(fp);  // Close file before returning
        return NULL; // File empty or read error
    }

    // Parse dimensions from header line
    int rows, cols;
    if (sscanf(buffer, "row=%d col=%d", &rows, &cols) != 2)
    {
        fprintf(stderr, "Error: invalid header format in %s\n", filename);
        fclose(fp);
        return NULL;
    }

    // Validate matrix dimensions against MAX_DIM constraint
    if (rows > MAX_DIM || cols > MAX_DIM) 
    {
        fprintf(stderr, "Error: matrix dimensions exceed maximum allowed (%d)\n", MAX_DIM);
        fclose(fp);
        return NULL;
    }

    // Allocate matrix structure
    Matrix *mat = create_matrix(rows, cols);

    // Read matrix elements row by row
    for (int i = 0; i < rows; i++) 
    {
        for (int j = 0; j < cols; j++) 
        {
            // Read single integer element
            if (fscanf(fp, "%d", &(mat->data[i][j])) != 1) 
            {
                fprintf(stderr, "Error reading matrix element (%d, %d) from %s\n", i, j, filename);
                free_matrix(mat);  // Cleanup partially read matrix
                fclose(fp);
                return NULL;
            }
        }
    }

    fclose(fp);  // Close file after successful read
    return mat;  // Return fully initialized matrix
}

// Writes a matrix to a text file in the specified format
void write_matrix_to_file(const char *filename, Matrix *mat) {

    // open the output file in write mode
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        perror(filename);  // Display system error message
        return;            // Abort operation if file can't be opened
    }

    // Write matrix dimensions header line
    fprintf(fp, "row=%d col=%d\n", mat->rows, mat->cols);

    // Write matrix elements row by row
    for (int i = 0; i < mat->rows; i++) 
    {
        // Write all columns in current row
        for (int j = 0; j < mat->cols; j++) 
        {
            fprintf(fp, "%d ", mat->data[i][j]);  // Space-separated values
        }
        fprintf(fp, "\n");  // Newline after each row
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
    int row; // row index to compute
} RowMultArgs;

// For method 3 (thread per element)
typedef struct {
    Matrix *A;
    Matrix *B;
    Matrix *C;
    int row; // row index
    int col; // column index
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

    
    for (int i = 0; i < A->rows; i++) // Iterate over all rows of matrix A
    {
        
        for (int j = 0; j < B->cols; j++) // Iterate over all columns of matrix B
        {
            int sum = 0;
            for (int k = 0; k < A->cols; k++) 
            {
                sum += A->data[i][k] * B->data[k][j]; // Compute the product of A's row and B's column
            }
            C->data[i][j] = sum;  // Store computed value
        }
    }
    free(args);  // Release argument memory
    return NULL;
}

// Method 2: thread per row.
void *multiply_row(void *arg) {

    RowMultArgs *args = (RowMultArgs *)arg;
    Matrix *A = args->A;
    Matrix *B = args->B;
    Matrix *C = args->C;
    const int i = args->row;  // Target row index

    // Compute all columns in assigned row
    for (int j = 0; j < B->cols; j++) // Iterate all columns in assigned row
    {
        int sum = 0;
        for (int k = 0; k < A->cols; k++) 
        {
            sum += A->data[i][k] * B->data[k][j]; // Dot product for single element
        }
        C->data[i][j] = sum;
    }
    free(args);  // Release argument memory
    return NULL;
}

// Method 3: One thread per element.
void *multiply_element(void *arg) {

    ElementMultArgs *args = (ElementMultArgs *)arg;
    Matrix *A = args->A;
    Matrix *B = args->B;
    Matrix *C = args->C;
    const int i = args->row;  // Target row
    const int j = args->col;  // Target column

    int sum = 0;
    
    for (int k = 0; k < A->cols; k++) 
    {
        sum += A->data[i][k] * B->data[k][j]; // Compute dot product for single element
    }

    C->data[i][j] = sum;  // Store result
    free(args);           // Compute dot product for single element
    return NULL;
}

//------------------------------
// Main Program
//------------------------------
int main(int argc, char *argv[]) //arguments count and array  stores it 
{
    // Determine input/output file names based on command-line arguments.

    char inA_filename[256], inB_filename[256], out_prefix[256];
    if (argc < 4)   // Default: input files "a.txt" and "b.txt", output prefix "c"
     { 
        strcpy(inA_filename, "a.txt");
        strcpy(inB_filename, "b.txt");
        strcpy(out_prefix, "c");
    } 
    else 
    {   // construct filenames from user input.
        snprintf(inA_filename, sizeof(inA_filename), "%s.txt", argv[1]);
        snprintf(inB_filename, sizeof(inB_filename), "%s.txt", argv[2]);
        strncpy(out_prefix, argv[3], sizeof(out_prefix) - 1);
        out_prefix[sizeof(out_prefix) - 1] = '\0';
    }

    // Read matrices A and B from their corresponding files.
    Matrix *A = read_matrix_from_file(inA_filename);
    Matrix *B = read_matrix_from_file(inB_filename);
    if (!A || !B) 
    {
        fprintf(stderr, "Error reading input matrices.\n");
        exit(EXIT_FAILURE);
    }

    // Verify dimensions: A's cols must equal B's rows.
    if (A->cols != B->rows) 
    {
        fprintf(stderr, "Error: Incompatible matrix dimensions for multiplication.\n");
        free_matrix(A);
        free_matrix(B);
        exit(EXIT_FAILURE);
    }

    int rows = A->rows;
    int cols = B->cols;

    // Allocate result matrices for each multiplication method.
    Matrix *C_matrix  = create_matrix(rows, cols);  // For method 1
    Matrix *C_row     = create_matrix(rows, cols);  // For method 2
    Matrix *C_element = create_matrix(rows, cols);  // For method 3

    // ------------------------------
    // Method 1: One thread per entire matrix.
    // ------------------------------
    pthread_t thread_matrix;  //Creates a thread identifier
    MatMultArgs *args_matrix = malloc(sizeof(MatMultArgs)); //Dynamically allocates memory for a MatMultArgs struct 
    if (!args_matrix) {perror("malloc"); exit(EXIT_FAILURE);} 
    
    //Initialize thread arguments
    args_matrix->A = A;
    args_matrix->B = B;
    args_matrix->C = C_matrix;

    if (pthread_create(&thread_matrix, NULL, multiply_matrix, args_matrix) != 0) //spawns a new thread that executes multiply_matrix.
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // ------------------------------
    // Method 2: One thread per row.
    // ------------------------------
    pthread_t threads_row[rows]; //Creates threads identifiers
    for (int i = 0; i < rows; i++) 
    {
        RowMultArgs *args_row = malloc(sizeof(RowMultArgs)); //Dynamically allocates memory for a RowMultArgs struct
        if (!args_row) {perror("malloc"); exit(EXIT_FAILURE);}   
        
        //Initialize thread arguments
        args_row->A = A;
        args_row->B = B;
        args_row->C = C_row;
        args_row->row = i;

        if (pthread_create(&threads_row[i], NULL, multiply_row, args_row) != 0) //spawns a new thread that executes multiply_row.
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // ------------------------------
    // Method 3: One thread per element.
    // ------------------------------
    int total_elements = rows * cols; //The total elements in the result matrix

    pthread_t threads_element[total_elements]; //Creates threads identifiers

    int index = 0; //Index to track threads in the array

    for (int i = 0; i < rows; i++) 
    {
        for (int j = 0; j < cols; j++) 
        {
            ElementMultArgs *args_elem = malloc(sizeof(ElementMultArgs)); //Dynamically allocates memory for a RowMultArgs struct
            if (!args_elem) { perror("malloc"); exit(EXIT_FAILURE); }

            //Initialize thread arguments
            args_elem->A = A;
            args_elem->B = B;
            args_elem->C = C_element;
            args_elem->row = i;
            args_elem->col = j;
            if (pthread_create(&threads_element[index], NULL, multiply_element, args_elem) != 0) //spawns a new thread that executes multiply_element.
            {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            index++;
        }
    }

    // Join all threads AFTER all have been created.
    pthread_join(thread_matrix, NULL);
    for (int i = 0; i < rows; i++) 
    {
        pthread_join(threads_row[i], NULL);
    }

    for (int i = 0; i < total_elements; i++) 
    {
        pthread_join(threads_element[i], NULL);
    }

    // ------------------------------
    // Write result matrices to output files.
    // ------------------------------
    char filename[256];

    // Output for method 1
    snprintf(filename, sizeof(filename), "%s_per_matrix.txt", out_prefix);
    write_matrix_to_file(filename, C_matrix);

    // Output for method 2
    snprintf(filename, sizeof(filename), "%s_per_row.txt", out_prefix);
    write_matrix_to_file(filename, C_row);

    // Output for method 3
    snprintf(filename, sizeof(filename), "%s_per_element.txt", out_prefix);
    write_matrix_to_file(filename, C_element);

    // Free all allocated matrices.
    free_matrix(A);
    free_matrix(B);
    free_matrix(C_matrix);
    free_matrix(C_row);
    free_matrix(C_element);

    return 0;
}
