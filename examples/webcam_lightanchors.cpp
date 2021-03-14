#include <iostream>
#include <time.h>

#include "opencv2/opencv.hpp"

extern "C" {
#include "apriltag.h"
#include "tag36h11.h"

#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"

#include "lightanchor.h"
#include "lightanchor_detector.h"
}

using namespace std;
using namespace cv;


int main(int argc, char *argv[])
{
    getopt_t *getopt = getopt_create();

    getopt_add_bool(getopt, 'h', "help", 0, "Show this help");
    getopt_add_bool(getopt, 'd', "debug", 0, "Enable debugging output (slow)");
    getopt_add_bool(getopt, 'q', "quiet", 0, "Reduce output");
    getopt_add_string(getopt, 'f', "family", "tag36h11", "Tag family to use");
    getopt_add_int(getopt, 't', "threads", "1", "Use this many CPU threads");
    getopt_add_double(getopt, 'x', "decimate", "2.0", "Decimate input image by this factor");
    getopt_add_double(getopt, 'b', "blur", "0.0", "Apply low-pass blur to input; negative sharpens");
    getopt_add_bool(getopt, '0', "refine-edges", 1, "Spend more time trying to align edges of tags");

    if (!getopt_parse(getopt, argc, argv, 1) || getopt_get_bool(getopt, "help")) {
        printf("Usage: %s [options]\n", argv[0]);
        getopt_do_usage(getopt);
        exit(0);
    }

    // Initialize camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Couldn't open video capture device" << endl;
        return -1;
    }
    // cap.set(CV_CAP_PROP_FPS, 30);
    cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

    double fps = cap.get(CAP_PROP_FPS);
    cout << "FPS using CAP_PROP_FPS: " << fps << endl;

    // Initialize tag detector with options
    apriltag_family_t *tf = NULL;
    const char *famname = getopt_get_string(getopt, "family");
    if (!strcmp(famname, "tag36h11")) {
        tf = tag36h11_create();
    } else {
        printf("Unsupported tag family name. This example only supports \"tag36h11\".\n");
        exit(-1);
    }

    apriltag_detector_t *td = apriltag_detector_create();
    apriltag_detector_add_family(td, tf);
    td->quad_decimate = getopt_get_double(getopt, "decimate");
    td->quad_sigma = getopt_get_double(getopt, "blur");
    td->nthreads = getopt_get_int(getopt, "threads");
    td->debug = getopt_get_bool(getopt, "debug");
    td->refine_edges = getopt_get_bool(getopt, "refine-edges");

    lightanchor_detector_t *ld = lightanchor_detector_create();
    lightanchor_detector_add_code(ld, 0xaf);

    int frames = 0;

    time_t start, end;
    time(&start);

    Mat frame, gray;
    while (true) {
        cap >> frame;
        if (frame.empty())
            break;

        frames++;

        cvtColor(frame, gray, COLOR_BGR2GRAY);

        // // Make an image_u8_t header for the Mat data
        image_u8_t im = {
            .width = gray.cols,
            .height = gray.rows,
            .stride = gray.cols,
            .buf = gray.data
        };

        zarray_t *quads = detect_quads(td, &im);

        zarray_t *lightanchors = decode_tags(td, ld, quads, &im);
        // cout << zarray_size(lightanchors) << " possible lightanchors detected" << endl;

        // Draw quad outlines
        for (int i = 0; i < zarray_size(lightanchors); i++) {
            lightanchor_t *lightanchor;
            zarray_get(lightanchors, i, &lightanchor);

            line(frame, Point(lightanchor->p[0][0], lightanchor->p[0][1]),
                    Point(lightanchor->p[1][0], lightanchor->p[1][1]),
                    Scalar(0xff, 0, 0), 1);
            line(frame, Point(lightanchor->p[0][0], lightanchor->p[0][1]),
                    Point(lightanchor->p[3][0], lightanchor->p[3][1]),
                    Scalar(0xff, 0, 0), 1);
            line(frame, Point(lightanchor->p[1][0], lightanchor->p[1][1]),
                    Point(lightanchor->p[2][0], lightanchor->p[2][1]),
                    Scalar(0xff, 0, 0), 1);
            line(frame, Point(lightanchor->p[2][0], lightanchor->p[2][1]),
                    Point(lightanchor->p[3][0], lightanchor->p[3][1]),
                    Scalar(0xff, 0, 0), 1);
            circle(frame, Point(lightanchor->c[0], lightanchor->c[1]), 1,
                   Scalar(0, 0, 0xff), 2);
            stringstream brightness;
            brightness << "0x" << hex << +(int)(lightanchor->valid & 0xffff);
            putText(frame, brightness.str(), Point(lightanchor->c[0], lightanchor->c[1]),
                    FONT_HERSHEY_DUPLEX, 0.5,
                    Scalar(0, 0, 0xff), 1);
        }

        imshow("Lightanchor Detections", frame);

        if (waitKey(30) >= 0)
            break;
    }

    time(&end);

    double seconds = difftime(end, start);
    fps = frames / seconds;
    cout << "Approx FPS: " << fps << endl;

    apriltag_detector_destroy(td);

    lightanchor_detector_destroy(ld);

    tag36h11_destroy(tf);

    getopt_destroy(getopt);

    return 0;
}
