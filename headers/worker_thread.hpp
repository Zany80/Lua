/**
 * Based on StackOverflow answer https://stackoverflow.com/questions/40553609/approach-of-using-an-stdatomic-compared-to-stdcondition-variable-wrt-pausing
 */

// TODO: make less messy. And preferably a class, if only for consistency.

#pragma once

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>

struct worker_thread {
	worker_thread( std::function<void()> t, bool play = true ):
	execute(play),
		task(std::move(t))
	{
		thread = std::async( std::launch::async, [this]{
			work();
		});
	}
	// move is not safe.  If you need this movable,
	// use unique_ptr<worker_thread>.
	worker_thread(worker_thread&& )=delete;
	~worker_thread() {
		if (!exit) finalize();
		wait();
	}
	void finalize() {
		std::unique_lock<std::mutex> l = lock();
		exit = true;
		cv.notify_one();
	}
	void pause() {
		std::unique_lock<std::mutex> l = lock();
		execute = false;
	}
	void play() {
		std::unique_lock<std::mutex> l = lock();
		execute = true;
		cv.notify_one();
	}
	void waitFrame() {
		while (running())
			std::this_thread::sleep_for(std::chrono::microseconds(50));
	}
	void wait() {
		o_assert(exit);
		thread.get();
	}
	bool running() {
		std::unique_lock<std::mutex> l = lock();
		return execute;
	}
	bool waitForResume() {
		// Used by e.g. the emit function to allow the main thread to pause mid-execution
		// Returns false is should exit.
		std::unique_lock<std::mutex> l = lock();
		cv.wait( l, [&]{
			return exit || execute;
		});
		return !exit;
	}
private:
	void work() {
		while(true) {
			bool done = false;
			{
				std::unique_lock<std::mutex> l = lock();
				cv.wait( l, [&]{
					return exit || execute;
				});
				done = exit; // have lock here
			}
			if (done) break;
			task();
		}
	}
	std::unique_lock<std::mutex> lock() {
		 return std::unique_lock<std::mutex>(m);
	}
	std::mutex m;
	std::condition_variable cv;
	bool exit = false;
	bool execute = true;
	std::function<void()> task;
	std::future<void> thread;
};
