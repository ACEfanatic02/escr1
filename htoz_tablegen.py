#!/usr/bin/env python
# -*- coding: utf-8 -*-

# These are the mapping strings used by `convert_htoz` for the 半角→全角 conversion.
# Neither one is in byte-order for SHIFT_JIS.
ZENKAKU = u"　！？…。「」、をぁぃぅぇぉゃゅょっーあいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわん"
HANKAKU = u"\xa0!?･｡｢｣､ｦｧｨｩｪｫｬｭｮｯｰｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝ"

# Store mappings as (byte_index, zenkaku_bytes)
MAPPING = []

def gen_mapping():

    for i, hk in enumerate(HANKAKU):
        zk = ZENKAKU[i]
        zk_bytes = zk.encode('shift-jis', 'backslashreplace')
        if i == 0:
            hk_byte = '\xa0'
        else:
            hk_byte = hk.encode('shift-jis')
        byte_index = ord(hk_byte)

        cur = int(byte_index), ord(zk_bytes[0]), ord(zk_bytes[1])
        MAPPING.append(cur)

def output():
    print "static htoz_table_entry htoz_table[] = {"
    for i, b0, b1 in MAPPING:
        print "    { 0x%02x, { 0x%02x, 0x%02x } }," % (i, b0, b1)
    print "};"

if __name__ == '__main__':
    gen_mapping()
    output()