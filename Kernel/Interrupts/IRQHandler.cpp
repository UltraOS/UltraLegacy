#include "IRQHandler.h"
#include "IRQManager.h"

namespace kernel {

IRQHandler::IRQHandler(Type type, u16 vector)
    : m_type(type)
{
    IRQManager::the().register_handler(*this, vector);
}

IRQHandler::~IRQHandler()
{
    IRQManager::the().unregister_handler(*this);
}

}