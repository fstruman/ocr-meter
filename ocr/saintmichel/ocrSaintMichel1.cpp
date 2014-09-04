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


int copyContentOfContour(Mat source, vector<Point> closed_contour, Mat dest) {
    int in = 0;
    for (int x = 0; x < source.cols; x++) {
        for (int y = 0; y < source.rows; y++) {
            double inside = pointPolygonTest(closed_contour, Point(x, y), false);
            if (inside > 0) ++in;
            if (inside >= 0) {
                if (source.channels() == 3) {
                    dest.at<cv::Vec3b>(y,x)[0] = source.at<cv::Vec3b>(y,x)[0];
                    dest.at<cv::Vec3b>(y,x)[1] = source.at<cv::Vec3b>(y,x)[1];
                    dest.at<cv::Vec3b>(y,x)[2] = source.at<cv::Vec3b>(y,x)[2];
                } else {
                    int val = (int) source.at<uchar>(y,x);
                    dest.at<uchar>(y,x) = val;
                }
            }
        }
    }
    if (debug)  printf("found %u points inside\n",in);
    if (debug) waitKey();
    return in;
}

int eraseContentOfContour(Mat source, vector<Point> closed_contour, Mat dest) {
    int in = 0;
    for (int x = 0; x < source.cols; x++) {
        for (int y = 0; y < source.rows; y++) {
            double inside = pointPolygonTest(closed_contour, Point(x, y), false);
            if (inside > 0) ++in;
            if (inside >= 0) {
                if (source.channels() == 3) {
                    dest.at<cv::Vec3b>(y,x)[0] = 0;
                    dest.at<cv::Vec3b>(y,x)[1] = 0;
                    dest.at<cv::Vec3b>(y,x)[2] = 0;
                } else {
                    int val = 0;
                    dest.at<uchar>(y,x) = val;
                }
            }
        }
    }
    if (debug)  printf("found %u points inside\n",in);
    if (debug) waitKey();
    return in;
}


void copyDigit(Mat img, vector<Point> closed_contour, Mat img_output) {
    Rect roi = boundingRect(closed_contour);
    Mat destinationBlob = img_output(roi);
    if(debug) namedWindow("digits found", CV_WINDOW_NORMAL);
    if(debug) imshow("digits found", destinationBlob);
    if (debug) waitKey();
    int copiedPx = copyContentOfContour(img, closed_contour, img_output);
    if (copiedPx<200) {
        Mat sourceBlob = img(roi);
        sourceBlob.copyTo(destinationBlob);
    }
    if (img_output.channels() == 1) {
        // This blur removes *big* specks (of flash reflexion) surrounding some digits
        medianBlur(destinationBlob, destinationBlob, 3);
        threshold(destinationBlob, destinationBlob, 0, 255, THRESH_BINARY | THRESH_OTSU);
        //equalizeHist(destinationBlob, destinationBlob);
        //threshold(destinationBlob, destinationBlob, 125, 255, THRESH_TOZERO);
    }
}


// variables for tesseract function ocrDigit
Pix *pix;
int offset;
float slope;

char* ocrDigit(Mat src) {

    /// Show in a window
    tesseract::TessBaseAPI tess;
    tess.Init(NULL, "liquidcrystal");
    //tess.Init(NULL, "meter2");
	//tess.SetVariable("tessedit_char_whitelist", "0123456789");

    long num = 70;
    string s = "" + num;
    if (debug)  printf("textord_min_xheight = %s\n", s.c_str());
	//tess.SetVariable("textord_min_xheight", "70");
    tess.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    //tess.SetImage(pix);
    //pix = pixRead("/tmp/out3.png");
	//tess.SetImage(pix);
	tess.SetImage((uchar*)src.data, src.cols, src.rows, 1, src.cols);
    
    char* outText = tess.GetUTF8Text();
    int conf1 = tess.MeanTextConf();
    tess.GetTextDirection(&offset, &slope);

    //if (debug)  printf("Confidence=%d\n", conf1);
    //if (debug)  printf("Offset: %d Slope: %.2f\n", offset, slope);
    //if (debug)  printf("OCR output:%s\n",outText);
	return outText;

}

Mat maskFullThreshold(const Mat src) {
    /*vector<Mat> bgrImgPlanes;
    cv::split(src, bgrImgPlanes);
    Mat mask = bgrImgPlanes[1] + bgrImgPlanes[2];*/

    std::vector<Mat> ycrcbImgPlanes;
    Mat mask;
    cvtColor(src, mask, CV_BGR2YCrCb);
    cv::split(mask, ycrcbImgPlanes);
    mask = ycrcbImgPlanes[2].clone();

    if(debug) namedWindow("maskFullThreshold black&red", CV_WINDOW_NORMAL );
    if(debug) imshow("maskFullThreshold black&red", mask);
    if (debug) waitKey();
    blur(mask, mask, Size(5,5), Point(-1,-1), BORDER_DEFAULT);
    threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
    erodeDilate(mask, false, true);
    return mask;
}

