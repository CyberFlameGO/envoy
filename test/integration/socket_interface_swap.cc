#include "test/integration/socket_interface_swap.h"

namespace Envoy {

SocketInterfaceSwap::SocketInterfaceSwap() {
  Envoy::Network::SocketInterfaceSingleton::clear();
  test_socket_interface_loader_ = std::make_unique<Envoy::Network::SocketInterfaceLoader>(
      std::make_unique<Envoy::Network::TestSocketInterface>(
          [writev_matcher = writev_matcher_](Envoy::Network::TestIoSocketHandle* io_handle,
                                             const Buffer::RawSlice*,
                                             uint64_t) -> absl::optional<Api::IoCallUint64Result> {
            Network::IoSocketError* error_override = writev_matcher->returnOverride(io_handle);
            if (error_override) {
              return Api::IoCallUint64Result(
                  0, Api::IoErrorPtr(error_override, Network::IoSocketError::deleteIoError));
            }
            return absl::nullopt;
          }));
}

void SocketInterfaceSwap::IoHandleMatcher::setResumeWrites() {
  absl::MutexLock lock(&mutex_);
  mutex_.Await(absl::Condition(
      +[](Network::TestIoSocketHandle** matched_iohandle) { return *matched_iohandle != nullptr; },
      &matched_iohandle_));
  error_ = nullptr;
  matched_iohandle_->activateInDispatcherThread(Event::FileReadyType::Write);
}

} // namespace Envoy
