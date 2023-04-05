#ifndef IMODULE_HPP
# define IMODULE_HPP

#include <stdexcept>

class IModule
{
public:
	virtual void run() throw (std::runtime_error) = 0;
	virtual ~IModule() {}
};

#endif
