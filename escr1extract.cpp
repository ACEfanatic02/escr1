#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>

typedef unsigned char uchar;
typedef uint32_t uint;


//  FORMAT NOTES:
//
// File magic is 'ESCR1_00' (0x45 0x53 0x43 0x52 0x31 0x5f 0x30 0x30).
//
// Multibyte integers are stored in little endian order.
//
// Files are split into three sections:
// - An index table
//   - Offsets into data section to the start of string literals.
// - Bytecode
// - Data (null-terminated Shift-JIS(?) encoded strings).

// Compile with:
// $ g++ ./escr1extract.cpp -o escr1extract.exe -std=c++0x

// BYTECODE NOTES:

// VM is largely stack based, but also provides globally-scoped variables and flags.
//
// Opcodes are one byte.  Parameters are 4 bytes (one uint).
// The VM declares 32 'reserved' opcodes.  The rest (up to 255) are left open, and the
// client code can declare an opcode by providing a function pointer.
//
// Parameters for client-defined opcodes are pushed to the stack prior to the call.  The 
// VM pops the params off the stack and passes them through a param array (maximum of 32 
// params).

const uchar magic[9] = "ESCR1_00";

struct script_file {
    uchar * contents;   // Raw contents of file.  All other pointers point into this buffer.
    uint file_size;     // Size of entire file.
    uint * index_ptr;   // Pointer to indices
    uint index_count;   // Number of indices
    uchar * code_ptr;   // Pointer to bytecode
    uint code_size;     // Size of code block
    uchar * data_ptr;   // Pointer to data
    uint data_size;     // Size of data block
};

int load_file(char * filename, uchar ** data) {
    FILE * fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: [%s]\n", filename);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int bytes = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uchar * contents = (uchar *)calloc(1, bytes);
    int read = fread(contents, 1, bytes, fp);
    if (read < bytes) {
        if (ferror(fp)) {
            fprintf(stderr, "Error while reading file [%s]\n", filename);
            exit(1);
        }
        else if (feof(fp)) {
#ifdef _DEBUG
            fprintf(stderr, "Expected to read %d bytes, read %d\n", bytes, read);
#endif 
        }
    }

    fclose(fp);
    *data = contents;

    // Sanity check.  Overflow should be unlikely with reasonable files.
    assert(read >= 0);
    return read;
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <FILE>\n", argv[0]);
        exit(1);
    }

    script_file script;
    uchar * data;
    int flen = load_file(argv[1], &data);

    script.contents = data;
    script.file_size = flen;

    if (memcmp(magic, data, 8)) {
        fprintf(stderr, "This is not an ESCR1_00 file.\n");
        exit(1);
    }

    uchar * p = data + 8;
    script.index_count = *((uint *)p);

    p += sizeof(uint);
    script.index_ptr = (uint *)p;

    p += script.index_count * sizeof(uint);
    script.code_size = *((uint *)p);

    p += sizeof(uint);
    script.code_ptr = p;

    p += script.code_size;
    script.data_size = *((uint *)p);

    p += sizeof(uint);
    script.data_ptr = p; 

    return 0;
}