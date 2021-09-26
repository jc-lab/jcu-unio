/**
 * @file	handle.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_EMITTER_H_
#define JCU_UNIO_EMITTER_H_

#include <memory>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <list>

#include "event.h"

namespace jcu {
namespace unio {

class Resource;

/**
 * event emitter
 */
class Emitter {
 public:
  virtual ~Emitter() = default;

 protected:
  bool inited_;
  InitEvent init_event_;

  class BaseHandler {
   public:
    virtual ~BaseHandler() = default;
    virtual void clear() = 0;
    virtual int size() const = 0;
  };

  template <typename U>
  class Handler : public BaseHandler {
   private:
    struct Listener {
      bool once;
      std::function<void(U& event, Resource& handle)> func;
    };
    std::list<Listener> callbacks_;

   public:
    void clear() override {
      callbacks_.clear();
    }
    int size() const override {
      return callbacks_.size();
    }
    int call(U& event, Resource& handle) {
      int count = 0;
      for (auto it = callbacks_.begin(); it != callbacks_.end(); count++) {
        it->func(event, handle);
        if (it->once) {
          it = callbacks_.erase(it);
        } else {
          it++;
        }
      }
      return count;
    }
    void on(std::function<void(U& event, Resource& handle)> callback) {
      callbacks_.emplace_back(std::move(Listener {false, std::move(callback)}));
    }
    void once(std::function<void(U& event, Resource& handle)> callback) {
      callbacks_.emplace_back(std::move(Listener {true, std::move(callback)}));
    }
  };

  std::unordered_map<size_t, std::unique_ptr<BaseHandler>> handlers_ {};

  void emitInit(InitEvent&& event) {
    emit(event);
    inited_ = true;
    init_event_ = std::move(event);
  }

  template <class U>
  Handler<U>* getHandler() {
    auto& p = handlers_[typeid(U).hash_code()];
    Handler<U>* q = static_cast<Handler<U>*>(p.get());
    if (!p) {
      q = new Handler<U>();
      p.reset(q);
    }
    return q;
  }

 public:
  Emitter() : inited_(false) {}

  /**
   * emit event
   * @return count
   */
  template <typename U>
  int emit(U& event) {
    auto it = handlers_.find(typeid(U).hash_code());
    if (it != handlers_.end()) {
      Handler<U>* handler = dynamic_cast<Handler<U>*>(it->second.get());
      if (handler) {
        return handler->call(event, dynamic_cast<Resource&>(*this));
      }
    }
    return 0;
  }

  template <typename U>
  void on(std::function<void(U& event, Resource& handle)> callback) {
    auto q = getHandler<U>();
    q->on(std::move(callback));
  }

  template <typename U>
  void once(std::function<void(U& event, Resource& handle)> callback) {
    auto q = getHandler<U>();
    q->once(std::move(callback));
  }

  template <typename U>
  void off() {
    auto it = handlers_.find(typeid(U).hash_code());
    if (it != handlers_.end()) {
      if (it->second) {
        it->second->clear();
      }
      handlers_.erase(it);
    }
  }

  void offAll() {
    for (auto it = handlers_.begin(); it != handlers_.end(); ) {
      if (it->second) {
        it->second->clear();
      }
      it = handlers_.erase(it);
    }
  }

  template <typename U>
  int getEventCount() {
    auto it = handlers_.find(typeid(U).hash_code());
    if (it != handlers_.end()) {
      if (it->second) {
        return it->second->size();
      }
    }
    return 0;
  }
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_EMITTER_H_
