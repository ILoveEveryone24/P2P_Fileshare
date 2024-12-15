#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 510

int main(){
    FILE *file = fopen("chunked_data.txt", "rb");
    if(file == NULL){
        printf("Failed to open file");
        return -1;
    }

    long file_length = 1029;

    int chunk_index_offset = 4;
    char additional_byte = 0;
    if(file_length > (file_length / CHUNK_SIZE * CHUNK_SIZE)){
        additional_byte = 1;
    }
    const long SIZE_OF_CHUNK = file_length / CHUNK_SIZE + chunk_index_offset + additional_byte;

    unsigned char *chunk = malloc(SIZE_OF_CHUNK);
    if(chunk == NULL){
        printf("Failed to allocate memory");
        fclose(file);
        return -1;
    }

    unsigned char *chunk_offset = chunk + chunk_index_offset;

    FILE *file2 = fopen("unchunked_data.txt", "wb");
    if(file2 == NULL){
        printf("Failed to open file");
        fclose(file);
        free(chunk);
        return -1;
    }

    ssize_t bytes_r;
    while((bytes_r = fread(chunk, sizeof(char), SIZE_OF_CHUNK, file)) > 0){
        fwrite(chunk_offset, sizeof(unsigned char), SIZE_OF_CHUNK - chunk_index_offset, file2);

        memset(chunk, 0, SIZE_OF_CHUNK);
    }
    
    fclose(file);
    free(chunk);
    return 0;
}
