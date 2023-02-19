#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

GST_DEBUG_CATEGORY_STATIC (gst_opencv_blur_debug_category);
#define GST_CAT_DEFAULT gst_opencv_blur_debug_category

#define GST_TYPE_OPENCV_BLUR (gst_opencv_blur_get_type())
G_DECLARE_FINAL_TYPE (GstOpenCVBlur, gst_opencv_blur, GST, TYPE_OPENCV_BLUR, GstBaseTransform)

struct _GstOpenCVBlur
{
GstBaseTransform base_transform;

GstPad *sinkpad;
GstPad *srcpad;

int kernel_size;
cv::Mat kernel;
};

struct _GstOpenCVBlurClass
{
GstBaseTransformClass base_transform_class;
};

enum
{
PROP_KERNEL_SIZE = 1
};

static void
gst_opencv_blur_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
GstOpenCVBlur *self = GST_OPENCV_BLUR (object);

switch (property_id) {
case PROP_KERNEL_SIZE:
self->kernel_size = g_value_get_int (value);
break;
default:
G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
break;
}
}

static void
gst_opencv_blur_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
GstOpenCVBlur *self = GST_OPENCV_BLUR (object);

switch (property_id) {
case PROP_KERNEL_SIZE:
g_value_set_int (value, self->kernel_size);
break;
default:
G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
break;
}
}

static GstCaps *
gst_opencv_blur_transform_caps (GstBaseTransform * base_transform, GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
GstOpenCVBlur *self = GST_OPENCV_BLUR (base_transform);

if (direction == GST_PAD_SINK) {
GstStructure *structure = gst_caps_get_structure (caps, 0);
gst_structure_set (structure, "format", G_TYPE_STRING, "GRAY8", NULL);
}

return caps;
}

static gboolean
gst_opencv_blur_set_caps (GstBaseTransform * base_transform, GstCaps * incaps, GstCaps * outcaps)
{
GstOpenCVBlur *self = GST_OPENCV_BLUR (base_transform);

return TRUE;
}

static GstFlowReturn
gst_opencv_blur_transform_ip (GstBaseTransform * base_transform, GstBuffer * buf)
{
GstOpenCVBlur *self = GST_OPENCV_BLUR (base_transform);
GstMapInfo map;

gst_buffer_map (buf, &map, GST_MAP_READWRITE);

cv::Mat image = cv::Mat (GST_VIDEO_FRAME_COMP_HEIGHT (GST_BUFFER_FRAME (buf)),
GST_VIDEO_FRAME_COMP_WIDTH (GST_BUFFER_FRAME (buf)),
CV_8UC1, map.data, GST_VIDEO_FRAME_COMP_STRIDE (GST_BUFFER_FRAME (buf)));

cv::Mat blurred;
cv::GaussianBlur (image, blurred, cv::Size (self->kernel_size, self->kernel_size), 0, 0);

memcpy (map.data, blurred.data, GST_VIDEO_FRAME_COMP_HEIGHT (GST_BUFFER_FRAME (buf)) * GST_VIDEO_FRAME_COMP_WIDTH (GST_BUFFER_FRAME (buf)));

gst_buffer_unmap (buf){
    return GST_FLOW_OK;
}
static gboolean
plugin_init (GstPlugin * plugin)
{
GST_DEBUG_CATEGORY_INIT (gst_opencv_blur_debug_category, "opencv_blur", 0, "OpenCV blur");

return gst_element_register (plugin, "opencv_blur", GST_RANK_NONE, GST_TYPE_OPENCV_BLUR);
}

G_DEFINE_TYPE (GstOpenCVBlur, gst_opencv_blur, GST_TYPE_BASE_TRANSFORM);

static void
gst_opencv_blur_class_init (GstOpenCVBlurClass * klass)
{
GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);

gobject_class->set_property = gst_opencv_blur_set_property;
gobject_class->get_property = gst_opencv_blur_get_property;

base_transform_class->transform_caps = gst_opencv_blur_transform_caps;
base_transform_class->set_caps = gst_opencv_blur_set_caps;
base_transform_class->transform_ip = gst_opencv_blur_transform_ip;

g_object_class_install_property (gobject_class, PROP_KERNEL_SIZE,
g_param_spec_int ("kernel-size", "Kernel Size", "The size of the kernel for Gaussian blur", 3, 31, 3, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_opencv_blur_init (GstOpenCVBlur * self)
{
self->sinkpad = gst_pad_new_from_static_template (&gst_video_converter_sink_template, "sink");
gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

self->srcpad = gst_pad_new_from_static_template (&gst_video_converter_src_template, "src");
gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

self->kernel_size = 3;
self->kernel = cv::getGaussianKernel (self->kernel_size, -1);
}

#ifndef PACKAGE
#define PACKAGE "opencv_blur"
#endif

#define VERSION "1.0"

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
GST_VERSION_MINOR,
opencv_blur,
"GStreamer OpenCV blur plugin",
plugin_init,
VERSION,
"LGPL",
"OpenCV",
"https://opencv.org/")