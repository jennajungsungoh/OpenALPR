
#include <stdio.h>
#include <stdlib.h>
#include "alpr_c.h"

long read_file(const char* file_path, unsigned char** buffer)
{
    FILE *fileptr;
    long filelen;

    fileptr = fopen(file_path, "rb");     // Open the file in binary mode
    if (!fileptr)
        return 0;
    
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    *buffer = (unsigned char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
    fread(*buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file

    return filelen;
}

int main(int argc, char *argv[])
{
    OPENALPR* alpr_obj;

    if (argc != 2)
    {
        printf("Usage: %s [path to image file]\n", argv[0]);
        return 1;
    }

    const char* file_path = argv[1];

    // Leaving the config and runtime directory blank to look for these in the current directory
    const int USE_GPU = 1;
    const int GPU_ID = 0;
    const int BATCH_SIZE = 10;
    alpr_obj = openalpr_init_gpu("us", "", "", "", USE_GPU, GPU_ID, BATCH_SIZE);


    if (openalpr_is_loaded(alpr_obj))
    {
        // We don't want to restrict the size of the recognition area, so we set this to an extremely large pixel size
        // rather than decode and find the actual image width/height
        struct AlprCRegionOfInterest roi;
        roi.x = 0;
        roi.y = 0;
        roi.width = 10000;
        roi.height = 10000;

        // Read the image file
        unsigned char* buffer;
        long long length = read_file(file_path, &buffer);

        printf("file size (bytes): %d\n", length);

        if (length > 0)
        {
            // Add the same image 10 times to the batch.  In actual usage you would use different 
            // images, but you must make sure that all images in the batch are the same size
            OPENALPR_IMAGE_BATCH* batch = openalpr_create_image_batch();
            for (int i = 0; i < 10; i++)
                openalpr_add_encoded_image_to_batch(batch, buffer, length, roi);

            char* plate_response = openalpr_recognize_batch(alpr_obj, batch);

            printf("Alpr Batch response:\n%s\n", plate_response);

            openalpr_release_image_batch(batch);
            openalpr_free_response_string(plate_response);
        }

        free(buffer);


    }

    openalpr_cleanup(alpr_obj);


    return 0;
}
