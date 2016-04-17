#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <iomanip>

using namespace cv;
using namespace std;

const unsigned char MAX_NUM_OF_SAMPLES_IN_AVERAGE_IMAGE = 250;
const bool MERGE_PREVIOUS_DIFF = false;

namespace {
    void help(char** av) {
       std::cout << "\nDo the analysis and extract the physics of a simple car game\n"
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
          StaticBackgroundProcessor( int param = 200 ) : pAverageImage_( nullptr ), param_( param ) {}
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

          virtual std::string getTitle() const override { return "Processing"; }

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
          DynamicBackgroundProcessor( std::vector<Vec2f>& trajectory,
                                      const cv::Mat* pStaticBackground = 0,
                                      unsigned char maxNumOfSamplesInAverageImage = MAX_NUM_OF_SAMPLES_IN_AVERAGE_IMAGE, int bigMapSize = 1000, short int maxstep = 5, bool mergePreviousDiff = MERGE_PREVIOUS_DIFF )
           : trajectory_( trajectory ), pStaticBackground_( pStaticBackground ), pPreviousMask_( nullptr ),
             maxNumOfSamplesInAverageImage_( maxNumOfSamplesInAverageImage ), bigMapRadius_( bigMapSize ), maxstep_( maxstep ), mergePreviousDiff_( mergePreviousDiff )
          {
             segmentedBackground_  = Mat::zeros( bigMapRadius_ * 2 + 1, bigMapRadius_ * 2 + 1, CV_64FC3 );
             numOfSamplesInAverage_= Mat::zeros( bigMapRadius_ * 2 + 1, bigMapRadius_ * 2 + 1, CV_8UC1 );
             segmentedBackground_.setTo(cv::Scalar(0.0,255.0,0.0));
             numOfSamplesInAverage_.setTo(cv::Scalar(0));
          }

          virtual bool process( const cv::Mat& frame, bool dropped ) override {
             short int dx = 0;
             short int dy = 0;
             if ( !dropped ) {
                if (frame.empty()) {
                    imwrite( "extract_background.png", getResult() );
                    return false;
                }
                Mat diff = frame.clone();
            
                Mat foregroundMask = calculateShift(getBeforeFrame(), frame, dx, dy);
                Mat totalMask( foregroundMask.size(), foregroundMask.type() ); 
                foregroundMask.copyTo( totalMask );

                if ( mergePreviousDiff_ ) {
                   // Managing previous mask and merging it to total
                   if ( pPreviousMask_ ) {
                      cv::bitwise_and( *pPreviousMask_, totalMask, totalMask );
                   } else {
                      pPreviousMask_ = new cv::Mat( totalMask.size(), totalMask.type() );
                   }
                   foregroundMask.copyTo( *pPreviousMask_ );
                }

                cv::erode( totalMask, totalMask, getStructuringElement( MORPH_RECT, Size(9,9), Point(5,5) ));
                addToBackground( getBeforeFrame(), totalMask, ax_, ay_ );
            
                ax_ += dx;
                ay_ += dy;
            
                Mat beforeFrame_Masked(getBeforeFrame().size(), getBeforeFrame().type(), cv::Scalar(0,255,0));
                getBeforeFrame().copyTo( beforeFrame_Masked, totalMask );
                imshow("binary", beforeFrame_Masked );
             }
             trajectory_.push_back( Vec2f( dx, dy ) );

             ImageProcessor::process( frame, dropped );
            
             return true;
          }
          virtual std::string getTitle() const override { return "Processing"; }

          const cv::Mat getResult() const {
             cv::Mat resultImage( segmentedBackground_.size(), CV_8UC3);
             segmentedBackground_.convertTo( resultImage, CV_8UC3);
             return resultImage;
          }

       private:
          // Roboust solution for calculating the shift between two frames after each other
          cv::Mat calculateShift( const Mat& before, const Mat& after, short int& rx, short int& ry ) {
             // Creating the diff image, converting it to binary, then dilate a bit -> filtering out areas with exactly the same pixels
             cv::Mat binaryMaskMat(before.size(), CV_8U);
             if ( pStaticBackground_ ) {
                pStaticBackground_->copyTo( binaryMaskMat );
                cv::bitwise_not( binaryMaskMat, binaryMaskMat );
             } else {
                cv::Mat diff(before.size(), before.type());
                cv::absdiff(before, after, diff);
                cv::Mat grayscaleMat( before.size(), CV_8U);
                cv::cvtColor( diff, grayscaleMat, CV_BGR2GRAY );
                cv::threshold(grayscaleMat, binaryMaskMat, 1, 255, cv::THRESH_BINARY);
                cv::dilate( binaryMaskMat, binaryMaskMat, getStructuringElement( MORPH_RECT, Size(7,7), Point(3,3) ));
             }
  
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
                   if ( pixels < minimum ) {
                      rx = -ix; // sorry, I wrote the entire logic in the opposite way and I don't feel like to rewrite everything
                      ry = -iy;
                      minimum = pixels;
                      diff.copyTo( diffStored );
                   }
                }
             }


