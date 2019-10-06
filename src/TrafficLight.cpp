#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <queue>
#include <future>

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
  
  std::unique_lock<std::mutex> unique_lock(_mutex);
  _condition.wait(unique_lock, [this] {return !_queue.empty();});
  
  T msg = std::move(_queue.back());
  _queue.pop_back();
  return msg;
  
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
  
      // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  
  std::lock_guard<std::mutex> unique_lock(_mutex);
  _queue.push_back(std::move(msg));
  _condition.notify_one();



}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    message_queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
  
  
  
}


    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
  
  void TrafficLight::waitForGreen() {
    while (true){
      std::this_thread::sleep_for(std::chrono::microseconds(1));
      auto current_phase = message_queue->receive();
      if (current_phase == green){
        return;
      }
    }
  }


TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
  
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

//virtual function which is executed in a thread

void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

  std::random_device rd;
  
  std::mt19937 eng(rd());
  std::uniform_int_distribution<> distribution(4, 6);

  std::unique_lock<std::mutex> lck(_mtx);
  std::cout << "Traffic Light #" << _id << "::Cycle Through Phases: thread id = " << std::this_thread::get_id() << std::endl;
    
  lck.unlock();

  int cycle_duration = distribution(eng);

  auto last_update = std::chrono::system_clock::now();
  while (true)
  {
    long time_since_last_update = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_update).count();

   
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (time_since_last_update >= cycle_duration)
    {
      if (_currentPhase == red)
      {
        _currentPhase = green;}
      
      else {
        _currentPhase = red;
      }

      auto msg = _currentPhase;
      auto is_sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, message_queue, std::move(msg));
      
      is_sent.wait();

      last_update = std::chrono::system_clock::now();
      cycle_duration = distribution(eng);
    }
  }
}

