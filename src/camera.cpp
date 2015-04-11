#include "camera.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <QImage>
#include <jpeglib.h>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

#include <libraw/libraw.h>

namespace {

static void
ctx_error_func (GPContext *context, const char *str, void *data)
{
    fprintf  (stderr, "\n*** Contexterror ***              \n%s\n",str);
    fflush   (stderr);
}

static void
ctx_status_func (GPContext *context, const char *str, void *data)
{
    fprintf  (stderr, "%s\n", str);
    fflush   (stderr);
}

static void errordumper(GPLogLevel level, const char *domain, const char *str,
                        void *data) {
    printf("%s\n", str);
}

static void
capture_to_file(Camera *canon, GPContext *canoncontext, char *fn) {
    int fd, retval;
    CameraFile *canonfile;
    CameraFilePath camera_file_path;

    printf("Capturing.\n");

    retval = gp_camera_capture(canon, GP_CAPTURE_IMAGE, &camera_file_path, canoncontext);
    printf("  Retval: %d\n", retval);

    printf("Pathname on the camera: %s/%s\n", camera_file_path.folder, camera_file_path.name);

    fd = open(fn, O_CREAT | O_WRONLY, 0644);
    retval = gp_file_new_from_fd(&canonfile, fd);
    printf("  Retval: %d\n", retval);
    retval = gp_camera_file_get(canon, camera_file_path.folder, camera_file_path.name,
                                GP_FILE_TYPE_NORMAL, canonfile, canoncontext);
    printf("  Retval: %d\n", retval);

    printf("Deleting.\n");
    retval = gp_camera_file_delete(canon, camera_file_path.folder, camera_file_path.name,
                                   canoncontext);
    printf("  Retval: %d\n", retval);

    gp_file_free(canonfile);
}


/*
 * This function looks up a label or key entry of
 * a configuration widget.
 * The functions descend recursively, so you can just
 * specify the last component.
 */

static int
_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
    int ret;
    ret = gp_widget_get_child_by_name (widget, key, child);
    if (ret < GP_OK)
        ret = gp_widget_get_child_by_label (widget, key, child);
    return ret;
}

/* Gets a string configuration value.
 * This can be:
 *  - A Text widget
 *  - The current selection of a Radio Button choice
 *  - The current selection of a Menu choice
 *
 * Sample (for Canons eg):
 *   get_config_value_string (camera, "owner", &ownerstr, context);
 */
int
get_config_value_string (Camera *camera, const char *key, char **str, GPContext *context) {
    CameraWidget		*widget = NULL, *child = NULL;
    CameraWidgetType	type;
    int			ret;
    char			*val;

    ret = gp_camera_get_config (camera, &widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_get_config failed: %d\n", ret);
        return ret;
    }
    ret = _lookup_widget (widget, key, &child);
    if (ret < GP_OK) {
        fprintf (stderr, "lookup widget failed: %d\n", ret);
        goto out;
    }

    /* This type check is optional, if you know what type the label
         * has already. If you are not sure, better check. */
    ret = gp_widget_get_type (child, &type);
    if (ret < GP_OK) {
        fprintf (stderr, "widget get type failed: %d\n", ret);
        goto out;
    }
    switch (type) {
    case GP_WIDGET_MENU:
    case GP_WIDGET_RADIO:
    case GP_WIDGET_TEXT:
        break;
    default:
        fprintf (stderr, "widget has bad type %d\n", type);
        ret = GP_ERROR_BAD_PARAMETERS;
        goto out;
    }

    /* This is the actual query call. Note that we just
         * a pointer reference to the string, not a copy... */
    ret = gp_widget_get_value (child, &val);
    if (ret < GP_OK) {
        fprintf (stderr, "could not query widget value: %d\n", ret);
        goto out;
    }
    /* Create a new copy for our caller. */
    *str = strdup (val);
out:
    gp_widget_free (widget);
    return ret;
}


/* Sets a string configuration value.
 * This can set for:
 *  - A Text widget
 *  - The current selection of a Radio Button choice
 *  - The current selection of a Menu choice
 *
 * Sample (for Canons eg):
 *   get_config_value_string (camera, "owner", &ownerstr, context);
 */
