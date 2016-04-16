#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <vector>
#include <stdio.h>
#include <sstream>

using namespace cv;
using namespace std;

namespace {
    void help(char** av) {
       std::cout << "\nDo the analysis and extract the physics of the simple car game\n"
                 << "Usage: " << av[0] << " <video device number>\n"
                 << "OR   : " << av[0] << " <.avi filename>\n"
                 << std::endl;
    }

    class ImageProcessor {
       public:
          virtual ~ImageProcessor() {}
          virtual bool process( const cv::Mat& frame, bool dropped ) {
             if ( !dropped ) {
                if (frame.empty()) {
                    return false;
                }
             }
             beforeFrame_ = frame.clone();
             return true;
          }
          virtual std::string getTitle() const { return "Unititled"; }
          const cv::Mat& getBeforeFrame() const { return beforeFrame_; }

       private:
          cv::Mat beforeFrame_;
    };


    class StaticBackgroundProcessor : public ImageProcessor {
       public:
          StaticBackgroundProcessor( int param = 200 ) : param_( param ) {}
          virtual ~StaticBackgroundProcessor() { delete pAverageImage_; }
          virtual bool process( const cv::Mat& frame, bool dropped ) override {
             if ( !dropped ) {
                if (frame.empty()) {
                   return false;
                }

                cv::Mat diff(getBeforeFrame().size(), getBeforeFrame().type());
                cv::absdiff(getBeforeFrame(), frame, diff);
                cv::Mat grayscaleMat( getBeforeFrame().size(), CV_8U);
                cv::cvtColor( diff, grayscaleMat, CV_BGR2GRAY );
                cv::Mat binaryMaskMat(grayscaleMat.size(), grayscaleMat.type());
                cv::threshold(grayscaleMat, binaryMaskMat, 0, 255, cv::THRESH_BINARY);
                cv::bitwise_not( binaryMaskMat, binaryMaskMat );
                cv::Mat doubleGrayscaleMate(getBeforeFrame().size(), CV_64F);
                binaryMaskMat.convertTo( doubleGrayscaleMate, CV_64F, 1. / 255.);

                if ( !pAverageImage_ ) {
                   pAverageImage_ = new cv::Mat(frame.size(), CV_64F);
                }

                // Modifying the avg image
                double dcounter = counter_;
                cv::addWeighted( *pAverageImage_, dcounter / ( dcounter + 1. ), doubleGrayscaleMate, 1 / ( dcounter + 1. ), 0., *pAverageImage_ );
                counter_++;

                imshow("binary", getResult() );
             }

             ImageProcessor::process( frame, dropped );
             return true;
          }

          virtual std::string getTitle() const override { return "Processor"; }

          const cv::Mat getResult() const {
             assert( pAverageImage_ );
             cv::Mat resultImage( getBeforeFrame().size(), CV_8U);
             pAverageImage_->convertTo( resultImage, CV_8U, 255.);
             cv::threshold(resultImage, resultImage, param_, 255, cv::THRESH_BINARY);
             return resultImage;
          }
       private:
          cv::Mat* pAverageImage_;
          int counter_ = 0;
          int param_;
    };

    class DynamicBackgroundProcessor : public ImageProcessor {
       public:
          DynamicBackgroundProcessor( int bigMapSize = 1000, short int maxstep = 5 ) : pStaticBackground_( 0 ), bigMapSize_( bigMapSize ), maxstep_( maxstep ) {
             init();
          }

