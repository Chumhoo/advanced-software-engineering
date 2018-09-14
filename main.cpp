//
//  main.cpp
//  opencvTest
//
//  Created by chumhoo on 16/8/19.
//  Copyright © 2016年 chumhoo. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
#include "Video.hpp"
using namespace std;
using namespace cv;

Mat drawHist(Mat hist,int bins,int height,Scalar rgb)
{
    double maxVal = 0;
    minMaxLoc(hist, 0, &maxVal, 0, 0);
    int scale=1;
    Mat histImg = Mat::zeros(height, bins, CV_8UC3);
    float *binVal = hist.ptr<float>(0);
    for (int i = 0; i < bins; i++)
    {
        int intensity = cvRound(binVal[i] * height / maxVal);
        rectangle(histImg,
                  Point(i*scale,0), Point((i+1)*scale, (intensity)),
                  rgb, CV_FILLED);
    }
    flip(histImg,histImg,0);
    return histImg;
}

Mat drawHistRGB(const Mat& src)
{
    Mat histB,histG,histR;
    
    int histRGBHeight = 200;
    int bins = 256;
    int histSize[] = {bins};
    float range[] = {0,256};
    const float* ranges[] = {range};
    int channelsB[] = {0};
    int channelsG[] = {1};
    int channelsR[] = {2};
    
    calcHist(&src,1,channelsB,Mat(),histB,1,histSize,ranges,true,false);
    calcHist(&src,1,channelsG,Mat(),histG,1,histSize,ranges,true,false);
    calcHist(&src,1,channelsR,Mat(),histR,1,histSize,ranges,true,false);
    
    Mat histBImg = drawHist(histB,bins,histRGBHeight,Scalar(255,0,0));
    Mat histGImg = drawHist(histG,bins,histRGBHeight,Scalar(0,255,0));
    Mat histRImg = drawHist(histR,bins,histRGBHeight,Scalar(0,0,255));
    
    //在一个窗口中显示多幅图像
    Mat display(histRGBHeight, bins * 3, CV_8UC3);
    Mat displayROI = display(Rect(0,0,bins,histRGBHeight));
    resize(histBImg,displayROI,displayROI.size());
    displayROI = display(Rect(bins,0,bins,histRGBHeight));
    resize(histGImg,displayROI,displayROI.size());
    displayROI = display(Rect(bins * 2,0,bins,histRGBHeight));
    resize(histRImg,displayROI,displayROI.size());
    
    return display;
    //    imshow("histRGB",display);
}

Mat drawHistHS( const Mat& src)
{
    Mat hsv;
    
    cvtColor(src, hsv, CV_BGR2HSV);
    
    // Quantize the hue to 30 levels
    // and the saturation to 32 levels
    int hbins = 30, sbins = 32;
    int histSize[] = {hbins, sbins};
    // hue varies from 0 to 179, see cvtColor
    float hranges[] = { 0, 180 };
    // saturation varies from 0 (black-gray-white) to
    // 255 (pure spectrum color)
    float sranges[] = { 0, 256 };
    const float* ranges[] = { hranges, sranges };
    MatND hist;
    // we compute the histogram from the 0-th and 1-st channels
    int channels[] = {0, 1};
    
    calcHist( &hsv, 1, channels, Mat(), // do not use mask
             hist, 2, histSize, ranges,
             true, // the histogram is uniform
             false );
    double maxVal=0;
    minMaxLoc(hist, 0, &maxVal, 0, 0);
    
    int scale = 10;
    Mat histImg = Mat::zeros(sbins * scale, hbins * scale, CV_8UC3);
    
    for( int h = 0; h < hbins; h++ )
        for( int s = 0; s < sbins; s++ )
        {
            float binVal = hist.at<float>(h, s);
            int intensity = cvRound(binVal*255/maxVal);
            rectangle( histImg, Point(h*scale, s*scale),
                      Point( (h+1)*scale - 1, (s+1)*scale - 1),
                      Scalar::all(intensity),
                      CV_FILLED );
            
        }
  
    return histImg;
}

