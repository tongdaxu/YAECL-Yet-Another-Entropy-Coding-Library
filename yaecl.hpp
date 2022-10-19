#ifndef INCLUDE_YAECL_HPP_
#define INCLUDE_YAECL_HPP_

#include <cassert>
#include <bitset>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace yaecl {

class BitStream {
  public:
    BitStream(){ _pos = 0; _fpos=0; }
    ~BitStream(){}
    void push_back(bool bit){
        if(_pos % 8==0) _data.push_back(0);
        _data[_pos / 8] |= (bit << (7 - (_pos % 8)));
        _pos++;
    }
    void push_back_byte(uint8_t byte){
        /* alined push for fast ANS
         */
        assert(_pos % 8 == 0);
        _data.push_back(byte);
        _pos+=8;
    }
    bool get(int pos){
        assert(pos < _pos);
        return _data[pos / 8] & (1 << (7 - (pos % 8)));
    }
    bool pop_front(){
        /* queue style pop, use only with ac
         */
        if (_fpos >= _pos) return 0;
        bool tmp = get(_fpos);
        _fpos++;
        return tmp;
    }
    bool pop_back(){
        /* queue style pop, use only with ans
         */
        if (_pos <= 0) return 0;
        _pos--;
        return _data[_pos / 8] & (1 << (7 - (_pos % 8)));
    }
    uint8_t pop_back_byte(){
        /* alined pop for fast ANS
         */
        assert(_pos % 8 == 0);
        _pos-=8;
        return _data[_pos / 8];
    }
    int size(){ return _pos; }
    void save(const std::string &fpath){
        std::ofstream of(fpath, std::ios::out | std::ios::binary);
        of.write(reinterpret_cast<const char*>(_data.data()), _pos / 8 + int(_pos % 8 != 0));
    }
    void load(const std::string &fpath){
        std::ifstream rf(fpath, std::ios::in | std::ios::binary);
        rf.seekg(0, rf.end);
        int flen = rf.tellg();
        rf.seekg(0, rf.beg);
        while(flen){
            uint8_t tmp;
            rf.read(reinterpret_cast<char*>(&tmp), 1);
            _data.push_back(tmp);
            _pos+=8;
            flen--;
        }
    }
  private:
    std::vector<uint8_t> _data;
    int _pos;
    int _fpos;
};
template <typename T_in, typename T_out>
/* T_in: 
 * * internal type doing computation. 
 * * default: uint64_t
 * T_out: 
 * * interface type for io.
 * * default: uint32_t for cxx, int32 for python
 */
