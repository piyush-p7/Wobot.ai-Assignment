#include <gst/gst.h>
#include <cairo.h>

#define WIDTH 800
#define HEIGHT 600

typedef struct {
  GstElement *pipeline;
  cairo_surface_t *surface;
  cairo_t *cr;
  guint64 duration;
  guint width;
  guint height;
} AppData;

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *)data;

  /* We can now link this pad with the audio decoder */
  sinkpad = gst_element_get_static_pad(decoder, "sink");
  gst_pad_link(pad, sinkpad);
  gst_object_unref(sinkpad);
}

static gboolean draw_waveform(AppData *app) {
  GstElement *pipeline = app->pipeline;
  GstBuffer *buffer = NULL;
  GstMapInfo map;
  guint num_samples = 0;
  guint i;
  gint16 *samples;
  gdouble scale_x, scale_y;
  gint x, y;

  /* Get the current position of the pipeline */
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position;
  if (!gst_element_query_position(pipeline, &fmt, &position)) {
    g_printerr("Could not query position.\n");
    return G_SOURCE_REMOVE;
  }

  /* Calculate the x-scale factor for the waveform */
  scale_x = (gdouble)app->width / (gdouble)app->duration;

  /* Get a buffer of audio data */
  if (!gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE)) {
    g_printerr("Could not get state of pipeline.\n");
    return G_SOURCE_REMOVE;
  }
  GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) {
    g_printerr("Could not get sample from sink.\n");
    return G_SOURCE_REMOVE;
  }
  buffer = gst_sample_get_buffer(sample);
  gst_sample_unref(sample);
  gst_object_unref(sink);

  /* Get the raw audio data from the buffer */
  gst_buffer_map(buffer, &map, GST_MAP_READ);
  num_samples = map.size / sizeof(gint16);
  samples = (gint16 *)map.data;

  /* Clear the surface */
  cairo_set_source_rgb(app->cr, 0.0, 0.0, 0.0);
  cairo_paint(app->cr);

  /* Draw the waveform */
  cairo_set_source_rgb(app->cr, 1.0, 1.0, 1.0);
  scale_y = (gdouble)app->height / (gdouble)G_MAXUINT16;
  for (i = 0; i < num_samples; i++) {
    x = (gint)(scale_x * (gdouble)(position + ((gdouble)i / (gdouble)num_samples) * (gdouble)app->duration));
    y = (gint)(app->height - (gdouble)samples[i] * scale_y);
    if (i == 0) {
      cairo_move_to(app->cr, x, y);
    } else {
      cairo_line_to(app->cr, x, y);
    }
  }
  cairo_stroke(app->cr);

  /* Release the buffer */
  gst_buffer_unmap(buffer, &map);

    /* Redraw the surface */
    cairo_surface_flush(app->surface);

    /* Continue drawing if there is more audio to process */
    if (position + GST_BUFFER_DURATION(buffer) < app->duration) {
    return G_SOURCE_CONTINUE;
    } else {
    return G_SOURCE_REMOVE;
    }
    }

    int main(int argc, char *argv[]) {
    GstElement *pipeline, *filesrc, *decodebin, *audioconvert, *appsrc, *audiosink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GMainLoop *loop;
    AppData app = {0};

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Initialize Cairo */
    app.width = WIDTH;
    app.height = HEIGHT;
    app.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, app.width, app.height);
    app.cr = cairo_create(app.surface);

    /* Create the pipeline */
    pipeline = gst_pipeline_new(NULL);

    /* Create the file source */
    filesrc = gst_element_factory_make("filesrc", NULL);
    g_object_set(G_OBJECT(filesrc), "location", argv[1], NULL);

    /* Create the decode bin */
    decodebin = gst_element_factory_make("decodebin", NULL);

    /* Create the audio converter */
    audioconvert = gst_element_factory_make("audioconvert", NULL);

    /* Create the app source */
    appsrc = gst_element_factory_make("appsrc", NULL);
    g_object_set(G_OBJECT(appsrc), "is-live", TRUE, "block", TRUE, NULL);
    g_signal_connect(appsrc, "need-data", G_CALLBACK(draw_waveform), &app);

    /* Create the audio sink */
    audiosink = gst_element_factory_make("autoaudiosink", NULL);

    /* Add the elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), filesrc, decodebin, audioconvert, appsrc, audiosink, NULL);

    /* Link the elements together */
    gst_element_link(filesrc, decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), audioconvert);
    gst_element_link(audioconvert, appsrc);
    gst_element_link(appsrc, audiosink);

    /* Set the pipeline to the playing state */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Failed to set pipeline to playing state.\n");
    return -1;
    }

    /* Get the duration of the audio file */
    if (!gst_element_query_duration(pipeline, GST_FORMAT_TIME, &app.duration)) {
    g_printerr("Could not query duration.\n");
    return -1;
    }

    /* Create the main loop */
    loop = g_main_loop_new(NULL, FALSE);

    /* Start the main loop */
    g_main_loop_run(loop);

    /* Free resources */
    g_main_loop_unref(loop);
    cairo_destroy(app.cr);
    cairo_surface_destroy(app.surface);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