          DynamicBackgroundProcessor( const cv::Mat& staticBackground, int bigMapSize = 1000, short int maxstep = 5 ) : pStaticBackground_( new cv::Mat( staticBackground.clone() )  ), bigMapSize_( bigMapSize ), maxstep_( maxstep ) {
             init();
          }
          void init() {
             segmentedBackground_  = Mat::zeros( bigMapSize_ * 2 + 1, bigMapSize_ * 2 + 1, CV_8UC3 );
             numOfSamplesInAverage_= Mat::zeros( bigMapSize_ * 2 + 1, bigMapSize_ * 2 + 1, CV_8UC1 );
             segmentedBackground_.setTo(cv::Scalar(0,255,0));
          }
          virtual bool process( const cv::Mat& frame, bool dropped ) override {
             if ( !dropped ) {
                if (frame.empty()) {
                    imwrite( "extract_background.png", segmentedBackground_ );
                    return false;
                }
                Mat diff = frame.clone();
            
                short int dx = 0;
                short int dy = 0;
                long sum = 0;
                Mat foregroundMask = calculateShift(getBeforeFrame(), frame, dx, dy, sum);
                Mat beforeFrame_Masked(getBeforeFrame().size(), getBeforeFrame().type(), cv::Scalar(0,255,0));
                getBeforeFrame().copyTo( beforeFrame_Masked, foregroundMask );
            
                addToBackground( beforeFrame_Masked, ax_, ay_ );
            
                ax_ += dx;
                ay_ += dy;
                std::cout << " D(" << dx << ";" << dy << ") -- A(" << ax_ << ";" << ay_ << ") VAL=" << sum << std::endl;
            
                imshow("binary", beforeFrame_Masked );
             }

             ImageProcessor::process( frame, dropped );
            
             return true;
          }
          virtual std::string getTitle() const override { return "Processor"; }
       private:
          // Roboust solution for calculating the shift between two frames after each other
          cv::Mat calculateShift( const Mat& before, const Mat& after, short int& rx, short int& ry, long int& sum ) {
             // Creating the diff image, converting it to binary, then dilate a bit -> filtering out areas with exactly the same pixels
             cv::Mat diff(before.size(), before.type());
             cv::absdiff(before, after, diff);
             cv::Mat grayscaleMat( before.size(), CV_8U);
             cv::cvtColor( diff, grayscaleMat, CV_BGR2GRAY );
             cv::Mat binaryMaskMat(grayscaleMat.size(), grayscaleMat.type());
             cv::threshold(grayscaleMat, binaryMaskMat, 1, 255, cv::THRESH_BINARY);
             cv::dilate( binaryMaskMat, binaryMaskMat, getStructuringElement( MORPH_RECT, Size(3,3), Point(1,1) ));
  
             // Do the filter both on the before and on the after image 
             cv::Mat beforeGrayscale( before.size(), CV_8U );
             cv::cvtColor( before, beforeGrayscale, CV_BGR2GRAY );
             cv::Mat beforeGrayscaleMasked( before.size(), CV_8U, cvScalar(0.) );
             beforeGrayscale.copyTo( beforeGrayscaleMasked, binaryMaskMat );
  
             cv::Mat afterGrayscale( after.size(), CV_8U );
             cv::cvtColor( after, afterGrayscale, CV_BGR2GRAY );
             cv::Mat afterGrayscaleMasked( after.size(), CV_8U, cvScalar(0.) );
             afterGrayscale.copyTo( afterGrayscaleMasked, binaryMaskMat );
  
             long minimum = 255 * afterGrayscale.cols * afterGrayscale.rows;
  
             cv::Mat diffStored(beforeGrayscaleMasked.size(), beforeGrayscaleMasked.type()); // debug
  
             // The claim is that if we apply the correct shift, then the diff image will contain a very few points
             for ( int ix = -maxstep_; ix <= maxstep_; ++ix ) {
                for ( int iy = -maxstep_; iy <= maxstep_; ++iy ) {
                   cv::Mat afterGrayscaleMaskedShifted = cv::Mat::zeros(afterGrayscaleMasked.size(), afterGrayscaleMasked.type());
  
                   // had no better idea
                   int px = ix > 0 ?  ix : 0;
                   int nx = ix < 0 ? -ix : 0;
                   int py = iy > 0 ?  iy : 0;
                   int ny = iy < 0 ? -iy : 0;
                   int sizex = afterGrayscaleMasked.cols - ( px > nx ? px : nx );
                   int sizey = afterGrayscaleMasked.rows - ( py > ny ? py : ny );
  
                   afterGrayscaleMasked( cv::Rect(px, py, sizex, sizey) ).copyTo( afterGrayscaleMaskedShifted( cv::Rect( nx, ny, sizex, sizey ) ) );
  
                   cv::Mat diff(beforeGrayscaleMasked.size(), beforeGrayscaleMasked.type(), cvScalar(0.));
                   cv::absdiff(beforeGrayscaleMasked, afterGrayscaleMaskedShifted, diff);
                   long pixels = cv::sum( diff )[0];
                   if ( pixels < minimum || pixels == minimum && rx == 0 && ry == 0 ) {
                      rx = -ix; // sorry, I wrote the entire logic in the opposite way and I don't feel like to rewrite everything
                      ry = -iy;
                      minimum = pixels;
                      diff.copyTo( diffStored );
                      sum = pixels;
                   }
                }
             }
             
             cv::Mat diffStoredBW( after.size(), CV_8U );
             cv::threshold( diffStored, diffStoredBW, 3, 255, THRESH_BINARY );
             cv::dilate( diffStoredBW, diffStoredBW, getStructuringElement( MORPH_RECT, Size(7,7), Point(3,3) ));
             cv::bitwise_not( diffStoredBW, diffStoredBW );
             cv::bitwise_and( binaryMaskMat, diffStoredBW, diffStoredBW );
  
             return diffStoredBW;
          }

