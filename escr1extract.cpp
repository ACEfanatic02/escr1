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
// UPDATE (2014-11-21):  Immediate param for user-defined opcodes is *not* a parameter for
// the function call, but rather a parameter *count* for opcodes that can accept a variable
// argument count.  (This doesn't effect parsing, but is important for execution.)

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
    { "USR_END      ", 1 },     // Ends current script, ???saves game state???
    { "USR_JUMP     ", 1 },     // Jump into a different script
    { "USR_CALL     ", 1 },     // Call into a different script
    { "USR_AUTOPLAY ", 1 },     // Enable/disable auto mode
    { "USR_FRAME    ", 1 },     // Update text frame (?what does this actually mean?)
    { "USR_TEXT     ", 2 },     // Show or hide text frame, with optional time
    { "USR_CLEAR    ", 1 },     // Clear message window
    { "USR_GAP      ", 2 },     // Message windo whitespace??
    { "USR_MES      ", 1 },     // Display text/name in message window.
    { "USR_TLK      ", -1 },    // Sets character name/face in message window, setup for voice playback??
    { "USR_MENU     ", 3 },     // Sets a menu option.  Params: menu id, option string, option enabled flag
    { "USR_SELECT   ", 1 },     // Runs the actual selection task for a menu.
    { "USR_LSF_INIT ", 1 },     // Initialize a sprite layer
    { "USR_LSF_SET  ", -1 },    // Set flags for sprite layer (??)
    { "USR_CG       ", -1 },    // Set up CG (sprites *AND* BG/EV)
    { "USR_EM       ", 5 },     // Set character sprite expression (?)
    { "USR_CLR      ", 1 },     // Clear flagged sprite layer(s)
    { "USR_DISP     ", 3 },     // Screen transition
    { "USR_PATH     ", -1 },    // Sets up interpolation for sprites (???)
    { "USR_TRANS    ", 0 },     // Fade out layer (?)  TRANSITION, duh.
    { "USR_BGMPLAY  ", 3 },     // Start BGM.  Params: id, fade time (for previous BGM?), start time
    { "USR_BGMSTOP  ", 1 },     // Stop BGM.  Param: fade time
    { "USR_BGMVOLUME", 2 },     // Set BGM volume, with optional fade
    { "USR_BGMFX    ", 1 },     // Apply effect to BGM
    { "USR_AMBPLAY  ", 3 },     //
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
    { "USR_QUAKE    ", 4 },     // Screenshake effect
    { "USR_FLASH    ", 2 },     // Flash effect
    { "USR_FILTER   ", 2 },     // Image filter
    { "USR_EFFECT   ", 1 },     // Particle effect
    { "USR_SYNC     ", 2 },     // Wait for / cancel screen effects (disolve, quake, flash, trans).
    { "USR_WAIT     ", 1 },     // Pause text ???
    { "USR_MOVIE    ", 1 },     // Stop ADV mode, play movie (returns to ADV afterwards).
    { "USR_CREDIT   ", 1 },     // Stop ADV mode, play credits
    { "USR_EVENT    ", 1 },     // Unlock event CG
    { "USR_SCENE    ", 1 },     // Unlock event scene
    { "USR_TITLE    ", 1 },     // Display scene title
    { "USR_NOTICE   ", 3 },     // Popup notices (?)
    { "USR_SET_PASS ", 2 },     // Record progress???
    { "USR_IS_PASS  ", 1 },
    { "USR_AUTO_SAVE", 0 },     // Autosave
    { "USR_PLACE    ", 1 },     // Display place name
    { "USR_OPEN_NAME", 1 },
    { "USR_NAME     ", 2 },
    { "USR_DATE     ", 0 },     // Display (in-game) date
    { "USR_HELP     ", -1 },    // Enable/disable help items

    { "USR_PLATY_GAME", 1 },    // Run mini-game mode
    { "USR_TRAINING", 0 },      // Run training mode
    { "USR_SPECIAL_TRAINING", 0 },  // Run special training mode

    { "USR_SET_GAME", 3 },
    { "USR_WHATDAY", 0 },
    { "USR_SET_UNIT", 4 },
    { "USR_GET_UNIT", 3 },
    { "USR_BTS_RESULT", 0 },
    { "USR_GAME_SETTING", 1 },
    { "USR_WATCH_ENEMY", 1 },
    { "USR_RND_RT", 1 },
};


