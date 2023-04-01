import sys

import paser_pb

def get_lower_case_name(text):
    lst = []
    for index, char in enumerate(text):
        if char.isupper():
            if index != 0:
                lst.append('_')
            lst.append(char.lower())
        else:
            lst.append(char)
    return ''.join(lst)

def get_upper_case_name(text):
    lst = []
    match_char = False
    for index, char in enumerate(text):
        if char ==  '_':
            match_char = True
            continue
        elif match_char == True:
            lst.append(char.upper())
        elif index == 0:
            lst.append(char)  # need upper on first char ?
        else:
            lst.append(char)
        match_char = False
    return ''.join(lst)

def gen_code(argv):
    # args cmd
    if len(argv) < 2:
        print('error: args error!\n')
        sys.exit(-1) 

    filename = argv[1]
    cpp_gen_path = argv[2]
    if len(cpp_gen_path) > 0:
        cpp_gen_path.replace('\\', '/')
        if cpp_gen_path[-1] != '/':
            cpp_gen_path = cpp_gen_path + '/'

    data_metas = paser_pb.LoadPbFile(filename)

    header_file_name = "{}.h".format('db_define')
    header_file_write = open("{}{}".format(cpp_gen_path, header_file_name), "w")

    header_file_write.write('\n#ifndef COMMON_DATABASE_DB_DEFINE_H_')
    header_file_write.write('\n#define COMMON_DATABASE_DB_DEFINE_H_')
    header_file_write.write('\n')
    header_file_write.write('\n#include <string>')
    header_file_write.write('\n#include <unordered_map>')
    header_file_write.write('\n#include <unordered_set>')
    header_file_write.write('\n#include <vector>')
    header_file_write.write('\n')
    header_file_write.write('\n#include "common/core_define.h"')
    header_file_write.write('\n')
    header_file_write.write('\nTONY_CAT_SPACE_BEGIN')
    header_file_write.write('\n')
    header_file_write.write('\n')
    header_file_write.write('\nstruct DbPbFieldKey {')
    header_file_write.write('\n    DbPbFieldKey() {')

    for meta in data_metas:
        if len(meta.comment) > 1 and meta.comment[1] == 'user_data':
			# format key=(key_name1,key_name2...) or key=key_name
            if len(meta.comment) > 2 and meta.comment[2].startswith('key'):
                keys_line = meta.comment[2][meta.comment[2].find('=') + len('='):]
                if keys_line.startswith('('):
                    keys_line = keys_line[keys_line.find('(') + len('(') : keys_line.find(')')]
                keys = keys_line.split(',')
                key_fields = []
                msg_name = ''
                if meta.namespace != '':
                    msg_name = "{}.{}".format(meta.namespace, meta.name)
                else:
                    msg_name = meta.name
                header_file_write.write('\n        mapMesssageKeys[std::string("{}")] = {{'.format(msg_name))
                for index, key_name in enumerate(keys):
                    field = meta.GetPbMetaFieldByName(key_name)
                    if None == field:
                        print('error: on message:{} key_name:{}!\n'.format(meta.name, key_name))
                        break
                    if field.type.startswith('int') == False and field.type.startswith('uint') == False and field.type.startswith('string') == False:
                        print("not support key type name:{} type:{} !".format(meta.name, meta.pb_type))
                        break
                    key_fields.append(field)
                    if index + 1 != len(keys):
                        header_file_write.write('\n            std::string("{}"),'.format(key_name))
                    else:
                        header_file_write.write('\n            std::string("{}")'.format(key_name))
                header_file_write.write('\n        };')
                continue

    header_file_write.write('\n    }')
    header_file_write.write('\n')
    header_file_write.write('\n    std::unordered_map<std::string, std::unordered_set<std::string>>')
    header_file_write.write('\n        mapMesssageKeys;')
    header_file_write.write('\n};')
    header_file_write.write('\n')
    header_file_write.write('\nTONY_CAT_SPACE_END')
    header_file_write.write('\n')
    header_file_write.write('\n#endif // COMMON_DATABASE_DB_DEFINE_H_')
    header_file_write.write('\n')

def main(argv):
    gen_code(argv)

if __name__ == "__main__":
    main(sys.argv)
