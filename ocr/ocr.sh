#!/bin/bash

UPLOAD_IMG_DIR="/var/www/mysite/polls/media/documents/"
OCR_RESULT_DIR="/var/www/ocr-results/"
SQLITE3_DB="/var/www/mysite/mysite/test.db"
OCR_BIN="/home/fstruman/src/ocr-test/Test"

declare -A ocrs
ocrs=( \
['00000000c83de86f_1']="/home/fstruman/src/ocr-meter/ocrJohn" \
['0000000001_1']="/home/fstruman/src/ocr-meter/ocrSaintMichel1" \
)

echo "";

for sound in "${!ocrs[@]}";  do
 NOW=$(date +"%m-%d-%Y %T")
 echo "$NOW loop through $sound*.jpg in $UPLOAD_IMG_DIR and ocr with ${ocrs["$sound"]}"
 for file in `find ${UPLOAD_IMG_DIR}/ -type f -name "$sound*.jpg"`
 do

  # check the file is a file that still exists
  if [ -f $file ];
  then
   echo "ocr meter image: $file `basename $file`";
  else
   echo "no file found";
   exit 0;
  fi

  # OCR the image
  IMG_NAME="`basename $file`"
  echo "ocr: ${ocrs["$sound"]} $file"
  ${ocrs["$sound"]} "$file" | grep 'DIGIT:' | perl -pi -e 's/^.*://' | perl -pi -e 's/ //g' > "${file}.txt"
  cat "${file}.txt"

  # Move processed image from upload directory into result directory - makes the file available for apache
  echo "save image $file into $OCR_RESULT_DIR"
  cp $file $OCR_RESULT_DIR
  # Write ocr digits in a file into results directory - for further insertion into cassandra
  mv "${file}.txt" $OCR_RESULT_DIR
  echo "ocr result written to: $OCR_RESULT_DIR/$IMG_NAME.txt "

  # Remove processed image from list of uploaded files in sqlite database
  echo "delete from polls_document where docfile  like '%/${IMG_NAME}';" | sqlite3 $SQLITE3_DB
  rm ${file}
  echo "deleted file $IMG_NAME from sqlite3 and django upload directory"
 done

done

