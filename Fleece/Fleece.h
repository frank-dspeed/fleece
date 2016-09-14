//
//  Fleece.h
//  Fleece
//
//  Created by Jens Alfke on 5/13/16.
//  Copyright (c) 2016 Couchbase. All rights reserved.
//

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

    // This is the C API! For the C++ API, see Fleece.hh.


    /** \defgroup Fleece Fleece
        @{ */

    /** \name Types and Basic Functions
        @{ */


    //////// TYPES

#ifndef FL_IMPL
    typedef const struct _FLValue* FLValue;     ///< A reference to a value of any type.
    typedef const struct _FLArray* FLArray;     ///< A reference to an array value.
    typedef const struct _FLDict*  FLDict;      ///< A reference to a dictionary (map) value.
    typedef struct _FLEncoder* FLEncoder;

    /** A simple reference to a block of memory. Does not imply ownership. */
    typedef struct {
        const void *buf;
        size_t size;
    } FLSlice;
#endif

#define FL_SLICE_DEFINED


    /** A block of memory returned from an API call. The caller takes ownership, may modify the
        bytes, and must call FLSliceFree when done. */
    typedef struct {
        void *buf;
        size_t size;
    } FLSliceResult;


    /** Frees the memory of a FLSliceResult. */
    void FLSlice_Free(FLSliceResult);


    /** Lexicographic comparison of two slices; basically like memcmp(), but taking into account
        differences in length. */
    int FLSlice_Compare(FLSlice, FLSlice);


    /** Types of Fleece values. Basically JSON, with the addition of Data (raw blob). */
    typedef enum {
        kFLUndefined = -1,  // Type of a NULL FLValue (i.e. no such value)
        kFLNull = 0,
        kFLBoolean,
        kFLNumber,
        kFLString,
        kFLData,
        kFLArray,
        kFLDict
    } FLValueType;


    typedef enum {
        NoError = 0,
        MemoryError,        // Out of memory, or allocation failed
        OutOfRange,         // Array index or iterator out of range
        InvalidData,        // Bad input data (NaN, non-string key, etc.)
        EncodeError,        // Structural error encoding (missing value, too many ends, etc.)
        JSONError,          // Error parsing JSON
        UnknownValue,       // Unparseable data in a Value (corrupt? Or from some distant future?)
        InternalError,      // Something that shouldn't happen
    } FLError;

    /** @} */


    //////// VALUE


    /** \name Parsing And Converting Values
        @{ */

    /** Returns a reference to the root value in the encoded data.
        Validates the data first; if it's invalid, returns NULL.
        Does NOT copy or take ownership of the data; the caller is responsible for keeping it
        intact. Any changes to the data will invalidate any FLValues obtained from it. */
    FLValue FLValue_FromData(FLSlice data, FLError *outError);

    /** Returns a pointer to the root value in the encoded data, _without_ validating.
        This is a lot faster, but "undefined behavior" occurs if the data is corrupt... */
    FLValue FLValue_FromTrustedData(FLSlice data, FLError *outError);

    /** Directly converts JSON data to Fleece-encoded data.
        You can then call FLValueFromTrustedData to get the root as a Value. */
    FLSliceResult FLData_ConvertJSON(FLSlice json, FLError *outError);

    /** Produces a human-readable dump of the Value encoded in the data. */
    FLSliceResult FLData_Dump(FLSlice data);

    /** @} */
    /** \name Value Accessors
        @{ */

    // Value accessors -- safe to call even if the value is NULL.

    /** Returns the data type of an arbitrary Value.
        (If the value is a NULL pointer, returns kFLUndefined.) */
    FLValueType FLValue_GetType(FLValue);

    /** Returns true if the value is non-NULL and is represents an integer. */
    bool FLValue_IsInteger(FLValue);

    /** Returns true if the value is non-NULL and represents an _unsigned_ integer that can only
        be represented natively as a `uint64_t`. In that case, you should not call `FLValueAsInt`
        because it will return an incorrect (negative) value; instead call `FLValueAsUnsigned`. */
    bool FLValue_IsUnsigned(FLValue);

    /** Returns true if the value is non-NULL and represents a 64-bit floating-point number. */
    bool FLValue_IsDouble(FLValue);

    /** Returns a value coerced to boolean. This will be true unless the value is NULL (undefined),
        null, false, or zero. */
    bool FLValue_AsBool(FLValue);

    /** Returns a value coerced to an integer. True and false are returned as 1 and 0, and
        floating-point numbers are rounded. All other types are returned as 0.
        Warning: Large 64-bit unsigned integers (2^63 and above) will come out wrong. You can
        check for these by calling `FLValueIsUnsigned`. */
    int64_t FLValue_AsInt(FLValue);

    /** Returns a value coerced to an unsigned integer.
        This is the same as `FLValueAsInt` except that it _can't_ handle negative numbers, but
        does correctly return large `uint64_t` values of 2^63 and up. */
    uint64_t FLValue_AsUnsigned(FLValue);

    /** Returns a value coerced to a 32-bit floating point number. */
    float FLValue_AsFloat(FLValue);

    /** Returns a value coerced to a 64-bit floating point number. */
    double FLValue_AsDouble(FLValue);

    /** Returns the exact contents of a string or data value, or null for all other types. */
    FLSlice FLValue_AsString(FLValue);

    /** If a FLValue represents an array, returns it cast to FLArray, else NULL. */
    FLArray FLValue_AsArray(FLValue);

    /** If a FLValue represents a dictionary, returns it as an FLDict, else NULL. */
    FLDict FLValue_AsDict(FLValue);

    /** Returns a string representation of any scalar value. Data values are returned in raw form.
        Arrays and dictionaries don't have a representation and will return NULL. */
    FLSliceResult FLValue_ToString(FLValue);

    /** Encodes a Fleece value as JSON (or a JSON fragment.) 
        Any Data values will become base64-encoded JSON strings. */
    FLSliceResult FLValue_ToJSON(FLValue);


    //////// ARRAY


    /** @} */
    /** \name Arrays
        @{ */

    /** Returns the number of items in an array, or 0 if the pointer is NULL. */
    uint32_t FLArray_Count(FLArray);

    /** Returns an value at an array index, or NULL if the index is out of range. */
    FLValue FLArray_Get(FLArray, uint32_t index);


    /** Opaque array iterator. Put one on the stack and pass its address to
        `FLArrayIteratorBegin`. */
    typedef struct {
        void* _private1;
        uint32_t _private2;
        bool _private3;
        void* _private4;
    } FLArrayIterator;

    /** Initializes a FLArrayIterator struct to iterate over an array.
        Call FLArrayIteratorGetValue to get the first item, then FLArrayIteratorNext. */
    void FLArrayIterator_Begin(FLArray, FLArrayIterator*);

    /** Returns the current value being iterated over. */
    FLValue FLArrayIterator_GetValue(const FLArrayIterator*);

    /** Advances the iterator to the next value, or returns false if at the end. */
    bool FLArrayIterator_Next(FLArrayIterator*);


    //////// DICT


    /** @} */
    /** \name Dictionaries
        @{ */

    /** Returns the number of items in a dictionary, or 0 if the pointer is NULL. */
    uint32_t FLDict_Count(FLDict);

    /** Looks up a key in a _sorted_ dictionary, returning its value.
        Returns NULL if the value is not found or if the dictionary is NULL. */
    FLValue FLDict_Get(FLDict, FLSlice keyString);

    /** Looks up a key in an unsorted (or sorted) dictionary. Slower than FLDictGet. */
    FLValue FLDict_GetUnsorted(FLDict, FLSlice keyString);


    /** Opaque dictionary iterator. Put one on the stack and pass its address to
        FLDictIteratorBegin. */
    typedef struct {
        void* _private1;
        uint32_t _private2;
        bool _private3;
        void* _private4;
        void* _private5;
    } FLDictIterator;

    /** Initializes a FLDictIterator struct to iterate over a dictionary.
        Call FLDictIteratorGetKey and FLDictIteratorGetValue to get the first item,
        then FLDictIteratorNext. */
    void FLDictIterator_Begin(FLDict, FLDictIterator*);

    /** Returns the current key being iterated over. */
    FLValue FLDictIterator_GetKey(const FLDictIterator*);

    /** Returns the current value being iterated over. */
    FLValue FLDictIterator_GetValue(const FLDictIterator*);

    /** Advances the iterator to the next value, or returns false if at the end. */
    bool FLDictIterator_Next(FLDictIterator*);


    /** Opaque key for a dictionary. You are responsible for creating space for these; they can
        go on the stack, on the heap, inside other objects, anywhere. 
        Be aware that the lookup operations that use these will write into the struct to store
        "hints" that speed up future searches. */
    typedef struct {
        void* _private1[3];
        uint32_t _private2;
        bool _private3;
    } FLDictKey;

    /** Initializes an FLDictKey struct with a key string.
        @param key  Pointer to a raw uninitialized FLDictKey struct.
        @param string  The key string (UTF-8).
        @param cachePointers  If true, the FLDictKey is allowed to cache a direct Value pointer
                representation of the key. This provides faster lookup, but means that it can
                only ever be used with Dicts that live in the same stored data buffer. */
    void FLDictKey_Init(FLDictKey* key, FLSlice string, bool cachePointers);

    /** Looks up a key in a dictionary using an FLDictKey. If the key is found, "hint" data will
        be stored inside the FLDictKey that will speed up subsequent lookups. */
    FLValue FLDict_GetWithKey(FLDict, FLDictKey*);

    /** Looks up multiple dictionary keys in parallel, which can be much faster than individual lookups.
        @param dict  The dictionary to search
        @param keys  Array of keys; MUST be sorted as per FLSliceCompare.
        @param values  The found values will be written here, or NULLs for missing keys.
        @param count  The number of keys in keys[] and the capacity of values[].
        @return  The number of keys that were found. */
    size_t FLDict_GetWithKeys(FLDict dict, FLDictKey keys[], FLValue values[], size_t count);


    //////// ENCODER


    /** @} */
    /** \name Encoder
        @{ */

    /** Creates a new encoder, for generating Fleece data. Call FLEncoder_Free when done. */
    FLEncoder FLEncoder_New(void);

    /** Creates a new encoder, allowing some options to be customized. You generally won't
        need to call this.
        @param reserveSize  The number of bytes to preallocate for the output. (Default is 256)
        @param uniqueStrings  If true, string values that appear multiple times will be written
            as a single shared value. This saves space but makes encoding slightly slower.
            You should only turn this off if you know you're going to be writing large numbers
            of non-repeated strings. Note also that the `cachePointers` option of FLDictKey
            will not work if `uniqueStrings` is off. (Default is true)
        @param sortKeys  If true, dictionary keys are written in sorted order. This speeds up
            lookup, especially with large dictionaries, but slightly slows down encoding.
            You should only turn this off if you care about encoding speed but not access time,
            and will be writing large dictionaries with lots of keys. Note that if you turn
            this off, you can only look up keys with FLDictGetUnsorted(). (Default is true) */
    FLEncoder FLEncoder_NewWithOptions(size_t reserveSize, bool uniqueStrings, bool sortKeys);

    /** Frees the space used by an encoder. */
    void FLEncoder_Free(FLEncoder);

    /** Resets the state of an encoder without freeing it. It can then be reused to encode
        another value. */
    void FLEncoder_Reset(FLEncoder);

    // Note: The functions that write to the encoder do not return error codes, just a 'false'
    // result on error. The actual error is attached to the encoder and can be accessed by calling
    // FLEncoder_GetError or FLEncoder_End.
    // After an error occurs, the encoder will ignore all subsequent writes.

    /** Writes a `null` value to an encoder. (This is an explicitly-stored null, like the JSON
        `null`, not the "undefined" value represented by a NULL FLValue pointer.) */
    bool FLEncoder_WriteNull(FLEncoder);

    /** Writes a boolean value (true or false) to an encoder. */
    bool FLEncoder_WriteBool(FLEncoder, bool);

    /** Writes an integer to an encoder. The parameter is typed as `int64_t` but you can pass any
        integral type (signed or unsigned) except for huge `uint64_t`s. */
    bool FLEncoder_WriteInt(FLEncoder, int64_t);

    /** Writes an unsigned integer to an encoder. This function is only really necessary for huge
        64-bit integers greater than or equal to 2^63, which can't be represented as int64_t. */
    bool FLEncoder_WriteUInt(FLEncoder, uint64_t);

    /** Writes a 32-bit floating point number to an encoder.
        (Note: as an implementation detail, if the number has no fractional part and can be
        represented exactly as an integer, it'll be encoded as an integer to save space. This is
        transparent to the reader, since if it requests the value as a float it'll be returned
        as floating-point.) */
    bool FLEncoder_WriteFloat(FLEncoder, float);

    /** Writes a 64-bit floating point number to an encoder.
        (Note: as an implementation detail, if the number has no fractional part and can be
        represented exactly as an integer, it'll be encoded as an integer to save space. This is
        transparent to the reader, since if it requests the value as a float it'll be returned
        as floating-point.) */
    bool FLEncoder_WriteDouble(FLEncoder, double);

    /** Writes a string to an encoder. The string must be UTF-8-encoded and must not contain any
        zero bytes.
        Do _not_ use this to write a dictionary key; use FLEncoder_WriteKey instead. */
    bool FLEncoder_WriteString(FLEncoder, FLSlice);

    /** Writes a raw data value (a blob) to an encoder. This can contain absolutely anything
        including null bytes. Note that this data type has no JSON representation, so if the
        resulting value is ever encoded to JSON via FLValueToJSON, it will be transformed into
        a base64-encoded string. */
    bool FLEncoder_WriteData(FLEncoder, FLSlice);


    /** Begins writing an array value to an encoder. This pushes a new state where each
        subsequent value written becomes an array item, until FLEncoder_EndArray is called.
        @param reserveCount  Number of array elements to reserve space for. If you know the size
            of the array, providing it here speeds up encoding slightly. If you don't know,
            just use zero. */
    bool FLEncoder_BeginArray(FLEncoder, size_t reserveCount);

    /** Ends writing an array value; pops back the previous encoding state. */
    bool FLEncoder_EndArray(FLEncoder);


    /** Begins writing a dictionary value to an encoder. This pushes a new state where each
        subsequent key and value written are added to the dictionary, until FLEncoder_EndDict is
        called.
        Before adding each value, you must call FLEncoder_WriteKey (_not_ FLEncoder_WriteString!),
        to write the dictionary key.
        @param reserveCount  Number of dictionary items to reserve space for. If you know the size
            of the dictionary, providing it here speeds up encoding slightly. If you don't know,
            just use zero. */
    bool FLEncoder_BeginDict(FLEncoder, size_t reserveCount);

    /** Specifies the key for the next value to be written to the current dictionary. */
    bool FLEncoder_WriteKey(FLEncoder, FLSlice);

    /** Ends writing a dictionary value; pops back the previous encoding state. */
    bool FLEncoder_EndDict(FLEncoder);


    /** Ends encoding; if there has been no error, it returns the encoded data, else null.
        This does not free the FLEncoder; call FLEncoder_Free (or FLEncoder_Reset) next. */
    FLSliceResult FLEncoder_Finish(FLEncoder, FLError*);

    /** Returns the error code of an encoder, or NoError (0) if there's no error. */
    FLError FLEncoder_GetError(FLEncoder e);

    /** Returns the error message of an encoder, or NULL if there's no error. */
    const char* FLEncoder_GetErrorMessage(FLEncoder e);

    
    /** @} */

#ifdef __cplusplus
}
#endif