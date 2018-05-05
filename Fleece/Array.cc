//
// Array.cc
//
// Copyright (c) 2016 Couchbase, Inc All rights reserved.
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

#include "Array.hh"
#include "MutableArray.hh"
#include "MutableDict.hh"
#include "Internal.hh"
#include "PlatformCompat.hh"
#include "varint.hh"


namespace fleece {
    using namespace internal;


    Array::impl::impl(const Value* v) noexcept {
        if (_usuallyFalse(v == nullptr)) {
            _first = nullptr;
            _width = kNarrow;
            _count = 0;
        } else if (_usuallyTrue(!v->isMutable())) {
            // Normal immutable case:
            _first = (const Value*)(&v->_byte[2]);
            _width = v->isWideArray() ? kWide : kNarrow;
            _count = v->countValue();
            if (_usuallyFalse(_count == kLongArrayCount)) {
                // Long count is stored as a varint:
                uint32_t extraCount;
                size_t countSize = GetUVarInt32(slice(_first, 10), &extraCount);
                if (_usuallyTrue(countSize > 0))
                    _count += extraCount;
                else
                    _count = 0;     // invalid data, but I'm not allowed to throw an exception
                _first = offsetby(_first, countSize + (countSize & 1));
            }
        } else {
            // Mutable Array or Dict:
            auto mcoll = (MutableCollection*)HeapValue::asHeapValue(v);
            MutableArray *mutArray;
            if (v->tag() == kArrayTag) {
                mutArray = (MutableArray*)mcoll;
                _count = mutArray->count();
            } else {
                mutArray = ((MutableDict*)mcoll)->kvArray();
                _count = mutArray->count() / 2;
            }
            _first = _count ? (const Value*)mutArray->first() : nullptr;
            _width = sizeof(MutableValue);
        }
    }

    const Value* Array::impl::deref(const Value *v) const noexcept {
        if (_usuallyFalse(isMutableArray()))
            return ((MutableValue*)v)->asValue();
        return Value::deref(v, _width == kWide);
    }

    const Value* Array::impl::operator[] (unsigned index) const noexcept {
        if (_usuallyFalse(index >= _count))
            return nullptr;
        if (_width == kNarrow)
            return Value::deref<false>(offsetby(_first, kNarrow * index));
        else if (_usuallyTrue(_width == kWide))
            return Value::deref<true> (offsetby(_first, kWide   * index));
        else
            return ((MutableValue*)_first + index)->asValue();
    }

    const Value* Array::impl::firstValue() const noexcept {
        if (_usuallyFalse(_count == 0))
            return nullptr;
        return deref(_first);
    }

    size_t Array::impl::indexOf(const Value *v) const noexcept {
        return ((size_t)v - (size_t)_first) / _width;
    }

    void Array::impl::offset(uint32_t n) {
        throwIf(n > _count, OutOfRange, "iterating past end of array");
        _count -= n;
        if (_usuallyTrue(_count > 0))
            _first = offsetby(_first, _width*n);
    }


    uint32_t Array::count() const noexcept {
        if (_usuallyFalse(isMutable()))
            return asMutable()->count();
        return impl(this)._count;
    }

    const Value* Array::get(uint32_t index) const noexcept {
        if (_usuallyFalse(isMutable()))
            return asMutable()->get(index);
        return impl(this)[index];
    }

    MutableArray* Array::asMutable() const {
        return (MutableArray*)HeapValue::asHeapValue(this);
    }

    static constexpr Array kEmptyArrayInstance;
    const Array* const Array::kEmpty = &kEmptyArrayInstance;



    Array::iterator::iterator(const Array *a) noexcept
    :impl(a),
     _value(firstValue())
    { }

    Array::iterator& Array::iterator::operator++() {
        offset(1);
        _value = firstValue();
        return *this;
    }

    Array::iterator& Array::iterator::operator += (uint32_t n) {
        offset(n);
        _value = firstValue();
        return *this;
    }

}
