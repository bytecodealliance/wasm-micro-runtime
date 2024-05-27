/*
 * The WebAssembly Live Migration Project
 *
 *  By: Aibo Hu
 *      Yiwei Yang
 *      Brian Zhao
 *      Andrew Quinn
 *
 *  Copyright 2024 Regents of the Univeristy of California
 *  UC Santa Cruz Sluglab.
 */

#ifndef MVVM_WAMR_SERIALIZER_H
#define MVVM_WAMR_SERIALIZER_H

#if __has_include(<expected>) && __cplusplus > 202002L
#include <type_traits>

template<typename T, typename K>
concept SerializerTrait = requires(T &t, K k)
{
    {
        t->dump_impl(k)
        } -> std::same_as<void>;
    {
        t->restore_impl(k)
        } -> std::same_as<void>;
};

template<typename T, typename K>
concept CheckerTrait = requires(T &t, K k)
{
    {
        t->dump_impl(k)
        } -> std::same_as<void>;
    {
        t->equal_impl(k)
        } -> std::convertible_to<bool>;
};

template<typename T, typename WriteDataType>
concept WriterStreamTrait = requires(T &t, const WriteDataType *data,
                                     std::size_t size)
{
    // Requires a write method that accepts WriteDataType and returns void
    // or a boolean.
    {
        t.write(data, size)
        } -> std::same_as<bool>;
};

template<typename T, typename ReadDataType>
concept ReaderStreamTrait = requires(T &t, ReadDataType *data, std::size_t size)
{
    // Requires a read method that accepts a pointer to ReadDataType and
    // size, returns bool.
    {
        t.read(data, size)
        } -> std::same_as<bool>;
    // Requires an ignore method that accepts size and returns bool.
    {
        t.ignore(size)
        } -> std::same_as<bool>;
    // Requires a tellg method that returns std::size_t.
    {
        t.tellg()
        } -> std::same_as<std::size_t>;
};
#endif
#endif // MVVM_WAMR_SERIALIZER_H