             cv::Mat diffStoredBW( after.size(), CV_8U, cvScalar(0.) );
             if ( rx == 0 && ry == 0 ) {
                return diffStoredBW;
             }

             cv::threshold( diffStored, diffStoredBW, 0, 255, THRESH_BINARY ); // threshold: 0, everything that was not in the last round
             cv::bitwise_not( diffStoredBW, diffStoredBW );
             cv::bitwise_and( binaryMaskMat, diffStoredBW, diffStoredBW );
  
             return diffStoredBW;
          }

          void addToBackground( const Mat& img, const Mat& mask, short int posx, short int posy ) {
             for(int y=0;y<img.rows;y++) {
                for(int x=0;x<img.cols;x++) {
                   if ( mask.at<unsigned char>(y, x) == 255 ) {
                      const int px = bigMapRadius_ + x + posx;
                      const int py = bigMapRadius_ + y + posy;
                      if ( 0 <= px && px < numOfSamplesInAverage_.cols && 0 <= py && py < numOfSamplesInAverage_.rows ) {
                         unsigned char& numOfSamples = numOfSamplesInAverage_.at<unsigned char>( py, px );
                         if ( numOfSamples < maxNumOfSamplesInAverageImage_ ) {
                            for (short int i = 0; i < 3; i++) { 
                               double dNumOfSamples = static_cast<double>( numOfSamples );
                               double& elem = segmentedBackground_.at<Vec3d>(py,px)[i];
                               elem = elem * dNumOfSamples/ ( dNumOfSamples + 1. ) + double ( img.at<Vec3b>(y,x)[i] ) * 1. / ( dNumOfSamples + 1. );
                            }
                            numOfSamples++;
                         }
                      }
                   }
                }
             }
          }

       private:
          std::vector<Vec2f>& trajectory_;
          const cv::Mat* pStaticBackground_;
          cv::Mat* pPreviousMask_;
          unsigned char maxNumOfSamplesInAverageImage_;
          int bigMapRadius_;
          short int maxstep_;
          const bool mergePreviousDiff_;

          cv::Mat segmentedBackground_;
          cv::Mat numOfSamplesInAverage_; 
          int ax_ = 0;
          int ay_ = 0;
    };

    class CarProcessor : public ImageProcessor {
       public:
          CarProcessor( const std::vector<Vec2f>& trajectory,
                        const cv::Mat& background,
                        const cv::Mat& sbpResult)
           : trajectory_( trajectory ), background_( background ), sbpResult_( sbpResult ), center_( sbpResult.cols / 2, sbpResult.rows / 2 ), radius_( ( background.cols - 1 ) / 2 ), ax_( radius_ ), ay_( radius_ )
          {}

          virtual bool process( const cv::Mat& frame, bool dropped ) override {
             short int dx = trajectory_[ index_ ][0];
             short int dy = trajectory_[ index_ ][1];
             ax_ += dx;
             ay_ += dy;
             index_++;

             if ( !dropped ) {
                if (frame.empty()) {
                    return false;
                }
                cv::Mat backgroundSlice = background_( cv::Rect(ax_, ay_, 320, 200) );

                // creating diff in HSV
                cv::Mat hsvFrame( frame.size(), CV_8UC3 );
                cv::cvtColor( frame, hsvFrame, CV_RGB2HSV );
                cv::Mat hsvBackgroundSlice( backgroundSlice.size(), CV_8UC3 );
                cv::cvtColor( backgroundSlice, hsvBackgroundSlice, CV_RGB2HSV );
                cv::Mat diff(frame.size(), frame.type());
                cv::absdiff( hsvFrame, hsvBackgroundSlice, diff );

                // breaking it into channels
                std::vector<cv::Mat> diffChannels(3);
                split(diff, diffChannels);

                // threshold
                cv::Mat binaryMaskMat(frame.size(), CV_8U);
                cv::threshold(diffChannels[0], binaryMaskMat, 20, 255, cv::THRESH_BINARY);
                cv::bitwise_not( binaryMaskMat, binaryMaskMat );
                cv::bitwise_or( binaryMaskMat, sbpResult_, binaryMaskMat );
                cv::bitwise_not( binaryMaskMat, binaryMaskMat );

                // detecting our blob
                cv::Point elem = findNearestBlobInBinaryImage( binaryMaskMat, center_ );

                if ( elem != cv::Point( 0, 0 ) ) {
                   cv::floodFill( binaryMaskMat, elem, cvScalar(127.0) );  
                   cv::Point2d centroid = calculateCentroid( binaryMaskMat );

                   // orientation
                   const double rawAngle = 0.5 * atan( 2.0 * calculateMoment( binaryMaskMat, centroid, 1, 1 ) / ( calculateMoment( binaryMaskMat, centroid, 2, 0 ) - calculateMoment( binaryMaskMat, centroid, 0, 2 ) ) );
                   const double signOfAngle = calculateSignOfAngle( binaryMaskMat, centroid, rawAngle );
                   const double jOfAngle = calculateJ( binaryMaskMat, centroid, rawAngle, signOfAngle );

                   const double PI = 3.141592653589793;
                   const double angle = signOfAngle * rawAngle + PI / 2. * jOfAngle;

                   // preserving important data
                   angles_.push_back( angle );
                   places_.push_back( cv::Point2d( dx + centroid.x, dy + centroid.y ) );

                   center_ = centroid;

                   cv::Point rad( 5, 5 );
                   cv::rectangle( binaryMaskMat, center_ - rad, center_ + rad, cvScalar(255.0) );
                   
                   for ( int k = 0; k < 2; ++k ) {
                      cv::Point2d dir ( cos( angle + PI * k ), sin( angle + PI * k ) );
                      cv::line( binaryMaskMat, center_, center_ + cv::Point( 30. * dir ), cvScalar(255.0) );
                   }

                } else {
                   center_ = cv::Point( frame.cols / 2, frame.rows / 2 );
                }
                imshow("binary" , binaryMaskMat );
             } 

             ImageProcessor::process( frame, dropped );
            
             return true;
          }
          virtual std::string getTitle() const override { return "Processing"; }
          void getResult( std::vector<cv::Point2d>& places, std::vector<double>& angles ) const {
             places = places_;
             angles = angles_;
          }

       private:
          cv::Point findNearestBlobInBinaryImage( const cv::Mat& img, const cv::Point center ) {
             const int border = 20;
             int cx = center.x;
             int cy = center.y;
             int maxrad = ( cx < cy ? cx : cy );

             for ( int radius = 0; radius < maxrad; ++radius ) {
                for ( int x = cx - radius; x <= cx + radius; ++x ) {
                   for ( int y = cy - radius; y <= cy + radius; ++y ) {
                      if ( border < x && x < img.cols - border &&
                           border < y && y < img.rows - border ) {
                         if ( img.at<unsigned char>( y, x ) == 255 ) {
                            return cv::Point( x, y );
                         }
                      }
                   }
                }
             }
             return cv::Point( 0, 0 );
          }

          cv::Point2d calculateCentroid( const cv::Mat& img ) {
             double sumx = 0.0;
             double sumy = 0.0;
             long num = 0;

             for ( int x = 0; x < img.cols; ++x ) {
                for ( int y = 0; y < img.rows; ++y ) {
                   if ( img.at<unsigned char>( y, x ) == 127 ) {
                      sumx += x;
                      sumy += y;
                      ++num;
                   }
                }
             }

             return cv::Point2d( ( sumx / num ), ( sumy / num ) );
          }

          double calculateMoment( const cv::Mat& img, const cv::Point& centroid, double ordX, double ordY ) {
             double sum = 0.0;
             long num = 0;

             for ( int x = 0; x < img.cols; ++x ) {
                for ( int y = 0; y < img.rows; ++y ) {
                   if ( img.at<unsigned char>( y, x ) == 127 ) {
                      sum += pow( x - centroid.x, ordX ) * pow( y - centroid.y, ordY );
                      ++num;
                   }
                }
             }

             return sum / num;
          }

          double calculateSignOfAngle( const cv::Mat& img, const cv::Point2d& centroid, double angle ) {
             const double PI = 3.141592653589793;
             int besti = 0;
             long bestintersect = 0;
             for ( int i = 0; i <= 1; ++i ) {
                long intersect = 0;
                for ( int j = 0; j < 4; ++j ) {
                   cv::Point2d dir ( cos( static_cast<double>( i* 2.0 - 1.0 ) * angle + PI / 2. * j), sin( static_cast<double>( i* 2.0 - 1.0 ) * angle + PI / 2. * j) );
                   intersect +=  calculateMirrorIntersect( img, centroid, dir );
                }
                if ( intersect > bestintersect ) {
                   bestintersect = intersect;
                   besti = i;
                }
             }
             return static_cast<double>(besti) * 2. - 1.;
          }

          double calculateJ( const cv::Mat& img, const cv::Point2d& centroid, double angle, double signOfAngle ) {
             const double PI = 3.141592653589793;
             double maxLen = 0.;
             int maxJ = 0;
             for ( int j = 0; j < 2; ++j ) {
                cv::Point2d dir ( cos( signOfAngle * angle + PI / 2. * j), sin( signOfAngle * angle + PI / 2. * j ) );
                const double len = calculateLen( img, centroid, dir );
                if ( len > maxLen ) {
                   maxLen = len;
                   maxJ = j;
                }
             }
             return maxJ;
          }


          long calculateMirrorIntersect( const cv::Mat& img, const cv::Point2d& centroid, const cv::Point2d& mir ) {
             long intersect = 0;

             for ( int x = 0; x < img.cols; ++x ) {
                for ( int y = 0; y < img.rows; ++y ) {
                   if ( img.at<unsigned char>( y, x ) == 127 ) {
                      cv::Point2d dir( x - centroid.x, y - centroid.y );
                      const double dot = dir.x * mir.x + dir.y * mir.y;
                      cv::Point2d target = 2.0 * dot * mir - dir;

                      if ( img.at<unsigned char>( target.y + centroid.y, target.x + centroid.x ) == 127 ) {
                         ++intersect;
                      }
                   }
                }
             }

             return intersect;
          }

          long calculateLen( const cv::Mat& img, const cv::Point2d& centroid, cv::Point2d mir ) {
             double len = 0;

             for ( int x = 0; x < img.cols; ++x ) {
                for ( int y = 0; y < img.rows; ++y ) {
                   if ( img.at<unsigned char>( y, x ) == 127 ) {
                      cv::Point2d dir( x - centroid.x, y - centroid.y );
                      const double dot = dir.x * mir.x + dir.y * mir.y;
                      if ( fabs( dot ) > len ) {
                         len = fabs( dot );
                      }
                   }
                }
             }

             return len;
          }

          const std::vector<Vec2f>& trajectory_;
          const cv::Mat& background_;
          const cv::Mat& sbpResult_;
          cv::Point center_;
          int radius_;
          int ax_;
          int ay_;

          int index_ = 0;
          std::vector<double> angles_;
          std::vector<cv::Point2d> places_;
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

       if ( processShell(capture, sbp) ) {
           return 0;
       }
       capture.release();

    }
    cv::Mat sbpResult = sbp.getResult();

    std::vector<Vec2f> trajectory;
    DynamicBackgroundProcessor dbp( trajectory, &sbpResult );
    {
       VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file
       if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
           capture.open(atoi(arg.c_str()));
       if (!capture.isOpened()) {
           cerr << "Failed to open a video device or video file!\n" << endl;
           help(av);
           return 1;
       }

       if ( processShell(capture, dbp) ) {
          return 0;
       }

       capture.release();
    } 
    cv::Mat dbpResult = dbp.getResult();

    CarProcessor cp( trajectory, dbpResult, sbpResult );
    {
       VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file
       if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
           capture.open(atoi(arg.c_str()));
       if (!capture.isOpened()) {
           cerr << "Failed to open a video device or video file!\n" << endl;
           help(av);
           return 1;
       }

       if ( processShell(capture, cp) ) {
          return 0;
       }

       capture.release();
    } 

    // printing the results
    std::cout << "X Y ANGLE" << std::endl;
    std::vector<cv::Point2d> places;
    std::vector<double>    angle;
    cp.getResult( places, angle );
    for ( int i = 0; i < places.size(); ++i ) {
       std::cout << std::fixed << std::setprecision(5) << places[i].x << " " << places[i].y << " " << angle[i] << std::endl;
    }

    return 0;
}