class ArithmeticCodingEncoder {
  public:
    BitStream bit_stream;
    ArithmeticCodingEncoder(const int &precision){
        /* precision: 
         * * internal precision of arithmetic coding, always >= 2 but smaller than T_in
         * * should be at least 2 bits larger than bits allowed for cdf precision
         * * and cdf_bits + precision should be smaller than T_in
         * * this means that with internal width 64, the best cdf precision you can have is
         * * 31, which means that precision bits should be 33
         * * default: 32
         */
        assert(precision >= 2 && precision <= std::numeric_limits<decltype(_full_range)>::digits);
        _precision = precision;
        _frequency_bits = std::numeric_limits<decltype(_full_range)>::digits - _precision;
        _frequency_bits = std::min(_frequency_bits, _precision - 2);
    	_full_range = static_cast<decltype(_full_range)>(1) << _precision;
        _half_range = _full_range >> 1;
        _quarter_range = _half_range >> 1;
        _three_forth_range = 3 * _quarter_range;
        _min_range = _quarter_range + 2;
        _max_total = (static_cast<decltype(_full_range)>(1) << _frequency_bits) - 1;
        /* bits for max cdf: 
         *    should not greater than precision bits - 2
         *    should not greater than internal representation bits - precision bits */
        _mask = _full_range - 1;
        _low = 0;
        _high = _mask;
        _pending_bits=0;
    }
    ~ArithmeticCodingEncoder(){}
    void encode(const T_out &sym, const T_out *cdf, const int &cdf_bits){
        /* sym:
         * * symbol to encode, start from 0, should statisfy 0 <= sym < sym_cnt (alphabet size)
         * cdf:
         * * cdf for symbol, should have sym_cnt + 1 bins, and pmf(sym) = cdf[sym + 1] - cdf[sym]
         * * the last element must be power of 2, which is 2 ** cdf_bits
         * cdf_bits:
         * * 2 ** cdf_bits == last element of cdf, always <= _frequency_bits
         */
        assert(_low < _high);
        assert((_low & _mask) == _low);
        assert((_high & _mask) == _high);
        T_in range = _high - _low + 1;
        assert(_min_range <= range);
        assert(range <= _full_range);
        T_in c_total = static_cast<decltype(c_total)>(1) << cdf_bits;
        assert(c_total <= _max_total);
        T_in c_low = cdf[sym];
        T_in c_high = cdf[sym + 1];
        assert(c_low != c_high);
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        _low  = _low + ((c_low  * range) >> cdf_bits);
        while(1){
            if(_high<_half_range||_low>=_half_range){
                bool bit = static_cast<bool>(_high>=_half_range);
                bit_stream.push_back(bit);        
                for (; _pending_bits > 0; _pending_bits--)
                    bit_stream.push_back(!bit);
            }else if(_low>=_quarter_range&&_high<_three_forth_range){
                assert(_pending_bits < std::numeric_limits<decltype(_pending_bits)>::max());
                _pending_bits++;
                _low-=_quarter_range;
                _high-=_quarter_range;
            }else{
                break;
            }
            _high <<= 1;
            _high++;
            _low <<=1;
            _high &= _mask;
            _low &= _mask;
        }
    }
    void flush(){
        /* call before the end of encoding
         */
        _pending_bits++;
        bool bit = static_cast<bool>(_low >= _quarter_range);
        bit_stream.push_back(bit);        
        for (; _pending_bits > 0; _pending_bits--)
            bit_stream.push_back(!bit);
    }
  private:
    T_in _precision;
    T_in _frequency_bits;
    T_in _full_range;
    T_in _half_range;
    T_in _quarter_range;
    T_in _three_forth_range;
    T_in _min_range;
    T_in _max_total;
    T_in _mask;
    T_in _low;
    T_in _high;
    T_in _pending_bits;
};
template <typename T_in, typename T_out>
/* template args: see ArithmeticCodingEncoder */
class ArithmeticCodingDecoder{
  public:
    BitStream bit_stream;
    ArithmeticCodingDecoder(const int &precision, const BitStream &encode_bit_stream){
        /* precision: 
         * * See ArithmeticCodingEncoder
         * encode_bit_stream:
         * * the BitStream ro decode from encoder / read from file
         */
        assert(precision >= 2 && precision <= std::numeric_limits<decltype(_full_range)>::digits);
        bit_stream = encode_bit_stream;
        _precision = precision;
        _frequency_bits = std::numeric_limits<decltype(_full_range)>::digits - _precision;
        _frequency_bits = std::min(_frequency_bits, _precision - 2);
    	_full_range = static_cast<decltype(_full_range)>(1) << _precision;
        _half_range = _full_range >> 1;
        _quarter_range = _half_range >> 1;
        _three_forth_range = 3 * _quarter_range;
        _min_range = _quarter_range + 2;
        _max_total = (static_cast<decltype(_full_range)>(1) << _frequency_bits) - 1;
        _mask = _full_range - 1;
        _low = 0;
        _high = _mask;
        _pending_bits = 0;
        _code = 0;
        for(int i=0;i<_precision;i++){
            _code <<= 1;
            _code += bit_stream.pop_front() ? 1: 0;
        }
    }
    ~ArithmeticCodingDecoder(){}
    T_out decode(const int &sym_cnt, const T_out *cdf, const int &cdf_bits){
        /* sym_cnt:
         * * sym_cnt is the alphabet size
         * cdf:
         * * cdf for symbol, should have sym_cnt + 1 bins, and pmf(sym) = cdf[sym + 1] - cdf[sym]
         * * the last element must be power of 2, which is 2 ** cdf_bits
         * cdf_bits:
         * * 2 ** cdf_bits == last element of cdf, always <= _frequency_bits
         */
        T_in c_total = static_cast<decltype(c_total)>(1) << cdf_bits;
        T_in range = _high - _low + 1;
        T_in scaled_range = _code - _low;
        T_in scaled_value = (((scaled_range + 1) << cdf_bits) - 1) / range;
        assert(scaled_value < c_total);
        T_in start = 0;
        T_in end = sym_cnt;
        while (end - start > 1) {
            T_in middle = (start + end) >> 1;
            if (cdf[middle] > scaled_value)
                end = middle;
            else
                start = middle;
        }
        assert(start + 1 == end);
        T_in sym = start;
        T_in c_low = cdf[sym];
        T_in c_high = cdf[sym + 1];
        assert(c_low != c_high);
        assert(c_total <= _max_total);
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        _low  = _low + ((c_low  * range) >> cdf_bits);
        while(1){
            if(_high < _half_range){
                // pass
            }else if(_low >= _half_range){
                _code -= _half_range;
                _low -= _half_range;
                _high -= _half_range;
            }else if(_low >= _quarter_range && _high < _three_forth_range){
                _code -= _quarter_range;
                _low -= _quarter_range;
                _high -= _quarter_range;
            }else{
                break;
            }
            _high <<= 1;
            _high++;
            _low <<=1;
            _code <<= 1;
            _code += bit_stream.pop_front() ? 1 : 0;
        }
        return static_cast<T_out>(sym);
    }
  private:
    T_in _precision;
    T_in _frequency_bits;
    T_in _full_range;
    T_in _half_range;
    T_in _quarter_range;
    T_in _three_forth_range;
    T_in _min_range;
    T_in _max_total;
    T_in _mask;
    T_in _low;
    T_in _high;
    T_in _pending_bits;
    T_in _code;
};
template <typename T_in, typename T_out>
/* template args: see ArithmeticCodingEncoder */
class RANSCodec {
  public:
    BitStream bit_stream;
    RANSCodec(const int &h_precision, const int &t_precision){
        /* h_precision: 
         * * precision for head part, divided by 8
         * * t_precision < h_precision <= t_precision * 2
         * * the larger it is, the more precise the pdf you can have
         * * but also leads to more overhead in flush()
         * * default: 64
         * t_precision:
         * * precision for tail part, divided by 8
         * * the larger it is, the faster rans is
         * * but also leads to more overhead in flush()
         * * default: 32
         */
        _h_precision = h_precision;
        _t_precision = t_precision;
        assert(_h_precision % 8 == 0);
        assert(_t_precision % 8 == 0);
        assert(_t_precision < _h_precision);
        assert(_h_precision <= _t_precision * 2);
        _h_min = static_cast<decltype(_h_min)>(1) << (_h_precision - _t_precision);
        _state = _h_min; // max state
    }
    RANSCodec(const int &h_precision, const int &t_precision, const BitStream &encode_bit_stream){
        _h_precision = h_precision;
        _t_precision = t_precision;
        _h_min = static_cast<decltype(_h_min)>(1) << (_h_precision - _t_precision);
        bit_stream = encode_bit_stream;
         _state = 0;
        for(int i = _h_precision - 1; i >= 0; i--){
            T_in bit = bit_stream.pop_back();
            _state |= (bit << i);
        }
    }
    ~RANSCodec(){}
    void encode(const T_out &sym, const T_out *cdf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder */
        T_in c_low = cdf[sym];
        T_in c_range = cdf[sym + 1] - c_low;
        T_in c_total = static_cast<decltype(c_total)>(1) << cdf_bits;
        T_in state = _state;
        T_in state_max = c_range << (_h_precision - cdf_bits);
        if(state >= state_max){
            T_in mask = 1;
            for(int i = 1; i <= _t_precision; i++){
                bit_stream.push_back(static_cast<bool>(state & mask));
                mask <<= 1;
            }
            state >>= _t_precision;
            assert(state < state_max);
        }
        _state = ((state / c_range) << cdf_bits) + (state % c_range) + c_low;
    }
    void flush(){
        T_in mask = 1;
        for(int i = 1; i <= _h_precision; i++){
            bit_stream.push_back(static_cast<bool>(_state & mask));
            mask <<= 1;
        }
        _state = 0;
    }
    T_out decode(const int &sym_cnt, const T_out *cdf, const int &cdf_bits){
        /* args: See ArithmeticCodingDecoder */
        T_in scaled_value = _state & ((static_cast<decltype(_state)>(1) << cdf_bits) - 1);
        T_in start = 0;
        T_in end = sym_cnt;
        while (end - start > 1) {
            T_in middle = (start + end) >> 1;
            if (cdf[middle] > scaled_value)
                end = middle;
            else
                start = middle;
        }
        assert(start + 1 == end);
        T_in sym = start;
        T_in c_low = cdf[sym];
        T_in c_range = cdf[sym + 1] - c_low;
        T_in state = _state;
        state = c_range * (state >> cdf_bits) + scaled_value - c_low;
        if (state < _h_min){
            state <<= _t_precision;
            for(int i = _t_precision - 1; i >= 0; i--){
                T_in bit = bit_stream.pop_back();
                state |= (bit << i);
            }
            assert (state >= _h_min);
        }
        _state = state;
        return static_cast<T_out>(sym);
    }
  private:
    T_in _state;
    T_in _h_precision;
    T_in _t_precision;
    T_in _t_mask;
    T_in _h_min;
};

}

#endif