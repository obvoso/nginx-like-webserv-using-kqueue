#include "EventLoop.hpp"
#include <sys/event.h>

void EventLoop::readCallback(struct kevent *e)
{
	std::cout << "\033[32m"; 
	std::cout<<"Read callback"<<std::endl;

	//set event
	Event *e_udata = static_cast<Event *>(e->udata);

	switch (e_udata->getEventType()){
		//check us server socket
		case E_SERVER_SOCKET:
			e_serverSocketReadCallback(e, e_udata);
			break;
		//check is client socket read
		case E_CLIENT_SOCKET:
			e_clientSocketReadCallback(e, e_udata);
			break;
		//check is pipe read
		case E_PIPE:
			e_pipeReadCallback(e, e_udata);
			break;
		//check is file read
		case E_FILE:
			e_fileReadCallback(e, e_udata);
			break;
		default:
			std::cout<<"unknown event type"<<std::endl;
			break;
	}
}

/**
 * @brief serverSocket의 read event callback.
 *
 * server socket에서 발생한 read event에 대해 
 * client socket을 생성하고, client socket에 대한 read event를 등록한다.
 * @param e
 */
void EventLoop::e_serverSocketReadCallback(struct kevent *e, Event *e_udata)
{
	std::cout << "\033[33m"; 
	std::cout<<"SERVER SOCKET CALLBACK"<<std::endl;
	//we need to verify http
	if (e_udata->getServerType() == HTTP_SERVER)
	{
		//create client socket
		//모든 pipe, file의 이벤트는 client socket에서 부터 시작한다.
		Event *new_udata = Event::createNewClientSocketEvent(e_udata);

		//handler 객체 설정
		new_udata->setRequestHandler(new HttpreqHandler());
		new_udata->setResponseHandler(new Response());

		//kqueue에 event 등록
		registerClientSocketReadEvent(new_udata);
	}
}

void EventLoop::e_clientSocketReadCallback(struct kevent *e, Event *e_udata)
{
	std::cout << "\033[34m"; 
	std::cout<<"client socket callback"<<std::endl;
	//socket의 readfilter-> EOF flag는 client의 disconnect.
	if (e->flags == EV_EOF)
	{
		close(e->ident);
		std::cout<<"client disconnected"<<std::endl;
	}

	//Http Server인 소켓에서 연결된 client_fd에 대한 read처리
	if (e_udata->getServerType() == HTTP_SERVER)
	{
		//read from client socket
		int client_fd = e_udata->getClientFd();
		ssize_t read_len = read(client_fd, HttpServer::getInstance().getHttpBuffer(), 1024);
		HttpServer::getInstance().getStringBuffer().insert(0, HttpServer::getInstance().getHttpBuffer(), read_len);
		if (read_len == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			else
				throw std::runtime_error("Failed to read from client socket, unknown err\n");
		}
		else
		{
			std::cout<<"[[[[[[[CLIENT REQUEST START]]]]]]]]"<<std::endl;
			std::cout<<HttpServer::getInstance().getHttpBuffer()<<std::endl;
			std::cout<<"[[[[[[[CLIENT REQUEST END]]]]]]]]"<<std::endl;

			HttpreqHandler *reqHandler = static_cast<HttpreqHandler *>(e_udata->getRequestHandler());

			//handle request
			e_udata->setBuffer(&HttpServer::getInstance().getStringBuffer());
			try {
				reqHandler->handle(e_udata);
			} catch (HttpException &exception) {
				/**
				 * client request exception handling by 
				 * register write event to client_fd, and finally send error response
				 * */
				registerRequestHttpExceptionEvent(e_udata);
				/**
				 * in here, the read event for this client_fd disabled
				 * and the write event to client socket add and enabled
				 * */
				return ;
			}



			//handle response by request
			/**
			 * pending state => client로부터 data를 더 받아야하는 상태
			 * */
			if (reqHandler->getIsPending())
				return ;
			else
			{
				/**
				 * response를 보내야하는 상태임.
				 * event disable?
				 * EV_DISBALE => kevent함수가 event를 받아오지않도록 설정한다.
				 * */
				EV_SET(e, client_fd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);

				/**
				 *
				 * handle error
				 *
				 * 1. check host name
				 * 2. check the uri
				 * */


				static_cast<Response *>(e_udata->getResponseHandler())->handle(e_udata);
				/**
				 * if need file i/o
				 * first, if method == POST -> file write event
				 * second, if method == GET && not CGI_PASS -> file read event
				 * then?
				 * */
				/**
				 * else if need cgi(pipe)
				 * 1. open pipe and set NON_BLOCK
				 * 2. set readfilter && E_pipeevent to server side pipe
				 * 3. fork
				 * 		3-1. parent: return ;
				 * 		3-2. child: execve by cgi environment in response handler
				 * */
			}
		}
	}
}

/**
* Fifos, Pipes
* Returns when there is data to read; data contains the number of bytes available.
* When the last writer disconnects, the filter will set EV_EOF in flags.
* This may be cleared by passing in EV_CLEAR, at which point the filter will
* resume waiting for data to become available before returning.
**/

void EventLoop::e_pipeReadCallback(struct kevent *e, Event *e_udata)
{
	std::cout << "\033[35m"; 
	std::cout<<"pipe callback"<<std::endl;
	if (e_udata->getServerType() == HTTP_SERVER)
	{
		//eof flag있어야 writer가 작성 끝낸거임.
		if (e->flags & EV_EOF)
		{
			//process pipe read 
		}
	}
}

void EventLoop::e_fileReadCallback(struct kevent *e, Event *e_udata)
{
	std::cout << "\033[36m"; 
	std::cout<<"file callback"<<std::endl;
	if (e_udata->getServerType() == HTTP_SERVER)
	{
		//eof flag있어야 writer가 작성 끝낸거임.
		if (e->flags & EV_EOF)
		{
			//process pipe read 
		}
	}
}