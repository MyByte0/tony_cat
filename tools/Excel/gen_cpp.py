import sys
import openpyxl

dict_type_base = {
    'int32' : 'int32_t',
    'int64' : 'int64_t',
    'string': 'std::string',
    'date' : 'time_t',
    '[int32]': 'std::vector<int32_t>',
    '[int64]': 'std::vector<int64_t>',
    '[string]': 'std::vector<std::string>',
    '[date]': 'std::vector<time_t>',
    '[{int32,int32}]': 'std::map<int32_t, int32_t>',
    '[{int32,int64}]': 'std::map<int32_t, int64_t>',
    '[{int32,string}]': 'std::map<int32_t, std::string>',
    '[{int64,int32}]': 'std::map<int64_t, int32_t>',
    '[{int64,int64}]': 'std::map<int64_t, int64_t>',
    '[{int64,string}]': 'std::map<int64_t, std::string>',
    '[{string,int32}]': 'std::map<std::string, int32_t>',
    '[{string,int64}]': 'std::map<std::string, int64_t>',
    '[{string,string}]': 'std::map<std::string, std::string>',
}

dict_type_paser_fun = {
    'int32' : 'ConfigValueToInt32',
    'int64' : 'ConfigValueToInt64',
    'string': 'ConfigValueToString',
    'date' : 'ConfigValueToTimestamp',
    '[int32]': 'ConfigValueToVecInt32',
    '[int64]': 'ConfigValueToVecInt64',
    '[string]': 'ConfigValueToVecString',
    '[date]': 'ConfigValueToVecTimestamp',
    '[{int32,int32}]': 'ConfigValueToInt32MapInt32',
    '[{int32,int64}]': 'ConfigValueToInt32MapInt64',
    '[{int32,string}]': 'ConfigValueToInt32MapString',
    '[{int64,int32}]': 'ConfigValueToInt64MapInt32',
    '[{int64,int64}]': 'ConfigValueToInt64MapInt64',
    '[{int64,string}]': 'ConfigValueToInt64MapString',
    '[{string,int32}]': 'ConfigValueToStringMapInt32',
    '[{string,int64}]': 'ConfigValueToStringMapInt64',
    '[{string,string}]': 'ConfigValueToStringMapString',
}

dict_type_var_prefix = {
    'int32' : 'n',
    'int64' : 'n',
    'string': 'str',
    'date' : 'n',
    '[int32]': 'vec',
    '[int64]': 'vec',
    '[string]': 'vec',
    '[date]': 'vec',
    '[{int32,int32}]': 'map',
    '[{int32,int64}]': 'map',
    '[{int32,string}]': 'map',
    '[{int64,int32}]': 'map',
    '[{int64,int64}]': 'map',
    '[{int64,string}]': 'map',
    '[{string,int32}]': 'map',
    '[{string,int64}]': 'map',
    '[{string,string}]': 'map',
}

