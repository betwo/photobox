#include "director.h"

#include "camera.h"

#include <chrono>
#include <thread>

Director::Director(EOSCamera& cam)
    : cam(cam), running(true), is_preview_running(false), is_picture_requested(false)
{

}

void Director::stop()
{
    running = false;
}

void Director::takePicture()
{
    is_picture_requested = true;

    {
        std::unique_lock<std::mutex> lock(mutex);
        while(is_preview_running) {
            cond_picture_possible.wait(lock);
        }
    }

    cam.takePicture();

    is_picture_requested = false;
    cond_preview_possible.notify_all();
}

void Director::setPreview(bool preview_requested)
{
    std::unique_lock<std::mutex> lock(mutex);
    is_preview_running = preview_requested;

    if(is_picture_requested && !preview_requested) {
        cond_picture_possible.notify_all();
    }
}


void Director::run()
{
    //    cam.testLoop();
    //    cam.autoFocus();
    while(running) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            while(is_picture_requested) {
                cond_preview_possible.wait(lock);
            }
        }


        setPreview(true);
//        cam.autoFocus();
//        cam.handleEvents();
        cam.takePreviewImage();

        setPreview(false);
    }
}

#include "moc_director.cpp"
