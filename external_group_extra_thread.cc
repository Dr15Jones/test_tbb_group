#include <thread>
#include <atomic>
#include <iostream>
#include <functional>
#include <condition_variable>
#include <mutex>

#include <tbb/task_group.h>
#include <tbb/task_arena.h>


int main() {

  std::atomic<bool> passedTask{false};
  std::unique_ptr<std::function<void()>> callbackHandle;

  //The mutex is held until the last task is run
  std::mutex mutex;
  std::unique_lock<std::mutex> topLock(mutex);

  tbb::task_group group;
  //use an 'extra' thread to hold group wait open
  group.run([&mutex]() {
      std::cout <<"holding wait open"<<std::endl;
      std::unique_lock<std::mutex> lock(mutex);
      std::cout <<"releasing wait open"<<std::endl;
    });

  tbb::task_arena arena(tbb::task_arena::attach{});

  //the non TBB thread which does 'external' work
  // and then does a callback to tbb
  std::thread t{[&passedTask, &callbackHandle]()
  {
    while(not passedTask.load());
    std::cout <<"start time"<<std::endl;
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
    
    (*callbackHandle)();
    callbackHandle.reset();
  }};

  group.run( [&]() {
      std::cout<<"launching callback"<<std::endl;

      auto callback = std::make_unique<std::function<void()>>([&] () {
	  arena.enqueue([&]() {
	      group.run([&](){
		  std::cout <<"callback start"<<std::endl;
		  using namespace std::chrono_literals;
		  std::this_thread::sleep_for(2s);
		  std::cout <<"callback done"<<std::endl;    
		  topLock.unlock();
		});
	    });
	});
      callbackHandle = std::move(callback);
      
      passedTask = true;
    });

  group.wait();
  std::cout <<"done waiting"<<std::endl;

  t.join();
  return 0;
}
