#include "EventLoop.hpp"
#include <cstdio>
#include <cstdlib>
#include <sys/fcntl.h>
#include <unistd.h>

bool setFcntlToPipe(Event *e)
{
	if (fcntl(e->CtoPPipe[0], F_SETFL, O_NONBLOCK) == -1)
	{
		close(e->CtoPPipe[0]);
		close(e->CtoPPipe[1]);
		close(e->PtoCPipe[0]);
		close(e->PtoCPipe[1]);
		e->setStatusCode(500);
		return false;
	}
	if (fcntl(e->CtoPPipe[1], F_SETFL, O_NONBLOCK) == -1)
	{
		close(e->CtoPPipe[0]);
		close(e->CtoPPipe[1]);
		close(e->PtoCPipe[0]);
		close(e->PtoCPipe[1]);
		e->setStatusCode(500);
		return false;
	}
	if (fcntl(e->PtoCPipe[0], F_SETFL, O_NONBLOCK) == -1)
	{
		close(e->CtoPPipe[0]);
		close(e->CtoPPipe[1]);
		close(e->PtoCPipe[0]);
		close(e->PtoCPipe[1]);
		e->setStatusCode(500);
		return false;
	}
	if (fcntl(e->PtoCPipe[1], F_SETFL, O_NONBLOCK) == -1)
	{
		close(e->CtoPPipe[0]);
		close(e->CtoPPipe[1]);
		close(e->PtoCPipe[0]);
		close(e->PtoCPipe[1]);
		e->setStatusCode(500);
		return false;
	}
	return true;
}

void setEnv(Event *e)
{
	HttpreqHandler *reqHandler = static_cast<HttpreqHandler *>(e->getRequestHandler());
	std::string tmp;
	e->getCgiEnv()[0] = strdup("AUTH_TYPE=Basic");
	e->getCgiEnv()[1] = strdup(("CONTENT_LENGTH="+ reqHandler->getRequestInfo().contentLength).c_str());
	e->getCgiEnv()[2] = strdup(("CONTENT_TYPE=" + reqHandler->getRequestInfo().contentType).c_str());
	e->getCgiEnv()[3] = strdup("GATEWAY_INTERFACE=CGI/1.1");
	e->getCgiEnv()[4] = strdup(("PATH_INFO=" + reqHandler->getRequestInfo().path).c_str());
	e->getCgiEnv()[5] = strdup(("PATH_TRANSLATED=" + e->getRoute()).c_str());
	e->getCgiEnv()[6] = strdup(("QUERY_STRING=" + reqHandler->getRequestInfo().queryParam).c_str());
	tmp = inet_ntoa(e->getSocketInfo().socket_addr.sin_addr);
	e->getCgiEnv()[7] = strdup(("REMOTE_ADDR=" + tmp).c_str());
	e->getCgiEnv()[8] = strdup("REMOTE_HOST=");
	e->getCgiEnv()[9] = strdup("REMOTE_IDENT=");
	e->getCgiEnv()[10] = strdup("REMOTE_USER=");
	e->getCgiEnv()[11] = strdup(("REQUEST_METHOD=" + reqHandler->getRequestInfo().method).c_str());
	e->getCgiEnv()[12] = strdup(("REQUEST_URI=" + reqHandler->getRequestInfo().path).c_str());
	e->getCgiEnv()[13] = strdup(("SCRIPT_NAME=" + reqHandler->getRequestInfo().path).c_str());
	e->getCgiEnv()[14] = strdup("SERVER_NAME=cgi");
	char pt[10];
    sprintf(pt, "%d", e->getDefaultServerData()->getListen());
	tmp = pt;
	e->getCgiEnv()[15] = strdup(("SERVER_PORT=" + tmp).c_str());
	e->getCgiEnv()[16] = strdup("SERVER_PROTOCOL=HTTP/1.1");
	e->getCgiEnv()[17] = strdup("SERVER_SOFTWARE=webserv/1.0");
	if (reqHandler->getRequestInfo().reqHeaderMap.find("X-Secret-Header-For-Test") != reqHandler->getRequestInfo().reqHeaderMap.end())
	{
		e->getCgiEnv()[18] = strdup(("HTTP_X_SECRET_HEADER_FOR_TEST=" + reqHandler->getRequestInfo().reqHeaderMap.find("X-Secret-Header-For-Test")->second).c_str());
		e->getCgiEnv()[19] = NULL;
	}
	else 
	{
		e->getCgiEnv()[18] = NULL;
	}
}

