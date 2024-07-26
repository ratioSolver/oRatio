#pragma once

#ifdef ENABLE_API
#include "json.hpp"
#endif

namespace ratio
{
    /**
     * @brief Represents a timeline.
     *
     * The timeline class provides an interface for working with timelines.
     * It is an abstract base class that can be inherited to create specific timeline implementations.
     */
    class timeline
    {
    public:
        /**
         * @brief Destructor for the timeline class.
         *
         * The destructor is declared as virtual to ensure that derived classes can be properly destroyed.
         */
        virtual ~timeline() = default;

#ifdef ENABLE_API
        /**
         * @brief Extracts the timeline data as JSON.
         *
         * This function extracts the timeline data and returns it as a JSON object.
         *
         * @return The timeline data as a JSON object.
         */
        virtual json::json extract() const noexcept = 0;
#endif
    };
} // namespace ratio
