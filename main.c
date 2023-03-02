#include <string.h>
#include <concord/discord.h>
#include <concord/log.h>
#include <stdlib.h>
#include <png.h>
#include <math.h>
#include <time.h>

#define HEIGHT 1080
#define WIDTH 920

struct Point {
    int x;
    int y;
};

void on_ready(struct discord *client, const struct discord_ready *event) {
    log_info("Logged on as %s!", event->user->username);
}

int **createArray(int m, int n) {
    int *values = calloc(m * n, sizeof(float));
    int **rows = malloc(m * sizeof(float *));
    for (int i = 0; i < m; ++i) {
        rows[i] = values + i * n;
    }
    return rows;
}

int **randimg(int width, int height) {
    srand(time(NULL));
    int **arr = createArray(height, width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            arr[i][j] = rand() % 256;
        }
    }
    return arr;
}

void writeImage(const char *filename, int **data) {
    // Open the output file for writing
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open output file\n");
        return;
    }

    // Initialize the PNG write structure and info structure
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Error: Could not initialize PNG write structure\n");
        fclose(fp);
        return;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Error: Could not initialize PNG info structure\n");
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        return;
    }

    // Set up error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: PNG write failed\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    // Set up the PNG write info
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Write the PNG header
    png_write_info(png_ptr, info_ptr);

    // Write the PNG image data
    png_bytep row_pointers[HEIGHT];
    for (int y = 0; y < HEIGHT; y++) {
        row_pointers[y] = (png_bytep)malloc(WIDTH * 3 * sizeof(png_byte)); // Allocate space for three color components per pixel
        for (int x = 0; x < WIDTH; x++) {
            // Calculate the index of the pixel in the png_bytep array
            int pixel_index = x * 3;

            // Set the red, green, and blue components of the pixel based on the grayscale value
            row_pointers[y][pixel_index] = (data[x][y] == 1) ? 255 : 0; // red component
            row_pointers[y][pixel_index + 1] = (data[x][y] == 1) ? 255 : 0; // green component
            row_pointers[y][pixel_index + 2] = (data[x][y] == 1) ? 0 : 0; // blue component
        }
    }
    png_write_image(png_ptr, row_pointers);

    // Finish writing the PNG file
    png_write_end(png_ptr, NULL);

    // Clean up
    for (int y = 0; y < HEIGHT; y++) {
        free(row_pointers[y]);
    }
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

void on_message(struct discord *client, const struct discord_message *event) {
    srand(time(NULL));
    setbuf(stdout, NULL);
    if (strcmp(event->content, ">>test") == 0) {
        int **arr = randimg(920, 1080);
        int **toWrite = createArray(920, 1080);

        int startingPoint = 0;
        for (int col = 0; col <= 920; col++) {
            if (arr[0][col] < arr[0][startingPoint]) {
                startingPoint = col;
            }
        }
        toWrite[startingPoint][0] = 1;

        int row = 0;
        int col = startingPoint;
        int oldRow = 0;
        int oldCol = 0;
        while (row < 1079) {
            struct Point neighbor1 = { .x = row, .y = col - 1 };
            struct Point neighbor2 = { .x = row, .y = col + 1 };
            struct Point neighbor3 = { .x = row + 1, .y = col - 1 };
            struct Point neighbor4 = { .x = row + 1, .y = col };
            struct Point neighbor5 = { .x = row + 1, .y = col + 1 };
            struct Point *neighbors[] = {
                &neighbor1,
                &neighbor2,
                &neighbor3,
                &neighbor4,
                &neighbor5
            };

            struct Point *minPoint = NULL;
            for (int i = 0; i < 5; i++) {
                struct Point* neighbor = neighbors[i];
                if (!(neighbor->x == oldRow && neighbor->y == oldCol) &&
                    neighbor->x >= 0 && neighbor->x < 1080 &&
                    neighbor->y >= 0 && neighbor->y < 920) {
                    if (minPoint == NULL || arr[neighbor->x][neighbor->y] < arr[minPoint->x][minPoint->y]) {
                        printf("minPoint assigned to neighbor (%d, %d)\n", neighbor->x, neighbor->y);
                        minPoint = neighbor;
                    }
                }
            }

            if (minPoint == NULL) {
                break;
            }

            oldRow = row;
            oldCol = col;
            row = minPoint->x;
            col = minPoint->y;
            toWrite[col][row] = 1;
        }

        writeImage("image.png", toWrite);

        struct discord_attachment attachment = { .content_type = "image/png", .filename = "image.png", .size = 0 };
        struct discord_attachments attachments = { .array = &attachment, .size = 1 };
        struct discord_create_message params = { .attachments = &attachments };
        discord_create_message(client, event->channel_id, &params, NULL);
    }
}

int main(int argc, char *argv[]) {
    struct discord *client = discord_init(argv[1]);
    discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);
    discord_set_on_ready(client, &on_ready);
    discord_set_on_message_create(client, &on_message);
    discord_run(client);
}
