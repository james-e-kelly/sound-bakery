#pragma once

#define NOT_COPYABLE(ObjectType)                        \
    ObjectType(const ObjectType&)             = delete; \
    ObjectType(const ObjectType&&)            = delete; \
    ObjectType& operator=(const ObjectType&)  = delete; \
    ObjectType& operator=(const ObjectType&&) = delete;