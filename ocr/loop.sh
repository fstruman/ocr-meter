#!/bin/bash

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DIR="/home/fstruman/ocr-results"
OCR_BIN="ocrSaintMichel1"
SOURCE_ID="0000000001_1"
count=0


## to keep useful logs for import into excel
## /home/fstruman/framework/ocr-meter/ocr/loop.sh | egrep '^Processing |^DIGIT:' | perl -pi -e 's/Processing \/home\/fstruman\/ocr-results\///' | perl -pi -e 's/DIGIT://' | grep -v 'alistair' > /tmp/logs.txt

## Before the images changed to 7 digits
#for f in `ls /home/fstruman/ocr-results/00000000c83de86f_1_2014-0[34]* | grep '00000000c83de86f_1_2014-03-30_20' -B 100`;

## After the images changed to 7 digits
for f in `ls $DIR/$SOURCE_ID* | grep -v 'compteur' | grep '0000000001_1_2014-08-27_14' -A 1000000000 | grep -v 'alistair'`;
do
 echo "Processing $f";
 $WORKDIR/$OCR_BIN $f
 #mv $f ${DIR}/tla.test_font.exp${count}.tif
 ((count++))
done;