bool EventLoop::processCgi(Event *e)
{
	/**
	 * first, check if the cgi file exists
	 * */
	e->setRoute(e->locationData->getRoot() + e->locationData->getCgiPass());
	if (stat(e->getRoute().c_str(), &e->statBuf) != 0)
	{
		e->setStatusCode(404);
		return false;
	}

	

	responseHandler *resHandler = static_cast<responseHandler *>(e->getResponseHandler());
	HttpreqHandler *reqHandler = static_cast<HttpreqHandler *>(e->getRequestHandler());

	/**
	 * 1. create pipe
	 * */
	if (pipe(e->CtoPPipe) == -1)
	{
		e->setStatusCode(500);
		return false;
	}
	if (pipe(e->PtoCPipe) == -1)
	{
		close(e->CtoPPipe[0]);
		close(e->CtoPPipe[1]);
		e->setStatusCode(500);
		return false;
	}


	(e->tmpOutFile = open(e->tmpOutFileName.c_str(), O_CREAT | O_NONBLOCK | O_RDWR, 0777 ));
	/* (e->tmpInFile = open(e->tmpInFileName.c_str(), O_CREAT | O_NONBLOCK | O_RDWR, 0777 )); */
	close(e->tmpOutFile);
	/* close(e->tmpInFile); */

	pipe(e->CtoPPipe);
	fcntl(e->CtoPPipe[0], F_SETFL, O_NONBLOCK);
	fcntl(e->CtoPPipe[1], F_SETFL, O_NONBLOCK);

	/**
	 * 3. fork
	 * */
	pid_t pid;
	if ((pid = fork())  == -1)
	{
		e->setStatusCode(500);
		return false;
	}

	/**
	 * 4. parent process
	 * */
	if (pid)
	{
		e->setStatusCode(200);
		e->childPid = pid;
		close(e->CtoPPipe[1]);
		if ((e->tmpOutFile = open(e->tmpOutFileName.c_str(), O_NONBLOCK | O_WRONLY)) == -1)
			std::cerr<<"error open file"<<e->tmpOutFileName<< errno<<std::endl;
		/* if ((e->tmpInFile = open(e->tmpInFileName.c_str(), O_NONBLOCK | O_RDONLY)) == -1) */
		/* 	std::cerr<<"error open file"<<e->tmpInFileName<< errno<<std::endl; */

		if (fcntl(e->tmpOutFile, F_SETFL, O_NONBLOCK) == -1)
			std::cerr<<"error fcntl"<<e->tmpOutFileName<< errno<<std::endl;
		/* if (fcntl(e->tmpInFile, F_SETFL, O_NONBLOCK) == -1) */
		/* 	std::cerr<<"error fcntl"<<e->tmpInFileName<< errno<<std::endl; */
			//reserve
		resHandler->getResBody().reserve(reqHandler->getRequestInfo().body.length());
		registerTmpFileWriteEvent(e);
		return true;
	}
	/**
	 * 5. child process
	 * */
	else 
	{
		setEnv(e);
		close(e->CtoPPipe[0]);
		setEnv(e);
		if ((e->tmpOutFile = open(e->tmpOutFileName.c_str(), O_RDONLY)) == -1)
			std::cerr<<"error open file"<<e->tmpOutFileName<< errno<<std::endl;
		/* if ((e->tmpInFile = open(e->tmpInFileName.c_str(), O_WRONLY)) == -1) */
		/* 	std::cout<<"error open file"<<e->tmpInFileName<< errno<<std::endl; */
		if (fcntl(e->tmpOutFile, F_SETFL, O_NONBLOCK) == -1)
			std::cerr<<"error fcntl"<<e->tmpOutFileName<< errno<<std::endl;
		/* if (fcntl(e->tmpInFile, F_SETFL, O_NONBLOCK) == -1) */
		/* 	std::cout<<"error fcntl"<<e->tmpInFileName<< errno<<std::endl; */

        if (dup2(e->tmpOutFile, STDIN_FILENO) == -1)
			std::cerr<<"dup2 error"<<errno<<std::endl;
        if (dup2(e->CtoPPipe[1], STDOUT_FILENO) == -1)
			std::cerr<<"dup2 error"<<errno<<std::endl;

		//실행
		char **env = new char*[e->getCgiEnv().size() + 1];
		for (size_t i = 0; i < e->getCgiEnv().size(); i++)
			env[i] = const_cast<char *>(e->getCgiEnv()[i]);
		env[e->getCgiEnv().size()] = NULL;
		if (execve(e->getRoute().c_str(), NULL, env) == -1)
		{
			std::cerr << "execve error" << std::endl;
			e->setStatusCode(404);
			exit(1);
		}
		exit(1);
	}
}
