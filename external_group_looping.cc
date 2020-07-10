#include <thread>
#include <atomic>
#include <iostream>
#include <functional>

#include <tbb/task_group.h>
#include <tbb/task_arena.h>


int main() {

  std::atomic<bool> passedTask{false};
  std::unique_ptr<std::function<void()>> callbackHandle;

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

  tbb::task_group group;
  tbb::task_arena arena(tbb::task_arena::attach{});

  std::atomic<bool> lastTaskRun{false};
  group.run( [&]() {
      std::cout<<"launching callback"<<std::endl;
      
      auto callback = std::make_unique<std::function<void()>>([&] () {
	  arena.enqueue([&]() {
	      group.run([&](){
		  std::cout <<"callback start"<<std::endl;
		  using namespace std::chrono_literals;
		  std::this_thread::sleep_for(2s);
		  std::cout <<"callback done"<<std::endl;
		  lastTaskRun = true;
		});
	    });
	});
      callbackHandle = std::move(callback);
      passedTask = true;
    });

  unsigned long long waitTries = 0;
  do {
    ++waitTries;
    group.wait();
  } while(not lastTaskRun.load());

  std::cout <<"nunmber of wait tries "<<waitTries<<std::endl;
  t.join();
  return 0;
}
