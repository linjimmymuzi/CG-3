set(GLM_VERSION "0.9.7")
set(GLM_INCLUDE_DIRS "/Users/muzi/Documents/GitHub/CG-3/hw3-starterCode/external/glm")

if (NOT CMAKE_VERSION VERSION_LESS "3.0")
    include("${CMAKE_CURRENT_LIST_DIR}/glmTargets.cmake")
endif()
