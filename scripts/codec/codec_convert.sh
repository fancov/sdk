#!/bin/bash

# 
# 对于encode，输入文件一定要存在，如果不存在，返回失败。如果存在，将文件转为PCMU/PCMA/G729净荷格式
# 对于decode，输入文件制定的文件名将不包括后缀名。如果输出文件存在，将直接返回成功；
#

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

if [ -z $5 ]; then
    OUTPUT_FILE=$OUTPUT_FILE_PATH/${4%.*}
else
    OUTPUT_FILE=$OUTPUT_FILE_PATH/$5
fi

case $1 in
    encodec)
        if [ ! -f $INPUT_FILE ]; then
			print_usage "Input file not exist."
            exit 2
        fi
    
        $SOX_APP $INPUT_FILE -c 1 -e u-law -r 8000 -t raw $OUTPUT_FILE.PCMU
        if [ $? -ne 0 ]; then
			print_usage "Encode input file to PCMU format FAIL."
            exit 1
        fi

        $SOX_APP $INPUT_FILE  -c 1 -e a-law -r 8000 -t raw $OUTPUT_FILE.PCMA
        if [ $? -ne 0 ]; then
			print_usage "Encode input file to PCMA format FAIL."
            exit 1
        fi

        $CODEC_APP -ic PCMA -if $OUTPUT_FILE.PCMA -oc G729 -of $OUTPUT_FILE.G729
        if [ $? -ne 0 ]; then
			print_usage "Encode input file to G729 format FAIL."
            exit 1
        fi
    ;;
    decodec)
        if [ -f $OUTPUT_FILE".wav" ]; then
			print_usage "Output file already exist."
            exit 0
        fi
    
        if [ -f  $INPUT_FILE"-in.PCMU" ]; then
            IN_CODEC="PCMU"
        elif [ -f  $INPUT_FILE"-in.PCMA" ]; then
            IN_CODEC="PCMA"
        elif [ -f  $INPUT_FILE"-in.G729" ]; then
            IN_CODEC="G729"
        fi
        
        if [ -f  $INPUT_FILE"-out.PCMU" ]; then
            OUT_CODEC="PCMU"
        elif [ -f  $INPUT_FILE"-out.PCMA" ]; then
            OUT_CODEC="PCMA"
        elif [ -f  $INPUT_FILE"-out.G729" ]; then
            OUT_CODEC="G729"
        fi
        
        if [ -z $IN_CODEC -a -z $OUT_CODEC ]; then
			print_usage "Input file not exist."$OUT_CODEC"   "$IN_CODEC
            exit -1
        fi
		
		echo $OUT_CODEC"   "$IN_CODEC
		echo $INPUT_FILE"   "$OUTPUT_FILE
        
        if [ -f $INPUT_FILE"-in."$IN_CODEC ]; then
            case $IN_CODEC in
                PCMA)
                    $SOX_APP -e a-law -r 8000 -t raw $INPUT_FILE"-in."$IN_CODEC $OUTPUT_FILE"-in.wav"
                    if [ $? != 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$INPUT_FILE"-in."$IN_CODEC
                        exit 1
                    fi
                ;;
                PCMU)
                    $SOX_APP -e u-law -r 8000 -t raw $INPUT_FILE"-in."$IN_CODEC $OUTPUT_FILE"-in.wav"
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$INPUT_FILE"-in."$IN_CODEC
                        exit 1
                    fi
                ;;
                G729)
                    $CODEC_APP -ic G729 -if $INPUT_FILE"-in."$IN_CODEC -oc PCMU -of $OUTPUT_FILE.tmp.PCMU
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to PCMA format FAIL. Input file: "$INPUT_FILE"-in."$IN_CODEC
                        exit 1
                    fi
                
                    $SOX_APP -e u-law -r 8000 -t raw $OUTPUT_FILE.tmp.PCMU $OUTPUT_FILE"-in.wav"
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$OUTPUT_FILE.tmp.PCMU
                        exit 1
                    fi
                    
                    rm -rf $OUTPUT_FILE.tmp.PCMU
                ;;
            esac
        fi
        
        if [ -f $INPUT_FILE"-out."$OUT_CODEC ]; then
            case $OUT_CODEC in
                PCMA)
                    $SOX_APP -e a-law -r 8000 -t raw $INPUT_FILE"-out."$OUT_CODEC $OUTPUT_FILE"-out.wav"
                    if [ $? != 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$INPUT_FILE"-out."$OUT_CODEC
                        exit 1
                    fi
                ;;
                PCMU)
                    $SOX_APP -e u-law -r 8000 -t raw $INPUT_FILE"-out."$OUT_CODEC $OUTPUT_FILE"-out.wav"
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$INPUT_FILE"-out."$OUT_CODEC
                        exit 1
                    fi
                ;;
                G729)
                    $CODEC_APP -ic G729 -if $INPUT_FILE"-out."$OUT_CODEC -oc PCMU -of $OUTPUT_FILE.tmp.PCMU
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to PCMA format FAIL. Input file: "$INPUT_FILE"-out."$OUT_CODEC
                        exit 1
                    fi
                
                    $SOX_APP -e u-law -r 8000 -t raw $OUTPUT_FILE.tmp.PCMU $OUTPUT_FILE"-out.wav"
                    if [ $? -ne 0 ]; then
						print_usage "Decodec the input file to wav format FAIL. Input file: "$OUTPUT_FILE.tmp.PCMU
                        exit 1
                    fi
                    
                    rm -rf $OUTPUT_FILE.tmp.PCMU
                ;;
            esac
        fi

        if [ -f $OUTPUT_FILE"-in.wav" -a -f $OUTPUT_FILE"-out.wav" ]; then
            $SOX_APP $OUTPUT_FILE"-in.wav" $OUTPUT_FILE"-out.wav" -M $OUTPUT_FILE".wav"
			rm -rf $OUTPUT_FILE"-in.wav" $OUTPUT_FILE"-out.wav"
			echo "Marge the to channel into one file"
        elif [ -f $OUTPUT_FILE"-in.wav" ]; then
            mv $OUTPUT_FILE"-in.wav" $OUTPUT_FILE".wav"
			echo "Rename the in channel to output file"
        elif [ -f $OUTPUT_FILE"-out.wav" ]; then
            mv $OUTPUT_FILE"-out.wav" $OUTPUT_FILE".wav"
			echo "Rename the out channel to output file"
        fi
        
        if [ ! -f $OUTPUT_FILE".wav" ]; then
			print_usage "Decodec the input file to wav file FAIL. "
            exit -1
        fi
    ;;
    *)
    print_usage
    exit 255
esac

exit 0
