#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

//Reads a little endian int from a byte buffer
int readInt(const char* buffer, int seek) {
    int b = (buffer[seek+3] & 0xff) << 24 | (buffer[seek+2] & 0xff) << 16 | (buffer[seek+1] & 0xff) << 8 | (buffer[seek+0] & 0xff);
    return b;
}

int main(int argc, char* argv[]){
    char usage[] = "Usage: checksum.exe [options] save.bin\n"
                   "Options:\n"
                   "   -s: Silent mode\n"
                   "   -f: Force mode\n";

    int opt;
    int silent = 0;                                                                     //Setting this to 1 disables all printing
    int force = 0;                                                                      //Setting this to 1 disables all validation checks

    while ((opt = getopt(argc, argv, "fs")) != -1) {                                    //Get all the flags from commandline arguments
        switch(opt){
            case 'f':
                force = 1;
                break;
            case 's':
                silent = 1;
                break;
            default:
                printf("%s", usage);
                return 0;
                break;
        }
    }

    if (optind!=argc-1) {
        printf("%s", usage);
        return 0;
    }

    char* filename = argv[optind];                                                      //Set the input filename to the last commandline argument

    FILE * file;
    fopen_s(&file, filename, "rb+");	                                                //Open the file in binary mode for reading and updating(writing)

    if (file == NULL) {                                                                 //Terminate if the file pointer is NULL indicating an error
        if(silent == 0)
            printf("File error: %s\n%s", strerror(errno), usage );
        fclose(file);
        return 0;
    }

    long filesize;
    size_t result;
    fseek(file, 0, SEEK_END);	                                                        //Go to the end of the file
    filesize = ftell(file);		                                                        //Get the file size, by checking what the last byte is

    if (force == 0 && filesize > 1000000) {
        if(silent == 0)
            printf("Error: File is over 1MB, so is most likely not a save game...\n");	//Terminate the program if the file is huge (not a save file)
        fclose(file);
        return 0;
    }

    rewind(file);				                                                        //Seek to the start of the file
    char buffer[filesize];													            //Make a buffer to store the bytes from the file in
    result = fread(buffer, 1, (size_t)filesize, file);									//Add said bytes to the buffer

    if (result != filesize) {                                                           //Terminate if we were unable to read all the bytes for some reason
        if(silent == 0)
            printf("Error: Only %ld bytes could be read\n", ftell(file));
        fclose(file);
        return 0;
    }

    if(silent == 0) {
        printf("All characters read successfully, filesize: %ld\n", filesize);          //Print that the file was read successfully
        printf("Region\t\tRead checksum\tCalculated checksum\tStatus\n");
    }

    int pos = 0x08;                                                                     //Skip past the 8 byte header
    while(pos < filesize){
        int bytecount = readInt(buffer, pos);							                //Read the size of the chunk
        int prevchecksum = readInt(buffer, pos + 0x04);								    //Read the old checksum

        if (force == 0 && (bytecount <= 0x20 || bytecount > 0xFFFF)) {
            if(silent == 0)
                printf("Error: Section %#x is %d bytes\n", pos + 0x08, bytecount);	    //Terminate if the chunk is 0 bytes long, or more than the max value
            fclose(file);
            return 0;
        }

        pos += 0x08;                                                                    //Skip past the 8 byte header
        int prevpos = pos;															    //Store the position of the start of the chunk

        int checksum = 0x8320;													        //Checksum starts out as 0x8320
        while(pos < prevpos + bytecount){							                    //Go through the chunk byte by byte and calculate the checksum
            checksum ^= ((buffer[pos] & 0xff) << 8);                                    //Apparently this is just some sort of CRC-16
            for (int ii = 7; ii >= 0; ii--) {
                checksum = (checksum & 0x8000) ? (checksum << 1) ^ 0x1F45 : (checksum << 1);
            }
            pos++;
        }
        checksum &= 0xffff;												                //The calculated checksum is 4 bytes, but the save only uses the first 2
        if(silent == 0)
            printf("%#x\t\t%#x\t\t%#x\t\t\t", prevpos, prevchecksum, checksum);

        if (checksum != prevchecksum) {
            fseek(file, prevpos - 4, SEEK_SET);								            //Seek to where the checksum is stored
            fwrite(&checksum, 4, 1, file);									            //Write the checksum
            if(silent == 0) printf("Fixed!\n");
        }else {
            if(silent == 0) printf("OK\n");
        }
    }
    fclose(file);
    if(silent == 0) printf("Checksums recalculated, enjoy :)\n");
    return 0;
}