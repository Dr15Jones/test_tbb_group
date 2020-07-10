#include <thread>
#include <atomic>
#include <iostream>
#include <functional>
#include <memory>

#include <tbb/task_group.h>
#include <tbb/task_arena.h>

namespace test {

  class task_group_run_handle;

  class task_group : public tbb::internal::task_group_base {
  public:
    task_group () : task_group_base( tbb::task_group_context::concurrent_wait ) {}

#if __SUNPRO_CC
    template<typename F>
    void run( tbb::task_handle<F>& h ) {
      internal_run< tbb::internal::task_handle_task<F> >( h );
    }
#else
    using tbb::internal::task_group_base::run;
#endif

#if __TBB_CPP11_RVALUE_REF_PRESENT
    template<typename F>
    void run( F&& f ) {
      tbb::task::spawn( *prepare_task< tbb::internal::function_task< typename tbb::internal::strip<F>::type > >(std::forward<F>(f)) );
    }
#else
    template<typename F>
    void run(const F& f) {
      tbb::task::spawn( *prepare_task< tbb::internal::function_task<F> >(f) );
    }
#endif

    task_group_run_handle make_run_handle();

    template<typename F>
    tbb::task_group_status run_and_wait( const F& f ) {
      return internal_run_and_wait<const F>( f );
    }

    // TODO: add task_handle rvalues support
    template<typename F>
    tbb::task_group_status run_and_wait( tbb::task_handle<F>& h ) {
      h.mark_scheduled();
      return internal_run_and_wait< tbb::task_handle<F> >( h );
    }
  private:
    friend class task_group_run_handle;
    void increment() {
      my_root->increment_ref_count();
    }
    void decrement() {
      my_root->decrement_ref_count();
    }
  }; // class task_group

  class task_group_run_handle {
  private:
    friend class task_group;

    task_group_run_handle(task_group* group) : group_(group) {
      group_->increment();
    }
    task_group* group_;
  public:
    task_group_run_handle(task_group_run_handle const& iOther):
      group_(iOther.group_) {
      if(group_) {
	group_->increment();
      }
    }
    task_group_run_handle(task_group_run_handle&& iOther):
      group_(iOther.group_) {
      iOther.group_ = nullptr;
    }

    task_group_run_handle& operator=(task_group_run_handle const& iOther) {
      if(iOther.group_) { iOther.group_->increment(); }
      if(group_) {group_->decrement();}
      group_ = iOther.group_;
      return *this;
    }

    task_group_run_handle& operator=(task_group_run_handle&& iOther) {
      if(group_) { group_->decrement();}
      group_ = iOther.group_;
      iOther.group_ = nullptr;
      return *this;
    }

    ~task_group_run_handle(){
      if(group_) {
	group_->decrement();
      }
    }
    
#if __TBB_CPP11_RVALUE_REF_PRESENT
    template<typename F>
    void run( F&& f ) {
      group_->run(std::forward<F>(f));
    }
#else
    template<typename F>
    void run(const F& f) {
      group_->run(f);
    }
#endif
    
  };

  task_group_run_handle task_group::make_run_handle() {
    return task_group_run_handle(this);
  }


}


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


  test::task_group group;
  tbb::task_arena arena(tbb::task_arena::attach{});

  group.run( [&]() {
      std::cout<<"launching callback"<<std::endl;
      
      auto run_handle = group.make_run_handle();
      auto callback = std::make_unique<std::function<void()>>([run_handle,&arena] () {
	  arena.enqueue([run_handle]() {
	      auto newH = run_handle;
	      newH.run([]() {
		  std::cout <<"callback start"<<std::endl;
		  using namespace std::chrono_literals;
		  std::this_thread::sleep_for(2s);
		  std::cout <<"callback done"<<std::endl;
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
