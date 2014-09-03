#!/bin/bash

UPLOAD_IMG_DIR="/var/www/mysite/polls/media/documents/"
OCR_RESULT_DIR="/var/www/ocr-results/"
SQLITE3_DB="/var/www/mysite/mysite/test.db"
OCR_BIN="/home/fstruman/src/ocr-test/Test"

echo "loop through jpg in $UPLOAD_IMG_DIR"
for file in `find ${UPLOAD_IMG_DIR}/ -type f -name "*.jpg"`
do

 if [ -f $file ];
 then
  echo "ocr meter image: $file `basename $file`";
 else
  echo "no file found";
  exit 0;
 fi

 IMG_NAME="`basename $file`"
 echo "processing $file"
 $OCR_BIN "$file" | grep 'DIGIT:' | perl -pi -e 's/^.*://' | perl -pi -e 's/ //g' > "${file}.txt"
 cat "${file}.txt"
 echo "save image $file into $OCR_RESULT_DIR"
 cp $file $OCR_RESULT_DIR
 mv "${file}.txt" $OCR_RESULT_DIR
 echo "ocr result written to: $OCR_RESULT_DIR/$IMG_NAME.txt "
 echo "delete from polls_document where docfile  like '%/${IMG_NAME}';" | sqlite3 $SQLITE3_DB
 rm ${file}
 echo "deleted file $IMG_NAME from sqlite3 and django upload directory"
done
