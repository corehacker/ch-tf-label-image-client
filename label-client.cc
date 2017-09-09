
#include "label-client.h"

using indexer::PacketHeader;
using indexer::Packet;
using indexer::Index;
using indexer::IndexEntry;

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
  NetworkMessage *message = (NetworkMessage *) arg;
  LabelClient *client = (LabelClient *) message->client();
  return client->networkRoutine(message);
}

void *LabelClient::imageRoutine () {
  fts->walk(LabelClient::_onFile, this);
  return NULL;
}

void *LabelClient::networkRoutine (NetworkMessage *message) {
  Packet packet;
  PacketHeader *header = packet.mutable_header();
  header->set_payload(0);
  Index *payload = packet.mutable_payload();

  printf ("Payload? %d\n", packet.has_payload());

  IndexEntry *entry = NULL;

  for (int pos = 0; pos < message->labels_size(); ++pos) {
    NetworkMessage_Label label = message->labels(pos);
    LOG(INFO) << message->image() << " (" << label.label() << "): " << label.score();
    entry = payload->add_entry();
    entry->set_path(message->image());
    entry->set_index(pos);
    entry->set_key(label.label());
    entry->set_probability(label.score());
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
  NetworkMessage *message = new NetworkMessage();
  printf ("Label Client: %p\n", this);
  message->set_client((::google::protobuf::uint64) this);
  message->set_image(image);

  for (unsigned int pos = 0; pos < labels.size(); ++pos) {
    NetworkMessage_Label *label = message->add_labels();
    label->set_label(labels[pos]);
    label->set_score(scores[pos]);
  }
  printf ("Labels size: %d\n", message->labels_size());
  ThreadJob *job = new ThreadJob (LabelClient::_networkRoutine, message);
  mNetworkPool->addJob(job);
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
