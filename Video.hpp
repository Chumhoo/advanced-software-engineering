//
//  Video.hpp
//  opencvTest
//
//  Created by chumhoo on 16/8/22.
//  Copyright © 2016年 chumhoo. All rights reserved.
//

#ifndef Video_hpp
#define Video_hpp

#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;
enum MODE{CAMERA, VIDEOFILE};

class Video{
private:
    char _fileName[256];
    int _frameH, _frameW;
    int _frameTotal, _frameNum;
    int _fps;
    VideoCapture cap;
    Mat _frame, _oldFrame;
    MODE _videoMode;

public:
    //! create a video with camera
    Video(int cameraWidth, int cameraHeight);
    //! for video file mode
    Video(const char *fileName);
    
    Mat readFrame();
    Mat& frame()     { return _frame; }
    Mat& oldFrame()  { return _oldFrame; }
    Mat halfFrame(double ratio);
    void operator>>( Mat &image );

    char* fileName();
    int frameH() const;
    int frameW() const;
    int frameTotal() const;
    int fps() const;
    int mode() const { return _videoMode; }
    
    bool isEnd()
    {
        if (_frame.empty()) return true;
        else return false;
    }
    
    //! compare the current frame and the old one, and return the values. compareRatio means how much the 'half frame' occupies the original frame
    Vector<double> compareHists(int method, double compareRatio);

    ~Video()
    {
        _frame.release();
        _oldFrame.release();
        cap.release();
    }
    
    
};

#endif /* Video_hpp */
