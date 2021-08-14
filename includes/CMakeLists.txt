add_library(Parser INTERFACE Parser.hpp buildExpr.hpp buildExprClassesUtils.hpp
            util.hpp)
target_include_directories(Parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
