// this is for opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/features2d/features2d.hpp"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>

// this is for tesseract
#include <baseapi.h>
#include <allheaders.h>

using namespace std;
using namespace cv;

bool debug = true;
bool writeImg = true;

static void help()
{
    cout << "\nThis program demonstrates the Maximal Extremal Region interest point detector.\n"
    "It finds the most stable (in size) dark and white regions as a threshold is increased.\n"
    "\nCall:\n"
    "./mser_sample <path_and_image_filename, Default is 'puzzle.png'>\n\n";
}

static const Vec3b bcolors[] =
{
    Vec3b(0,0,255),
    Vec3b(0,128,255),
    Vec3b(0,255,255),
    Vec3b(0,255,0),
    Vec3b(255,128,0),
    Vec3b(255,255,0),
    Vec3b(255,0,0),
    Vec3b(255,0,255),
    Vec3b(255,255,255)
};


void erodeDilate(Mat input, bool doErode, bool doDilate) {
	int erode_size = 1;
	Mat element = getStructuringElement( MORPH_ELLIPSE,
			                           Size( 2*erode_size + 1, 2*erode_size+1 ),
			                           Point( erode_size, erode_size ) );
    if (doErode) erode( input, input, element );
  	if (doDilate) dilate( input, input, element );
    //if(debug) namedWindow("erodeDilate: edges dilated", CV_WINDOW_NORMAL );
    //if(debug) imshow("erodeDilate: edges dilated", input);
    //if (debug) waitKey();

}


// variables for tesseract function ocrDigit
Pix *pix;
int offset;
float slope;

char* ocrDigit(Mat src) {

    /// Show in a window
    tesseract::TessBaseAPI tess;
    //tess.Init(NULL, "tla");
    tess.Init(NULL, "meter2");
	tess.SetVariable("tessedit_char_whitelist", "0123456789");

    long num = 70;
    string s = "" + num;
    if (debug) printf("textord_min_xheight = %s\n", s.c_str());
	//tess.SetVariable("textord_min_xheight", "70");
    tess.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    //tess.SetImage(pix);
    //pix = pixRead("/tmp/out3.png");
	//tess.SetImage(pix);
	tess.SetImage((uchar*)src.data, src.cols, src.rows, 1, src.cols);
    
    char* outText = tess.GetUTF8Text();
    int conf1 = tess.MeanTextConf();
    tess.GetTextDirection(&offset, &slope);

    //if (debug) printf("Confidence=%d\n", conf1);
    //if (debug) printf("Offset: %d Slope: %.2f\n", offset, slope);
    //if (debug) printf("OCR output:%s\n",outText);
	return outText;

}

Mat maskFullThreshold(const Mat src) {
    Mat mask;
    cvtColor(src, mask, CV_BGR2GRAY);
    blur(mask, mask, Size(3,3), Point(-1,-1), BORDER_DEFAULT);
    threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
    erodeDilate(mask, false, true);
    return mask;
}

Mat maskBlueRedAdaptiveThreshold(const Mat src, bool blur, int thres_type, int thres_kernel_size) {
    vector<Mat> bgrImgPlanes;
    cv::split(src, bgrImgPlanes);
    Mat mask = bgrImgPlanes[0] + bgrImgPlanes[1];
    if (blur) bilateralFilter(mask.clone(), mask, -1, 10, 10, BORDER_DEFAULT );
    adaptiveThreshold(mask, mask, 255, thres_type, CV_THRESH_BINARY, thres_kernel_size, 0);
    return mask;
}

Mat maskChromaInYCrCb(const Mat src) {
    std::vector<Mat> ycrcbImgPlanes;
    Mat mask;
    cvtColor(src, mask, CV_BGR2YCrCb);
    cv::split(mask, ycrcbImgPlanes);
    mask = ycrcbImgPlanes[1].clone();
    threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
    mask = 255 - mask;
    erodeDilate(mask, true, false);
    return mask;
}

char * deblank(char *str)
{
  char *out = str, *put = str;

  for(; *str != '\0'; ++str)
  {
    if(*str != ' ')
      *put++ = *str;
  }
  *put = '\0';

  return out;
}

