//
// MutableValue.cc
//
// Copyright © 2018 Couchbase. All rights reserved.
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

#include "MutableValue.hh"
#include "MutableArray.hh"
#include "MutableDict.hh"
#include "varint.hh"
#include <algorithm>

namespace fleece { namespace internal {
    using namespace std;
    using namespace internal;


    MutableValue::MutableValue(Null)
    :_isInline(true)
    ,_inlineData{(kSpecialTag << 4) | kSpecialValueNull}
    {
        static_assert(sizeof(MutableValue) == 2*sizeof(void*), "MutableValue is wrong size");
        static_assert(offsetof(MutableValue, _inlineData) + MutableValue::kInlineCapacity
                        == offsetof(MutableValue, _isInline), "kInlineCapacity is wrong");
    }


    MutableValue::MutableValue(MutableCollection *md)
    :_asValue( retain(md)->asValue() )
    { }



    MutableValue::MutableValue(const MutableValue &other) noexcept {
        *this = other; // invoke operator=, below
    }


    MutableValue& MutableValue::operator= (const MutableValue &other) noexcept {
        releaseValue();
        _isInline = other._isInline;
        if (_isInline)
            memcpy(&_inlineData, &other._inlineData, kInlineCapacity);
        else
            _asValue = retain(other._asValue);
        return *this;
    }


    MutableValue::MutableValue(MutableValue &&other) noexcept {
        _isInline = other._isInline;
        if (_isInline) {
            memcpy(&_inlineData, &other._inlineData, kInlineCapacity);
        } else {
            _asValue = other._asValue;
            other._asValue = nullptr;
        }
    }

    MutableValue& MutableValue::operator= (MutableValue &&other) noexcept {
        releaseValue();
        _isInline = other._isInline;
        if (_isInline) {
            memcpy(&_inlineData, &other._inlineData, kInlineCapacity);
        } else {
            _asValue = other._asValue;
            other._asValue = nullptr;
        }
        return *this;
    }



    MutableValue::~MutableValue() {
        if (!_isInline)
            release(_asValue);
    }


    MutableCollection* MutableCollection::mutableCopy(const Value *v, tags ifType) {
        if (!v || v->tag() != ifType)
            return nullptr;
        if (v->isMutable())
            return (MutableCollection*)asHeapValue(v);
        switch (ifType) {
            case kArrayTag: return new MutableArray((const Array*)v);
            case kDictTag:  return new MutableDict((const Dict*)v);
            default:        return nullptr;
        }
    }


    void MutableValue::releaseValue() {
        if (!_isInline) {
            release(_asValue);
            _asValue = nullptr;
        }
    }


    const Value* MutableValue::asValue() const {
        return _isInline ? (const Value*)&_inlineData : _asValue;
    }


    void MutableValue::setInline(internal::tags valueTag, int tiny) {
        releaseValue();
        _isInline = true;
        _inlineData[0] = uint8_t((valueTag << 4) | tiny);
    }

    void MutableValue::set(Null) {
        setInline(kSpecialTag, kSpecialValueNull);
    }


    void MutableValue::set(bool b) {
        setInline(kSpecialTag, b ? kSpecialValueTrue : kSpecialValueFalse);
    }


    void MutableValue::set(int i)       {setInt(i, false);}
    void MutableValue::set(unsigned i)  {setInt(i, true);}
    void MutableValue::set(int64_t i)   {setInt(i, false);}
    void MutableValue::set(uint64_t i)  {setInt(i, true);}

    template <class INT>
    void MutableValue::setInt(INT i, bool isUnsigned) {
        if (i < 2048 && (isUnsigned || -i < 2048)) {
            setInline(kShortIntTag, (i >> 8) & 0x0F);
            _inlineData[1] = (uint8_t)(i & 0xFF);
        } else {
            uint8_t buf[8];
            auto size = PutIntOfLength(buf, i, isUnsigned);
            setValue(kIntTag,
                     (int)(size-1) | (isUnsigned ? 0x08 : 0),
                     {buf, size});
        }
    }


    void MutableValue::set(float f) {
        littleEndianFloat lf(f);
        setValue(kFloatTag, 0, {&lf, sizeof(lf)});
    }

    void MutableValue::set(double d) {
        littleEndianDouble ld(d);
        setValue(kFloatTag, 8, {&ld, sizeof(ld)});
    }


    void MutableValue::set(const Value *v) {
        if (!_isInline) {
            if (v == _asValue)
                return;
            release(_asValue);
        }
        if (v && v->tag() < kArrayTag) {
            auto size = v->dataSize();
            if (size <= kInlineCapacity) {
                // Copy value inline if it's small enough
                _isInline = true;
                memcpy(&_inlineData, v, size);
                return;
            }
        }
        // else point to it
        _isInline = false;
        _asValue = retain(v);
    }


    void MutableValue::setValue(tags valueTag, int tiny, slice bytes) {
        releaseValue();
        if (1 + bytes.size <= kInlineCapacity) {
            _inlineData[0] = uint8_t((valueTag << 4) | tiny);
            memcpy(&_inlineData[1], bytes.buf, bytes.size);
            _isInline = true;
        } else {
            _asValue = retain(HeapValue::create(valueTag, tiny, bytes)->asValue());
            _isInline = false;
        }
    }


    void MutableValue::_setStringOrData(tags valueTag, slice s) {
        if (s.size + 1 <= kInlineCapacity) {
            // Short strings can go inline:
            setInline(valueTag, (int)s.size);
            memcpy(&_inlineData[1], s.buf, s.size);
        } else {
            releaseValue();
            _asValue = retain(HeapValue::createStr(valueTag, s)->asValue());
            _isInline = false;
        }
    }


    MutableCollection* MutableValue::makeMutable(tags ifType) {
        if (_isInline)
            return nullptr;
        Retained<MutableCollection> mval = MutableCollection::mutableCopy(_asValue, ifType);
        if (mval)
            set(mval);
        return mval;
    }

} }
