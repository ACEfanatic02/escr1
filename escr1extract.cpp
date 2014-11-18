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

// BYTECODE NOTES:

// VM is largely stack based, but also provides globally-scoped variables and flags.
//
// Opcodes are one byte.  Parameters are 4 bytes (one uint).
// The VM declares 33 'reserved' opcodes.  The rest (up to 255) are left open, and the
// client code can declare an opcode by providing a function pointer.
//
// Parameters for client-defined opcodes are pushed to the stack prior to the call.  The 
// VM pops the params off the stack and passes them through a param array (maximum of 32 
// params).
//
// Several reserved opcodes (and, optionally, any user-defined opcode) take an immediate
// param -- i.e., the next 4 byte integer in the code. 

// RESERVED OPCODES
enum {
    ROP_END = 0,
    ROP_JUMP,       // param
    ROP_JUMPZ,      // param
    ROP_CALL,       // param
    ROP_RET,
    ROP_PUSH,       // param
    ROP_POP,
    ROP_STR,        // param
    ROP_SETVAR,
    ROP_GETVAR,
    ROP_SETFLAG,
    ROP_GETFLAG,
    ROP_NEG,
    ROP_ADD,
    ROP_SUB,
    ROP_MUL,
    ROP_DIV,
    ROP_MOD,
    ROP_AND,
    ROP_OR,
    ROP_NOT,
    ROP_SHR,
    ROP_SHL,
    ROP_EQ,
    ROP_NE,
    ROP_GT,
    ROP_GE,
    ROP_LT,
    ROP_LE,
    ROP_LNOT,
    ROP_LAND,
    ROP_LOR,
    ROP_FILELINE,   // param

    ROP_COUNT
};

const char * ROP_NAMES[ROP_COUNT] = {
    "end     ",
    "jump    ",
    "jumpz   ",
    "call    ",
    "ret     ",
    "push    ",
    "pop     ",
    "str     ",
    "setvar  ",
    "getvar  ",
    "setflag ",
    "getflag ",
    "neg     ",
    "add     ",
    "sub     ",
    "mul     ",
    "div     ",
    "mod     ",
    "and     ",
    "or      ",
    "not     ",
    "shr     ",
    "shl     ",
    "eq      ",
    "ne      ",
    "gt      ",
    "ge      ",
    "lt      ",
    "le      ",
    "lnot    ",
    "land    ",
    "lor     ",
    "fileline",
};

struct opcode {
    uint offset;
    uint op;
    uint param;
};

struct usr_op {
    const char * name;
    int param_count;
};

// USER DEFINED OPCODES

// SENSUIBU
usr_op USR_OPS[] = {
    { "USR_END      ", 1 },
    { "USR_JUMP     ", 1 },
    { "USR_CALL     ", 1 },
    { "USR_AUTOPLAY ", 1 },
    { "USR_FRAME    ", 1 },
    { "USR_TEXT     ", 2 },
    { "USR_CLEAR    ", 1 },
    { "USR_GAP      ", 2 },
    { "USR_MES      ", 1 },
    { "USR_TLK      ", -1 },
    { "USR_MENU     ", 3 },
    { "USR_SELECT   ", 1 },
    { "USR_LSF_INIT ", 1 },
    { "USR_LSF_SET  ", -1 },
    { "USR_CG       ", -1 },
    { "USR_EM       ", 5 },
    { "USR_CLR      ", 1 },
    { "USR_DISP     ", 3 },
    { "USR_PATH     ", -1 },
    { "USR_TRANS    ", 0 },
    { "USR_BGMPLAY  ", 3 },
    { "USR_BGMSTOP  ", 1 },
    { "USR_BGMVOLUME", 2 },
    { "USR_BGMFX    ", 1 },
    { "USR_AMBPLAY  ", 3 },
    { "USR_AMBSTOP  ", 1 },
    { "USR_AMBVOLUME", 2 },
    { "USR_AMBFX    ", 1 },
    { "USR_SEPLAY   ", 5 },
    { "USR_SESTOP   ", 2 },
    { "USR_SEWAIT   ", 1 },
    { "USR_SEVOLUME ", 3 },
    { "USR_SEFX     ", 1 },
    { "USR_VOCPLAY  ", 4 },
    { "USR_VOCSTOP  ", 2 },
    { "USR_VOCWAIT  ", 1 },
    { "USR_VOCVOLUME", 3 },
    { "USR_VOCFX    ", 1 },
    { "USR_QUAKE    ", 4 },
    { "USR_FLASH    ", 2 },
    { "USR_FILTER   ", 2 },
    { "USR_EFFECT   ", 1 },
    { "USR_SYNC     ", 2 },
    { "USR_WAIT     ", 1 },
    { "USR_MOVIE    ", 1 },
    { "USR_CREDIT   ", 1 },
    { "USR_EVENT    ", 1 },
    { "USR_SCENE    ", 1 },
    { "USR_TITLE    ", 1 },
    { "USR_NOTICE   ", 3 },
    { "USR_SET_PASS ", 2 },
    { "USR_IS_PASS  ", 1 },
    { "USR_AUTO_SAVE", 0 },
    { "USR_PLACE    ", 1 },
    { "USR_OPEN_NAME", 1 },
    { "USR_NAME     ", 2 },
    { "USR_DATE     ", 0 },
    { "USR_HELP     ", -1 },

    { "USR_PLATY_GAME", 1 },
    { "USR_TRAINING", 0 },
    { "USR_SPECIAL_TRAINING", 0 },

    { "USR_SET_GAME", 3 },
    { "USR_WHATDAY", 0 },
    { "USR_SET_UNIT", 4 },
    { "USR_GET_UNIT", 3 },
    { "USR_BTS_RESULT", 0 },
    { "USR_GAME_SETTING", 1 },
    { "USR_WATCH_ENEMY", 1 },
    { "USR_RND_RT", 1 },
};

