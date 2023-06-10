#!/bin/bash


p=102011604035381881 # 0x16A6B036D7F2A79
x1=9433814980383617 # 0x21840136C85381
x2=19168316694801104 # 0x44197B83892AD0
x3=90078616228674308 # 0x1400606322B3B04
x4=90078616228674308 # 0x1400606322B3B04
pub=65537 # 0x10001


skip=0
if [[ $1 == "-h" ]] ; then
   echo "Usage: $0 [options]"
   echo "  -h : Display this help"
   echo "  -s : Skip the longest part to solve and use precomputed orders mod small primes"
   exit 0
elif [[ $1 == "-s" ]] ; then
   skip=1
fi

mkdir tmp


# Computations with 23 and greater become too slow and will increase the whole solving time instead of reducing it.
ell_todo=(5 11 13 17 19)
curve="[0, $x1, $x2, $x3, $x4, 1]"

if [[ $skip -eq 0 ]]; then
    len=0
    for ell_i in "${ell_todo[@]}"; do
        echo -e "\n---------- Solving order mod $ell_i ----------"
        filename="tmp/input_XP_ell_$ell_i.txt"
        echo $p > $filename
        echo $curve >> $filename
        echo $ell_i >> $filename
        ./main -o "tmp/ell_tmp.txt" < $filename
        res=`cat tmp/ell_tmp.txt`
        IFS=' '
        read -ra ell_res <<< "$res"
        ell[len]=$ell_i
        s1p[len]=${ell_res[1]}
        s2p[len]=${ell_res[2]}
        len=$((len+1))
    done
else
    echo -e "\n---------- Skipping solving of orders mod small primes ----------"
    echo "Setting precomputed values:"
    # When testing, order mod 23 was computed so it is included here. This makes the LMPMCT step even faster to solve.
    ell=(5 11 13 17 19 23)
    s1p=(4  1  5 16 15  8)
    s2p=(0  2 10 16  2  7)
    len=${#ell[@]}
    
    for (( i = 0; i < $len; i++ )); do
        echo "(s1, s2) mod ${ell[$i]} = ${s1p[$i]}, ${s2p[$i]}"
    done
fi


# Adapt the main program results to the CRT program input file format
for (( i = 0; i < $len; i++ )); do
    crt_arr_ell="${crt_arr_ell}${ell[$i]},"
    crt_arr_s1p="${crt_arr_s1p}${s1p[$i]},"
    crt_arr_s2p="${crt_arr_s2p}${s2p[$i]},"
done

# Remove last commas
crt_arr_ell=${crt_arr_ell::-1}
crt_arr_s1p=${crt_arr_s1p::-1}
crt_arr_s2p=${crt_arr_s2p::-1}


echo -e "\n---------- Calculating bigger modular information using CRT ----------"
filename="tmp/input_XP_crt.txt"
echo $crt_arr_ell > $filename
echo $crt_arr_s1p >> $filename
echo $crt_arr_s2p >> $filename
crt=$(./CRT -q < $filename)
IFS=' '
read -ra crt_res <<< "$crt"
crt_mod=${crt_res[2]}
crt_s1p=${crt_res[0]}
crt_s2p=${crt_res[1]}

echo "CRT mod = $crt_mod"
echo "CRT s1p = $crt_s1p"
echo "CRT s2p = $crt_s2p"


echo -e "\n---------- Solving order from CRT results ----------"
filename="tmp/input_XP_lmpmct.txt"
echo $p > $filename
echo 0 >> $filename
echo $x1 >> $filename
echo $x2 >> $filename
echo $x3 >> $filename
echo $x4 >> $filename
echo $crt_mod >> $filename
echo $crt_s1p >> $filename
echo $crt_s2p >> $filename
./LMPMCT -o "tmp/lmpmct_tmp.txt" < $filename
order=`cat tmp/lmpmct_tmp.txt`


echo -e "\n---------- Calculating private key from order ----------"
priv=$(./InvMod $pub $order)

echo -e "\nPrivate key: $priv"

rm -r tmp
