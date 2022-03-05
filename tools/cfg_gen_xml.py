import sys
import openpyxl

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
    file_write = open("{}{}.xml".format(cpp_gen_path, config_name), "w")
    file_write.write( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>".format())
    file_write.write( "\n<root>".format())

    list_type = []
    list_name = []

    i_row = 0
    for row in sheet_data.values:
        i_row = i_row + 1
        if i_row < 3: 
            continue

        i_col = 0
        for cell_value in row:
            i_col = i_col + 1

            if i_row == 3:                          # field type
                list_type.append(cell_value)
                continue

            if i_row == 4:                          # field name
                list_name.append(cell_value)
                continue

            if i_row > 4:
                if i_col > len(list_name):
                    break
                if i_col == 1:
                    file_write.write( "\n\t<element ".format())

                file_write.write( "{}=\"{}\" ".format(list_name[i_col-1], cell_value if cell_value!=None else ""))
                
                if i_col == len(list_name):
                    file_write.write( "/>".format())

    file_write.write( "\n</root>".format())
    file_write.close()

def main(argv):
    gen_code(argv)

if __name__ == "__main__":
    main(sys.argv)