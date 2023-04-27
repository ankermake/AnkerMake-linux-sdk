#!/bin/bash
if [ $# != 1 ];
then
	echo "    ratio     : 0 ~ 100 "
	exit 1;
fi
> soc/x1830/mscaler/coefficient_file.txt
ratio=$1

if [ $ratio -lt 0 ];
then
	echo "    error ratio less than !!!"
	exit 1;
fi
if [ $ratio -gt 100 ];
then
	echo "    error ratio greater than 100 !!!"
	exit 1;
fi

sc_coe_gen(){
	x=$1
	a=$2
	type=$3

	if [ $type -eq 0 ];
	then
		out=$(( ( ( ($a * ($x ** 3)) >> 18 )  - ( ( 5 * $a * ($x ** 2)) >> 9 )  + (8 * $a * $x) - (4 * $a * (2 ** 9)) ) >> 2 ))
	fi
	if [ $type -eq 1 ];
	then
		out=$(( (  ( ( ($a+8) * ($x ** 3) ) >> 18) - ( ( ($a+12) * ($x**2) ) >> 9) + (4 * (2**9))  ) >> 2))
	fi

	echo $out
}

for i in `seq 1 256`;
do
	sc_coe_0=`echo $(sc_coe_gen $(( 512 + $i )) -2 0)`
	sc_coe_1=`echo $(sc_coe_gen $((       $i )) -2 1)`
	sc_coe_2=`echo $(sc_coe_gen $(( 512 - $i )) -2 1)`
	sc_coe_3=`echo $(sc_coe_gen $((1024 - $i )) -2 0)`

	jz_coe_0=128
	jz_coe_1=128
	jz_coe_2=128
	jz_coe_3=128

	coe_0=$(( (sc_coe_0 * ratio + jz_coe_0 * (100 - ratio))/100 ))
	coe_1=$(( (sc_coe_1 * ratio + jz_coe_1 * (100 - ratio))/100 ))
	coe_2=$(( (sc_coe_2 * ratio + jz_coe_2 * (100 - ratio))/100 ))
	coe_3=$(( (sc_coe_3 * ratio + jz_coe_3 * (100 - ratio))/100 ))


	h_value=`echo $(( (2**30) + ( ($i - 1) * (2 ** 22)) + ( ($coe_0 & 0x7ff)* (2**11) ) + ($coe_1 & 0x7ff) ))`
	l_value=`echo $((           ( ($i - 1) * (2 ** 22)) + ( ($coe_2 & 0x7ff)* (2**11) ) + ($coe_3 & 0x7ff) ))`
	h_cmd=`printf " 0x%08x,"  $h_value`
	l_cmd=`printf " 0x%08x,"  $l_value`
	echo $h_cmd >> soc/x1830/mscaler/coefficient_file.txt
	echo $l_cmd >> soc/x1830/mscaler/coefficient_file.txt
done