int
set_config_value_string (Camera *camera, const char *key, const char *val, GPContext *context) {
    CameraWidget		*widget = NULL, *child = NULL;
    CameraWidgetType	type;
    int			ret;

    ret = gp_camera_get_config (camera, &widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_get_config failed: %d\n", ret);
        return ret;
    }
    ret = _lookup_widget (widget, key, &child);
    if (ret < GP_OK) {
        fprintf (stderr, "lookup widget failed: %d\n", ret);
        goto out;
    }

    /* This type check is optional, if you know what type the label
         * has already. If you are not sure, better check. */
    ret = gp_widget_get_type (child, &type);
    if (ret < GP_OK) {
        fprintf (stderr, "widget get type failed: %d\n", ret);
        goto out;
    }
    switch (type) {
    case GP_WIDGET_MENU:
    case GP_WIDGET_RADIO:
    case GP_WIDGET_TEXT:
        break;
    default:
        fprintf (stderr, "widget has bad type %d\n", type);
        ret = GP_ERROR_BAD_PARAMETERS;
        goto out;
    }

    /* This is the actual set call. Note that we keep
         * ownership of the string and have to free it if necessary.
         */
    ret = gp_widget_set_value (child, val);
    if (ret < GP_OK) {
        fprintf (stderr, "could not set widget value: %d\n", ret);
        goto out;
    }
    /* This stores it on the camera again */
    ret = gp_camera_set_config (camera, widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_set_config failed: %d\n", ret);
        return ret;
    }
out:
    gp_widget_free (widget);
    return ret;
}


/*
 * This enables/disables the specific canon capture mode.
 *
 * For non canons this is not required, and will just return
 * with an error (but without negative effects).
 */
int
canon_enable_capture (Camera *camera, int onoff, GPContext *context) {
    CameraWidget		*widget = NULL, *child = NULL;
    CameraWidgetType	type;
    int			ret;

    ret = gp_camera_get_config (camera, &widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_get_config failed: %d\n", ret);
        return ret;
    }
    ret = _lookup_widget (widget, "capture", &child);
    if (ret < GP_OK) {
        /*fprintf (stderr, "lookup widget failed: %d\n", ret);*/
        goto out;
    }

    ret = gp_widget_get_type (child, &type);
    if (ret < GP_OK) {
        fprintf (stderr, "widget get type failed: %d\n", ret);
        goto out;
    }
    switch (type) {
    case GP_WIDGET_TOGGLE:
        break;
    default:
        fprintf (stderr, "widget has bad type %d\n", type);
        ret = GP_ERROR_BAD_PARAMETERS;
        goto out;
    }
    /* Now set the toggle to the wanted value */
    ret = gp_widget_set_value (child, &onoff);
    if (ret < GP_OK) {
        fprintf (stderr, "toggling Canon capture to %d failed with %d\n", onoff, ret);
        goto out;
    }
    /* OK */
    ret = gp_camera_set_config (camera, widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_set_config failed: %d\n", ret);
        return ret;
    }
out:
    gp_widget_free (widget);
    return ret;
}


/* calls the Nikon DSLR or Canon DSLR autofocus method. */
int
camera_auto_focus(Camera *camera, GPContext *context) {
    CameraWidget		*widget = NULL, *child = NULL;
    CameraWidgetType	type;
    int			ret,val;

    ret = gp_camera_get_config (camera, &widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_get_config failed: %d\n", ret);
        return ret;
    }
    ret = _lookup_widget (widget, "autofocusdrive", &child);
    if (ret < GP_OK) {
        fprintf (stderr, "lookup 'autofocusdrive' failed: %d\n", ret);
        goto out;
    }

    /* check that this is a toggle */
    ret = gp_widget_get_type (child, &type);
    if (ret < GP_OK) {
        fprintf (stderr, "widget get type failed: %d\n", ret);
        goto out;
    }
    switch (type) {
    case GP_WIDGET_TOGGLE:
        break;
    default:
        fprintf (stderr, "widget has bad type %d\n", type);
        ret = GP_ERROR_BAD_PARAMETERS;
        goto out;
    }

    ret = gp_widget_get_value (child, &val);
    if (ret < GP_OK) {
        fprintf (stderr, "could not get widget value: %d\n", ret);
        goto out;
    }
    val++;
    ret = gp_widget_set_value (child, &val);
    if (ret < GP_OK) {
        fprintf (stderr, "could not set widget value to 1: %d\n", ret);
        goto out;
    }

    ret = gp_camera_set_config (camera, widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "could not set config tree to autofocus: %d\n", ret);
        goto out;
    }