          void addToBackground( const Mat& img, short int posx, short int posy ) {
             for(int y=0;y<img.rows;y++) {
                for(int x=0;x<img.cols;x++) {
                   int px = bigMapSize_ + x + posx;
                   int py = bigMapSize_ + y + posy;
                   //int alpha = ov.at<Vec4b>(y,x)[3];
                   if (!(img.at<Vec3b>(y,x)[0] == 0 && img.at<Vec3b>(y,x)[1] == 255 && img.at<Vec3b>(y,x)[2] == 0)) {
                      if ( 0 <= px && px < numOfSamplesInAverage_.cols && 0 <= py && py < numOfSamplesInAverage_.rows ) {
                         unsigned char oldnums = numOfSamplesInAverage_.at<Vec3b>(py,px)[0];
                         if (oldnums < 255) {
                            for (short int i = 0; i < 3; i++) { 
                               segmentedBackground_.at<Vec3b>(py,px)[i] = 
                                  (unsigned char) (( int(img.at<Vec3b>(y,x)[i]) + 
                                                   int(segmentedBackground_.at<Vec3b>(py,px)[i] * oldnums) ) / int(oldnums + 1));
                            }
                            numOfSamplesInAverage_.at<Vec3b>(py,px)[0]++;
                         }
                      }
                   }
                }
             }
          }

          cv::Mat numOfSamplesInAverage_; 

          int ax_ = 0;
          int ay_ = 0;
          int bigMapSize_;
          short int maxstep_;

          cv::Mat segmentedBackground_;
          cv::Mat* pStaticBackground_;
    };

    int processShell(VideoCapture& capture, ImageProcessor& processor) {
        int n = 0;
        char filename[200];
        string window_name = processor.getTitle();
        namedWindow(window_name, CV_WINDOW_KEEPRATIO); //resizable window;
        Mat frame;
        Mat before;
        capture >> frame;
          
        int sWidth = 200;
        int sHeight = 200;
        int sX = 90;
        int sY = 0;
        int dom = 15;
        int drop = 10;

        for (;;) {
            capture >> frame;
            if ( !processor.process( frame, drop > 0 ) ) {
               break;
            }

            imshow(window_name, frame);

            if ( drop > 0 ) {
               --drop;
            }

            switch ( (char)waitKey(5) ) {
                case 'q':
                case 'Q':
                case 27: //escape key
                    return 1;
                default:
                    break;
            }
        }
        return 0;
    }

}

int main(int ac, char** av) {

    if (ac != 2) {
        help(av);
        return 1;
    }
    std::string arg = av[1];

    StaticBackgroundProcessor sbp;
    {
       VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file
       if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
           capture.open(atoi(arg.c_str()));
       if (!capture.isOpened()) {
           cerr << "Failed to open a video device or video file!\n" << endl;
           help(av);
           return 1;
       }

       if ( !processShell(capture, sbp) ) {
          capture.release();
       }

    }

    {
       DynamicBackgroundProcessor dbp( sbp.getResult() );

       VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file
       if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
           capture.open(atoi(arg.c_str()));
       if (!capture.isOpened()) {
           cerr << "Failed to open a video device or video file!\n" << endl;
           help(av);
           return 1;
       }

       if ( !processShell(capture, dbp) ) {
          return 0;
       }

       capture.release();
    } 

    return 0;
}
