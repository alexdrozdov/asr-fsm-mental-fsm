#!/bin/bash

#������ ������������ ��� macos. ��������� ����������� ��������� �
#���������������� �����.

E_BADARGS=65
EXPECTED_ARGS=3

if [ $# -ne $EXPECTED_ARGS ]
then
    echo "Usage: `basename $0` executable_name lib_name subst_lib_name"
    echo "      executable_name - full or relative name of executable to patch"
    echo "      lib_name - original library name without pathes to replace"
    echo "      subst_lib_name - new library name, perchaps with relative path"
    exit $E_BADARGS
fi

#�������� �����, ������� ���������� ��������
exec_file=$1
#����� �������� ����������, ���� � ������� ���� ��������
search_lib_name=$2
#����� �������� ����������, � �����
replace_lib_name=$3

current_inst_path=`otool -L $exec_file | grep $search_lib_name`
current_lib_arr=( $current_inst_path )
current_lib=${current_lib_arr[0]}
install_name_tool -change $current_lib @executable_path/$replace_lib_name $exec_file 