out:
    gp_widget_free (widget);
    return ret;
}


/* Manual focusing a camera...
 * xx is -3 / -2 / -1 / 0 / 1 / 2 / 3
 */
int
camera_manual_focus (Camera *camera, int xx, GPContext *context) {
    CameraWidget		*widget = NULL, *child = NULL;
    CameraWidgetType	type;
    int			ret;
    float			rval;
    char			*mval;

    ret = gp_camera_get_config (camera, &widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "camera_get_config failed: %d\n", ret);
        return ret;
    }
    ret = _lookup_widget (widget, "manualfocusdrive", &child);
    if (ret < GP_OK) {
        fprintf (stderr, "lookup 'manualfocusdrive' failed: %d\n", ret);
        goto out;
    }

    /* check that this is a toggle */
    ret = gp_widget_get_type (child, &type);
    if (ret < GP_OK) {
        fprintf (stderr, "widget get type failed: %d\n", ret);
        goto out;
    }
    switch (type) {
    case GP_WIDGET_RADIO: {
        int choices = gp_widget_count_choices (child);

        ret = gp_widget_get_value (child, &mval);
        if (ret < GP_OK) {
            fprintf (stderr, "could not get widget value: %d\n", ret);
            goto out;
        }
        if (choices == 7) { /* see what Canon has in EOS_MFDrive */
            ret = gp_widget_get_choice (child, xx+4, (const char**)&mval);
            if (ret < GP_OK) {
                fprintf (stderr, "could not get widget choice %d: %d\n", xx+2, ret);
                goto out;
            }
        }
        ret = gp_widget_set_value (child, mval);
        if (ret < GP_OK) {
            fprintf (stderr, "could not set widget value to 1: %d\n", ret);
            goto out;
        }
        break;
    }
    case GP_WIDGET_RANGE:
        ret = gp_widget_get_value (child, &rval);
        if (ret < GP_OK) {
            fprintf (stderr, "could not get widget value: %d\n", ret);
            goto out;
        }

        switch (xx) { /* Range is on Nikon from -32768 <-> 32768 */
        case -3:	rval = -1024;break;
        case -2:	rval =  -512;break;
        case -1:	rval =  -128;break;
        case  0:	rval =     0;break;
        case  1:	rval =   128;break;
        case  2:	rval =   512;break;
        case  3:	rval =  1024;break;

        default:	rval = xx;	break; /* hack */
        }

        fprintf(stderr,"manual focus %d -> %f\n", xx, rval);

        ret = gp_widget_set_value (child, &rval);
        if (ret < GP_OK) {
            fprintf (stderr, "could not set widget value to 1: %d\n", ret);
            goto out;
        }
        break;
    default:
        fprintf (stderr, "widget has bad type %d\n", type);
        ret = GP_ERROR_BAD_PARAMETERS;
        goto out;
    }


    ret = gp_camera_set_config (camera, widget, context);
    if (ret < GP_OK) {
        fprintf (stderr, "could not set config tree to autofocus: %d\n", ret);
        goto out;
    }
out:
    gp_widget_free (widget);
    return ret;
}

}



EOSCamera::EOSCamera()
{
    canoncontext = gp_context_new();
    gp_context_set_error_func (canoncontext, ctx_error_func, NULL);
    gp_context_set_status_func (canoncontext, ctx_status_func, NULL);


    gp_log_add_func(GP_LOG_ERROR, errordumper, NULL);
    gp_camera_new(&canon);

    /* When I set GP_LOG_DEBUG instead of GP_LOG_ERROR above, I noticed that the
     * init function seems to traverse the entire filesystem on the camera.  This
     * is partly why it takes so long.
     * (Marcus: the ptp2 driver does this by default currently.)
     */
    printf("Camera init.  Takes about 10 seconds.\n");
    int retval = gp_camera_init(canon, canoncontext);
    if (retval != GP_OK) {
        printf("  Retval: %d\n", retval);
        exit (1);
    }
    canon_enable_capture(canon, TRUE, canoncontext);
}

EOSCamera::~EOSCamera()
{
    gp_camera_exit(canon, canoncontext);
}

