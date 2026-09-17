#pragma once

#include <csignal>
#include <cstddef>

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

static inline constexpr int to_signum(signal s) noexcept
{
    return static_cast<int>(s);
}

template <
    signal Signal,
    std::size_t MaxCallback,
    std::size_t StorageSize = 64,
    std::size_t StorageAlignment = alignof(std::max_align_t)>
struct signal_config
{
    static constexpr signal signum = Signal;
    static constexpr std::size_t max_callback = MaxCallback;
    static constexpr std::size_t storage_size = StorageSize;
    static constexpr std::size_t storage_alignment = StorageAlignment;
};

} // namespace events
} // namespace castle
