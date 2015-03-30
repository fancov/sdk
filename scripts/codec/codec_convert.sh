#!/bin/bash

CODEC_APP="/usr/bin/codec_convert"
SOX_APP="sox"

print_usage()
{
    echo "    ERROR: "$1
    echo ""
    echo "    Use this command as: codec_convert decodec|encodec <input file path> <output file path> <input file> <output file>"
    echo ""
}

if [ $# -lt 4 ]; then
    print_usage
    exit 255
fi

if [ ! -f $CODEC_APP ]; then
	echo "Cannot find the codec_convert cmd."
	exit 2
fi

INPUT_FILE_PATH=$2
OUTPUT_FILE_PATH=$3

INPUT_FILE="$INPUT_FILE_PATH/$4"
if [ ! -f $INPUT_FILE ]; then
    exit 2
fi

if [ -z $5 ]; then
    OUTPUT_FILE=$OUTPUT_FILE_PATH/${INPUT_FILE%.*}
else
    OUTPUT_FILE=$OUTPUT_FILE_PATH/$5
fi

case $1 in
    encodec)
        $SOX_APP $INPUT_FILE -c 1 -e u-law -r 8000 -t raw $OUTPUT_FILE.PCMU
        if [ $? -ne 0 ]; then
            exit 1
        fi

        $SOX_APP $INPUT_FILE  -c 1 -e a-law -r 8000 -t raw $OUTPUT_FILE.PCMA
        if [ $? -ne 0 ]; then
            exit 1
        fi

        $CODEC_APP -ic PCMA -if $INPUT_FILE -oc G729 -of $OUTPUT_FILE.G729
        if [ $? -ne 0 ]; then
            exit 1
        fi
    ;;
    decodec)
        FILE_TYPE=${INPUT_FILE##*.}
        case $FILE_TYPE in
            PCMA)
                $SOX_APP -e a-law -r 8000 -t raw $INPUT_FILE $OUTPUT_FILE.wav
                if [ $? != 0 ]; then
                    exit 1
                fi
            ;;
            PCMU)
                $SOX_APP -e u-law -r 8000 -t raw $INPUT_FILE $OUTPUT_FILE.wav
                if [ $? -ne 0 ]; then
                    exit 1
                fi
            ;;
            G729)
                $CODEC_APP -ic G729 -if $INPUT_FILE -oc PCMU -of $OUTPUT_FILE.tmp.PCMU
                if [ $? -ne 0 ]; then
                    exit 1
                fi
            
                $SOX_APP -e u-law -r 8000 -t raw $4.tmp.PCMU $OUTPUT_FILE.wav
                if [ $? -ne 0 ]; then
                    exit 1
                fi
                
                rm -rf $OUTPUT_FILE.tmp.PCMU
            ;;
            *)
                print_usage
                exit 255
            ;;
        esac        
    ;;
    *)
    print_usage
    exit 255
esac

exit 0