bool show_strings = false;
bool htoz = false;
opcode * opcode_list = NULL;
uint opcode_count;

const char * missing_string = "STRING_DATA_NOT_FOUND";

bool data_lookup_string(script_file * file, uint id, char ** out) {
    if (id >= file->index_count) {
        fprintf(stderr, "Reference to string not in file, id: %08x\n", id);
        *out = (char *)missing_string;
        return false;
    }
    uint offset = file->index_ptr[id];

    assert(((file->data_ptr + offset) - file->contents) < file->file_size);

    char * str = (char *)(file->data_ptr + offset);
    if (*str) {
        *out = str;
        return true;
    }
    else {
        *out = (char *)missing_string;
        return false;
    }
}

struct htoz_table_entry {
    uchar hankaku;
    uchar zenkaku[2];
};

static uint htoz_table_size = 64;

static htoz_table_entry htoz_table[htoz_table_size] = {
    { 0xa0, { 0x81, 0x40 } },
    { 0x21, { 0x81, 0x49 } },
    { 0x3f, { 0x81, 0x48 } },
    { 0xa5, { 0x81, 0x63 } },
    { 0xa1, { 0x81, 0x42 } },
    { 0xa2, { 0x81, 0x75 } },
    { 0xa3, { 0x81, 0x76 } },
    { 0xa4, { 0x81, 0x41 } },
    { 0xa6, { 0x82, 0xf0 } },
    { 0xa7, { 0x82, 0x9f } },
    { 0xa8, { 0x82, 0xa1 } },
    { 0xa9, { 0x82, 0xa3 } },
    { 0xaa, { 0x82, 0xa5 } },
    { 0xab, { 0x82, 0xa7 } },
    { 0xac, { 0x82, 0xe1 } },
    { 0xad, { 0x82, 0xe3 } },
    { 0xae, { 0x82, 0xe5 } },
    { 0xaf, { 0x82, 0xc1 } },
    { 0xb0, { 0x81, 0x5b } },
    { 0xb1, { 0x82, 0xa0 } },
    { 0xb2, { 0x82, 0xa2 } },
    { 0xb3, { 0x82, 0xa4 } },
    { 0xb4, { 0x82, 0xa6 } },
    { 0xb5, { 0x82, 0xa8 } },
    { 0xb6, { 0x82, 0xa9 } },
    { 0xb7, { 0x82, 0xab } },
    { 0xb8, { 0x82, 0xad } },
    { 0xb9, { 0x82, 0xaf } },
    { 0xba, { 0x82, 0xb1 } },
    { 0xbb, { 0x82, 0xb3 } },
    { 0xbc, { 0x82, 0xb5 } },
    { 0xbd, { 0x82, 0xb7 } },
    { 0xbe, { 0x82, 0xb9 } },
    { 0xbf, { 0x82, 0xbb } },
    { 0xc0, { 0x82, 0xbd } },
    { 0xc1, { 0x82, 0xbf } },
    { 0xc2, { 0x82, 0xc2 } },
    { 0xc3, { 0x82, 0xc4 } },
    { 0xc4, { 0x82, 0xc6 } },
    { 0xc5, { 0x82, 0xc8 } },
    { 0xc6, { 0x82, 0xc9 } },
    { 0xc7, { 0x82, 0xca } },
    { 0xc8, { 0x82, 0xcb } },
    { 0xc9, { 0x82, 0xcc } },
    { 0xca, { 0x82, 0xcd } },
    { 0xcb, { 0x82, 0xd0 } },
    { 0xcc, { 0x82, 0xd3 } },
    { 0xcd, { 0x82, 0xd6 } },
    { 0xce, { 0x82, 0xd9 } },
    { 0xcf, { 0x82, 0xdc } },
    { 0xd0, { 0x82, 0xdd } },
    { 0xd1, { 0x82, 0xde } },
    { 0xd2, { 0x82, 0xdf } },
    { 0xd3, { 0x82, 0xe0 } },
    { 0xd4, { 0x82, 0xe2 } },
    { 0xd5, { 0x82, 0xe4 } },
    { 0xd6, { 0x82, 0xe6 } },
    { 0xd7, { 0x82, 0xe7 } },
    { 0xd8, { 0x82, 0xe8 } },
    { 0xd9, { 0x82, 0xe9 } },
    { 0xda, { 0x82, 0xea } },
    { 0xdb, { 0x82, 0xeb } },
    { 0xdc, { 0x82, 0xed } },
    { 0xdd, { 0x82, 0xf1 } },
};

