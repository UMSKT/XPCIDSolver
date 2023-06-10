#!/bin/bash

cwd=$(pwd)

build_dir="$cwd/build"
ntljac2_makefile_libs_dir=".."

gmp_archive="$cwd/gmp-4.2.4.tar.gz"
gmp_folder="$build_dir/gmp-4.2.4"

ntl_archive="$cwd/ntl-5.4.tar.gz"
ntl_folder="$build_dir/ntl-5.4"

ntljac2_archive="$cwd/NTLJac2-0.1-1.tgz"
ntljac2_folder="$build_dir/NTLJac2"

ntljac2_patch_folder="$cwd/NTLJac2-0.1-1-patch"
ntljac2_xp_patch_folder="$cwd/NTLJac2-0.1-1-XP-patch"

makefile_patch_find="FLAGS = -O4 -Wall"
makefile_patch_replace="FLAGS = -I$ntljac2_makefile_libs_dir/include -L$ntljac2_makefile_libs_dir/lib -Wl,-R$ntljac2_makefile_libs_dir/lib -O4 -Wall"


if [ ! -f "$gmp_archive" ]; then
    echo "Error: '$gmp_archive' does not exist."
	exit -4
fi

if [ ! -f "$ntl_archive" ]; then
    echo "Error: '$ntl_archive' does not exist."
	exit -5
fi

if [ ! -f "$ntljac2_archive" ]; then
    echo "Error: '$ntljac2_archive' does not exist."
	exit -6
fi

if [ ! -d "$ntljac2_patch_folder" ]; then
    echo "Error: '$ntljac2_patch_folder' does not exist."
	exit -7
fi

if [ ! -d "$ntljac2_xp_patch_folder" ]; then
    echo "Error: '$ntljac2_xp_patch_folder' does not exist."
	exit -8
fi


mkdir $build_dir

echo -e "\n############### Extracting GMP ###############"
tar xvzf "$gmp_archive" -C $build_dir

echo -e "\n############### Extracting NTL ###############"
tar xvzf "$ntl_archive" -C $build_dir

echo -e "\n############### Extracting NTLJac2 ###############"
tar xvzf "$ntljac2_archive" -C $build_dir


echo -e "\n############### Patching NTLJac2 To Fix Build Errors ###############"
cp -a $ntljac2_patch_folder/* $ntljac2_folder

echo -e "\n############### Patching NTLJac2 To Add XP Solver ###############"
cp -a $ntljac2_xp_patch_folder/* $ntljac2_folder

echo -e "\n############### Patching Makefile ###############"
sed -i "s|${makefile_patch_find}|${makefile_patch_replace}|" "$ntljac2_folder/Makefile"


echo -e "\n############### Building GMP ###############"
cd $gmp_folder
./configure --prefix=$build_dir --enable-cxx
make
cd $cwd

echo -e "\n############### Installing GMP ###############"
cd $gmp_folder
make install
cd $cwd


echo -e "\n############### Building NTL ###############"
cd $ntl_folder/src
./configure PREFIX=$build_dir GMP_PREFIX=$build_dir
make
cd $cwd

echo -e "\n############### Installing NTL ###############"
cd $ntl_folder/src
make install
cd $cwd


echo -e "\n############### Building NTLJac2 (with XP Solver Patch) ###############"
cd $ntljac2_folder
make
cd $cwd
