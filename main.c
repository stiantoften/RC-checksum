#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

// Read a little endian unsigned int from a byte buffer
uint32_t readInt(const uint8_t *buffer, const uint32_t seek) {
    return ((buffer[seek + 3u] & 0xFFu) << 24u
            | (buffer[seek + 2u] & 0xFFu) << 16u
            | (buffer[seek + 1u] & 0xFFu) << 8u
            | (buffer[seek] & 0xFFu));
}

int main(int argc, char *argv[]) {
    char usage[] = "Usage: checksum.exe [options] save.bin\n"
                   "Options:\n"
                   "   -s: Silent mode\n"
                   "   -f: Force mode\n";

    // Silent disables all printing, force disables all checks
    int opt, silent = 0, force = 0;

    while ((opt = getopt(argc, argv, "fs")) != -1) {
        switch (opt) {
            case 'f':
                force = 1;
                break;
            case 's':
                silent = 1;
                break;
            default:
                printf("%s", usage);
                exit(1);
        }
    }

    if (optind != argc - 1) {
        printf("%s", usage);
        exit(1);
    }

    char *filename = argv[optind];
    FILE *file = fopen(filename, "rb+");

    if (file == NULL) {
        if (silent == 0)
            printf("File error: %s\n%s", strerror(errno), usage);
        fclose(file);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);

    if (force == 0 && filesize > 1000000) {
        if (silent == 0)
            printf("Error: File is over 1MB, so is most likely not a save game...\n");
        fclose(file);
        exit(1);
    }

    rewind(file);
    uint8_t buffer[filesize];
    size_t result = fread(buffer, 1, (size_t) filesize, file);

    if (result != filesize) {
        if (silent == 0)
            printf("Error: Only %ld bytes could be read\n", ftell(file));
        fclose(file);
        exit(1);
    }

    if (silent == 0) {
        printf("File read successfully, filesize: %ld\n", filesize);
        printf("Region\t\tRead checksum\tCalculated checksum\tStatus\n");
    }

    // Skip past the 8 byte file header
    uint32_t pos = 0x08;
    while (pos < filesize) {
        uint32_t bytecount = readInt(buffer, pos);
        uint32_t prevchecksum = readInt(buffer, pos + 4);
        pos += 0x08;
        uint32_t prevpos = pos;

        // Exit if the section length seems incorrect
        if (force == 0 && (bytecount <= 0x20 || bytecount > 0xFFFF)) {
            if (silent == 0)
                printf("Error: Section %#x is %d bytes\n", pos + 0x08, bytecount);
            fclose(file);
            exit(1);
        }

        // Calculate checksum of the section
        uint32_t checksum = 0x8320;
        while (pos < prevpos + bytecount) {
            checksum ^= ((buffer[pos++] & 0xFFu) << 8u);
            for (int i = 0; i < 8; i++) {
                checksum = checksum & 0x8000u ? (checksum << 1u) ^ 0x1F45u : checksum << 1u;
            }
        }

        // The calculated checksum is 4 bytes, but the save only uses the lower 2
        checksum &= 0xFFFFu;

        if (silent == 0) printf("%#x\t\t%#x\t\t%#x\t\t\t", prevpos, prevchecksum, checksum);

        // Write the new checksum
        if (checksum != prevchecksum) {
            fseek(file, prevpos - 4, SEEK_SET);
            fwrite(&checksum, 4, 1, file);
            if (silent == 0) printf("Fixed!\n");
        } else {
            if (silent == 0) printf("OK\n");
        }
    }

    fclose(file);
    if (silent == 0) printf("Checksums recalculated, enjoy :)\n");
    return 0;
}