//! 计算预览图位置
Rect calPosition(Point center, int frameW, int frameH, double compareRatio)
{
    Point p;
    Size s;
    double height = frameH, width = frameW;
    double maxW = 230, maxH = 140;
    if (1.0 * frameW / frameH > maxW / maxH)
    {
        height /= frameW / maxW;
        return Rect(Point(center.x - maxW/2, center.y - height/2 + height*(1-compareRatio)), Size(maxW, height * compareRatio));
    }
    else
    {
        width /= frameH / maxH;
        return Rect(Point(center.x - width/2, center.y - maxH/2 + maxH*(1-compareRatio)), Size(width, maxH * compareRatio));
    }
    
}
void mainProgram(int correctClipsNum, double compareRatio)
{
    int clip = 1, startFrame = 0;
    char clipFileName[512], text[256];
    bool stepPlay = false;
    char key ;
    bool methodPass[4], framePass;
    int methodCorrectNum[4] = {0, 0, 0, 0};
    int speedDelay = 0;
    double accuracy[4];
    
    Mat lineFrame(Size(1000, 600), CV_8UC3);
    lineFrame = Scalar::all(0);
    for (int i = 0; i < 100; i++)
    {
        line(lineFrame, Point(0, i*10), Point(1000, i * 10), Scalar(60, 60, 60));
        if (i % 10 == 0) line(lineFrame, Point(0, i*10), Point(1000, i * 10), Scalar(255, 255, 255));
    }
    
    //Video video(800, 600);
    Video video("src/Try1.mp4");
    
    snprintf(text, sizeof(text), "result/ratio_%f.txt", compareRatio);
    ofstream file(text);
    file << "- method\t\t1\t\t\t2\t\t\t3\t\t\t4 -" << endl;
    snprintf(text, sizeof(text), "result/summary_ratio_%f.txt", compareRatio);
    ofstream summary(text);
    summary << "- summary of " << compareRatio << " - " << endl;
    
    namedWindow("Video", WINDOW_AUTOSIZE);
    namedWindow("Info", WINDOW_AUTOSIZE);
    
    //定义视频的宽度和高度
    Size s(video.frameW(), video.frameH());
    //定义剪辑文件名
    sprintf(clipFileName, "clips/%.1f_%d.mp4", compareRatio, clip++);
    //创建 writer,并指定 FOURCC 及 FPS 等参数
    VideoWriter writer = VideoWriter(clipFileName, CV_FOURCC('m','p','4','v'), video.fps(), s);
    //检查是否成功创建
    if(!writer.isOpened())
    {
        cerr << "Can not create video file.\n" << endl;
        return;
    }
    
    Mat edges;
    createTrackbar("Speed Delay", "Video", &speedDelay, 100);
    //从 cap 中读一帧,存到 frame
    writer << video.readFrame();
    
    
    ///////////////////////////////////  run  //////////////////////////////////
    
    
    int frameNum;
    for (frameNum = 1; ; frameNum++)
    {
        video.readFrame();
        if (video.isEnd()) break;
        
        //将读到的图像转为灰度图
        //cvtColor(video.frame(), edges, CV_BGR2GRAY);
        //进行边缘提取操作
        //Canny(edges, edges, 100, 90, 3);
        //显示结果
        
        
        int infoFrameW = 800, infoFrameH = 600;
        Mat infoFrame(Size(infoFrameW, infoFrameH), CV_8UC3);
        infoFrame = Scalar::all(0);
        
        if (video.mode() == VIDEOFILE)
        {
            //将剪辑数绘到画面上
            snprintf(text, sizeof(text), "File name: %s | Total frames: %d | FPS: %d | Total time: %ds",
                     video.fileName(),
                     video.frameTotal(), video.fps(),
                     video.frameTotal() / video.fps());
            putText(infoFrame, text, Point(5, 30),
                    FONT_HERSHEY_DUPLEX, 0.6,
                    Scalar(255,255,255), 1.0, 4);
        }
        else
        {
            snprintf(text, sizeof(text), "Camera mode. Resolution: %d * %d", video.frameW(), video.frameH());
            putText(infoFrame, text, Point(5, 30),
                    FONT_HERSHEY_DUPLEX, 0.6,
                    Scalar(255,255,255), 1.0, 4);
        }
        line(infoFrame, Point(0, 50), Point(infoFrameW, 50), Scalar(100, 100, 100));
        line(infoFrame, Point(0, 280), Point(infoFrameW, 280), Scalar(100, 100, 100));
        line(infoFrame, Point(0, 450), Point(infoFrameW, 450), Scalar(100, 100, 100));
        line(infoFrame, Point(0, 535), Point(infoFrameW, 535), Scalar(100, 100, 100));
        
        snprintf(text, sizeof(text), "clip %d", clip-1);
        putText(infoFrame, text, Point(5, infoFrameH - 40),
                FONT_HERSHEY_DUPLEX, 0.6,
                Scalar(255,255,255), 1.5, 8);
        snprintf(text, sizeof(text), "Step play mode: ");
        if (stepPlay)
        {
            strcat(text, "ON");
            putText(infoFrame, text, Point(100, infoFrameH - 40), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0,255,0), 1.5, 8);
        }
        else
        {
            strcat(text, "OFF");
            putText(infoFrame, text, Point(100, infoFrameH - 40), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0,0,255), 1.5, 8);
        }
        snprintf(text, sizeof(text), "Speed delay: %d ms", speedDelay * 10);
        putText(infoFrame, text, Point(400, infoFrameH - 40), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0,0,255), 1, 1);
        
        snprintf(text, sizeof(text), "      HS hist         \
                 RGB hist");
        putText(infoFrame, text, Point(50, 260),
                FONT_HERSHEY_DUPLEX, 0.4,
                Scalar(255, 255, 255), 1.5, 8);
        
        
        ////////  cost time : 5~7ms  //////////////
        //在一个窗口中显示多幅图像
        Rect position = Rect(10, 70, 200, 170);
        Mat displayROI = infoFrame(position);
        resize(drawHistHS(video.frame()), displayROI, displayROI.size());
        rectangle(displayROI, Point(0, 0), Point(position.width, position.height), Scalar(255, 255, 255), 2);
        position = Rect(250, 70, 500, 170);
        displayROI = infoFrame(position);
        resize(drawHistRGB(video.frame()), displayROI, displayROI.size());
        rectangle(displayROI, Point(0, 0), Point(position.width, position.height), Scalar(255, 255, 255), 2);
        /////////////////////////////////////////
        
        
        ////////  cost time : 1~2ms //////////////
        //将进度绘到画面上
        rectangle(infoFrame, Point(10, infoFrameH - 30),
                  Point(infoFrameW - 10, infoFrameH - 10),
                  Scalar(255, 255, 255), 2);
        rectangle(infoFrame, Point(10, infoFrameH - 23),
                  Point(1.0 * (infoFrameW - 20) * frameNum / video.frameTotal() + 10, infoFrameH - 17),
                  Scalar(255, 255, 255), 10);
        snprintf(text, sizeof(text), "%.2f%%",
                 100.0 * frameNum / video.frameTotal());
        putText(infoFrame, text, Point(infoFrameW / 2 - 10, infoFrameH - 15),
                FONT_HERSHEY_DUPLEX, 0.4,
                Scalar(0,0,255), 1.5, 8);
        
        position = calPosition(Point(133, 365), video.frameW(), video.frameH(), 1.0);
        displayROI = infoFrame(position);
        resize(video.frame(), displayROI, displayROI.size());
        position = calPosition(Point(380, 365), video.frameW(), video.frameH(), compareRatio);
        displayROI = infoFrame(position);
        resize(video.halfFrame(compareRatio), displayROI, displayROI.size());
        position = calPosition(Point(627, 365), video.frameW(), video.frameH(), 1.0);
        displayROI = infoFrame(position);
        resize(video.oldFrame(), displayROI, displayROI.size());
        snprintf(text, sizeof(text), "self");
        putText(infoFrame, text, Point(120, 445), FONT_HERSHEY_DUPLEX, 0.4, Scalar(255, 255, 255), 1.5, 8);
        snprintf(text, sizeof(text), "half-self");
        putText(infoFrame, text, Point(350, 445), FONT_HERSHEY_DUPLEX, 0.4, Scalar(255, 255, 255), 1.5, 8);
        snprintf(text, sizeof(text), "last frame");
        putText(infoFrame, text, Point(600, 445), FONT_HERSHEY_DUPLEX, 0.4, Scalar(255, 255, 255), 1.5, 8);
        
        snprintf(text, sizeof(text), "self-self   self-half   self-test");
        putText(infoFrame, text, Point(100, 465),
                FONT_HERSHEY_DUPLEX, 0.4,
                Scalar(255,255,0), 1.5, 8);
        ////////////////////////////////////////////
        
        ////////  cost time : +-50ms //////////////
        for (int method = 0; method < 4; method++)
        {
            Vector<double> compareResult;
            Scalar color(0, 255, 0);
            
            methodPass[method] = false;
            
            if (abs(video.compareHists(method, compareRatio)[2] - video.compareHists(method, compareRatio)[0]) >
                abs(video.compareHists(method, compareRatio)[1] - video.compareHists(method, compareRatio)[0]))
            {
                methodPass[method] = false;
                color = Scalar(0, 0, 255);
            }
            else
            {
                methodPass[method] = true;
            }
            compareResult = video.compareHists(method, compareRatio);
            snprintf(text, sizeof(text), "method %d   %.3f       %.3f       %.3f",
                     method + 1, compareResult[0],
                     compareResult[1], compareResult[2]);
            putText(infoFrame, text, Point(20, 480 + method * 15),
                    FONT_HERSHEY_DUPLEX, 0.4,
                    color, 1.5, 8);
        }
        ///////////////////////////////////////////
        
        
        // time cost : 0ms  ///
        framePass = true;

        if (methodPass[3] == false && methodPass[0] == false &&
            methodPass[2] == false && methodPass[1] == false )
        {
            //            snprintf(text, sizeof(text), "Please judge is it a new clip? Anykey for true, N/n for false");
            //            putText(infoFrame, text, Point(infoFrameW - 600, infoFrameH - 40),
            //                    FONT_HERSHEY_DUPLEX, 0.5,
            //                    Scalar(0,255,255), 1.5, 8);
            //            imshow("Info", infoFrame);
            //            key = waitKey(0);
            //            if (key == 'n' || key == 'N') { framePass = true; }
            //            else
            {
                framePass = false;
                //定义剪辑文件名
                sprintf(clipFileName, "clips/%.1f_%d.mp4", compareRatio, clip++);
                //创建 writer,并指定 FOURCC 及 FPS 等参数
                writer = VideoWriter(clipFileName, CV_FOURCC('m','p','4','v'), 25, s);
                //检查是否成功创建
                if(!writer.isOpened())
                {
                    cerr << "Can not create video file.\n" << endl;
                    return;
                }
                startFrame = frameNum;
            }
        }
        snprintf(text, sizeof(text), "Accuracy");
        putText(infoFrame, text, Point(350, 465),
                FONT_HERSHEY_DUPLEX, 0.4,
                Scalar(0, 255, 255), 1.5, 8);
        
        file << "Frame " << frameNum << "/" << video.frameTotal() << "\t\t";
        
        for (int method = 0; method < 4; method++)
        {
            double tempAccuracy = accuracy[method];
            accuracy[method] = 100.0 * methodCorrectNum[method]/frameNum;
            if (methodPass[method] == framePass)
            {
                methodCorrectNum[method]++;
            }
            snprintf(text, sizeof(text), "%.2f%%", accuracy[method]);
            putText(infoFrame, text, Point(350, 480 + method * 15),
                    FONT_HERSHEY_DUPLEX, 0.4,
                    Scalar(0, 255, 255), 1.5, 8);
            file << accuracy[method] << "\t\t\t";
            Scalar color;
            switch(method)
            {
                case 0: color = Scalar(0, 255, 255); break;
                case 1: color = Scalar(0, 255, 0); break;
                case 2: color = Scalar(0, 0, 255); break;
                case 3: color = Scalar(255, 255, 255); break;
            }
            line(lineFrame, Point(frameNum-1, frameNum/1000*100 + 100 - tempAccuracy),
                 Point(frameNum, frameNum/1000*100 + 100 - accuracy[method]), color);
            if(tempAccuracy > 80 || accuracy[method] > 80)
                line(lineFrame, Point(frameNum-1, frameNum/1000*200 + 400 - (tempAccuracy-80)*10),
                     Point(frameNum, frameNum/1000*200 + 400 - (accuracy[method]-80)*10), color);
            if(tempAccuracy > 90 || accuracy[method] > 90)
                line(lineFrame, Point(frameNum-1, frameNum/1000*200 + 600 - (tempAccuracy-90)*20),
                     Point(frameNum, frameNum/1000*200 + 600 - (accuracy[method]-90)*20), color);
            if (frameNum % 80 == 0)
            {
                snprintf(text, sizeof(text), "%d:%.2f%%", method, accuracy[method]);
                putText(lineFrame, text,
                        Point(frameNum, 600 - 0.06 * accuracy[method] * accuracy[method]),
                        FONT_HERSHEY_DUPLEX, 0.4, color, 1.5, 8);
            }
        }
        file << endl;
        ///////////////////////////////////////////
        
        
        ///////////////// time cost : 30+ms   ////////
        imshow("Info", infoFrame);
        if (video.mode() == VIDEOFILE)
        {
            int min = startFrame/video.fps()/60, sec = startFrame/video.fps()%60;
            snprintf(text, sizeof(text), "Clip: %d  Start Time: %d'%d''%02.0f", clip-1,
                     min, sec, 100 * (1.0 * startFrame / video.fps() - 60 * min - sec));
            putText(video.frame(), text, Point(10, 10),
                    FONT_HERSHEY_DUPLEX, 0.4,
                    Scalar(255, 255, 255), 1.5, 8);
        }
        //将图像写入视频
        
        writer << video.frame();
        imshow("Video", video.frame());
        imshow("Graph", lineFrame);
        
        ///////////////////////////////////////////

        
        if (!stepPlay) key = (char)waitKey( speedDelay * 10 + 1 ); //delay N millis, usually long enough to display and capture input
        else key = (char) waitKey (0);
        switch (key) {
            case 's': case 'S':
                if (stepPlay) stepPlay = false;
                else stepPlay = true;
                break;
            case 'q':
            case 'Q':
            case 27: //escape key
                summary.close();
                file.close();
                writer.release();
                snprintf(text, sizeof(text), "result/Ratio_%f.jpg", compareRatio);
                imwrite(text, lineFrame);
                return;
            case 32: //space key
                waitKey(0);
                break;
            default:
                break;
        }
    }
    summary << "Four method:" << accuracy[0] << " " << accuracy[1] << " " <<  accuracy[2] << " " <<  accuracy[3] << endl;
    summary << "Total clips produced: " << clip << "\tCorrect Clip number: " << correctClipsNum <<  "  Accuracy: " << 1.0 * clip / correctClipsNum;
    snprintf(text, sizeof(text), "result/Ratio_%f.jpg", compareRatio);
    imwrite(text, lineFrame);
    summary.close();
    writer.release();
    file.close();
}

int main(int argc, char** argv)
{
//    for (double compareRatio = 0.4; compareRatio <= 1.0; compareRatio+=0.1)
    {
        mainProgram(15 , 0.5);
    }
    return 0;
}
