#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *decoder, *scale, *encoder, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    source = gst_element_factory_make("filesrc", "source");
    decoder = gst_element_factory_make("decodebin", "decoder");
    scale = gst_element_factory_make("videoscale", "scale");
    encoder = gst_element_factory_make("jpegenc", "encoder");
    sink = gst_element_factory_make("filesink", "sink");

    /* Create the pipeline */
    pipeline = gst_pipeline_new("video-pipeline");

    if (!pipeline || !source || !decoder || !scale || !encoder || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Set the input file location */
    g_object_set(source, "location", "input.mp4", NULL);

    /* Set the output file location */
    g_object_set(sink, "location", "output.jpg", NULL);

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, scale, encoder, sink, NULL);
    if (gst_element_link(source, decoder) != TRUE ||
        gst_element_link(decoder, scale) != TRUE ||
        gst_element_link(scale, encoder) != TRUE ||
        gst_element_link(encoder, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Connect to the bus */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, [](GstBus *bus, GstMessage *msg, gpointer user_data) -> gboolean {
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug_info;
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            }
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                break;
            default:
                break;
        }
        return TRUE;
    }, NULL);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    /* Clean up */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
