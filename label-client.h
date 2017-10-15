

#include <ch-pal/exp_pal.h>
#include <ch-utils/exp_sock_utils.h>
#include <ch-cpp-utils/fts.hpp>
#include <ch-cpp-utils/fs-watch.hpp>
#include <ch-cpp-utils/thread-pool.hpp>
#include <ch-protos/packet.pb.h>
#include <ch-protos/communication.pb.h>
#include <ch-protos/label-client-internal.pb.h>

#include "label-image.h"


using label_client_internal::NetworkMessage;
using label_client_internal::NetworkMessage_Label;

using ChCppUtils::FsWatch;
using ChCppUtils::ThreadPool;
using ChCppUtils::Fts;

class LabelClient;

class LabelClient {
private:
    LabelImage *labelImage;
    Fts *fts;
    FsWatch *fsWatch;
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
    void *networkRoutine (NetworkMessage *message);

    static void _onNewFile (std::string path, void *this_);
    void onNewFile (std::string path);
public:
    LabelClient();
    LabelClient(uint8_t *puc_dns_name_str, uint16_t us_host_port_ho);
    ~LabelClient();
    void init();
    void process();
};
