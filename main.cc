/* Copyright 2017 The TensorFlow Authors & Sandeep Prakash. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// A minimal but useful C++ example showing how to load an Imagenet-style object
// recognition TensorFlow model, prepare input images for it, run them through
// the graph, and interpret the results.
//
// It's designed to have as few dependencies and be as clear as possible, so
// it's more verbose than it could be in production code. In particular, using
// auto for the types of a lot of the returned values from TensorFlow calls can
// remove a lot of boilerplate, but I find the explicit types useful in sample
// code to make it simple to look up the classes involved.
//
// To use it, compile and then run in a working directory with the
// learning/brain/tutorials/ch-tf-label-image-client/data/ folder below it, and you should
// see the top five labels for the example Lena image output. You can then
// customize it to use your own models or images by changing the file names at
// the top of the main() function.
//
// The googlenet_graph.pb file included by default is created from Inception.
//
// Note that, for GIF inputs, to reuse existing code, only single-frame ones
// are supported.

#include <ch-pal/exp_pal.h>
#include <ch-utils/exp_sock_utils.h>
#include <ch-cpp-utils/fts.hpp>
#include <ch-cpp-utils/thread-pool.hpp>

#include "label-image.h"

#include "packet.pb.h"
#include "communication.pb.h"


using indexer::PacketHeader;
using indexer::Packet;
using indexer::Index;
using indexer::IndexEntry;

class LabelClient;

typedef struct _NETWORK_MESSAGE_X {
  LabelClient *client; 
  std::string image;
  std::vector<std::string> labels;
  std::vector<float> scores;
} NETWORK_MESSAGE_X;


class LabelClient {
private:
    LabelImage *labelImage;
    Fts *fts;
    PAL_SOCK_HDL hl_sock_hdl;
    uint8_t *puc_dns_name_str;
    uint16_t us_host_port_ho;
    ThreadPool *mImagePool;
    ThreadPool *mNetworkPool;

    static void _onFile (std::string name, std::string ext, std::string path, void *this_);
    void onFile (std::string name, std::string ext, std::string path);

    static void _onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores, void *this_);
    void onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores);

    static void *_imageRoutine (void *arg, struct event_base *base);
    static void *_networkRoutine (void *arg, struct event_base *base);

    void *imageRoutine ();
    void *networkRoutine (NETWORK_MESSAGE_X *message);
public:
    LabelClient();
    LabelClient(uint8_t *puc_dns_name_str, uint16_t us_host_port_ho);
    ~LabelClient();
    void init();
    void process();
};

LabelClient::LabelClient() {
  labelImage = NULL;
  fts = NULL;
  mImagePool = NULL;
  mNetworkPool = NULL;
  hl_sock_hdl = NULL;
  puc_dns_name_str = (uint8_t *) "127.0.0.1";
  us_host_port_ho = 8888; 
}

LabelClient::LabelClient(uint8_t *puc_dns_name_str, uint16_t us_host_port_ho) {
  this->puc_dns_name_str = puc_dns_name_str;
  this->us_host_port_ho = us_host_port_ho; 
}

LabelClient::~LabelClient() {
}

void * LabelClient::_imageRoutine (void *arg, struct event_base *base) {
  LabelClient *client = (LabelClient *) arg;
  return client->imageRoutine();
}

void * LabelClient::_networkRoutine (void *arg, struct event_base *base) {
  NETWORK_MESSAGE_X *message = (NETWORK_MESSAGE_X *) arg;
  LabelClient *client = message->client;
  return client->networkRoutine(message);
}

void *LabelClient::imageRoutine () {
  fts->walk(LabelClient::_onFile, this);
  return NULL;
}

void *LabelClient::networkRoutine (NETWORK_MESSAGE_X *message) {

  std::string image = message->image;
  std::vector<std::string> labels = message->labels;
  std::vector<float> scores = message->scores;

  Packet packet;
  PacketHeader *header = packet.mutable_header();
  header->set_payload(0);
  Index *payload = packet.mutable_payload();

  printf ("Payload? %d\n", packet.has_payload());

#if 0
  IndexEntry *entry = NULL;

  for (unsigned int pos = 0; pos < labels.size(); ++pos) {
    LOG(INFO) << image << " (" << labels[pos] << "): " << scores[pos];
    entry = payload->add_entry();
    entry->set_path(image.data());
    entry->set_index(pos);
    entry->set_key(labels[pos].data());
    entry->set_probability(scores[pos]);
  }
  printf ("Num of entries: %d\n", payload->entry_size());

  int size = packet.ByteSize();
  printf ("Size before: %d\n", size);

  header->set_payload(size);

  printf ("payload? %d\n", header->has_payload());
  size = packet.ByteSize();
  printf ("Size after: %d\n", size);

  uint8_t *buffer = (uint8_t *) malloc(size);
  memset(buffer, 0x00, size);
  packet.SerializeToArray(buffer, size);

  PAL_RET_E e_error = ePAL_RET_FAILURE;
  e_error = pal_sock_send (hl_sock_hdl,
      (uint8_t *) buffer,
      (uint32_t *) &size,
      0);
  if (ePAL_RET_SUCCESS != e_error) {
    LOG(ERROR) << "Error sending to server. " << e_error;
  } else {
    LOG(INFO) << "Success sending to server. " << e_error;
  }
#endif

  return NULL;
}

void LabelClient::init() {
    PAL_RET_E e_error = ePAL_RET_FAILURE;
    SOCK_UTIL_HOST_INFO_X x_host_info = {0};
    x_host_info.ui_bitmask |= eSOCK_UTIL_HOST_INFO_DNS_NAME_BM;
    x_host_info.ui_bitmask |= eSOCK_UTIL_HOST_INFO_HOST_PORT_BM;
    x_host_info.puc_dns_name_str = puc_dns_name_str;
    x_host_info.us_host_port_ho = us_host_port_ho;
       
    e_error = tcp_connect_sock_create (&hl_sock_hdl, &x_host_info, 1000);
    if (ePAL_RET_SUCCESS != e_error) {
      LOG(ERROR) << "Error connecting to server. " << e_error;
    } else {
      LOG(INFO) << "Success connecting to server. " << e_error;
    }

    labelImage = new LabelImage();
    labelImage->init(LabelClient::_onLabel, this);

    FtsOptions options;
    options.filters.emplace_back<string>("jpg");
    fts = new Fts ("./tensorflow/examples/ch-tf-label-image-client", &options);

    mImagePool = new ThreadPool (1, false);
    mNetworkPool = new ThreadPool (1, false);
}

void LabelClient::process() {
  ThreadJob *job = new ThreadJob (LabelClient::_imageRoutine, this);
  mImagePool->addJob(job);
  std::chrono::milliseconds ms(1000);
  while (true) {
     std::this_thread::sleep_for(ms);
  }
}

void LabelClient::_onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  client->onLabel(image, labels, scores);
}

void LabelClient::onLabel (std::string image, std::vector<std::string> labels, std::vector<float> scores) {

  NETWORK_MESSAGE_X *px_message = (NETWORK_MESSAGE_X *) malloc(sizeof(NETWORK_MESSAGE_X));
  px_message->image = image;
  px_message->labels = labels;
  px_message->scores = scores;
  px_message->client = this;
  ThreadJob *job = new ThreadJob (LabelClient::_networkRoutine, px_message);
  mNetworkPool->addJob(job);
  
#if 0
  Packet packet;
  PacketHeader *header = packet.mutable_header();
  header->set_payload(0);
  Index *payload = packet.mutable_payload();

  printf ("Payload? %d\n", packet.has_payload());

  IndexEntry *entry = NULL;

  for (unsigned int pos = 0; pos < labels.size(); ++pos) {
    LOG(INFO) << image << " (" << labels[pos] << "): " << scores[pos];
    entry = payload->add_entry();
    entry->set_path(image.data());
    entry->set_index(pos);
    entry->set_key(labels[pos].data());
    entry->set_probability(scores[pos]);
  }
  printf ("Num of entries: %d\n", payload->entry_size());

  int size = packet.ByteSize();
  printf ("Size before: %d\n", size);

  header->set_payload(size);

  printf ("payload? %d\n", header->has_payload());
  size = packet.ByteSize();
  printf ("Size after: %d\n", size);

  uint8_t *buffer = (uint8_t *) malloc(size);
  memset(buffer, 0x00, size);
  packet.SerializeToArray(buffer, size);

  PAL_RET_E e_error = ePAL_RET_FAILURE;
  e_error = pal_sock_send (hl_sock_hdl,
      (uint8_t *) buffer,
      (uint32_t *) &size,
      0);
  if (ePAL_RET_SUCCESS != e_error) {
    LOG(ERROR) << "Error sending to server. " << e_error;
  } else {
    LOG(INFO) << "Success sending to server. " << e_error;
  }
#endif
}

void LabelClient::_onFile (std::string name, std::string ext, std::string path, void *this_) {
  LabelClient *client = (LabelClient *) this_;
  client->onFile(name, ext, path);
}

void LabelClient::onFile (std::string name, std::string ext, std::string path) {
  printf ("File: %10s %50s %s\n",
    ext.data(), name.data(), path.data());
  labelImage->process(path);
}

int main(int argc, char* argv[]) {

  PAL_LOGGER_INIT_PARAMS_X x_init_params = {false};
  pal_env_init ();                                                                 
  
  x_init_params.e_level = eLOG_LEVEL_HIGH;                                         
  x_init_params.b_enable_console_logging = true;                                   
  pal_logger_env_init(&x_init_params);

  // These are the command-line flags the program can understand.
  // They define where the graph and input data is located, and what kind of
  // input the model expects. If you train your own model, or use something
  // other than inception_v3, then you'll need to update these.
  string image = "tensorflow/examples/ch-tf-label-image-client/data/grace_hopper.jpg";
  string graph =
      "tensorflow/examples/ch-tf-label-image-client/data/inception_v3_2016_08_28_frozen.pb";
  string labels =
      "tensorflow/examples/ch-tf-label-image-client/data/imagenet_slim_labels.txt";
  int32 input_width = 299;
  int32 input_height = 299;
  float input_mean = 0;
  float input_std = 255;
  string input_layer = "input";
  string output_layer = "InceptionV3/Predictions/Reshape_1";
  bool self_test = false;
  string root_dir = "";
  std::vector<Flag> flag_list = {
      Flag("image", &image, "image to be processed"),
      Flag("graph", &graph, "graph to be executed"),
      Flag("labels", &labels, "name of file containing labels"),
      Flag("input_width", &input_width, "resize image to this width in pixels"),
      Flag("input_height", &input_height,
           "resize image to this height in pixels"),
      Flag("input_mean", &input_mean, "scale pixel values to this mean"),
      Flag("input_std", &input_std, "scale pixel values to this std deviation"),
      Flag("input_layer", &input_layer, "name of input layer"),
      Flag("output_layer", &output_layer, "name of output layer"),
      Flag("self_test", &self_test, "run a self test"),
      Flag("root_dir", &root_dir,
           "interpret image and graph file names relative to this directory"),
  };
  string usage = tensorflow::Flags::Usage(argv[0], flag_list);
  const bool parse_result = tensorflow::Flags::Parse(&argc, argv, flag_list);
  if (!parse_result) {
    LOG(ERROR) << usage;
    return -1;
  }

  // We need to call this to set up global state for TensorFlow.
  tensorflow::port::InitMain(argv[0], &argc, &argv);
  if (argc > 1) {
    LOG(ERROR) << "Unknown argument " << argv[1] << "\n" << usage;
    return -1;
  }

  LabelClient *client = new LabelClient();
  client->init();
  client->process();

  return 0;
}
