//
// JSONConverter.hh
//
// Copyright (c) 2015 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include "Encoder.hh"
#include "Doc.hh"
#include "FleeceException.hh"
#include "fleece/slice.hh"
#include <map>

extern "C" {
    struct jsonsl_state_st;
    struct jsonsl_st;
}

namespace fleece { namespace impl {

    /** Parses JSON data and writes the values in it to a Fleece encoder. */
    class JSONConverter {
    public:
        JSONConverter(Encoder&) noexcept;
        ~JSONConverter();

        /** Parses JSON data and writes the values to the encoder.
            @return  True if parsing succeeded, false if the JSON is invalid. */
        bool encodeJSON(slice json);

        /** See jsonsl_error_t for error codes, plus a few more defined below. */
        int jsonError() noexcept                {return _jsonError;}
        ErrorCode errorCode() noexcept          {return _errorCode;}
        const char* errorMessage() noexcept;
        
        /** Byte offset in input where error occurred */
        size_t errorPos() noexcept              {return _errorPos;}

        /** Extra error codes beyond those in jsonsl_error_t. */
        enum {
            kErrTruncatedJSON = 1000,
            kErrExceptionThrown
        };

        /** Resets the converter, as though you'd deleted it and constructed a new one. */
        void reset();

        /** Convenience method to convert JSON to Fleece data. Throws FleeceException on error. */
        static alloc_slice convertJSON(slice json, SharedKeys *sk =nullptr);

    //private:
        void push(struct jsonsl_state_st *state NONNULL);
        void pop(struct jsonsl_state_st *state NONNULL);
        int gotError(int err, size_t pos) noexcept;
        int gotError(int err, const char *errat) noexcept;
        void gotException(ErrorCode code, const char *what NONNULL, size_t pos) noexcept;

    private:
        typedef std::map<size_t, uint64_t> startToLengthMap;

        Encoder &_encoder;                  // encoder to write to
        struct jsonsl_st * _jsn {nullptr};  // JSON parser
        int _jsonError {0};                 // Parse error from jsonsl
        ErrorCode _errorCode {NoError};
        std::string _errorMessage;
        size_t _errorPos {0};               // Byte index where parse error occurred
        slice _input;                       // Current JSON being parsed
    };

} }
