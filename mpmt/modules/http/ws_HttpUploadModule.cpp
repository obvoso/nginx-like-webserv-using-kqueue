#include "ws_HttpUploadModule.hpp"
#include <sys/fcntl.h>


bool ws_HttpUploadModule::processEvent(Event *e)
{
	std::cout<<"uploda store: "<<e->locationData->getUploadStore()<<std::endl;
	/**
	 * POST method do not allow create directory
	 * */
	if (e->getResource().back() == '/')
	{
		// 400? 403?
		std::cout<<"do not allow dir creation"<<std::endl;
		e->setStatusCode(403);
		return false;
	}

	std::string filePath = e->locationData->getRoot() + "/" + e->locationData->getUploadStore() + "/"+ e->getResource();
	std::cout<<"upload file path: "<<filePath<<std::endl;
	if((e->file_fd = open(filePath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
	{
		std::cout<<"cannot open file"<<std::endl;
		e->setStatusCode(500);
		return false;
	}

	if ((fcntl(e->file_fd, F_SETFL, O_NONBLOCK)) == -1)
	{
		std::cout<<"cannot set nonblock"<<std::endl;
		e->setStatusCode(500);
		return false;
	}
	e->setStatusCode(201);
	return true;
}