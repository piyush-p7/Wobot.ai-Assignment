#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create a new pipeline */
  pipeline = gst_pipeline_new("my-pipeline");

  /* Create a bus to monitor the pipeline */
  bus = gst_element_get_bus(pipeline);

  /* Start the pipeline */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait for the pipeline to finish */
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref(msg);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
