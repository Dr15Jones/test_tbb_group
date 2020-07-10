#include <thread>
#include <tbb/task.h>
#include <tbb/task_arena.h>
#include <atomic>
#include <iostream>
#include <functional>

template <typename F>
class FunctorTask : public tbb::task {
public:
  explicit FunctorTask(F f) : func_(std::move(f)) {}

  task* execute() override {
    func_();
    return nullptr;
  };

private:
  F func_;
};

template <typename ALLOC, typename F>
FunctorTask<F>* make_functor_task(ALLOC&& iAlloc, F f) {
  return new (iAlloc) FunctorTask<F>(std::move(f));
}


int main() {

  std::atomic<bool> passedTask{false};
  std::unique_ptr<std::function<void()>> callbackHandle;

  //the non TBB thread which does 'external' work
  // and then does a callback to tbb
  std::thread t{[&]()
  {
    while(not passedTask.load());
    std::cout <<"start time"<<std::endl;
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
    
    (*callbackHandle)();
    callbackHandle.reset();
  }};

  auto waitTask = new(tbb::task::allocate_root()) tbb::empty_task{};
  waitTask->set_ref_count(2);

  tbb::task_arena arena(tbb::task_arena::attach{});

  auto startTask = make_functor_task(tbb::task::allocate_root(), [&]() {
      std::cout<<"launching callback"<<std::endl;

      callbackHandle = std::make_unique<std::function<void()>>([&](){
	arena.enqueue([&]() {
	    auto callback = make_functor_task(tbb::task::allocate_root(), [&] () {
		std::cout <<"callback start"<<std::endl;
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);
		std::cout <<"callback done"<<std::endl;
		waitTask->decrement_ref_count();
	      });
	    tbb::task::spawn(*callback);
	  });
	});
      passedTask = true;
    });
  tbb::task::spawn(*startTask);

  waitTask->wait_for_all();
  
  tbb::task::destroy(*waitTask);

  t.join();
  return 0;
}