Mat filterContours(Mat src, bool debug) {
    cvtColor(src, src, CV_BGR2GRAY);
    threshold(src, src, 0, 255, THRESH_BINARY | THRESH_OTSU);
    Mat result = Mat::zeros(src.rows, src.cols, src.type());
    Mat contourMat = src.clone();
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(contourMat, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
    if (debug) namedWindow("src after contour", CV_WINDOW_NORMAL );
    if (debug) imshow("src after contour", src);
    if (debug) waitKey();
	if (debug) namedWindow("src", CV_WINDOW_NORMAL );
    RNG rng(12346);
    for( int i = 0; i < contours.size(); i++ ) {
        Rect roi = boundingRect(contours[i]);
        if (debug) printf("NEXT SHAPE (%u): height=%u width=%u roi.y=%u src.rows=%u ?\n", i, roi.height, roi.width, roi.y, src.rows);
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        if (debug) drawContours(contourMat, contours, i, color, 3, 8, hierarchy, 0);
		if (debug) namedWindow("countour final", CV_WINDOW_NORMAL );
		if (debug) imshow("countour final", contourMat);
		if (debug) waitKey();
        if (hierarchy[i][3]>-1) {
            if (debug) printf("skip: inside parent\n",i);
            continue;
        }
        if (roi.height < src.rows*2/5) {
            if (debug) printf("skip: height too small: roi.height < src.rows*2/5\n");
            continue;
        }
        if (roi.height > src.rows - 10) {
            if (debug) printf("skip: height too big: roi.height > src.rows - 10\n");
            continue;
        }
        if (roi.y >= src.rows/2) {
            if (debug) printf("skip: shape in bottom: roi.y >= src.rows/2\n");
            continue;
        }
        if (roi.y + roi.height <= src.rows/2) {
            if (debug) printf("skip: shape in top: roi.y + roi.height <= src.rows/2\n");
            continue;
        }
        int area = contourArea(contours[i]);
        if (area < 100) {
            if (debug) printf("skip: area too small: %u < 100\n", area);
            continue;
        }
        if (debug) printf("ACCEPT %u %u !!!\n", roi.height, roi.width);
        int newY = src.rows - 4 - roi.height;
        Rect alignedRoi = Rect(roi.x,newY,roi.width,roi.height);
        Mat destRoi = result(alignedRoi);
        Mat srcRoi = src(roi);
        srcRoi.copyTo(destRoi);
		if (debug) imshow("result", result);
		if (debug) waitKey();
    }
    return result;
}

int main( int argc, char** argv )
{
    if (argc<2) {
        //if (debug) printf("You must provide at least one argument\n");
        exit(0);
    }

    Mat src = imread(argv[1]);
    if (argc>=3) debug = true;
    else debug = false;
    if (argc>=4) writeImg = true;
    else writeImg = true;
    Rect cropRoi = Rect(0,2,src.cols,src.rows-4);
    src = src(cropRoi);
    if(debug) namedWindow("orig", CV_WINDOW_NORMAL );
    if(debug) imshow("orig", src);
    if (debug) waitKey();

    Mat mask = maskFullThreshold(src);
    if(debug) namedWindow("mask full threshold", CV_WINDOW_NORMAL );
    if(debug) imshow("mask full threshold", mask);
    if (debug) waitKey();

    Mat mask2 = maskBlueRedAdaptiveThreshold(src, true, ADAPTIVE_THRESH_MEAN_C, 15);
    if(debug) namedWindow("mask 2", CV_WINDOW_NORMAL );
    if(debug) imshow("mask 2", mask2);
    if (debug) waitKey();

    Mat mask3 = maskBlueRedAdaptiveThreshold(src, false, ADAPTIVE_THRESH_GAUSSIAN_C, 17);
    if(debug) namedWindow("mask 3", CV_WINDOW_NORMAL );
    if(debug) imshow("mask 3", mask3);
    if (debug) waitKey();

    Mat mask4 = maskChromaInYCrCb(src);
    if(debug) namedWindow("mask 4", CV_WINDOW_NORMAL );
    if(debug) imshow("mask 4", mask4);
    if (debug) waitKey();

    mask = mask & mask2 & mask3 & mask4;
    if(debug) namedWindow("combined masks", CV_WINDOW_NORMAL );
    if(debug) imshow("combined masks", mask);
    if (debug) waitKey();

    erodeDilate(mask, true, true);
    if(debug) namedWindow("eroded/dilated mask", CV_WINDOW_NORMAL );
    if(debug) imshow("eroded/dilated mask", mask);
    if (debug) waitKey();


    Mat maskedSrc;
    src.copyTo(maskedSrc, mask); 
    if(debug) namedWindow("masked src", CV_WINDOW_NORMAL );
    if(debug) imshow("masked src", maskedSrc);
    if (debug) waitKey();

    Mat filteredSrc = filterContours(maskedSrc, debug);
    if(debug) namedWindow("filteredSrc", CV_WINDOW_NORMAL );
    if(debug) imshow("filteredSrc", filteredSrc);
    if (debug) waitKey();

    Mat inputOCR = 255 - filteredSrc;
    if(debug) namedWindow("INPUT OCR", CV_WINDOW_NORMAL );
    if(debug) imshow("INPUT OCR", inputOCR);
    if (debug) waitKey();
    char* foundDigits;
    char buffer[128];
    if (writeImg) {
        string s = string(argv[1]);
        int pos1 = s.find_last_of("/");
        string name = "/tmp/" + s.substr(pos1+1) + ".tif";
        if (debug) printf("img %s written to disk\n",name.c_str());
        imwrite( name, inputOCR);
    }

    foundDigits = ocrDigit(inputOCR);
    foundDigits[strcspn(foundDigits, "\n")] = '\0';
    char* digits = deblank(foundDigits);
    printf("DIGIT:%s\n",foundDigits);


    // Wait for user to press q
    while (debug) {
        char c = waitKey();
        if( c == 113 ) break;
    }
}