Mat maskBlueRedAdaptiveThreshold(const Mat src, bool blur, int thres_type, int thres_kernel_size) {
/*    vector<Mat> bgrImgPlanes;
    cv::split(src, bgrImgPlanes);
    Mat mask = bgrImgPlanes[1] + bgrImgPlanes[2];
    mask = 255 - mask;
*/
    std::vector<Mat> ycrcbImgPlanes;
    Mat mask;
    cvtColor(src, mask, CV_BGR2YCrCb);
    cv::split(mask, ycrcbImgPlanes);
    mask = ycrcbImgPlanes[2].clone();

    if(debug) namedWindow("black and red", CV_WINDOW_NORMAL );
    if(debug) imshow("black and red", mask);
    if (debug) waitKey();
    if (blur) bilateralFilter(mask.clone(), mask, -1, 3, 3, BORDER_DEFAULT );
    adaptiveThreshold(mask, mask, 255, thres_type, CV_THRESH_BINARY, thres_kernel_size, 0);
    return mask;
}

Mat maskChromaInYCrCb(const Mat src) {
    std::vector<Mat> ycrcbImgPlanes;
    Mat mask;
    cvtColor(src, mask, CV_BGR2YCrCb);
    cv::split(mask, ycrcbImgPlanes);
    mask = ycrcbImgPlanes[2].clone();
    if(debug) namedWindow("maskChromaInYCrCb Y", CV_WINDOW_NORMAL );
    if(debug) imshow("maskChromaInYCrCb Y", mask);
    if (debug) waitKey();
    threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
    if(debug) namedWindow("maskChromaInYCrCb Y thresh", CV_WINDOW_NORMAL );
    if(debug) imshow("maskChromaInYCrCb Y thresh", mask);
    if (debug) waitKey();

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
        if (debug)  printf("NEXT SHAPE (%u): height=%u width=%u roi.y=%u src.rows=%u ?\n", i, roi.height, roi.width, roi.y, src.rows);
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        if (debug) drawContours(contourMat, contours, i, color, 3, 8, hierarchy, 0);
		if (debug) namedWindow("countour final", CV_WINDOW_NORMAL );
		if (debug) imshow("countour final", contourMat);
		if (debug) waitKey();
        if (hierarchy[i][3]>-1) {
            if (debug)  printf("skip: inside parent\n");
            continue;
        }
        if (roi.height < src.rows*2/5) {
            if (debug)  printf("skip: height too small: roi.height < src.rows*2/5\n");
            continue;
        }
        int area = contourArea(contours[i]);
        if (area < 300) {
            if (debug)  printf("skip: area too small: %u < 100\n", area);
            continue;
        }
        if (debug)  printf("ACCEPT %u %u !!!\n", roi.height, roi.width);
        copyDigit(src, contours[i], result);
		if (debug) imshow("result", result);
		if (debug) waitKey();
    }
    return result;
}

vector<Point> findComma(const Mat src_orig, int cropX, int cropY) {
    // Restrict the search for the comma to this rectangle
    Rect lastDigit = Rect(cropX, cropY, src_orig.cols - cropX, src_orig.rows - cropY);
    Mat src = src_orig(lastDigit);
    std::vector<Mat> ycrcbImgPlanes;
    Mat mask;
    cvtColor(src, mask, CV_BGR2YCrCb);
    cv::split(mask, ycrcbImgPlanes);
    mask = ycrcbImgPlanes[0].clone();
    if(debug) namedWindow("findComma: mask", CV_WINDOW_NORMAL );
    if(debug) imshow("findComma: mask", mask);
    if (debug) waitKey();
    bilateralFilter(mask.clone(), mask, -1, 9, 15, BORDER_DEFAULT );
    if(debug) namedWindow("findComma: mask blured", CV_WINDOW_NORMAL );
    if(debug) imshow("findComma: mask blured", mask);
    if (debug) waitKey();
    adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 29, 0);
    mask = 255 - mask;
    erodeDilate(mask, true, false);
    if(debug) namedWindow("findComma: adaptiveThreshold", CV_WINDOW_NORMAL );
    if(debug) imshow("findComma: adaptiveThreshold", mask);
    if (debug) waitKey();

    Mat contourMat = mask.clone();
    Mat contourMatSelected = mask.clone();
    Mat result = Mat::zeros(src.rows, src.cols, src.type());
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(contourMat, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
    if (debug) namedWindow("src after contour", CV_WINDOW_NORMAL );
    if (debug) imshow("src after contour", src);
    if (debug) waitKey();
	if (debug) namedWindow("src", CV_WINDOW_NORMAL );
    RNG rng(12346);
    int winner = -1;
    for( int i = 0; i < contours.size(); i++ ) {
        Rect roi = boundingRect(contours[i]);
        if (debug)  printf("NEXT SHAPE (%u): height=%u width=%u roi.y=%u src.rows=%u ?\n", i, roi.height, roi.width, roi.y, src.rows);
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        if (debug) drawContours(contourMat, contours, i, color, 3, 8, hierarchy, 0);
		if (debug) namedWindow("countour final", CV_WINDOW_NORMAL );
		if (debug) imshow("countour final", contourMat);
		if (debug) waitKey();
        if (winner >= 0 && roi.y < boundingRect(contours[winner]).y) {
            if (debug) printf("current winner y is lower, no change of winner\n",winner, i);
            continue;
        }
        int area = contourArea(contours[i]);
        if (area < 45 || area > 100) {
            if (debug)  printf("skip: area (x,y,w,h) (%u,%u,%u,%u) not within range: 120 < %u < 180 \n", roi.x, roi.y, roi.width, roi.height, area);
            continue;
        }
        if (debug) drawContours(contourMatSelected, contours, i, color, 3, 8, hierarchy, 0);
		if (debug) namedWindow("findComma: countour candidate", CV_WINDOW_NORMAL );
    	if (debug) imshow("findComma: countour candidate", contourMatSelected);
	    if (debug) waitKey();
        if (debug) printf("change of winner from %u to %u\n",winner, i);
        winner = i;
        if (debug)  printf("ACCEPT %u %u !!!\n", roi.height, roi.width);
        if (debug) drawContours(result, contours, winner, color, 3, 8, hierarchy, 0);
    	if (debug) namedWindow("findComma: comma detected", CV_WINDOW_NORMAL );
	    if (debug) imshow("findComma: comma detected", result);
        if (debug) waitKey();
    }
    if (winner == -1) winner = 0;
    vector<Point> contourComma = contours[winner];
    for (int i = 0; i < contourComma.size(); i++) {
        contourComma[i].x = contourComma[i].x + cropX;
        contourComma[i].y = contourComma[i].y + cropY;
    }
    return contourComma;
}

