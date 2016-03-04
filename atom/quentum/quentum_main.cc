#include "quentum_main.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/threading/thread.h"
#include "base/memory/scoped_ptr.h"

#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_channel_proxy.h"

namespace quentum {

class QuentumMain : IPC::Listener {
public:
    QuentumMain() : _have_switch(false) {
    }

    ~QuentumMain() {
    }

    void Start() {
        std::string token, name;
        if (!HaveSwitch(token, name)) {
            return;
        }

        _main_thread.reset(new base::Thread("QuentumMain"));
        _main_thread->Start();
        _main_thread->message_loop()->PostTask(FROM_HERE,
                base::Bind(&QuentumMain::Initialize, base::Unretained(this), token, name));
    }

    void Stop() {
        if (!_have_switch) {
            return;
        }

        _main_thread->StopSoon();
    }
private:
    bool HaveSwitch(std::string& token, std::string& name) {
        base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
        _have_switch = cmd->HasSwitch("quentum-token") && cmd->HasSwitch("quentum-pipe-name");
        if (_have_switch) {
            token = cmd->GetSwitchValueASCII("quentum-token");
            name = cmd->GetSwitchValueASCII("quentum-pipe-name");
        }

        return _have_switch;
    }
private:
    void Initialize(const std::string& token, const std::string& name) {
        base::Thread::Options options;
        options.message_loop_type = base::MessageLoop::TYPE_IO;
        _io_thread.reset(new base::Thread("QuentumIOThread"));
        _io_thread->StartWithOptions(options);

        IPC::ChannelHandle handle;
        handle.name = name;
        _channel_proxy = IPC::ChannelProxy::Create(handle,
                                                   IPC::Channel::MODE_NAMED_CLIENT,
                                                   this,
                                                   _io_thread->task_runner());
    }
public:
    void OnChannelConnected(int32_t peer_pid) override {

    }

    bool OnMessageReceived(const IPC::Message& message) override {
        return true;
    }

private:
    bool _have_switch;
    scoped_ptr<base::Thread> _main_thread;
    scoped_ptr<base::Thread> _io_thread;
    scoped_ptr<IPC::ChannelProxy> _channel_proxy;
};

static base::LazyInstance<QuentumMain> lazy_quentum_main = LAZY_INSTANCE_INITIALIZER;

void QuentumMainStart() {
    lazy_quentum_main.Get().Start();
}

void QuentumMainStop() {
    lazy_quentum_main.Get().Stop();
}

}