void EOSCamera::handleEvents()
{
    int fd, retval;
    CameraFile *file;
    CameraEventType	evttype;
    CameraFilePath	*path;
    void	*evtdata;

    retval = gp_camera_wait_for_event (canon, 1000, &evttype, &evtdata, canoncontext);
    if (retval != GP_OK)
        return;
    switch (evttype) {
    case GP_EVENT_FILE_ADDED:
        //        path = (CameraFilePath*)evtdata;
        printf("File added on the camera: %s/%s\n", path->folder, path->name);

        //        fd = open(path->name, O_CREAT | O_WRONLY, 0644);
        //        retval = gp_file_new_from_fd(&file, fd);
        //        printf("  Downloading %s...\n", path->name);
        //        retval = gp_camera_file_get(canon, path->folder, path->name,
        //                                    GP_FILE_TYPE_NORMAL, file, canoncontext);

        //        printf("  Deleting %s on camera...\n", path->name);
        //        retval = gp_camera_file_delete(canon, path->folder, path->name, canoncontext);
        //        gp_file_free(file);
        break;
    case GP_EVENT_FOLDER_ADDED:
        path = (CameraFilePath*)evtdata;
        printf("Folder added on camera: %s / %s\n", path->folder, path->name);
        break;
    case GP_EVENT_CAPTURE_COMPLETE:
        printf("Capture Complete.\n");
        break;
    case GP_EVENT_TIMEOUT:
        printf("Timeout.\n");
        break;
    case GP_EVENT_UNKNOWN:
        if (evtdata) {
            printf("Unknown event: %s.\n", (char*)evtdata);
        } else {
            printf("Unknown event.\n");
        }
        break;
    default:
        printf("Type %d?\n", evttype);
        break;
    }
    std::cout.flush();
}

void EOSCamera::autoFocus()
{
    CameraEventType evttype;
    void    	*evtdata;


    int retval;
    do {
        retval = gp_camera_wait_for_event (canon, 10, &evttype, &evtdata, canoncontext);
    } while ((retval == GP_OK) && (evttype != GP_EVENT_TIMEOUT));

    //    retval = gp_file_new(&file);
    //    if (retval != GP_OK) {
    //        printf("gp_file_new: %d\n", retval);
    //        exit(1);
    //    }

    //    gp_file_free (file);


    //    if (i%10 == 9) {
    //camera_auto_focus (canon, canoncontext);
    //    } else {
    //        camera_manual_focus (canon, (i/10-5)/2, canoncontext);
    //    }
}