opcode * opcode_list = NULL;
uint opcode_count;

bool opcode_has_param(uint op) {
    // NOTE:
    // This only covers the reserved opcodes.
    // User-defined opcodes may *also* take an immediate param.  This is a bit of
    // an issue here, because the user-defined opcodes differ from game to game, so
    // there is no canonical list of which user-defined opcodes need to eat a param.
    //
    // This shows up in the listing as, among other things, a call to an offset *well*
    // beyond the end of the code block.
    if (op < ROP_COUNT) {
        return op == ROP_JUMP  ||
               op == ROP_JUMPZ ||
               op == ROP_CALL  ||
               op == ROP_PUSH  ||
               op == ROP_STR   ||
               op == ROP_FILELINE;
    }
    else {
        return USR_OPS[op - ROP_COUNT].param_count < 0;
    }
}

int next_opcode(script_file * file, uint offset, opcode * op) {
    if (offset >= file->code_size) {
        return -1;
    }

    uint bytes_read = 0;
    opcode opc;
    opc.offset = offset;
    opc.op = (uint)file->code_ptr[offset];
    bytes_read++;
    if (opcode_has_param(opc.op)) {
        if (offset + bytes_read >= file->code_size) {
            return -1;
        }

        uchar * param_ptr = file->code_ptr + (offset + bytes_read);
        opc.param = *((uint *)param_ptr);
        bytes_read += sizeof(uint);
    }
    else {
        opc.param = -1;
    }

    *op = opc;

    return bytes_read;
}

const char * opcode_string(uint op) {
    if (op >= ROP_COUNT) {
        uint uop = op - ROP_COUNT;
        return USR_OPS[uop].name;
    }
    return ROP_NAMES[op];
}

void print_opcode(opcode * op) {
    if (opcode_has_param(op->op)) { 
        printf("%08x:\t%-20s\t%08x\n", op->offset, opcode_string(op->op), op->param);
    }
    else {
        printf("%08x:\t%s\n", op->offset, opcode_string(op->op));
    }
} 

void parse_opcodes(script_file * file) {
    uint code_size = file->code_size;
    assert(code_size > 0);

    uint current_offset = 0;
    while (current_offset < code_size) {
        opcode op;
        int bytes_read = next_opcode(file, current_offset, &op);
        if (bytes_read < 0) {
            fprintf(stderr, "Unexpected end of code block.  Size: %d; Current Offset: %d\n", code_size, current_offset);
            break;
        }

        print_opcode(&op);
        current_offset += bytes_read;
    }
}

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

    parse_opcodes(&script);

    return 0;
}