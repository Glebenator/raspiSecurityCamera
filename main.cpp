#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "peripherals.hpp"

using namespace cv;
using namespace std;

mutex frame_mutex;
condition_variable frame_cond;
queue<Mat> frame_queue;
bool finished = false;
mutex processed_frame_mutex;
mutex motion_frames_mutex;
condition_variable processed_frame_cond;
queue<Mat> processed_frame_queue;
queue<Mat> motion_frames_queue;

CascadeClassifier fullbody_cascade;

atomic<bool> body_detected{false};
const int DETECTION_THRESHOLD = 2;
const double MIN_CONTOUR_AREA = 500;

enum ButtonState {
    Idle,
    ButtonPressed,
    Flashing
};

template<typename T>
void clearQueue(queue<T>& q) {
    queue<T> empty;
    swap(q, empty);
}

void bodyDetection() {
    int consecutiveDetections = 0;
    namedWindow("Body Detected - Live", WINDOW_AUTOSIZE); // Create the body detection display window

    while (true) {
        unique_lock<mutex> lock(motion_frames_mutex);
        if (motion_frames_queue.empty()) {
            if (finished) {
                lock.unlock();
                break;
            }
            lock.unlock();
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        printf("motion frame size: %d\n", motion_frames_queue.size());
        Mat frame = motion_frames_queue.back();
        motion_frames_queue.pop();
        lock.unlock();

        if (consecutiveDetections < DETECTION_THRESHOLD) {
            vector<Rect> bodies;
            fullbody_cascade.detectMultiScale(frame, bodies);

            if (!bodies.empty()) {
                consecutiveDetections++;
                
                for (const auto& body : bodies) {
                    // Draw a rectangle around the detected body
                    rectangle(frame, body, Scalar(0, 255, 0), 3);
                }
                
                // Display the frame in the body detection window
                imshow("Body Detected - Live", frame);
                waitKey(1); // Minimal wait time, so it does not block execution

                // Optionally do something with the detected bodies
            } else {
                consecutiveDetections = 0;
            }
        } else {
            body_detected.store(true);
            lock.lock();
            // while (!motion_frames_queue.empty()) {
            //     motion_frames_queue.pop();
            // }
            clearQueue(motion_frames_queue);
            lock.unlock();
            consecutiveDetections = 0;
            
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    destroyWindow("Body Detected - Live"); // Destroy the window when finished
}

queue<Mat> detectMotion(queue<Mat>& frames) {
    queue<Mat> motionFrames;
    Mat prevFrame, currentFrame, diffFrame, threshFrame;

    // Ensure there are enough frames to compare
    if (frames.size() >= 2) {
        // Initial setup for the first frame
        prevFrame = frames.front();
        cvtColor(prevFrame, prevFrame, COLOR_BGR2GRAY);
        GaussianBlur(prevFrame, prevFrame, Size(21, 21), 0);
        frames.pop();

        // Process the queue for motion detection
        while (!frames.empty()) {
            currentFrame = frames.front();
            frames.pop();
            cvtColor(currentFrame, currentFrame, COLOR_BGR2GRAY);
            GaussianBlur(currentFrame, currentFrame, Size(21, 21), 0);
            absdiff(prevFrame, currentFrame, diffFrame);
            threshold(diffFrame, threshFrame, 25, 255, THRESH_BINARY);
            dilate(threshFrame, threshFrame, Mat(), Point(-1, -1), 2);
            vector<vector<Point>> contours;
            findContours(threshFrame, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            for (const auto& contour : contours) {
                // If the contour is bigger than the threshold, consider it motion
                if (contourArea(contour) > MIN_CONTOUR_AREA) {
                    // cout << "motion detected! "<< endl;
                    motionFrames.push(currentFrame);
                    break;  // Stop looking through contours once motion is detected
                }
            }
            prevFrame = currentFrame;
        }
    }
    return motionFrames;
}
void analysis() {
    queue<Mat> localQueue;
    int frameCounter = 0;  // Use a frame counter instead of a separate queue

    while (true) {
        // We might be waiting on the condition variable for new frames to process.
        unique_lock<mutex> lock(frame_mutex);
        frame_cond.wait(lock, [] { return !frame_queue.empty() || finished; });

        while (!frame_queue.empty()) {
            Mat frame = frame_queue.front();
            
            localQueue.push(frame);   // Push the frame to the local queue for motion detection
            frame_queue.pop();

            // Push all frames directly to the processed_frame_queue for display
            unique_lock<mutex> processed_lock(processed_frame_mutex);
            processed_frame_queue.push(frame);
            processed_lock.unlock();
            processed_frame_cond.notify_one();
        }
        
        lock.unlock(); // Unlock as soon as possible to not hold up other operations

        // Now, perform motion detection every other frame. 
        // This keeps the motion detection logic independent of the display logic.
        if (++frameCounter % 2 == 0 && localQueue.size() >= 2) {
            queue<Mat> motionFrames = detectMotion(localQueue);
            
            // Store frames with motion detected in a separate queue
            unique_lock<mutex> motion_lock(motion_frames_mutex);
            while (!motionFrames.empty()) {
                Mat frameWithMotion = motionFrames.front();
                motionFrames.pop();
                // Here, you could choose to perform further operations on framesWithMotion
                // For now, we just store them
                motion_frames_queue.push(frameWithMotion);
            }
            motion_lock.unlock();

            // Clear the localQueue and store the last frame for the next motion detection
            while (localQueue.size() > 1) {
                localQueue.pop();
            }
        }

        if (finished && frame_queue.empty()) {
            // If finished is true and frame_queue is empty, it's time to exit.
            break;
        }
    }

    // Notify the display thread that the analysis work has completed, and there are no more frames coming.
    processed_frame_cond.notify_all();
}

void captureVideo() {
    VideoCapture capture;
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_V4L2;       // 0 = autodetect default API
    // open selected camera using selected API
    capture.open(deviceID, apiID);
    if (!capture.isOpened()) {
        cerr << "--(!)Error opening video capture\n";
        finished = true;
        frame_cond.notify_one();
        return;
    }

    Mat frame;
    while (true) {
        capture.read(frame);
        unique_lock<mutex> lock(frame_mutex);
        frame_queue.push(frame.clone());
        lock.unlock();
        frame_cond.notify_one();
        if (waitKey(1) >= 0)
            break;
    }
    finished = true;
    frame_cond.notify_one();
}

void displayVideo() {
    namedWindow("Capture - Live", WINDOW_AUTOSIZE);
    while (true) {
        unique_lock<mutex> lock(processed_frame_mutex);
        // Wait for a processed frame or for the analysis to finish
        processed_frame_cond.wait(lock, [] { return !processed_frame_queue.empty() || finished; });

        if (finished && processed_frame_queue.empty()) {
            break;
        }

        Mat displayFrame = processed_frame_queue.front();
        processed_frame_queue.pop();
        lock.unlock(); // Unlock as soon as possible

        if (!displayFrame.empty()) {
            imshow("Capture - Live", displayFrame);
            if (waitKey(1) >= 0) {
                finished = true;
            }
        }
    }
    destroyWindow("Capture - Live");
}

int main() {
    if (!fullbody_cascade.load("/home/pi/opencv/data/haarcascades/haarcascade_frontalface_alt.xml")) {
    cerr << "--(!)Error loading full body cascade\n";
    return -1;
    }
    exportAllGpioPin();
    openFiles();

    ButtonState buttonState = Idle;
     bool buttonPressedAgain = false;
    cout << "starting! " << endl;


    // Start video capture thread
    thread capture_thread(captureVideo);
    // Start analysis thread
    thread analysis_thread(analysis);
     // start body detection thread
    thread body_detection_thread(bodyDetection);
    // Start display thread
    thread display_thread(displayVideo);

   while(true){
   int buttonValue = returnButtonValue();
        switch (buttonState) {
            case Idle:
                if (buttonValue == 1) {
                    buttonState = ButtonPressed;
                    buttonPressedAgain = false;
                }
                break;

            case ButtonPressed:
                if (buttonValue == 0) {
                    buttonState = Flashing;
                }
                break;

            case Flashing:
                // Flash LEDs and buzzer
                flashingRed(50000);
                if (body_detected.load()){
                    flashingBuzzer(50000);
                    // sleep(5);
                    body_detected.store(false);
                }

                // Check if the button is pressed again to stop flashing
                if (buttonValue == 1) {
                    buttonPressedAgain = true;
                }

                // Check if the button is released to go back to Idle
                if (buttonValue == 0 && buttonPressedAgain) {
                    buttonState = Idle;
                }
                break;
        }
   }

    // Wait for threads to finish
    capture_thread.join();
    analysis_thread.join();
    body_detection_thread.join();
    display_thread.join();

    return 0;
}