Mat buildCombinedMask(Mat src) {

    Mat mask2 = maskBlueRedAdaptiveThreshold(src, true, ADAPTIVE_THRESH_MEAN_C, 31);
    if(debug) namedWindow("mask 2", CV_WINDOW_NORMAL );
    if(debug) imshow("mask 2", mask2);
    if (debug) waitKey();

    Mat mask3 = maskBlueRedAdaptiveThreshold(src, true, ADAPTIVE_THRESH_GAUSSIAN_C, 63);
    if(debug) namedWindow("mask 3", CV_WINDOW_NORMAL );
    if(debug) imshow("mask 3", mask3);
    if (debug) waitKey();

    Mat mask = mask2 & mask3; // & mask4 & mask;
    if(debug) namedWindow("combined masks", CV_WINDOW_NORMAL );
    if(debug) imshow("combined masks", mask);
    if (debug) waitKey();

    return mask;
}

int main( int argc, char** argv )
{

    RNG rng(12345);
    if (argc<2) {
        //if (debug)  printf("You must provide at least one argument\n");
        exit(0);
    }

    Mat src = imread(argv[1]);
    if (argc>=3) debug = true;
    else debug = false;
    if (argc>=4) writeImg = true;
    else writeImg = true;

    Rect cropRoi = Rect(5,0,src.cols-52,src.rows-50);
    src = src(cropRoi);
    if(debug) namedWindow("orig", CV_WINDOW_NORMAL );
    if(debug) imshow("orig", src);
    if (debug) waitKey();

    if (debug) printf("image after cropping (cols,rows) = (%u,%u)\n",src.cols, src.rows);

    Mat tmp = src.clone();
    int cropX = 3*src.cols/4, cropY = 2 * src.rows / 3;
    vector<Point> contourComma = findComma(src, cropX, cropY);
    cv::Point2f center; float radius = 0;
    minEnclosingCircle( contourComma, center, radius );
    if (debug) {
		circle( tmp, center, radius, 255, 2, 8, 0 );
		if (debug) printf("comma circle center (%f,%f) and radius %f", center.x, center.y, radius);
		if(debug) namedWindow("comma detected", CV_WINDOW_NORMAL );
		if(debug) imshow("comma detected", tmp);
		if (debug) waitKey();
    }

    Mat mask = buildCombinedMask(src);

    //return 0;

    /*Rect commaRoi = Rect(235,src.rows - 50, 10, 50);
    Mat comma = src(commaRoi);
    if(debug) namedWindow("comma", CV_WINDOW_NORMAL );
    if(debug) imshow("comma", comma);
    if (debug) waitKey();
    Vec3b colour = src.at<Vec3b>(Point(240, src.rows - 51));
    comma=cv::Scalar(colour);
    if(debug) namedWindow("orig without comma", CV_WINDOW_NORMAL );
    if(debug) imshow("orig without comma", src);
    if (debug) waitKey();*/

    radius = radius + 3;
    circle( mask, center, radius, 0, -1, 8, 0 );
    if(debug) namedWindow("mask without comma", CV_WINDOW_NORMAL );
    if(debug) imshow("mask without comma", mask);
    if (debug) waitKey();

    Mat filteredSrc = filterContours(mask, debug);
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
        if (debug)  printf("img %s written to disk\n",name.c_str());
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
