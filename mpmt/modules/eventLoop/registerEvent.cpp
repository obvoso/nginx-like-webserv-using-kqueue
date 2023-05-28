#include "EventLoop.hpp"
#include <sys/event.h>

void EventLoop::registerClientSocketReadEvent(Event *e)
{
	//client socket을 읽기전용으로  kqueue에 등록
	EV_SET(&(dummyEvent), e->getClientFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register client socket read with kqueue\n");
}

void EventLoop::registerPipeReadEvent(Event *e)
{
	EV_SET(&(dummyEvent), e->getPipeFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register pipe read with kqueue\n");
}

void EventLoop::registerFileReadEvent(Event *e)
{
	EV_SET(&(dummyEvent), e->getFileFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register file read with kqueue\n");
}

void EventLoop::registerClientSocketWriteEvent(Event *e)
{
	//client socket을 읽기전용으로  kqueue에 등록
	EV_SET(&(dummyEvent), e->getClientFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register client socket write with kqueue\n");
}

void EventLoop::registerPipeWriteEvent(Event *e)
{
	EV_SET(&(dummyEvent), e->getPipeFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register pipe write with kqueue\n");
}

/**
 * @brief register file write Event
 *
 * would be used in POST method
 *
 * @param e
 */
void EventLoop::registerFileWriteEvent(Event *e)
{
	EV_SET(&(dummyEvent), e->getFileFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, e);
	if (kevent(this->kq_fd, &(dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register file write with kqueue\n");
}

/**
 * @brief register http exception event
 * first, disable read event to client fd.
 * second, register write event to client socket.
 * and then, when the client socket write event catched in writeCallback
 * @param e
 */
void EventLoop::registerRequestHttpExceptionEvent(Event *e)
{
	//disable client fd read event
	EV_SET(&this->dummyEvent, e->getClientFd(), EVFILT_READ, EV_DISABLE | EV_DELETE, 0, 0, 0);
	if (kevent(this->kq_fd, &(this->dummyEvent), 1, NULL, 0, NULL) == -1) 
		throw std::runtime_error("Failed to register request http exception with kqueue\n");

	//register client fd write event
	registerClientSocketWriteEvent(e);
}

void EventLoop::registerResponseHttpExceptionEvent(Event *e)
{}