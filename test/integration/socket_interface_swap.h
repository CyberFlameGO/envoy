#pragma once

#include "source/common/network/socket_interface.h"

#include "test/integration/filters/test_socket_interface.h"

namespace Envoy {

// Enables control at the socket level to stop and resume writes.
// Useful for tests want to temporarily stop Envoy from draining data.

class SocketInterfaceSwap {
public:
  // Object of this class hold the state determining the IoHandle which
  // should return EAGAIN from the `writev` call.
  struct IoHandleMatcher {
    Network::IoSocketError* returnOverride(Envoy::Network::TestIoSocketHandle* io_handle) {
      absl::MutexLock lock(&mutex_);
      if (error_ && (io_handle->localAddress()->ip()->port() == src_port_ ||
                     io_handle->peerAddress()->ip()->port() == dst_port_)) {
        ASSERT(matched_iohandle_ == nullptr || matched_iohandle_ == io_handle,
               "Matched multiple io_handles, expected at most one to match.");
        matched_iohandle_ = io_handle;
        return error_;
      }
      return nullptr;
    }

    // Source port to match. The port specified should be associated with a listener.
    void setSourcePort(uint32_t port) {
      absl::WriterMutexLock lock(&mutex_);
      dst_port_ = 0;
      src_port_ = port;
    }

    // Destination port to match. The port specified should be associated with a listener.
    void setDestinationPort(uint32_t port) {
      absl::WriterMutexLock lock(&mutex_);
      src_port_ = 0;
      dst_port_ = port;
    }

    void setWritevReturnsEgain() {
      setWritevOverride(Network::IoSocketError::getIoSocketEagainInstance());
    }

    // The caller is responsible for memory management.
    void setWritevOverride(Network::IoSocketError* error) {
      absl::WriterMutexLock lock(&mutex_);
      ASSERT(src_port_ != 0 || dst_port_ != 0);
      error_ = error;
    }

    void setResumeWrites();

  private:
    mutable absl::Mutex mutex_;
    uint32_t src_port_ ABSL_GUARDED_BY(mutex_) = 0;
    uint32_t dst_port_ ABSL_GUARDED_BY(mutex_) = 0;
    Network::IoSocketError* error_ ABSL_GUARDED_BY(mutex_) = nullptr;
    Network::TestIoSocketHandle* matched_iohandle_{};
  };

  SocketInterfaceSwap();

  ~SocketInterfaceSwap() {
    test_socket_interface_loader_.reset();
    Envoy::Network::SocketInterfaceSingleton::initialize(previous_socket_interface_);
  }

  Envoy::Network::SocketInterface* const previous_socket_interface_{
      Envoy::Network::SocketInterfaceSingleton::getExisting()};
  std::shared_ptr<IoHandleMatcher> writev_matcher_{std::make_shared<IoHandleMatcher>()};
  std::unique_ptr<Envoy::Network::SocketInterfaceLoader> test_socket_interface_loader_;
};

} // namespace Envoy
