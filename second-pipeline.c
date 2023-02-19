#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline, *source, *output, *vertigotv;
  GstBus *bus;
  GstMessage *msg;
  GMainLoop *loop;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);
  loop = g_main_loop_new(NULL, FALSE);

  /* Create elements */
  pipeline = gst_pipeline_new("pipeline");
  source = gst_element_factory_make("videotestsrc", "source");
  output = gst_element_factory_make("autovideosink", "output");
  vertigotv = gst_element_factory_make("vertigotv", "vertigotv");

  /* Check that all elements were created successfully */
  if (!pipeline || !source || !output || !vertigotv) {
    g_printerr("Not all elements could be created\n");
    return -1;
  }

  /* Configure elements */
  g_object_set(source, "pattern", 0, NULL);

  /* Add elements to the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, vertigotv, output, NULL);

  /* Link elements together */
  if (!gst_element_link_many(source, vertigotv, output, NULL)) {
    g_printerr("Elements could not be linked\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Start playing the pipeline */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref(bus);

  /* Free resources */
  if (msg != NULL) {
    gst_message_unref(msg);
  }
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  g_main_loop_unref(loop);
  return 0;
}
