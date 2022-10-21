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
  /* according to paper: ARITHMETIC CODING FOR DATA COMPRESSION
   */
  public:
    BitStream bit_stream;
    ArithmeticCodingEncoder(const int &precision){
        /* precision: 
         * * aka Code_value_bits
         * * internal precision of arithmetic coding
         * * following paper ARITHMETIC CODING FOR DATA COMPRESSION
         * * requires:
         * * f \le c - 2 && f + c \le p
         * * default: 32
         */
        assert(precision >= 2 && precision < std::numeric_limits<decltype(_full_range)>::digits);
        _precision = precision;
    	_full_range = (static_cast<decltype(_full_range)>(1) << _precision) - 1;
        _quarter_range = (_full_range >> 2) + 1;
        _half_range = _quarter_range * 2;
        _three_forth_range = _quarter_range * 3;
        int max_total_bits = std::min(_precision - 2, std::numeric_limits<decltype(_full_range)>::digits - _precision);
        _max_total = (static_cast<decltype(_full_range)>(1) << max_total_bits) - 1;
        _low = 0;
        _high = _full_range;
        _pending_bits = 0;
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
        assert((_low & _full_range) == _low);
        assert((_high & _full_range) == _high);
        T_in range = _high - _low + 1;
        T_in c_total = static_cast<decltype(c_total)>(1) << cdf_bits;
        assert(c_total <= _max_total);
        T_in c_low = cdf[sym];
        T_in c_high = cdf[sym + 1];
        assert(c_low != c_high);
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        _low  = _low + ((c_low  * range) >> cdf_bits);
        _renormalize();
    }
    void flush(){
        /* call before the end of encoding */
        _pending_bits++;
        bool bit = static_cast<bool>(_low >= _quarter_range);
        bit_stream.push_back(bit);        
        for (; _pending_bits > 0; _pending_bits--)
            bit_stream.push_back(!bit);
    }
  private:
    void _renormalize(){
        while(1){
            if(_high < _half_range){
                bit_stream.push_back(0);        
                for (; _pending_bits > 0; _pending_bits--)
                    bit_stream.push_back(1);
            } else if (_low >= _half_range){
                bit_stream.push_back(1);        
                for (; _pending_bits > 0; _pending_bits--)
                    bit_stream.push_back(0);
                _low -= _half_range;
                _high -= _half_range;
            }else if(_low>=_quarter_range&&_high<_three_forth_range){
                assert(_pending_bits < std::numeric_limits<decltype(_pending_bits)>::max());
                _pending_bits++;
                _low-=_quarter_range;
                _high-=_quarter_range;
            }else{
                break;
            }
            _high = (_high << 1) + 1;
            _low <<=1;
        }
    }
    T_in _precision;
    T_in _full_range;
    T_in _half_range;
    T_in _quarter_range;
    T_in _three_forth_range;
    T_in _max_total;
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
        bit_stream = encode_bit_stream;
        assert(precision >= 2 && precision < std::numeric_limits<decltype(_full_range)>::digits);
        _precision = precision;
    	_full_range = (static_cast<decltype(_full_range)>(1) << _precision) - 1;
        _quarter_range = (_full_range >> 2) + 1;
        _half_range = _quarter_range * 2;
        _three_forth_range = _quarter_range * 3;
        _max_total = (static_cast<decltype(_full_range)>(1) << (_precision - 2)) - 1;
        _max_total = std::min(_max_total, (static_cast<decltype(_full_range)>(1) << (std::numeric_limits<decltype(_full_range)>::digits - _precision - 1)) - 1);
        _low = 0;
        _high = _full_range;
        _pending_bits = 0;
        _code = 0;
        for(int i=0;i<_precision;i++){
            _code <<= 1;
            _code += static_cast<int>(bit_stream.pop_front());
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
        assert(c_total <= _max_total);
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
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        _low  = _low + ((c_low  * range) >> cdf_bits);
        _renormalize();
        return static_cast<T_out>(sym);
    }
  private:
    void _renormalize(){
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
            _high = (_high << 1) + 1;
            _low <<=1;
            _code = (_code << 1) + static_cast<int>(bit_stream.pop_front());
        }
    }
    T_in _precision;
    T_in _full_range;
    T_in _half_range;
    T_in _quarter_range;
    T_in _three_forth_range;
    T_in _max_total;
    T_in _low;
    T_in _high;
    T_in _pending_bits;
    T_in _code;
};

class RANGEEncoder {
  public:
  BitStream bit_stream;
    RANGEEncoder(){
        _h_precision = 26;
        _t_precision = 8; // 1 byte
        _o_precision = _h_precision - _t_precision;
        _low = 0;
        _range = (1 << _h_precision); // 2 ** 32
    }
    ~RANGEEncoder(){}
    void encode(const int &sym, const uint32_t *cdf, const int &cdf_bits){
        uint64_t c_low = cdf[sym];
        uint64_t c_high = cdf[sym + 1];
        uint64_t c_total = static_cast<decltype(c_total)>(1) << cdf_bits;
        uint64_t c_range = c_high - c_low;
        _range >>= cdf_bits;
        printf("%lld %lld\n", static_cast<long long>(_low), static_cast<long long>(_range));
        _low = _low + c_low * _range;
        printf("%lld %lld\n", static_cast<long long>(_low), static_cast<long long>(_range));
        _range *= c_range;
        printf("%lld %lld\n", static_cast<long long>(_low), static_cast<long long>(_range));
        assert(0);
        while ((_low >> _o_precision) == ((_low + _range) >> _o_precision)){
            bit_stream.push_back_byte(static_cast<uint8_t>(_low >> (_o_precision)));
            // emit one byte
            _low = (_low % (static_cast<decltype(_low)>(1) << _o_precision)) << _t_precision;
            _range <<= _t_precision;
            printf("%d %d\n", static_cast<int>(_low), static_cast<int>(_range));
            if(_range == 0 && _low == 0) assert (0);
        }
    }
  private:
    uint64_t _h_precision;
    uint64_t _t_precision;
    uint64_t _o_precision;
    uint64_t _low;
    uint64_t _high;
    uint64_t _range;
    uint64_t _pending_bits;
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
        for(int i = 1; i <= _h_precision / 8; i++){
            _state <<= 8;
            uint8_t byte = bit_stream.pop_back_byte();
            _state |= byte;
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
            T_in mask = 0xff;
            for(int i = 1; i <= _t_precision / 8; i++){
                bit_stream.push_back_byte(static_cast<uint8_t>(state & mask));
                state >>= 8;
            }
            assert(state < state_max);
        }
        _state = ((state / c_range) << cdf_bits) + (state % c_range) + c_low;
    }
    void flush(){
        T_in mask = 0xff;
        for(int i = 1; i <= _h_precision / 8; i++){
            bit_stream.push_back_byte(static_cast<uint8_t>(_state & mask));
            _state >>= 8;
        }
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
            for(int i = 1; i <= _t_precision / 8; i++){
                state <<= 8;
                uint8_t byte = bit_stream.pop_back_byte();
                state |= byte;
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