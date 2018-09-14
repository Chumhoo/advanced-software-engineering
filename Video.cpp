 //
//  Video.cpp
//  opencvTest
//
//  Created by chumhoo on 16/8/22.
//  Copyright © 2016年 chumhoo. All rights reserved.
//

#include "Video.hpp"

Video::Video(int cameraWidth, int cameraHeight)   //for the camera mode
{
    //打开第一个摄像头
    cap.open(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, cameraWidth);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,cameraHeight);
    _frameH = cameraHeight;
    _frameW = cameraWidth;
    _frameNum = 0;

    //检查是否成功打开
    if(!cap.isOpened())
    {
        cerr << "Can not open a camera or file." << endl;
    }
    else
    {
        cap.read(_frame);
        _frame.copyTo(_oldFrame);
    }
    _videoMode = CAMERA;
}

Video::Video(const char *fileName)
{
    strcpy(_fileName, fileName);
    //打开视频文件
    cap.open(fileName);
    
    CvCapture* pCapture = NULL;
    if( !(pCapture = cvCaptureFromFile(fileName)))
    {
        fprintf(stderr, "Can not open video file.\n");
    }
    _frameH = (int) cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_HEIGHT);
    _frameW = (int) cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_WIDTH);
    _fps         = (int) cvGetCaptureProperty(pCapture, CV_CAP_PROP_FPS);
    _frameTotal = (int) cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_COUNT);
    _frameNum = 0;
    
    //检查是否成功打开
    if(!cap.isOpened())
    {
        cerr << "Can not open a camera or file." << endl;
    }
    else
    {
        cap.read(_frame);
        _frame.copyTo(_oldFrame);
    }
    _videoMode = VIDEOFILE;
}

Mat Video::readFrame()
{
    if (_frame.empty())
    {
        cap.read(_frame);
        _frame.copyTo(_oldFrame);
    }
    else
    {
        _oldFrame.release();
        _frame.copyTo(_oldFrame);
        _frameNum++;
        _frame.release();
        cap.read(_frame);
    }
    return _frame;
}

void Video::operator>>( Mat &image )
{
    _oldFrame.release();
    _frame.copyTo(_oldFrame);
    _frameNum++;
    _frame.release();
    cap.read(_frame);
    _frame.copyTo(image);
}

char* Video::fileName()
{
    if (cap.isOpened()) return _fileName;
    else
    {
        cout << "The video hasn't been open!" << endl;
        return NULL;
    }
}
int Video::frameH() const
{
    if (cap.isOpened()) return _frameH;
    else
    {
        cout << "The video hasn't been open!" << endl;
        return -1;
    }
}
int Video::frameW() const
{
    if (cap.isOpened()) return _frameW;
    else
    {
        cout << "The video hasn't been open!" << endl;
        return -1;
    }
}
int Video::frameTotal() const
{
    if (cap.isOpened()) return _frameTotal;
    else
    {
        cout << "The video hasn't been open!" << endl;
        return -1;
    }
}

int Video::fps() const
{
    if (cap.isOpened()) return _fps;
    else
    {
        cout << "The video hasn't been open!" << endl;
        return -1;
    }
}

Mat Video::halfFrame(double ratio)
{
    return _frame( Range( _frame.rows*(1-ratio), _frame.rows - 1 ), Range( 0, _frame.cols - 1 ) );
}


Vector<double> Video::compareHists(int method, double compareRatio)
{
    Vector<double> result;
    Mat src_base;
    Mat src_test;
    Mat hsv_base, hsv_test, hsv_half;
    
    _oldFrame.copyTo(src_base);
    _frame.copyTo(src_test);
    
    /// 转换到 HSV
    cvtColor( src_base, hsv_base, CV_BGR2HSV );
    cvtColor( src_test, hsv_test, CV_BGR2HSV );
    
    hsv_half = hsv_base( Range( hsv_base.rows*(1-compareRatio), hsv_base.rows - 1 ), Range( 0, hsv_base.cols - 1 ) );
    
    /// 对hue通道使用30个bin,对saturatoin通道使用32个bin
    int h_bins = 50; int s_bins = 60;
    int histSize[] = { h_bins, s_bins };
    
    // hue的取值范围从0到256, saturation取值范围从0到180
    float h_ranges[] = { 0, 256 };
    float s_ranges[] = { 0, 180 };
    
    const float* ranges[] = { h_ranges, s_ranges };
    
    // 使用第0和第1通道
    int channels[] = { 0, 1 };
    
    /// 直方图
    MatND hist_base;
    MatND hist_half_down;
    MatND hist_test;
    
    /// 计算HSV图像的直方图
    calcHist( &hsv_base, 1, channels, Mat(), hist_base, 2, histSize, ranges, true, false );
    normalize( hist_base, hist_base, 0, 1, NORM_MINMAX, -1, Mat() );
    
    calcHist( &hsv_half, 1, channels, Mat(), hist_half_down, 2, histSize, ranges, true, false );
    normalize( hist_half_down, hist_half_down, 0, 1, NORM_MINMAX, -1, Mat() );
    
    calcHist( &hsv_test, 1, channels, Mat(), hist_test, 2, histSize, ranges, true, false );
    normalize( hist_test, hist_test, 0, 1, NORM_MINMAX, -1, Mat() );
    
    ///应用不同的直方图对比方法
    double base_base = compareHist( hist_base, hist_base, method );
    double base_half = compareHist( hist_base, hist_half_down, method );
    double base_test = compareHist( hist_base, hist_test, method );
    
    result.push_back(base_base);
    result.push_back(base_half);
    result.push_back(base_test);
    return result;
}
