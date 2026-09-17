// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file signal_ipc_config.hpp
 * @brief Compile-time POSIX signal configuration for Castle signal event components.
 * @details Use this header when configuring a fixed set of operating-system
 * signals for Castle signal dispatchers. The enum provides a strongly typed
 * mapping to POSIX `SIG*` values, while `signal_ipc_config` binds each signal to
 * a fixed callback capacity and optional inline callback storage. The header is
 * allocation-free and contains only compile-time metadata.
 *
 * @code
 * #include "castle/events/signal_ipc_config.hpp"
 *
 * using sigint_config = castle::events::signal_ipc_config<
 *     castle::events::signal::sigint,
 *     2U,
 *     32U>;
 *
 * int signum = castle::events::to_signum(sigint_config::signum);
 * @endcode
 */
#ifndef CASTLE_EVENTS_SIGNAL_IPC_CONFIG_HPP
#define CASTLE_EVENTS_SIGNAL_IPC_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <signal.h>

namespace castle
{
namespace events
{

/**
 * @brief Strongly typed wrapper around the supported POSIX signal numbers.
 * @note The enumerator values are the platform `SIG*` macros converted to `int`.
 */
enum class signal : int
{
    sighup    = SIGHUP,    ///< Hangup on controlling terminal.     
    sigint    = SIGINT,    ///< Interactive interrupt request.     
    sigquit   = SIGQUIT,   ///< Interactive quit request.    
    sigill    = SIGILL,    ///< Illegal instruction fault.     
    sigtrap   = SIGTRAP,   ///< Trace or breakpoint trap.    
    sigabrt   = SIGABRT,   ///< Abort signal.    
    sigbus    = SIGBUS,    ///< Bus error condition.     
    sigfpe    = SIGFPE,    ///< Arithmetic fault.     
    sigusr1   = SIGUSR1,   ///< User-defined signal 1.    
    sigsegv   = SIGSEGV,   ///< Invalid memory reference.    
    sigusr2   = SIGUSR2,   ///< User-defined signal 2.    
    sigpipe   = SIGPIPE,   ///< Write on a pipe with no reader.    
    sigalrm   = SIGALRM,   ///< Real-time timer expiration.    
    sigterm   = SIGTERM,   ///< Termination request.    
    sigchld   = SIGCHLD,   ///< Child process state change.    
    sigcont   = SIGCONT,   ///< Continue a stopped process.    
    sigtstp   = SIGTSTP,   ///< Interactive terminal stop.    
    sigttin   = SIGTTIN,   ///< Background read from controlling terminal.    
    sigttou   = SIGTTOU,   ///< Background write to controlling terminal.    
    sigurg    = SIGURG,    ///< Urgent socket condition.     
    sigxcpu   = SIGXCPU,   ///< CPU time limit exceeded.    
    sigxfsz   = SIGXFSZ,   ///< File size limit exceeded.    
    sigvtalrm = SIGVTALRM, ///< Virtual timer expiration.  
    sigprof   = SIGPROF,   ///< Profiling timer expiration.    
    sigsys    = SIGSYS     ///< Bad system call.      
};

/**
 * @brief Returns the native POSIX signal number for a Castle signal enum value.
 * @param s Signal enumerator to convert.
 * @return Platform `int` value that can be passed to POSIX signal APIs.
 */
static inline CASTLE_CONSTEXPR int to_signum(signal s) CASTLE_NOEXCEPT
{
    return static_cast<int>(s);
}

/**
 * @brief Describes one managed POSIX signal for Castle signal dispatchers.
 * @tparam Signal Signal enumerator selected for the dispatcher slot.
 * @tparam MaxCallback Maximum number of callbacks allowed for the signal.
 * @tparam StorageSize Inline callback storage reserved by owning signal dispatchers.
 * @tparam StorageAlignment Alignment of the inline callback storage.
 * @note Components such as `signal_ipc_event` consume these constants at compile time.
 */
template <
    signal Signal,
    size_type MaxCallback,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
struct signal_ipc_config
{
    /**
     * @brief Signal enumerator associated with this configuration.
     */
    static CASTLE_CONSTEXPR signal signum = Signal;

    /**
     * @brief Maximum number of callbacks that may subscribe to the signal.
     */
    static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;

    /**
     * @brief Inline callback storage size reserved by owning signal dispatchers.
     */
    static CASTLE_CONSTEXPR size_type storage_size = StorageSize;

    /**
     * @brief Inline callback storage alignment reserved by owning signal dispatchers.
     */
    static CASTLE_CONSTEXPR size_type storage_alignment = StorageAlignment;
};

} 
} 

#endif 
