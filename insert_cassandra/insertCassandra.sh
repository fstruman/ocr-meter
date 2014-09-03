#!/bin/bash

OCR_RESULT_DIR="/var/www/ocr-results/"
SQLITE3_DB="/var/www/mysite/mysite/test.db"
CQL_BIN="/home/fstruman/src/cpp-driver/demo/cql_demo"

echo "loop through *.txt files in $OCR_RESULT_DIR"
for file in `find ${OCR_RESULT_DIR}/ -type f -name "*.txt"`
do

 if [ -f $file ];
 then
  echo "ocr meter image: $file";
 else
  echo "no file found";
  exit 0;
 fi

 OCR_VALUE=`cat $file`
 IMG_NAME="`basename $file`"
 echo "insert $OCR_VALUE for $IMG_NAME"
 $CQL_BIN --file $IMG_NAME --value $OCR_VALUE
 echo "remove $file to prevent re-processing at next run"
 rm -f $file
done

