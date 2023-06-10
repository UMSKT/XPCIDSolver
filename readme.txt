sudo apt-get install build-essential m4 libc6-dev

./Install.sh

cd build/NTLJac2
./solve.py

Use "-s" option to skip the first step by using precomputed values and speed up the LMPMCT process.