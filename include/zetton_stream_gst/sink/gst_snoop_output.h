#pragma once

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <functional>
#include <iostream>
#include <string>

#include "glibconfig.h"
#include "gst/gstpad.h"

/// \brief Class representing a GStreamer pipeline with an input element and an
/// output element.
/// \n Signals can be registered for data modification as well as
/// pipeline state changes.
class GstSnoopOutput {
 public:
  /// \brief Constructor for GstSnoopPipe objects.
  /// \param pipeline_src String defining the input pipeline.
  /// \param pipeline_sink String defining the output pipeline.
  GstSnoopOutput(std::string pipeline_src, std::string pipeline_sink);

  /// \brief Destructor for GstSnoopPipe objects.
  ~GstSnoopOutput();

 public:
  /// \brief Method for initializing the pipeline.
  /// \return True if initialization was successful.
  bool Init();

  /// \brief Method for starting the pipeline.
  void Start();

  /// \brief Method for stopping the pipeline.
  void Stop();

  /// \brief Method for registering a data modification callback.
  /// \param callback std::function object containing the callback function.
  void RegisterCallback(std::function<void(GstMapInfo *)> callback);

 private:
  /// \brief User modifies the data here.
  /// \param map GstMapInfo object passed by reference containing data to
  /// modify.
  /// \return GstFlowReturn object indicating flow status.
  GstFlowReturn ModifyInData(GstMapInfo *map);

  /// \brief Called when the appsink notifies us that there is a new buffer
  /// ready for processing.
  /// \param elt GstElement object passed by reference.
  /// \param user_data GstSnoopPipe object passed by reference.
  /// \return GstFlowReturn object indicating flow status.
  static GstFlowReturn OnNewSampleFromSink(GstElement *elt,
                                           GstSnoopOutput *user_data);

  /// \brief Called when we get a GstMessage from the source pipeline. When we
  /// get EOS, we notify the appsrc of it.
  /// \param bus GstBus object passed by reference.
  /// \param message GstMessage object passed by reference.
  /// \param user_data GstSnoopPipe object passed by reference.
  /// \return gboolean object indicating status.
  static gboolean OnSourceMessage(GstBus *bus, GstMessage *message,
                                  GstSnoopOutput *user_data);

  /// \brief Called when we get a GstMessage from the sink pipeline. When we
  /// get EOS, we exit the mainloop and this testapp.
  /// \param bus GstBus object passed by reference.
  /// \param message GstMessage object passed by reference.
  /// \param user_data GstSnoopPipe object passed by reference.
  /// \return gboolean object indicating status.
  static gboolean OnSinkMessage(GstBus *bus, GstMessage *message,
                                GstSnoopOutput *user_data);

 private:
  /// \brief GMainLoop object used for GStreamer pipeline.
  GMainLoop *loop_;
  /// \brief GstElement object at the beginning of the pipeline.
  GstElement *src_;
  /// \brief GstElement object at the end of the pipeline.
  GstElement *sink_;
  /// \brief User registered data modification callback function.
  std::function<void(GstMapInfo *)> callback_;

  /// \brief String defining the input pipeline.
  std::string pipeline_src_;
  /// \brief String defining the output pipeline.
  std::string pipeline_sink_;
};
