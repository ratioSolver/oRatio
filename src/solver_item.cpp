#include "solver_item.hpp"
#include "logging.hpp"

namespace ratio
{
    std::shared_ptr<riddle::item> enum_item::get(const std::string &name) const noexcept
    {
        LOG_ERR("enum_item::get not implemented");
        return nullptr;
    }
} // namespace ratio