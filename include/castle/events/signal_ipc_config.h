#ifndef CASTLE_EVENTS_SIGNAL_IPC_CONFIG_H
#define CASTLE_EVENTS_SIGNAL_IPC_CONFIG_H

#include "castle/core/compiler.h"
#include "castle/core/config.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

#include <signal.h>

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// castle::events::signal
// -----------------------------------------------------------------------------
// Strongly-typed enumeration of the POSIX signals that signal_event can
// manage. The underlying values are the platform's SIG* macros so the enum
// converts losslessly to the `int` expected by <csignal> APIs (sigaction,
// raise, kill, ...).
// -----------------------------------------------------------------------------
enum class signal : int
{
    sighup    = SIGHUP,     // controlling terminal closed / config reload convention
    sigint    = SIGINT,     // interactive attention (Ctrl-C)
    sigquit   = SIGQUIT,    // interactive termination + core dump (Ctrl-\)
    sigill    = SIGILL,     // illegal instruction
    sigtrap   = SIGTRAP,    // trace / breakpoint trap
    sigabrt   = SIGABRT,    // abort()
    sigbus    = SIGBUS,     // bus error / misaligned access
    sigfpe    = SIGFPE,     // floating-point / integer arithmetic error
    sigusr1   = SIGUSR1,    // user-defined 1
    sigsegv   = SIGSEGV,    // invalid memory reference
    sigusr2   = SIGUSR2,    // user-defined 2
    sigpipe   = SIGPIPE,    // write to pipe with no reader
    sigalrm   = SIGALRM,    // alarm(2) timer
    sigterm   = SIGTERM,    // termination request
    sigchld   = SIGCHLD,    // child process state change
    sigcont   = SIGCONT,    // continue if stopped
    sigtstp   = SIGTSTP,    // interactive stop (Ctrl-Z)
    sigttin   = SIGTTIN,    // background read from tty
    sigttou   = SIGTTOU,    // background write to tty
    sigurg    = SIGURG,     // out-of-band data on socket
    sigxcpu   = SIGXCPU,    // CPU time limit exceeded
    sigxfsz   = SIGXFSZ,    // file size limit exceeded
    sigvtalrm = SIGVTALRM,  // virtual timer expired
    sigprof   = SIGPROF,    // profiling timer expired
    sigsys    = SIGSYS      // bad system call
};

static inline CASTLE_CONSTEXPR int to_signum(signal s) CASTLE_NOEXCEPT
{
    return static_cast<int>(s);
}

template <
    signal Signal,
    size_type MaxCallback,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
struct signal_ipc_config
{
    static CASTLE_CONSTEXPR signal signum = Signal;
    static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;
    static CASTLE_CONSTEXPR size_type storage_size = StorageSize;
    static CASTLE_CONSTEXPR size_type storage_alignment = StorageAlignment;
};

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_SIGNAL_IPC_CONFIG_H
