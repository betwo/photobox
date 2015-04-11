#include "director.h"

#include "camera.h"

Director::Director(EOSCamera& cam)
    : cam(cam), running(true)
{

}

void Director::stop()
{
    running = false;
}

void Director::takePicture()
{
    std::unique_lock<std::mutex> lock(mutex);
    cam.takePicture();
}

void Director::run()
{
//    cam.testLoop();
//    cam.autoFocus();
    while(running) {
        std::unique_lock<std::mutex> lock(mutex);
//        cam.autoFocus();
        cam.takePreviewImage();
    }
}

#include "moc_director.cpp"