void EOSCamera::takePicture()
{
    int fd;
    CameraFile *canonfile;
    CameraFilePath camera_file_path;

    int retval;
    CameraEventType evttype;
    void    	*evtdata;

    //    printf("Waiting.\n");
    //    do {
    //        retval = gp_camera_wait_for_event (canon, 10, &evttype, &evtdata, canoncontext);
    //    } while ((retval == GP_OK) && (evttype != GP_EVENT_TIMEOUT));

    printf("Capturing.\n");
    std::cout.flush();

    /* NOP: This gets overridden in the library to /capt0000.jpg */
    strcpy(camera_file_path.folder, "/");
    strcpy(camera_file_path.name, "foo.jpg");

    retval = gp_camera_capture(canon, GP_CAPTURE_IMAGE, &camera_file_path, canoncontext);
    printf("  Retval: %d\n", retval);

    printf("Pathname on the camera: %s/%s\n", camera_file_path.folder, camera_file_path.name);
    std::cout.flush();

    long now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::string file = std::to_string(now) + camera_file_path.name;
    fd = open(file.c_str(), O_CREAT | O_WRONLY, 0644);
    retval = gp_file_new_from_fd(&canonfile, fd);
    printf("  Retval: %d\n", retval);
    retval = gp_camera_file_get(canon, camera_file_path.folder, camera_file_path.name,
                                GP_FILE_TYPE_NORMAL, canonfile, canoncontext);
    printf("  Retval: %d\n", retval);
    std::cout.flush();

    printf("Deleting.\n");
    std::cout.flush();

    retval = gp_camera_file_delete(canon, camera_file_path.folder, camera_file_path.name,
                                   canoncontext);
    printf("  Retval: %d\n", retval);
    std::cout.flush();

    gp_file_free(canonfile);


    int  i, ret, verbose=0, output_thumbs=0;
    char outfn[1024],thumbfn[1024];

    // Creation of image processing object
    LibRaw RawProcessor;

    // The date in TIFF is written in the local format; let us specify the timezone for compatibility with dcraw
    putenv ((char*)"TZ=UTC");

    // Let us define variables for convenient access to fields of RawProcessor

#define P1  RawProcessor.imgdata.idata
#define S   RawProcessor.imgdata.sizes
#define C   RawProcessor.imgdata.color
#define T   RawProcessor.imgdata.thumbnail
#define P2  RawProcessor.imgdata.other
#define OUT RawProcessor.imgdata.params

    OUT.output_tiff = 1; // Let us output TIFF

    // Let us open the file
    if( (ret = RawProcessor.open_file(file.c_str())) != LIBRAW_SUCCESS)
    {
        fprintf(stderr,"Cannot open %s: %s\n",file.c_str(),libraw_strerror(ret));

        // recycle() is needed only if we want to free the resources right now.
        // If we process files in a cycle, the next open_file()
        // will also call recycle(). If a fatal error has happened, it means that recycle()
        // has already been called (repeated call will not cause any harm either).

        RawProcessor.recycle();
        goto end;
    }

    // Let us unpack the image
    if( (ret = RawProcessor.unpack() ) != LIBRAW_SUCCESS)
    {
        fprintf(stderr,"Cannot unpack_thumb %s: %s\n",file.c_str(),libraw_strerror(ret));

        if(LIBRAW_FATAL_ERROR(ret))
            goto end;
        // if there has been a non-fatal error, we will try to continue
    }
    // Let us unpack the thumbnail
    if( (ret = RawProcessor.unpack_thumb() ) != LIBRAW_SUCCESS)
    {
        // error processing is completely similar to the previous case
        fprintf(stderr,"Cannot unpack_thumb %s: %s\n",file.c_str(),libraw_strerror(ret));
        if(LIBRAW_FATAL_ERROR(ret))
            goto end;
    }
    else // We have successfully unpacked the thumbnail, now let us write it to a file
    {
        snprintf(thumbfn,sizeof(thumbfn),"%s.%s",file.c_str(),T.tformat == LIBRAW_THUMBNAIL_JPEG ? "thumb.jpg" : "thumb.ppm");
        if( LIBRAW_SUCCESS != (ret = RawProcessor.dcraw_thumb_writer(thumbfn)))
        {
            fprintf(stderr,"Cannot write %s: %s\n",thumbfn,libraw_strerror(ret));

            // in the case of fatal error, we should terminate processing of the current file
            if(LIBRAW_FATAL_ERROR(ret))
                goto end;
        }
    }
    // Data unpacking
//    ret = RawProcessor.dcraw_process();

//    if(LIBRAW_SUCCESS != ret ) // error at the previous step
//    {
//        fprintf(stderr,"Cannot do postprocessing on %s: %s\n",file.c_str(),libraw_strerror(ret));
//        if(LIBRAW_FATAL_ERROR(ret))
//            goto end;
//    }
//    else  // Successful document processing
//    {
//        snprintf(outfn,sizeof(outfn),"%s.%s", file.c_str(), "tiff");
//        if( LIBRAW_SUCCESS != (ret = RawProcessor.dcraw_ppm_tiff_writer(outfn)))
//            fprintf(stderr,"Cannot write %s: error %d\n",outfn,ret);
//    }

    // we don't evoke recycle() or call the desctructor; C++ will do everything for us

    {
        QImage* reimport = new QImage(QString::fromStdString(file + ".thumb.jpg"));
        emit newImage(reimport);

    }
    return;
end:
    // got here after an error
    return;
}