uint htoz_table_lookup(uchar hk) {
    for (uint i = 0; i < htoz_table_size; ++i) {
        htoz_table_entry * e = &htoz_table[i];
        if (e->hankaku == hk) {
            return i;
        } 
    }
    return -1;
}

static bool is_half_kana(uchar c) {
    // 0xa0 is an invalid lead byte, but is used by the engine to encode a full-width space.
    //return c >= 0xa0 && c <= 0xdd;
    return htoz_table_lookup(c) != -1;
}

void convert_string_htoz(char ** str) {
    uchar buffer[2048] = {0};  // We'll assume this is large enough.
    assert(strlen(*str) < 1024);

    uchar *src = (uchar *)(*str);
    uchar *dest = &buffer[0];
    while (*src) {
        if ((*src >= 0x81 && *src <= 0x9f) || (*src >= 0xe0 && *src <= 0xef)) {
            // Two byte character.
            *dest++ = *src++;
            *dest++ = *src++;
        }
        else if (*src == 0x1b) {
            // ESC, used to excape following character.
            src++;
            *dest++ = *src++;
        }
        else if (is_half_kana(*src)) {
            uint idx = htoz_table_lookup(*src);
            assert(idx >= 0 && idx < htoz_table_size);
            *dest++ = htoz_table[idx].zenkaku[0];
            *dest++ = htoz_table[idx].zenkaku[1];
            src++;
        }
        else {
            // Single byte character.
            *dest++ = *src++;
        }
    }
    *dest = '\0';
    uint size = (dest - buffer);
    assert(size > 0);

    char * newstr = (char *)calloc(1, size + 1);
    strncpy(newstr, (char *)buffer, size);
    *str = newstr;
}

bool opcode_has_param(uint op) {

    // Reserved opcodes
    if (op < ROP_COUNT) {
        return op == ROP_JUMP  ||
               op == ROP_JUMPZ ||
               op == ROP_CALL  ||
               op == ROP_PUSH  ||
               op == ROP_STR   ||
               op == ROP_FILELINE;
    }
    else {
        // User opcodes take an immediate param if param_count is negative.
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

void print_opcode(script_file * file, opcode * op) {
    if (opcode_has_param(op->op)) { 
        printf("%08x:\t%-20s\t%08x\n", op->offset, opcode_string(op->op), op->param);

        if (show_strings && op->op == ROP_STR) {
            char * str = NULL;
            if (data_lookup_string(file, op->param, &str)) {
                if (htoz) convert_string_htoz(&str);
                printf("\t\t%s\n\n", str);
                if (htoz) free(str);
            }
        }
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

        print_opcode(file, &op);
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

const char * version = "v0.1";

char * input_filename;

void usage(const char * argv0) {
    fprintf(stderr, "USAGE:  %s <INPUT FILE> [options]\n\n", argv0);
    fprintf(stderr, "Options\n");
    fprintf(stderr, "--help    | -h    Show this listing and exit.\n");
    fprintf(stderr, "--str     | -s    Print string literals inline.\n");
    fprintf(stderr, "--convert | -c    Convert half-width katakana to full-width hiragana.\n");
}

void parse_argv(int argc, char ** argv) {
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (!strcmp(argv[i], "--str") || !strcmp(argv[i], "-s")) {
                show_strings = true;
            }
            else if (!strcmp(argv[i], "--convert") || !strcmp(argv[i], "-c")) {
                htoz = true;
            }
            else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
                usage(argv[0]);
                exit(0);
            }
            else {
                input_filename = argv[i];
            }
        }
    }
}

int main(int argc, char ** argv) {
    fprintf(stderr, "ESCR1 Extractor %s\n\n", version);

    if (argc < 2) {
//        fprintf(stderr, "USAGE: %s <FILE>\n", argv[0]);
        usage(argv[0]);
        exit(1);
    }

    parse_argv(argc, argv);

    fprintf(stderr, "WARNING: This program outputs directly to stdout.  Redirect to a file.\n");
    fprintf(stderr, "Continue? [Y/N]\n");
    char yn = fgetc(stdin);
    if (yn != 'Y' && yn != 'y') {
        exit(0);
    }

    script_file script;
    uchar * data;
    int flen = load_file(input_filename, &data);

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