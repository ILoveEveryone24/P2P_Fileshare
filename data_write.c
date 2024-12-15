#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 510

int main(){
    FILE *file = fopen("original_data.txt", "rb");

    if(file == NULL){
        printf("Failed to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, 0, SEEK_SET);

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
    int total_chunks = file_length / (file_length / CHUNK_SIZE + additional_byte);

    if(total_chunks > 255){
        chunk[2] = 255;
        chunk[3] = total_chunks - 255;
    }

    size_t bytes_r;
    
    unsigned char index = 0;
    char second_byte = 0;

    FILE *file2 = fopen("chunked_data.txt", "w");
    if(file2 == NULL){
        printf("Failed to open file");
        fclose(file);
        free(chunk);
        return -1;
    }

    while((bytes_r = fread(chunk_offset, sizeof(char), SIZE_OF_CHUNK - chunk_index_offset, file)) > 0){

        if(total_chunks > 255){
            chunk[2] = 255;
            chunk[3] = total_chunks - 255;
        }
        if(second_byte){
            chunk[0] = 255;
            chunk[1] = index;
        }
        else{
            chunk[0] = index;
        }

        fwrite(chunk, sizeof(unsigned char), SIZE_OF_CHUNK, file2);
        printf("chunk 0: %d %s\n", chunk[0] + chunk[1], chunk_offset);

        if(index == 255){
            index++;
            second_byte = 1;
        }

        index++;

        memset(chunk, 0, SIZE_OF_CHUNK);
    }

    fclose(file);
    fclose(file2);
    free(chunk);

    return 0;
}