def gen_code(argv):
    # args cmd
    if len(argv) < 2:
        print('error: args error!\n')
        print('arg1: excel filename, arg2: out path, ... VarName=VarType\n')
        sys.exit(-1) 

    filename = argv[1]
    cpp_gen_path = argv[2]
    dict_types_mod = {}
    if len(cpp_gen_path) > 0:
        if cpp_gen_path[-1] != '\\' and cpp_gen_path[-1] != '/':
            cpp_gen_path = cpp_gen_path + '/'

    for i in range(len(argv)):
        if i < 2:
            continue
        list_splite = argv[i].split('=', 1)
        if len(list_splite) < 2:
            continue
        var_name = list_splite[0]
        var_type = list_splite[1]
        dict_types_mod[var_name] = var_type


    # loading file
    excel_data = openpyxl.load_workbook(filename, data_only=True)

    sheet_names = excel_data.sheetnames 
    config_name = sheet_names[0]
    sheet_data = excel_data[sheet_names[0]]

    list_type = []
    list_name = []

    i_row = 0
    for row in sheet_data.values:
        i_row = i_row + 1
        if i_row < 3: 
            continue

        for cell_value in row:
            if i_row == 3:                          # field type
                list_type.append(cell_value)
                continue

            if i_row == 4:                          # field name
                list_name.append(cell_value)
                continue

            if i_row > 4:
                break

    if len(list_type) != len(list_name):
        print('error: {} list_type len not match list_name!\n'.format(config_name))
        sys.exit(-1) 

    if len(list_type) == 0:
        print('error: {} list_type empty!\n'.format(config_name))
        sys.exit(-1) 

    for i in range(len(list_name)):
        var_name = list_name[i]
        if var_name in dict_types_mod:
            list_type[i] = dict_types_mod[var_name]

    cpp_struct_name = "{}ConfigData".format(config_name)
    file_write = open("{}{}.h".format(cpp_gen_path, cpp_struct_name), "w")

    # common header
    file_write.write('#ifndef {}_'.format(cpp_struct_name))
    file_write.write('\n#define {}_'.format(cpp_struct_name))
    file_write.write('\n')
    file_write.write('\n#include "common/core_define.h"')
    file_write.write('\n#include "common/utility/config_field_paser.h"')
    file_write.write('\n#include "log/log_module.h"')
    file_write.write('\n')
    file_write.write('\n#include <tinyxml2.h>')
    file_write.write('\n')
    file_write.write('\n#include <cstdint>')
    file_write.write('\n#include <ctime>')
    file_write.write('\n#include <map>')
    file_write.write('\n#include <string>')
    file_write.write('\n#include <unordered_map>')
    file_write.write('\n#include <vector>')
    file_write.write('\n')
    file_write.write('\nSER_NAME_SPACE_BEGIN')
    file_write.write('\n')

    # struct dec begin
    file_write.write( "\nstruct {} {{".format(cpp_struct_name))


    # struct members begin
    for i in range(len(list_type)):
        var_name = list_name[i]
        var_type = list_type[i]
        member_name = dict_type_var_prefix[var_type] + var_name
        if var_type not in dict_type_base:
            print('infomation: not support type:{} on:{}\n'.format(var_type, var_name))
            continue
        var_code_type = dict_type_base[var_type]
        file_write.write( '\n    {} {}'.format(var_code_type, member_name))
        if dict_type_var_prefix[var_type] == 'n' :
            file_write.write( ' = 0;')
        else :
            file_write.write( ';')

    # function define begin
    file_write.write('\n')
    file_write.write('\n    bool LoadXmlElement(const tinyxml2::XMLAttribute* pNodeAttribute) {')
    first_var_name = ''
    first_member_name = ''
    for i in range(len(list_type)):
        var_name = list_name[i]
        var_type = list_type[i]
        call_fun = dict_type_paser_fun[var_type]
        member_name = dict_type_var_prefix[var_type] + var_name
        if i == 0:
            file_write.write('\n        if (std::string("{}") == pNodeAttribute->Name()) {{'.format(var_name))
            first_var_name = var_name
            first_member_name = member_name
        else :
            file_write.write('\n        else if (std::string("{}") == pNodeAttribute->Name()) {{'.format(var_name))

        file_write.write('\n            if (false == {}(pNodeAttribute->Value(), {})) {{'.format(call_fun, member_name))
        file_write.write('\n                LOG_ERROR("Paser error on {}:{}", {});'.format(first_var_name, '{}', first_member_name))
        file_write.write('\n                return false;')
        file_write.write('\n            }')
        file_write.write('\n        }')
    
    file_write.write('\n        return true;')
    
    # function define end
    file_write.write('\n    }')


    file_write.write( "\n}};".format())
    # struct dec end
    file_write.write('\n')
    file_write.write('\nSER_NAME_SPACE_END')
    file_write.write('\n')
    file_write.write('\n#endif // _{}_H'.format(cpp_struct_name))
    file_write.write('\n')

    file_write.close()

def main(argv):
    gen_code(argv)

if __name__ == "__main__":
    main(sys.argv)