void EOSCamera::takePreviewImage()
{
    static int i = 0;

    CameraFile *file;
    char output_file[32];

    int retval = gp_file_new(&file);
    if (retval != GP_OK) {
        fprintf(stderr,"gp_file_new: %d\n", retval);
        exit(1);
    }

    /* autofocus every 10 shots */
    //    if ((i%10) == 9) {
    //        camera_auto_focus (canon, canoncontext);
    //        i = 0;
    //    } else {
    //        camera_manual_focus (canon, (i/10-5)/2, canoncontext);
    //    }

    //    ++i;

    retval = gp_camera_capture_preview(canon, file, canoncontext);
    if (retval != GP_OK) {
        fprintf(stderr,"gp_camera_capture_preview: %d\n",retval);
        exit(1);
    }

    const char* data;
    unsigned long size;
    gp_file_get_data_and_size(file, &data, &size);

    {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        JSAMPROW row_pointer[1];
        int i = 0;
        int location = 0;
        unsigned char *raw_image = NULL;

        cinfo.err = jpeg_std_error(&jerr);

        jpeg_create_decompress(&cinfo);
        jpeg_mem_src(&cinfo, (unsigned char *)data, size);
        jpeg_read_header(&cinfo, TRUE);

        //        printf( "JPEG File Information: \n" );
        //        printf( "Image width and height: %d pixels and %d pixels.\n", cinfo.image_width, cinfo.image_height );
        //        printf( "Color components per pixel: %d.\n", cinfo.num_components );
        //        printf( "Color space: %d.\n", cinfo.jpeg_color_space );

        jpeg_start_decompress( &cinfo );

        raw_image = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
        row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );

        while( cinfo.output_scanline < cinfo.image_height )
        {
            jpeg_read_scanlines( &cinfo, row_pointer, 1 );
            for( i=0; i<cinfo.image_width*cinfo.num_components;i++)
                raw_image[location++] = row_pointer[0][i];
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        free(row_pointer[0]);

        QImage* image = new QImage(raw_image, cinfo.image_width, cinfo.image_height, QImage::Format_RGB888);
        image->bits();
        emit newPreview(image);
    }


    sprintf(output_file, "snapshot.jpg");
    retval = gp_file_save(file, output_file);
    if (retval != GP_OK) {
        fprintf(stderr,"gp_camera_capture_preview: %d\n", retval);
        exit(1);
    }
    gp_file_unref(file);
}


void EOSCamera::testLoop()
{
    int	i, retval;
    /*set_capturetarget(canon, canoncontext);*/
    printf("Taking 100 previews and saving them to snapshot-XXX.jpg ...\n");

    for (i=0;i<100;i++) {
        CameraFile *file;
        char output_file[32];

        fprintf(stderr,"preview %d\n", i);
        retval = gp_file_new(&file);
        if (retval != GP_OK) {
            fprintf(stderr,"gp_file_new: %d\n", retval);
            exit(1);
        }

        /* autofocus every 10 shots */
        if (i%10 == 9) {
            camera_auto_focus (canon, canoncontext);
        } else {
            camera_manual_focus (canon, (i/10-5)/2, canoncontext);
        }

        retval = gp_camera_capture_preview(canon, file, canoncontext);
        if (retval != GP_OK) {
            fprintf(stderr,"gp_camera_capture_preview(%d): %d\n", i, retval);
            exit(1);
        }

        const char* data;
        unsigned long size;
        gp_file_get_data_and_size(file, &data, &size);

        {
            struct jpeg_decompress_struct cinfo;
            struct jpeg_error_mgr jerr;
            JSAMPROW row_pointer[1];
            int i = 0;
            int location = 0;
            unsigned char *raw_image = NULL;

            cinfo.err = jpeg_std_error(&jerr);

            jpeg_create_decompress(&cinfo);
            jpeg_mem_src(&cinfo, (unsigned char *)data, size);
            jpeg_read_header(&cinfo, TRUE);

            printf( "JPEG File Information: \n" );
            printf( "Image width and height: %d pixels and %d pixels.\n", cinfo.image_width, cinfo.image_height );
            printf( "Color components per pixel: %d.\n", cinfo.num_components );
            printf( "Color space: %d.\n", cinfo.jpeg_color_space );

            jpeg_start_decompress( &cinfo );

            raw_image = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
            row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );
            printf("BEFORE\n");
            while( cinfo.output_scanline < cinfo.image_height )
            {
                jpeg_read_scanlines( &cinfo, row_pointer, 1 );
                for( i=0; i<cinfo.image_width*cinfo.num_components;i++)
                    raw_image[location++] = row_pointer[0][i];
            }
            printf("AFTER\n");

            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            free(row_pointer[0]);

            //        QImage* image = new QImage(cinfo.image_width, cinfo.image_height, QImage::Format_ARGB32);

            QImage* image = new QImage(raw_image, cinfo.image_width, cinfo.image_height, QImage::Format_RGB888);
            image->bits();
            emit newPreview(image);
        }


        sprintf(output_file, "snapshot-%03d.jpg", i);
        retval = gp_file_save(file, output_file);
        if (retval != GP_OK) {
            fprintf(stderr,"gp_camera_capture_preview(%d): %d\n", i, retval);
            exit(1);
        }
        gp_file_unref(file);
        /*
        sprintf(output_file, "image-%03d.jpg", i);
            capture_to_file(canon, canoncontext, output_file);
    */
    }
}

#include "moc_camera.cpp"
